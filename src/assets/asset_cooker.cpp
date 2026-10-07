#include <gloom/assets/asset_cooker.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <gloom/assets/texture_asset.hpp>
#include <utility>

namespace gloom::assets {
namespace {
std::expected<TextureSemantic, std::string> texture_semantic(const ImportedScene& scene, uint32 image_index) {
    uint32 semantics = 0;
    constexpr uint32 slots[]{1, 2, 4, 1, 2, 2, 1, 2, 1, 1};
    for (const ImportedMaterial& material : scene.materials) {
        const std::array<uint32, render::material_texture_count> textures = material_textures(material);
        for (size_t slot = 0; slot < textures.size(); ++slot)
            if (textures[slot] != no_asset_index && scene.textures[textures[slot]].image == image_index)
                semantics |= slots[slot];
    }
    if (semantics && (semantics & (semantics - 1)))
        return std::unexpected{"One image is used with incompatible color/data/normal semantics; use separate image resources"};
    return semantics == 4 ? TextureSemantic::normal : semantics == 2 ? TextureSemantic::data : TextureSemantic::color;
}
}

std::expected<GltfCookResult, std::string> cook_gltf(const VirtualFileSystem& filesystem, const VirtualPath& source, const VirtualPath& cooked_output) {
    const std::expected<std::filesystem::path, std::string> source_path = filesystem.resolve(source);
    if (!source_path)
        return std::unexpected{source_path.error()};
    std::error_code size_error;
    if (std::filesystem::file_size(*source_path, size_error) > 1024ULL * 1024ULL * 1024ULL || size_error)
        return std::unexpected{"glTF file is missing or exceeds the cooker size limit"};
    const std::expected<ImportedScene, std::string> imported = import_gltf(*source_path);
    if (!imported)
        return std::unexpected{imported.error()};
    std::vector<std::byte> scene_payload = encode_imported_scene(*imported);
    GltfCookResult result{.scene = {.id = make_asset_id(source, AssetType::scene),
                              .type = AssetType::scene,
                              .source = source,
                              .cooked = cooked_output,
                              .source_fingerprint = fingerprint(scene_payload)}};
    result.dependencies.reserve(imported->images.size());
    result.scene.dependencies.reserve(imported->images.size());
    Array<TextureSemantic> semantics;
    semantics.reserve(imported->images.size());
    for (uint32 image_index = 0; image_index < imported->images.size(); ++image_index) {
        const ImportedImage& image = imported->images[image_index];
        if (image.embedded)
            return std::unexpected{"Embedded glTF images are not supported by this cooker version"};
        if (image.external_uri.empty())
            continue;
        const std::expected<VirtualPath, std::string> dependency_source = dependency_source_path(source, image.external_uri);
        if (!dependency_source)
            return std::unexpected{dependency_source.error()};
        const AssetId dependency_id = make_asset_id(*dependency_source, AssetType::texture);
        const std::expected<TextureSemantic, std::string> semantic = texture_semantic(*imported, image_index);
        if (!semantic)
            return std::unexpected{semantic.error()};
        // ponytail: linear cold deduplication; add an index if large image catalogs dominate measured cooking cost.
        size_t duplicate = 0;
        while (duplicate < result.dependencies.size() && result.dependencies[duplicate].id != dependency_id)
            ++duplicate;
        if (duplicate < result.dependencies.size()) {
            if (semantics[duplicate] != *semantic)
                return std::unexpected{"Image URI aliases use incompatible texture semantics"};
            continue;
        }
        const std::expected<std::vector<std::byte>, std::string> dependency_bytes = filesystem.read(*dependency_source);
        if (!dependency_bytes)
            return std::unexpected{"Could not read glTF image dependency: " + dependency_bytes.error()};
        const std::expected<VirtualPath, std::string> dependency_output = dependency_cooked_path(cooked_output, dependency_id);
        if (!dependency_output)
            return std::unexpected{dependency_output.error()};
        const uint64 source_hash = fingerprint(*dependency_bytes);
        std::expected<std::vector<std::byte>, std::string> texture_payload = cook_texture_ktx2(*dependency_bytes, *semantic);
        if (!texture_payload)
            return std::unexpected{"Could not cook glTF texture '" + image.external_uri + "': " + texture_payload.error()};
        const CookedAsset cooked_texture{
            .id = dependency_id, .type = AssetType::texture, .source_fingerprint = source_hash, .payload = std::move(*texture_payload)};
        const std::expected<void, std::string> written = filesystem.write(*dependency_output, encode_cooked_asset(cooked_texture));
        if (!written)
            return std::unexpected{written.error()};
        result.dependencies.push_back(
            {.id = dependency_id, .type = AssetType::texture, .source = *dependency_source, .cooked = *dependency_output, .source_fingerprint = source_hash});
        result.scene.dependencies.push_back(dependency_id);
        semantics.push_back(*semantic);
    }
    for (size_t i = 1; i < result.dependencies.size(); ++i) {
        AssetRecord record = std::move(result.dependencies[i]);
        size_t position = i;
        while (position && result.dependencies[position - 1].id.value > record.id.value) {
            result.dependencies[position] = std::move(result.dependencies[position - 1]);
            --position;
        }
        result.dependencies[position] = std::move(record);
    }
    for (size_t i = 0; i < result.dependencies.size(); ++i)
        result.scene.dependencies[i] = result.dependencies[i].id;
    const CookedAsset cooked_scene{.id = result.scene.id,
        .type = AssetType::scene,
        .source_fingerprint = result.scene.source_fingerprint,
        .dependencies = result.scene.dependencies,
        .payload = std::move(scene_payload)};
    const std::expected<void, std::string> written = filesystem.write(cooked_output, encode_cooked_asset(cooked_scene));
    if (!written)
        return std::unexpected{written.error()};
    return result;
}
}
