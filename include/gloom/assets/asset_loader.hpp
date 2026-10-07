#pragma once
#include <gloom/assets/asset.hpp>
#include <gloom/core/job_system.hpp>

namespace gloom::assets {
enum class AssetLoadState : uint8 { ready, error_placeholder };
struct AssetLoadResult {
    AssetLoadState state{AssetLoadState::error_placeholder};
    CookedAsset asset;
    Array<char> error;
};
struct AssetLoaderMetrics {
    uint64 requests{0}, cache_hits{0}, loaded{0}, failed{0}, bytes_loaded{0}, invalidations{0}, stale_completions{0};
};
struct AssetLoadTicket {
    uint32 index{0}, generation{0};
    [[nodiscard]] bool operator==(const AssetLoadTicket&) const noexcept = default;
};
class AsyncAssetLoader;
// Owns a pin on an immutable result. The loader outlives all handles; copies
// coalesce, moves transfer the pin. get requires ready(); it never waits on the frame.
class AssetLoadHandle {
  public:
    AssetLoadHandle() = default;
    ~AssetLoadHandle();
    AssetLoadHandle(const AssetLoadHandle& other);
    AssetLoadHandle& operator=(const AssetLoadHandle& other);
    AssetLoadHandle(AssetLoadHandle&& other) noexcept;
    AssetLoadHandle& operator=(AssetLoadHandle&& other) noexcept;
    [[nodiscard]] bool ready() const;
    [[nodiscard]] const AssetLoadResult& get() const;
    [[nodiscard]] AssetLoadTicket ticket() const noexcept {
        return ticket_;
    }

  private:
    AssetLoadHandle(AsyncAssetLoader* owner, AssetLoadTicket ticket) : owner_{owner}, ticket_{ticket} {}
    AsyncAssetLoader* owner_{nullptr};
    AssetLoadTicket ticket_;
    friend class AsyncAssetLoader;
};
// Copy an external error at the data boundary; output owns its terminating NUL.
void set_asset_error(Array<char>& output, const char* message);
class AsyncAssetLoader final {
  public:
    AsyncAssetLoader(core::JobSystem& jobs, const VirtualFileSystem& filesystem, const AssetCatalog& catalog);
    ~AsyncAssetLoader();
    AsyncAssetLoader(const AsyncAssetLoader&) = delete;
    AsyncAssetLoader& operator=(const AsyncAssetLoader&) = delete;
    [[nodiscard]] AssetLoadHandle request(AssetId id);
    // Catalog mutation is serialized with request(); executing jobs own metadata snapshots.
    // Retire the cached generation immediately, including an executing load.
    // Existing pins stay valid; a subsequent request receives a fresh generation.
    [[nodiscard]] bool invalidate(AssetId id);
    // Join producers before wait/destruction; catalog/VFS outlive their jobs.
    void wait();
    [[nodiscard]] AssetLoaderMetrics metrics() const noexcept;

  private:
    struct Slot;
    struct LoadContext;
    struct Storage {
        Slot* slot{nullptr};
        uint32 generation{0}, next{0xffffffffU};
    };
    struct CacheEntry {
        AssetId id;
        uint32 index{0};
    };
    size_t lower_bound(AssetId id) const;
    Slot& slot_locked(AssetLoadTicket ticket) const;
    void dispose_locked(uint32 index);
    void retain(AssetLoadTicket ticket);
    void release(AssetLoadTicket ticket);
    bool ready(AssetLoadTicket ticket) const;
    const AssetLoadResult& result(AssetLoadTicket ticket) const;
    void publish(AssetLoadTicket ticket, AssetLoadResult result);
    static AssetLoadResult placeholder(AssetId id, AssetType type, const char* error);
    core::JobSystem& jobs_;
    const VirtualFileSystem& filesystem_;
    const AssetCatalog& catalog_;
    core::TaskGroup tasks_;
    mutable SRWLOCK mutex_{SRWLOCK_INIT};
    Array<Storage> slots_;
    Array<CacheEntry> cache_;
    uint32 free_{0xffffffffU};
    AssetLoaderMetrics metrics_;
    friend class AssetLoadHandle;
};
}
