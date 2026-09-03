#pragma once

#include <gloom/core/subsystem.hpp>
#include <gloom/render/gpu_assets.hpp>
#include <gloom/render/scene.hpp>
#include <gloom/render/temporal.hpp>

#include <cstdint>

namespace gloom::render {

enum class GraphicsApi {
    vulkan,
    direct3d12,
};

struct RenderCapabilities {
    GraphicsApi api{GraphicsApi::vulkan};
    bool async_compute{false};
    bool mesh_shaders{false};
    bool ray_tracing{false};
    bool variable_rate_shading{false};
    bool temporal_upscaling{false};
    bool texture_compression_bc{false};
    bool gpu_timestamps{false};
    TemporalCapabilities temporal;
};

struct FrameRenderMetrics {
    bool gpu_timing_supported{false};
    std::uint64_t frame_index{0};
    std::uint64_t shadow_nanoseconds{0};
    std::uint64_t opaque_nanoseconds{0};
    std::uint64_t tone_map_nanoseconds{0};
    std::uint64_t temporal_resolve_nanoseconds{0};
    std::uint64_t point_lights{0};
    std::uint64_t light_references{0};
    std::uint32_t render_width{0};
    std::uint32_t render_height{0};
    std::uint32_t output_width{0};
    std::uint32_t output_height{0};
    float render_scale{1.0F};
    float filtered_gpu_frame_milliseconds{0.0F};
    std::uint64_t dynamic_resolution_changes{0};
    bool temporal_history_valid{false};
};

struct RendererSettings {
    bool vertical_sync{true};
    std::uint64_t upload_budget_bytes_per_frame{16U * 1024U * 1024U};
    std::uint64_t resident_budget_bytes{512U * 1024U * 1024U};
    TemporalSettings temporal;
};

// Gloom owns this API. Diligent is the first interchangeable implementation.
class Renderer : public core::Subsystem {
public:
    [[nodiscard]] virtual RenderCapabilities capabilities() const noexcept = 0;
    virtual void resize(std::uint32_t width, std::uint32_t height) = 0;
    virtual void enqueue(MeshUpload upload) = 0;
    virtual void enqueue(TextureUpload upload) = 0;
    virtual void enqueue(MaterialUpload upload) = 0;
    virtual void release(RenderAssetId id) = 0;
    [[nodiscard]] virtual GpuAssetState asset_state(RenderAssetId id) const noexcept = 0;
    [[nodiscard]] virtual GpuResidencyMetrics residency_metrics() const noexcept = 0;
    [[nodiscard]] virtual FrameRenderMetrics frame_metrics() const noexcept { return {}; }
    virtual void begin_frame() = 0;
    virtual void draw(const RenderSnapshot& snapshot) = 0;
    virtual void end_frame() = 0;
};

} // namespace gloom::render
