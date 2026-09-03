#pragma once

#include <gloom/assets/asset_loader.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/core/job_system.hpp>
#include <gloom/render/renderer.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gloom::assets {

struct SceneTicket {
    std::uint64_t value{0};
    [[nodiscard]] bool operator==(const SceneTicket&) const noexcept = default;
};

enum class AssetPriority : std::uint8_t { background, normal, high, critical };
enum class SceneResidencyState : std::uint8_t {
    queued,
    loading_scene,
    loading_dependencies,
    preparing,
    uploading,
    ready,
    failed,
    cancelled,
};

struct ResidentScene {
    AssetId asset;
    std::uint64_t generation{0};
    std::vector<render::RenderInstance> instances;
    std::shared_ptr<const ImportedScene> bind_rig;
};

struct ResidencyCoordinatorSettings {
    std::uint32_t new_scene_requests_per_update{2};
};

struct ResidencyCoordinatorMetrics {
    std::uint64_t requested{0};
    std::uint64_t dispatched{0};
    std::uint64_t cancelled{0};
    std::uint64_t reloaded{0};
    std::uint64_t ready{0};
    std::uint64_t failed{0};
    std::uint64_t shared_resource_hits{0};
};

class AssetResidencyCoordinator final {
public:
    AssetResidencyCoordinator(core::JobSystem& jobs,
                              AsyncAssetLoader& loader,
                              const AssetCatalog& catalog,
                              render::Renderer& renderer,
                              ResidencyCoordinatorSettings settings = {});
    ~AssetResidencyCoordinator();

    AssetResidencyCoordinator(const AssetResidencyCoordinator&) = delete;
    AssetResidencyCoordinator& operator=(const AssetResidencyCoordinator&) = delete;

    [[nodiscard]] SceneTicket request_scene(
        AssetId scene, AssetPriority priority = AssetPriority::normal);
    void cancel(SceneTicket ticket);
    void reload(SceneTicket ticket);
    void update();

    [[nodiscard]] SceneResidencyState state(SceneTicket ticket) const;
    [[nodiscard]] const ResidentScene* scene(SceneTicket ticket) const noexcept;
    [[nodiscard]] std::string_view error(SceneTicket ticket) const noexcept;
    [[nodiscard]] ResidencyCoordinatorMetrics metrics() const noexcept;

private:
    struct PreparedScene;
    struct Request;

    void release_resources(Request& request);
    void fail(Request& request, std::string error);

    core::JobSystem& jobs_;
    AsyncAssetLoader& loader_;
    const AssetCatalog& catalog_;
    render::Renderer& renderer_;
    ResidencyCoordinatorSettings settings_;
    core::TaskGroup preparation_tasks_;
    std::unordered_map<std::uint64_t, Request> requests_;
    std::unordered_map<render::RenderAssetId,
                       std::uint32_t,
                       render::RenderAssetIdHash>
        resource_references_;
    ResidencyCoordinatorMetrics metrics_;
    std::uint64_t next_ticket_{1};
};

} // namespace gloom::assets
