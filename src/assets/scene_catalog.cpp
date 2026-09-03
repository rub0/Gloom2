#include <gloom/assets/scene_catalog.hpp>

#include <gloom/assets/gltf_importer.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <unordered_set>
#include <vector>

namespace gloom::assets {
namespace {

[[nodiscard]] std::expected<VirtualPath, std::string>
join_source(const VirtualPath& source, std::string dependency) {
    std::ranges::replace(dependency, '\\', '/');
    const auto separator = source.relative().find_last_of('/');
    std::string combined = separator == std::string_view::npos
                               ? std::string{}
                               : std::string{source.relative().substr(0, separator + 1)};
    combined += dependency;
    std::vector<std::string> parts;
    std::size_t begin = 0;
    while (begin <= combined.size()) {
        const auto end = combined.find('/', begin);
        const auto part = combined.substr(begin, end - begin);
        if (part == "..") {
            if (parts.empty()) {
                return std::unexpected{"Scene dependency escapes its virtual mount"};
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
    std::string result{source.mount()};
    result += ":/";
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) {
            result += '/';
        }
        result += parts[index];
    }
    return VirtualPath::parse(result);
}

[[nodiscard]] std::expected<VirtualPath, std::string>
dependency_cooked_path(const VirtualPath& scene, const AssetId id) {
    std::array<char, 16> hexadecimal{};
    const auto conversion =
        std::to_chars(hexadecimal.data(), hexadecimal.data() + hexadecimal.size(), id.value, 16);
    const auto separator = scene.relative().find_last_of('/');
    std::string result{scene.mount()};
    result += ":/";
    if (separator != std::string_view::npos) {
        result += scene.relative().substr(0, separator + 1);
    }
    result += "dependencies/";
    result.append(hexadecimal.data(), conversion.ptr);
    result += ".gasset";
    return VirtualPath::parse(result);
}

} // namespace

std::expected<DiscoveredSceneCatalog, std::string>
discover_cooked_scene(const VirtualFileSystem& filesystem,
                      const VirtualPath& source,
                      const VirtualPath& cooked) {
    const auto envelope = filesystem.read(cooked);
    if (!envelope) {
        return std::unexpected{envelope.error()};
    }
    const auto scene_asset = decode_cooked_asset(*envelope);
    if (!scene_asset || scene_asset->type != AssetType::scene) {
        return std::unexpected{scene_asset ? "Cooked asset is not a scene" : scene_asset.error()};
    }
    const auto scene = decode_imported_scene(scene_asset->payload);
    if (!scene) {
        return std::unexpected{scene.error()};
    }
    const auto expected_scene_id = make_asset_id(source, AssetType::scene);
    if (scene_asset->id != expected_scene_id) {
        return std::unexpected{"Cooked scene ID does not match its source virtual path"};
    }

    DiscoveredSceneCatalog result{.scene = scene_asset->id};
    std::unordered_set<AssetId, AssetIdHash> found;
    for (const auto& image : scene->images) {
        if (image.embedded || image.external_uri.empty()) {
            continue;
        }
        const auto dependency_source = join_source(source, image.external_uri);
        if (!dependency_source) {
            return std::unexpected{dependency_source.error()};
        }
        const auto id = make_asset_id(*dependency_source, AssetType::texture);
        if (!found.emplace(id).second) {
            continue;
        }
        const auto dependency_cooked = dependency_cooked_path(cooked, id);
        if (!dependency_cooked) {
            return std::unexpected{dependency_cooked.error()};
        }
        const auto dependency_envelope = filesystem.read(*dependency_cooked);
        if (!dependency_envelope) {
            return std::unexpected{dependency_envelope.error()};
        }
        const auto dependency = decode_cooked_asset(*dependency_envelope);
        if (!dependency || dependency->id != id || dependency->type != AssetType::texture) {
            return std::unexpected{dependency ? "Cooked scene dependency is inconsistent"
                                              : dependency.error()};
        }
        result.catalog.add({.id = id,
                            .type = AssetType::texture,
                            .source = *dependency_source,
                            .cooked = *dependency_cooked,
                            .source_fingerprint = dependency->source_fingerprint});
    }
    if (found.size() != scene_asset->dependencies.size() ||
        !std::ranges::all_of(scene_asset->dependencies,
                            [&](const AssetId id) { return found.contains(id); })) {
        return std::unexpected{"Cooked scene dependency set does not match its imported images"};
    }
    result.catalog.add({.id = scene_asset->id,
                        .type = AssetType::scene,
                        .source = source,
                        .cooked = cooked,
                        .source_fingerprint = scene_asset->source_fingerprint,
                        .dependencies = scene_asset->dependencies});
    return result;
}

} // namespace gloom::assets
