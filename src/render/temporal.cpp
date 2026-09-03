#include <gloom/render/temporal.hpp>

#include <algorithm>
#include <cmath>

namespace gloom::render {
namespace {

[[nodiscard]] float radical_inverse(std::uint64_t index, const std::uint64_t base) noexcept {
    float result = 0.0F;
    float fraction = 1.0F / static_cast<float>(base);
    while (index != 0) {
        result += static_cast<float>(index % base) * fraction;
        index /= base;
        fraction /= static_cast<float>(base);
    }
    return result;
}

[[nodiscard]] bool built_in_available(const TemporalTechnique technique,
                                      const TemporalCapabilities& capabilities) noexcept {
    switch (technique) {
    case TemporalTechnique::disabled:
        return true;
    case TemporalTechnique::taa:
        return capabilities.motion_vectors && capabilities.jittered_camera &&
               capabilities.history_resources && capabilities.taa;
    case TemporalTechnique::fsr:
        return capabilities.fsr;
    case TemporalTechnique::dlss:
        return capabilities.dlss;
    }
    return false;
}

} // namespace

TemporalSelection negotiate_temporal_feature(
    TemporalSettings requested,
    const TemporalCapabilities& capabilities,
    const std::span<const TemporalFeatureHook* const> hooks) noexcept {
    requested.render_scale = std::clamp(requested.render_scale, 0.5F, 1.0F);
    requested.history_weight = std::clamp(requested.history_weight, 0.0F, 0.98F);
    TemporalSelection result{.technique = requested.technique,
                             .render_scale = requested.render_scale,
                             .history_weight = requested.history_weight,
                             .sharpness = std::clamp(requested.sharpness, 0.0F, 1.0F)};
    if (requested.technique == TemporalTechnique::disabled) {
        result.render_scale = 1.0F;
        result.history_weight = 0.0F;
        result.sharpness = 0.0F;
        return result;
    }
    if (requested.technique == TemporalTechnique::taa &&
        built_in_available(requested.technique, capabilities)) {
        return result;
    }
    for (const auto* hook : hooks) {
        if (hook != nullptr && hook->technique() == requested.technique &&
            hook->available(capabilities)) {
            result.hook = hook;
            return result;
        }
    }
    result.fell_back = true;
    result.hook = nullptr;
    if (built_in_available(TemporalTechnique::taa, capabilities)) {
        result.technique = TemporalTechnique::taa;
        return result;
    }
    result.technique = TemporalTechnique::disabled;
    result.render_scale = 1.0F;
    result.history_weight = 0.0F;
    result.sharpness = 0.0F;
    return result;
}

DynamicResolutionController::DynamicResolutionController(DynamicResolutionSettings settings,
                                                         const float initial_scale) noexcept
    : settings_{settings} {
    settings_.minimum_scale = std::clamp(settings_.minimum_scale, 0.5F, 1.0F);
    settings_.target_frame_milliseconds =
        std::clamp(settings_.target_frame_milliseconds, 1.0F, 1000.0F);
    settings_.scale_step = std::clamp(settings_.scale_step, 0.01F, 0.25F);
    settings_.settle_frames = std::max(1U, settings_.settle_frames);
    reset(initial_scale);
}

float DynamicResolutionController::update(const float gpu_frame_milliseconds) noexcept {
    if (!settings_.enabled || !std::isfinite(gpu_frame_milliseconds) ||
        gpu_frame_milliseconds <= 0.0F) {
        return metrics_.scale;
    }
    metrics_.filtered_frame_milliseconds =
        has_sample_ ? std::lerp(metrics_.filtered_frame_milliseconds,
                               gpu_frame_milliseconds,
                               0.15F)
                    : gpu_frame_milliseconds;
    has_sample_ = true;
    ++frames_since_change_;
    if (frames_since_change_ < settings_.settle_frames) {
        return metrics_.scale;
    }
    const float overload_threshold = settings_.target_frame_milliseconds * 1.05F;
    const float headroom_threshold = settings_.target_frame_milliseconds * 0.80F;
    float next = metrics_.scale;
    if (metrics_.filtered_frame_milliseconds > overload_threshold) {
        ++metrics_.overload_samples;
        next -= settings_.scale_step;
    } else if (metrics_.filtered_frame_milliseconds < headroom_threshold) {
        ++metrics_.headroom_samples;
        next += settings_.scale_step;
    }
    next = std::clamp(next, settings_.minimum_scale, 1.0F);
    if (std::abs(next - metrics_.scale) >= 0.005F) {
        metrics_.scale = next;
        ++metrics_.scale_changes;
        frames_since_change_ = 0;
    }
    return metrics_.scale;
}

void DynamicResolutionController::reset(const float scale) noexcept {
    metrics_.scale = std::clamp(scale, settings_.minimum_scale, 1.0F);
    metrics_.filtered_frame_milliseconds = 0.0F;
    frames_since_change_ = 0;
    has_sample_ = false;
}

const DynamicResolutionMetrics& DynamicResolutionController::metrics() const noexcept {
    return metrics_;
}

RenderExtent scaled_render_extent(const RenderExtent output, const float scale) noexcept {
    if (output.width == 0 || output.height == 0) {
        return {0, 0};
    }
    const float bounded = std::clamp(scale, 0.5F, 1.0F);
    return {
        .width = std::max(1U, static_cast<std::uint32_t>(std::lround(output.width * bounded))),
        .height = std::max(1U, static_cast<std::uint32_t>(std::lround(output.height * bounded))),
    };
}

CameraJitter temporal_jitter(const std::uint64_t frame_index,
                             const RenderExtent render_extent) noexcept {
    if (render_extent.width == 0 || render_extent.height == 0) {
        return {};
    }
    const std::uint64_t sample = frame_index % 8U + 1U;
    return {
        .x = (radical_inverse(sample, 2U) - 0.5F) * 2.0F /
             static_cast<float>(render_extent.width),
        .y = (radical_inverse(sample, 3U) - 0.5F) * 2.0F /
             static_cast<float>(render_extent.height),
    };
}

} // namespace gloom::render
