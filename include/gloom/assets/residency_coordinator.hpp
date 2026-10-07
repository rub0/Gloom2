#pragma once
#include <gloom/assets/asset_loader.hpp>
#include <gloom/assets/animation.hpp>
#include <gloom/render/renderer.hpp>

namespace gloom::assets {
struct SceneTicket {
    uint64 value{0};
    [[nodiscard]] bool operator==(const SceneTicket&) const noexcept = default;
};
enum class AssetPriority : uint8 { background, normal, high, critical };
enum class SceneResidencyState : uint8 { queued, loading_scene, loading_dependencies, preparing, uploading, ready, failed, cancelled };
// Owned by a stable request. Instance/rig views expire on reload, cancel or coordinator
// destruction; callers must retire their CPU views before those operations.
struct ResidentScene {
    AssetId asset;
    uint64 generation{0};
    Array<render::RenderInstance> instances;
    AnimationRig animation_rig;
};
struct ResidencyCoordinatorSettings {
    uint32 new_scene_requests_per_update{2};
};
struct ResidencyCoordinatorMetrics {
    uint64 requested{0}, dispatched{0}, cancelled{0}, reloaded{0}, ready{0}, failed{0}, shared_resource_hits{0}, stale_preparations{0};
};
// Context outlives coordinator and drained tasks; uploads transfer ownership.
// release cancels queued uploads and retires executing GPU use according to the backend.
struct AssetUploadSink {
    void* context{nullptr};
    bool (*texture_compression_bc)(const void*){nullptr};
    void (*mesh)(void*, render::MeshUpload){nullptr};
    void (*texture)(void*, render::TextureUpload){nullptr};
    void (*material)(void*, render::MaterialUpload){nullptr};
    void (*release)(void*, render::RenderAssetId){nullptr};
    render::GpuAssetState (*state)(const void*, render::RenderAssetId){nullptr};
};
class AssetResidencyCoordinator final {
  public:
    AssetResidencyCoordinator(
        core::JobSystem& jobs, AsyncAssetLoader& loader, const AssetCatalog& catalog, render::Renderer& renderer, ResidencyCoordinatorSettings settings = {});
    AssetResidencyCoordinator(
        core::JobSystem& jobs, AsyncAssetLoader& loader, const AssetCatalog& catalog, AssetUploadSink renderer, ResidencyCoordinatorSettings settings = {});
    ~AssetResidencyCoordinator();
    AssetResidencyCoordinator(const AssetResidencyCoordinator&) = delete;
    AssetResidencyCoordinator& operator=(const AssetResidencyCoordinator&) = delete;
    [[nodiscard]] SceneTicket request_scene(AssetId scene, AssetPriority priority = AssetPriority::normal);
    void cancel(SceneTicket ticket);
    void reload(SceneTicket ticket);
    void update();
    // All public operations are serialized by the consumer. Jobs only publish under publication_.
    // state requires a ticket from this coordinator; invalid tickets assert.
    [[nodiscard]] SceneResidencyState state(SceneTicket ticket) const;
    [[nodiscard]] const ResidentScene* scene(SceneTicket ticket) const noexcept;
    [[nodiscard]] const char* error(SceneTicket ticket) const noexcept;
    [[nodiscard]] ResidencyCoordinatorMetrics metrics() const noexcept;

  private:
    struct PreparedScene;
    struct PreparationContext;
    struct Request;
    struct Reference {
        render::RenderAssetId id;
        uint32 count{0};
    };
    struct Queued {
        uint64 ticket{0};
        Request* request{nullptr};
    };
    Request* find(SceneTicket ticket) const;
    size_t resource_position(render::RenderAssetId id) const;
    void release_resources(Request& request);
    void discard(Request& request);
    void fail(Request& request, const char* error);
    core::JobSystem& jobs_;
    AsyncAssetLoader& loader_;
    const AssetCatalog& catalog_;
    AssetUploadSink renderer_;
    ResidencyCoordinatorSettings settings_;
    core::TaskGroup preparation_tasks_;
    mutable SRWLOCK publication_{SRWLOCK_INIT};
    Array<Request*> requests_;
    Array<Reference> references_;
    Array<Queued> queued_;
    ResidencyCoordinatorMetrics metrics_;
};
}
