#pragma once

#include <gloom/assets/asset.hpp>

#include <expected>
#include <string>
#include <vector>

namespace gloom::assets {

struct GltfCookResult {
  AssetRecord scene;
  std::vector<AssetRecord> dependencies;
};

[[nodiscard]] std::expected<GltfCookResult, std::string>
cook_gltf(const VirtualFileSystem &filesystem, const VirtualPath &source,
          const VirtualPath &cooked_output);

} // namespace gloom::assets
