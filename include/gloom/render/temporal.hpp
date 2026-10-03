#pragma once

#include <cstdint>

namespace gloom::render {

struct RenderExtent {
    std::uint32_t width{1};
    std::uint32_t height{1};
    [[nodiscard]] bool operator==(const RenderExtent&) const noexcept = default;
};

enum class TemporalTechnique : std::uint8_t {
    disabled,
    taa,
};

struct TemporalCapabilities {
    bool motion_vectors{false};
    bool jittered_camera{false};
    bool history_resources{false};
    bool resolution_scaling{false};
    bool taa{false};
    bool depth_disocclusion{false};
    bool reactive_history{false};
    bool contrast_adaptive_sharpening{false};
    bool dynamic_resolution{false};
};

struct DynamicResolutionSettings {
    bool enabled{false};
    float minimum_scale{0.5F};
    float target_frame_milliseconds{16.667F};
    float scale_step{0.05F};
    std::uint32_t settle_frames{30};
};

struct TemporalSettings {
    TemporalTechnique technique{TemporalTechnique::taa};
    float render_scale{1.0F};
    float history_weight{0.9F};
    float sharpness{0.2F};
    DynamicResolutionSettings dynamic_resolution;
};

struct TemporalSelection {
    TemporalTechnique technique{TemporalTechnique::disabled};
    float render_scale{1.0F};
    float history_weight{0.0F};
    float sharpness{0.0F};
    bool fell_back{false};
};

struct DynamicResolutionMetrics {
    float scale{1.0F};
    float filtered_frame_milliseconds{0.0F};
    std::uint64_t scale_changes{0};
    std::uint64_t overload_samples{0};
    std::uint64_t headroom_samples{0};
};

class DynamicResolutionController final {
public:
    explicit DynamicResolutionController(DynamicResolutionSettings settings = {},
                                         float initial_scale = 1.0F) noexcept;
    [[nodiscard]] float update(float gpu_frame_milliseconds) noexcept;
    void reset(float scale) noexcept;
    [[nodiscard]] const DynamicResolutionMetrics& metrics() const noexcept;

private:
    DynamicResolutionSettings settings_;
    DynamicResolutionMetrics metrics_;
    std::uint32_t frames_since_change_{0};
    bool has_sample_{false};
};

struct CameraJitter {
    float x{0.0F};
    float y{0.0F};
    [[nodiscard]] bool operator==(const CameraJitter&) const noexcept = default;
};

[[nodiscard]] TemporalSelection negotiate_temporal_feature(
    TemporalSettings requested,
    const TemporalCapabilities& capabilities) noexcept;

[[nodiscard]] RenderExtent scaled_render_extent(RenderExtent output, float scale) noexcept;
[[nodiscard]] CameraJitter temporal_jitter(std::uint64_t frame_index,
                                           RenderExtent render_extent) noexcept;

} // namespace gloom::render
