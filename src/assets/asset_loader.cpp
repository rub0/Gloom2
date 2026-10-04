#include <gloom/core/allocation_profile.hpp>
#include <gloom/assets/asset_loader.hpp>

#include <array>
#include <chrono>
#include <utility>

namespace gloom::assets {

AsyncAssetLoader::AsyncAssetLoader(core::JobSystem& jobs, const VirtualFileSystem& filesystem, const AssetCatalog& catalog)
    : jobs_{jobs}, filesystem_{filesystem}, catalog_{catalog}, tasks_{jobs.create_group()} {}

AsyncAssetLoader::~AsyncAssetLoader() {
    wait();
}

// Cold asset data/results retain their existing format until 117/118. The scheduler
// owns this stable context; it never borrows a cache entry or an initializer-list span.
struct AsyncAssetLoader::LoadContext {
    AsyncAssetLoader* loader;
    AssetRecord record;
    std::promise<AssetLoadResult> promise;
    void operator()() noexcept {
        AssetLoadResult result;
        decltype(loader->filesystem_.read(record.cooked)) encoded = loader->filesystem_.read(record.cooked);
        if (!encoded) {
            result = placeholder(record.id, record.type, encoded.error());
        } else {
            decltype(decode_cooked_asset(*encoded)) decoded = decode_cooked_asset(*encoded);
            if (!decoded) {
                result = placeholder(record.id, record.type, decoded.error());
            } else if (decoded->id != record.id || decoded->type != record.type || decoded->source_fingerprint != record.source_fingerprint ||
                       decoded->dependencies != record.dependencies) {
                result = placeholder(record.id, record.type, "Cooked asset does not match its catalog record");
            } else {
                result = {.state = AssetLoadState::ready, .asset = static_cast<CookedAsset&&>(*decoded)};
            }
        }
        AcquireSRWLockExclusive(&loader->mutex_);
        if (result.state == AssetLoadState::ready) {
            ++loader->metrics_.loaded;
            loader->metrics_.bytes_loaded += result.asset.payload.size();
        } else
            ++loader->metrics_.failed;
        ReleaseSRWLockExclusive(&loader->mutex_);
        promise.set_value(static_cast<AssetLoadResult&&>(result));
    }
};

std::shared_future<AssetLoadResult> AsyncAssetLoader::request(const AssetId id) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::assets);
    AcquireSRWLockExclusive(&mutex_);
    ++metrics_.requests;
    const decltype(requests_)::iterator found = requests_.find(id);
    if (found != requests_.end()) {
        ++metrics_.cache_hits;
        std::shared_future<AssetLoadResult> future = found->second;
        ReleaseSRWLockExclusive(&mutex_);
        return future;
    }
    const AssetRecord* record = catalog_.find(id);
    if (!record) {
        std::promise<AssetLoadResult> promise;
        std::shared_future<AssetLoadResult> future = promise.get_future().share();
        requests_.emplace(id, future);
        ++metrics_.failed;
        promise.set_value(placeholder(id, AssetType::binary, "Asset is absent from the catalog"));
        ReleaseSRWLockExclusive(&mutex_);
        return future;
    }
    LoadContext* context = new LoadContext{.loader = this, .record = *record};
    std::shared_future<AssetLoadResult> future = context->promise.get_future().share();
    requests_.emplace(id, future);
    // A saturated queue may execute another loader job here. Never hold the cache lock
    // across schedule, or assistance would deadlock trying to publish that job's result.
    ReleaseSRWLockExclusive(&mutex_);
    jobs_.schedule(tasks_, core::JobSystem::Job{context});
    return future;
}

bool AsyncAssetLoader::invalidate(const AssetId id) {
    AcquireSRWLockExclusive(&mutex_);
    const decltype(requests_)::iterator found = requests_.find(id);
    if (found == requests_.end()) {
        ReleaseSRWLockExclusive(&mutex_);
        return true;
    }
    if (found->second.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
        ReleaseSRWLockExclusive(&mutex_);
        return false;
    }
    requests_.erase(found);
    ++metrics_.invalidations;
    ReleaseSRWLockExclusive(&mutex_);
    return true;
}

void AsyncAssetLoader::wait() {
    if (tasks_.valid()) {
        jobs_.wait(tasks_);
    }
}

AssetLoaderMetrics AsyncAssetLoader::metrics() const noexcept {
    AcquireSRWLockShared(&mutex_);
    const AssetLoaderMetrics result = metrics_;
    ReleaseSRWLockShared(&mutex_);
    return result;
}

AssetLoadResult AsyncAssetLoader::placeholder(const AssetId id, const AssetType type, std::string error) {
    constexpr std::array<std::byte, 8> placeholder_payload{
        std::byte{'M'}, std::byte{'I'}, std::byte{'S'}, std::byte{'S'}, std::byte{'I'}, std::byte{'N'}, std::byte{'G'}, std::byte{0}};
    return {
        .state = AssetLoadState::error_placeholder,
        .asset = {.id = id, .type = type, .payload = {placeholder_payload.begin(), placeholder_payload.end()}},
        .error = std::move(error),
    };
}

} // namespace gloom::assets
