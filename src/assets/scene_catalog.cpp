#include <gloom/assets/scene_catalog.hpp>

#include <gloom/assets/gltf_importer.hpp>

#include <array>
#include <charconv>
#include <vector>

namespace gloom::assets {

[[nodiscard]] std::expected<VirtualPath, std::string> dependency_source_path(const VirtualPath& source, std::string_view dependency) {
    const size_t separator = source.relative().find_last_of('/');
    std::string combined = separator == std::string_view::npos ? std::string{} : std::string{source.relative().substr(0, separator + 1)};
    combined += dependency;
    for (char& character : combined)
        if (character == '\\')
            character = '/';
    std::vector<std::string_view> parts;
    size_t begin = 0;
    while (begin <= combined.size()) {
        const size_t end = combined.find('/', begin);
        const std::string_view part{combined.data() + begin, (end == std::string::npos ? combined.size() : end) - begin};
        if (part == "..") {
            if (parts.empty()) {
                return std::unexpected{"Asset dependency escapes its virtual mount"};
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
    for (size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) {
            result += '/';
        }
        result += parts[index];
    }
    return VirtualPath::parse(result);
}

[[nodiscard]] std::expected<VirtualPath, std::string> dependency_cooked_path(const VirtualPath& scene, const AssetId id) {
    std::array<char, 16> hexadecimal{};
    const std::to_chars_result conversion = std::to_chars(hexadecimal.data(), hexadecimal.data() + hexadecimal.size(), id.value, 16);
    const size_t separator = scene.relative().find_last_of('/');
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

std::expected<DiscoveredSceneCatalog, std::string> discover_cooked_scene(
    const VirtualFileSystem& filesystem, const VirtualPath& source, const VirtualPath& cooked) {
    const std::expected<std::vector<std::byte>, std::string> envelope = filesystem.read(cooked);
    if (!envelope) {
        return std::unexpected{envelope.error()};
    }
    const std::expected<CookedAsset, std::string> scene_asset = decode_cooked_asset(*envelope);
    if (!scene_asset || scene_asset->type != AssetType::scene) {
        return std::unexpected{scene_asset ? "Cooked asset is not a scene" : scene_asset.error()};
    }
    const std::expected<ImportedScene, std::string> scene = decode_imported_scene(scene_asset->payload);
    if (!scene) {
        return std::unexpected{scene.error()};
    }
    const AssetId expected_scene_id = make_asset_id(source, AssetType::scene);
    if (scene_asset->id != expected_scene_id) {
        return std::unexpected{"Cooked scene ID does not match its source virtual path"};
    }

    DiscoveredSceneCatalog result{.scene = scene_asset->id};
    for (const ImportedImage& image : scene->images) {
        if (image.embedded || image.external_uri.empty()) {
            continue;
        }
        const std::expected<VirtualPath, std::string> dependency_source = dependency_source_path(source, image.external_uri);
        if (!dependency_source) {
            return std::unexpected{dependency_source.error()};
        }
        const AssetId id = make_asset_id(*dependency_source, AssetType::texture);
        if (result.catalog.find(id)) {
            continue;
        }
        const std::expected<VirtualPath, std::string> dependency_cooked = dependency_cooked_path(cooked, id);
        if (!dependency_cooked) {
            return std::unexpected{dependency_cooked.error()};
        }
        const std::expected<std::vector<std::byte>, std::string> dependency_envelope = filesystem.read(*dependency_cooked);
        if (!dependency_envelope) {
            return std::unexpected{dependency_envelope.error()};
        }
        const std::expected<CookedAsset, std::string> dependency = decode_cooked_asset(*dependency_envelope);
        if (!dependency || dependency->id != id || dependency->type != AssetType::texture) {
            return std::unexpected{dependency ? "Cooked scene dependency is inconsistent" : dependency.error()};
        }
        result.catalog.add({.id = id,
            .type = AssetType::texture,
            .source = *dependency_source,
            .cooked = *dependency_cooked,
            .source_fingerprint = dependency->source_fingerprint});
    }
    if (result.catalog.size() != scene_asset->dependencies.size()) {
        return std::unexpected{"Cooked scene dependency set does not match its imported images"};
    }
    for (AssetId id : scene_asset->dependencies)
        if (!result.catalog.find(id))
            return std::unexpected{"Cooked scene dependency set does not match its imported images"};
    result.catalog.add({.id = scene_asset->id,
        .type = AssetType::scene,
        .source = source,
        .cooked = cooked,
        .source_fingerprint = scene_asset->source_fingerprint,
        .dependencies = scene_asset->dependencies});
    return result;
}

} // namespace gloom::assets
