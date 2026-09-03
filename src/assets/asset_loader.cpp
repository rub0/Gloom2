#include <gloom/assets/asset_loader.hpp>

#include <array>
#include <chrono>
#include <exception>
#include <memory>
#include <utility>

namespace gloom::assets {

AsyncAssetLoader::AsyncAssetLoader(core::JobSystem &jobs,
                                   const VirtualFileSystem &filesystem,
                                   const AssetCatalog &catalog)
    : jobs_{jobs}, filesystem_{filesystem}, catalog_{catalog},
      tasks_{jobs.create_group()} {}

AsyncAssetLoader::~AsyncAssetLoader() {
  try {
    wait();
  } catch (...) {
  }
}

std::shared_future<AssetLoadResult>
AsyncAssetLoader::request(const AssetId id) {
  std::scoped_lock lock{mutex_};
  ++metrics_.requests;
  if (const auto found = requests_.find(id); found != requests_.end()) {
    ++metrics_.cache_hits;
    return found->second;
  }

  const AssetRecord *catalog_record = catalog_.find(id);
  auto promise = std::make_shared<std::promise<AssetLoadResult>>();
  auto future = promise->get_future().share();
  requests_.emplace(id, future);
  if (catalog_record == nullptr) {
    ++metrics_.failed;
    promise->set_value(
        placeholder(id, AssetType::binary, "Asset is absent from the catalog"));
    return future;
  }
  const AssetRecord record = *catalog_record;
  jobs_.schedule(tasks_, [this, promise, record] {
    AssetLoadResult result;
    const auto encoded = filesystem_.read(record.cooked);
    if (!encoded) {
      result = placeholder(record.id, record.type, encoded.error());
    } else {
      const auto decoded = decode_cooked_asset(*encoded);
      if (!decoded) {
        result = placeholder(record.id, record.type, decoded.error());
      } else if (decoded->id != record.id || decoded->type != record.type ||
                 decoded->source_fingerprint != record.source_fingerprint ||
                 decoded->dependencies != record.dependencies) {
        result = placeholder(record.id, record.type,
                             "Cooked asset does not match its catalog record");
      } else {
        result = {.state = AssetLoadState::ready, .asset = std::move(*decoded)};
      }
    }
    {
      const std::scoped_lock result_lock{mutex_};
      if (result.state == AssetLoadState::ready) {
        ++metrics_.loaded;
        metrics_.bytes_loaded += result.asset.payload.size();
      } else {
        ++metrics_.failed;
      }
    }
    promise->set_value(std::move(result));
  });
  return future;
}

bool AsyncAssetLoader::invalidate(const AssetId id) {
  std::scoped_lock lock{mutex_};
  const auto found = requests_.find(id);
  if (found == requests_.end()) {
    return true;
  }
  if (found->second.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
    return false;
  }
  requests_.erase(found);
  ++metrics_.invalidations;
  return true;
}

void AsyncAssetLoader::wait() {
  if (tasks_.valid()) {
    jobs_.wait(tasks_);
  }
}

AssetLoaderMetrics AsyncAssetLoader::metrics() const noexcept {
  const std::scoped_lock lock{mutex_};
  return metrics_;
}

AssetLoadResult AsyncAssetLoader::placeholder(const AssetId id,
                                              const AssetType type,
                                              std::string error) {
  constexpr std::array<std::byte, 8> placeholder_payload{
      std::byte{'M'}, std::byte{'I'}, std::byte{'S'}, std::byte{'S'},
      std::byte{'I'}, std::byte{'N'}, std::byte{'G'}, std::byte{0}};
  return {
      .state = AssetLoadState::error_placeholder,
      .asset = {.id = id,
                .type = type,
                .payload = {placeholder_payload.begin(),
                            placeholder_payload.end()}},
      .error = std::move(error),
  };
}

} // namespace gloom::assets
