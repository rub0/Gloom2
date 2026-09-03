#pragma once

#include <gloom/assets/asset.hpp>
#include <gloom/core/job_system.hpp>

#include <cstdint>
#include <future>
#include <mutex>
#include <unordered_map>

namespace gloom::assets {

enum class AssetLoadState {
  ready,
  error_placeholder,
};

struct AssetLoadResult {
  AssetLoadState state{AssetLoadState::error_placeholder};
  CookedAsset asset;
  std::string error;
};

struct AssetLoaderMetrics {
  std::uint64_t requests{0};
  std::uint64_t cache_hits{0};
  std::uint64_t loaded{0};
  std::uint64_t failed{0};
  std::uint64_t bytes_loaded{0};
  std::uint64_t invalidations{0};
};

class AsyncAssetLoader final {
public:
  AsyncAssetLoader(core::JobSystem &jobs, const VirtualFileSystem &filesystem,
                   const AssetCatalog &catalog);
  ~AsyncAssetLoader();

  AsyncAssetLoader(const AsyncAssetLoader &) = delete;
  AsyncAssetLoader &operator=(const AsyncAssetLoader &) = delete;

  [[nodiscard]] std::shared_future<AssetLoadResult> request(AssetId id);
  [[nodiscard]] bool invalidate(AssetId id);
  void wait();
  [[nodiscard]] AssetLoaderMetrics metrics() const noexcept;

private:
  [[nodiscard]] static AssetLoadResult placeholder(AssetId id, AssetType type,
                                                   std::string error);

  core::JobSystem &jobs_;
  const VirtualFileSystem &filesystem_;
  const AssetCatalog &catalog_;
  core::TaskGroup tasks_;
  mutable std::mutex mutex_;
  std::unordered_map<AssetId, std::shared_future<AssetLoadResult>, AssetIdHash>
      requests_;
  AssetLoaderMetrics metrics_;
};

} // namespace gloom::assets
