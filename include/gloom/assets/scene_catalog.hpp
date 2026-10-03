#pragma once

#include <gloom/assets/asset.hpp>

#include <expected>
#include <string>

namespace gloom::assets {

// Resolve image references inside the source mount and match the cooker's output layout.
[[nodiscard]] std::expected<VirtualPath, std::string> dependency_source_path(const VirtualPath& source, std::string dependency);
[[nodiscard]] std::expected<VirtualPath, std::string> dependency_cooked_path(const VirtualPath& scene, AssetId id);

struct DiscoveredSceneCatalog {
    AssetCatalog catalog;
    AssetId scene;
};

// Reconstructs the runtime catalog for one cooked glTF scene and the external
// texture files emitted beside it by gloom_asset_cooker.
[[nodiscard]] std::expected<DiscoveredSceneCatalog, std::string> discover_cooked_scene(
    const VirtualFileSystem& filesystem, const VirtualPath& source, const VirtualPath& cooked);

} // namespace gloom::assets
