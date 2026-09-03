#pragma once

#include <gloom/assets/asset.hpp>

#include <expected>
#include <string>

namespace gloom::assets {

struct DiscoveredSceneCatalog {
    AssetCatalog catalog;
    AssetId scene;
};

// Reconstructs the runtime catalog for one cooked glTF scene and the external
// texture files emitted beside it by gloom_asset_cooker.
[[nodiscard]] std::expected<DiscoveredSceneCatalog, std::string>
discover_cooked_scene(const VirtualFileSystem& filesystem,
                      const VirtualPath& source,
                      const VirtualPath& cooked);

} // namespace gloom::assets
