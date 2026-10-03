#include <gloom/render/temporal.hpp>
#include <gloom/core/types.hpp>

#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main() {
    using namespace gloom::render;
    const TemporalCapabilities native{.motion_vectors = true, .jittered_camera = true, .history_resources = true, .taa = true};
    const TemporalSelection taa = negotiate_temporal_feature({.render_scale = 0.75F, .history_weight = 0.91F}, native);
    assert(taa.technique == TemporalTechnique::taa && !taa.fell_back && taa.render_scale == 0.75F && taa.history_weight == 0.91F);
    const RenderExtent extent = scaled_render_extent({.width = 1920, .height = 1080}, taa.render_scale);
    assert((extent == RenderExtent{.width = 1440, .height = 810}));
    const CameraJitter first = temporal_jitter(0, extent);
    assert(first == temporal_jitter(8, extent) && fabsf(first.x) <= 1.0F / extent.width && fabsf(first.y) <= 1.0F / extent.height);
    const TemporalSelection bounded = negotiate_temporal_feature({.render_scale = 0.1F, .history_weight = 2.0F, .sharpness = -1.0F}, native);
    assert(bounded.render_scale == 0.5F && bounded.history_weight == 0.98F && bounded.sharpness == 0.0F);
    const TemporalSelection off = negotiate_temporal_feature({.technique = TemporalTechnique::disabled, .render_scale = 0.75F}, native);
    assert(off.technique == TemporalTechnique::disabled && !off.fell_back && off.render_scale == 1.0F && off.history_weight == 0.0F && off.sharpness == 0.0F);
    for (gloom::uint32 missing = 0; missing < 4; ++missing) {
        TemporalCapabilities capabilities = native;
        if (missing == 0)
            capabilities.motion_vectors = false;
        if (missing == 1)
            capabilities.jittered_camera = false;
        if (missing == 2)
            capabilities.history_resources = false;
        if (missing == 3)
            capabilities.taa = false;
        const TemporalSelection fallback = negotiate_temporal_feature({}, capabilities);
        assert(fallback.technique == TemporalTechnique::disabled && fallback.fell_back && fallback.render_scale == 1.0F);
        assert(fallback.history_weight == 0.0F && fallback.sharpness == 0.0F);
    }
    assert((scaled_render_extent({.width = 0, .height = 720}, 0.75F) == RenderExtent{.width = 0, .height = 0}));
    assert((temporal_jitter(1, {.width = 0, .height = 0}) == CameraJitter{}));
    DynamicResolutionController controller{
        {.enabled = true, .minimum_scale = 0.6F, .target_frame_milliseconds = 10.0F, .scale_step = 0.1F, .settle_frames = 2}, 1.0F};
    assert(fabsf(controller.update(20.0F) - 1.0F) < 0.0001F && fabsf(controller.update(20.0F) - 0.9F) < 0.0001F);
    assert(fabsf(controller.update(20.0F) - 0.9F) < 0.0001F && fabsf(controller.update(20.0F) - 0.8F) < 0.0001F);
    for (gloom::uint32 sample = 0; sample < 80; ++sample)
        static_cast<void>(controller.update(2.0F));
    assert(controller.metrics().scale > 0.8F && controller.metrics().scale <= 1.0F && controller.metrics().scale_changes >= 3);
    assert(controller.metrics().overload_samples >= 2 && controller.metrics().headroom_samples > 0);
    puts("Gloom temporal render-feature tests completed successfully.");
}
