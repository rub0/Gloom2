#include <gloom/assets/asset_cooker.hpp>

#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/texture_asset.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <string_view>
#include <unordered_map>

namespace gloom::assets {
namespace {

[[nodiscard]] std::expected<VirtualPath, std::string>
join_source_path(const VirtualPath &source, const std::string_view dependency) {
  std::vector<std::string> parts;
  const std::string_view source_relative = source.relative();
  const std::size_t source_separator = source_relative.find_last_of('/');
  std::string combined =
      source_separator == std::string_view::npos
          ? std::string{}
          : std::string{source_relative.substr(0, source_separator)};
  if (!combined.empty()) {
    combined.push_back('/');
  }
  combined.append(dependency);
  std::ranges::replace(combined, '\\', '/');
  std::size_t begin = 0;
  while (begin <= combined.size()) {
    const std::size_t end = combined.find('/', begin);
    const std::string part = combined.substr(
        begin, (end == std::string::npos ? combined.size() : end) - begin);
    if (part == "..") {
      if (parts.empty()) {
        return std::unexpected{"glTF dependency escapes its virtual mount"};
      }
      parts.pop_back();
    } else if (!part.empty() && part != ".") {
      parts.push_back(part);
    }
    if (end == std::string::npos) {
      break;
    }
    begin = end + 1;
  }
  std::string path{source.mount()};
  path += ":/";
  for (std::size_t index = 0; index < parts.size(); ++index) {
    if (index > 0)
      path.push_back('/');
    path += parts[index];
  }
  return VirtualPath::parse(path);
}

[[nodiscard]] std::string hexadecimal(const std::uint64_t value) {
  std::array<char, 16> buffer{};
  const auto result =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
  return std::string{buffer.data(), result.ptr};
}

[[nodiscard]] std::expected<VirtualPath, std::string>
dependency_output_path(const VirtualPath &scene_output, const AssetId id) {
  const std::string_view relative = scene_output.relative();
  const std::size_t separator = relative.find_last_of('/');
  std::string path{scene_output.mount()};
  path += ":/";
  if (separator != std::string_view::npos) {
    path.append(relative.substr(0, separator + 1));
  }
  path += "dependencies/" + hexadecimal(id.value) + ".gasset";
  return VirtualPath::parse(path);
}

[[nodiscard]] std::expected<TextureSemantic,std::string> texture_semantic(const ImportedScene& scene,
                                               const std::uint32_t image_index) {
  unsigned semantics=0;
  for (const auto& material : scene.materials) {
    const auto uses_image = [&](const std::uint32_t texture_index) {
      return texture_index != no_asset_index && texture_index < scene.textures.size() &&
             scene.textures[texture_index].image == image_index;
    };
    if (uses_image(material.normal_texture)) {
      semantics|=4;
    }
    if (uses_image(material.metallic_roughness_texture) || uses_image(material.extra_textures[1]) ||
        uses_image(material.extra_textures[2]) || uses_image(material.extra_textures[4])) {
      semantics|=2;
    }
    if (uses_image(material.base_color_texture) || uses_image(material.extra_textures[0]) ||
        uses_image(material.extra_textures[3]) || uses_image(material.extra_textures[5]) || uses_image(material.extra_textures[6])) semantics|=1;
  }
  if (semantics && (semantics&(semantics-1))) return std::unexpected{"One image is used with incompatible color/data/normal semantics; use separate image resources"};
  return semantics==4 ? TextureSemantic::normal : semantics==2 ? TextureSemantic::data : TextureSemantic::color;
}

} // namespace

std::expected<GltfCookResult, std::string>
cook_gltf(const VirtualFileSystem &filesystem, const VirtualPath &source,
          const VirtualPath &cooked_output) {
  const auto source_bytes = filesystem.read(source);
  const auto source_path = filesystem.resolve(source);
  if (!source_bytes || !source_path) {
    return std::unexpected{source_bytes ? source_path.error()
                                        : source_bytes.error()};
  }
  const auto imported = import_gltf(*source_path);
  if (!imported) {
    return std::unexpected{imported.error()};
  }
  const auto scene_payload = encode_imported_scene(*imported);

  GltfCookResult result{
      .scene = {.id = make_asset_id(source, AssetType::scene),
                .type = AssetType::scene,
                .source = source,
                .cooked = cooked_output,
                .source_fingerprint = fingerprint(scene_payload)},
  };
  std::unordered_map<std::string, AssetId> discovered;
  for (std::uint32_t image_index = 0; image_index < imported->images.size(); ++image_index) {
    const auto& image = imported->images[image_index];
    if (image.embedded) {
      return std::unexpected{
          "Embedded glTF images are not supported by this cooker version"};
    }
    if (image.external_uri.empty() || discovered.contains(image.external_uri)) {
      continue;
    }
    const auto dependency_source = join_source_path(source, image.external_uri);
    if (!dependency_source) {
      return std::unexpected{dependency_source.error()};
    }
    const auto dependency_bytes = filesystem.read(*dependency_source);
    if (!dependency_bytes) {
      return std::unexpected{"Could not read glTF image dependency: " +
                             dependency_bytes.error()};
    }
    const AssetId dependency_id =
        make_asset_id(*dependency_source, AssetType::texture);
    const auto dependency_output =
        dependency_output_path(cooked_output, dependency_id);
    if (!dependency_output) {
      return std::unexpected{dependency_output.error()};
    }
    const std::uint64_t source_hash = fingerprint(*dependency_bytes);
    const auto semantic=texture_semantic(*imported,image_index);
    if (!semantic) return std::unexpected{semantic.error()};
    const auto texture_payload =
        cook_texture_ktx2(*dependency_bytes, *semantic);
    if (!texture_payload) {
      return std::unexpected{"Could not cook glTF texture '" + image.external_uri +
                             "': " + texture_payload.error()};
    }
    const CookedAsset cooked_texture{
        .id = dependency_id,
        .type = AssetType::texture,
        .source_fingerprint = source_hash,
        .payload = *texture_payload,
    };
    const auto write = filesystem.write(*dependency_output,
                                        encode_cooked_asset(cooked_texture));
    if (!write) {
      return std::unexpected{write.error()};
    }
    result.dependencies.push_back({.id = dependency_id,
                                   .type = AssetType::texture,
                                   .source = *dependency_source,
                                   .cooked = *dependency_output,
                                   .source_fingerprint = source_hash});
    result.scene.dependencies.push_back(dependency_id);
    discovered.emplace(image.external_uri, dependency_id);
  }
  std::ranges::sort(result.scene.dependencies, {}, &AssetId::value);
  std::ranges::sort(result.dependencies, {},
                    [](const AssetRecord &record) { return record.id.value; });
  const CookedAsset cooked_scene{
      .id = result.scene.id,
      .type = AssetType::scene,
      .source_fingerprint = result.scene.source_fingerprint,
      .dependencies = result.scene.dependencies,
      .payload = scene_payload,
  };
  const auto write_scene =
      filesystem.write(cooked_output, encode_cooked_asset(cooked_scene));
  if (!write_scene) {
    return std::unexpected{write_scene.error()};
  }
  return result;
}

} // namespace gloom::assets
