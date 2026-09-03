#include <gloom/render/temporal.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

class FsrHook final : public gloom::render::TemporalFeatureHook {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "test FSR"; }
    [[nodiscard]] gloom::render::TemporalTechnique technique() const noexcept override {
        return gloom::render::TemporalTechnique::fsr;
    }
    [[nodiscard]] bool available(
        const gloom::render::TemporalCapabilities& capabilities) const noexcept override {
        return capabilities.fsr;
    }
};

} // namespace

int main() try {
    const gloom::render::TemporalCapabilities native{.motion_vectors = true,
                                                      .jittered_camera = true,
                                                      .history_resources = true,
                                                      .resolution_scaling = true,
                                                      .taa = true};
    const auto taa = gloom::render::negotiate_temporal_feature(
        {.technique = gloom::render::TemporalTechnique::taa,
         .render_scale = 0.75F,
         .history_weight = 0.91F},
        native);
    require(taa.technique == gloom::render::TemporalTechnique::taa && !taa.fell_back &&
                taa.render_scale == 0.75F && taa.history_weight == 0.91F,
            "Native TAA negotiation changed a supported request");
    const auto extent = gloom::render::scaled_render_extent({1920, 1080}, taa.render_scale);
    require(extent == gloom::render::RenderExtent{1440, 810},
            "Render/output resolution separation is incorrect");
    const auto first = gloom::render::temporal_jitter(0, extent);
    const auto ninth = gloom::render::temporal_jitter(8, extent);
    require(first.x == ninth.x && first.y == ninth.y &&
                std::abs(first.x) <= 1.0F / extent.width &&
                std::abs(first.y) <= 1.0F / extent.height,
            "Halton jitter is not bounded and repeatable");

    const FsrHook fsr_hook;
    const gloom::render::TemporalFeatureHook* hooks[]{&fsr_hook};
    auto vendor = native;
    vendor.fsr = true;
    const auto fsr = gloom::render::negotiate_temporal_feature(
        {.technique = gloom::render::TemporalTechnique::fsr, .render_scale = 0.67F},
        vendor,
        hooks);
    require(fsr.technique == gloom::render::TemporalTechnique::fsr && fsr.hook == &fsr_hook &&
                !fsr.fell_back,
            "Interchangeable temporal hook was not selected");
    const auto fallback = gloom::render::negotiate_temporal_feature(
        {.technique = gloom::render::TemporalTechnique::dlss, .render_scale = 0.1F}, native);
    require(fallback.technique == gloom::render::TemporalTechnique::taa && fallback.fell_back &&
                fallback.render_scale == 0.5F,
            "Unavailable vendor feature did not fall back to native TAA");
    const auto disabled = gloom::render::negotiate_temporal_feature(
        {.technique = gloom::render::TemporalTechnique::taa}, {});
    require(disabled.technique == gloom::render::TemporalTechnique::disabled &&
                disabled.render_scale == 1.0F && disabled.sharpness == 0.0F,
            "Unsupported temporal path was not disabled safely");
    require(gloom::render::scaled_render_extent({0, 720}, 0.75F) ==
                gloom::render::RenderExtent{0, 0} &&
                gloom::render::temporal_jitter(1, {0, 0}) == gloom::render::CameraJitter{},
            "Minimized output did not suppress temporal dimensions and jitter");

    gloom::render::DynamicResolutionController controller{
        {.enabled = true,
         .minimum_scale = 0.6F,
         .target_frame_milliseconds = 10.0F,
         .scale_step = 0.1F,
         .settle_frames = 2},
        1.0F};
    const auto near = [](const float left, const float right) {
        return std::abs(left - right) < 0.0001F;
    };
    require(near(controller.update(20.0F), 1.0F) &&
                near(controller.update(20.0F), 0.9F) &&
                near(controller.update(20.0F), 0.9F) &&
                near(controller.update(20.0F), 0.8F),
            "Dynamic resolution did not apply overload hysteresis");
    for (int sample = 0; sample < 80; ++sample) {
        static_cast<void>(controller.update(2.0F));
    }
    require(controller.metrics().scale > 0.8F && controller.metrics().scale <= 1.0F &&
                controller.metrics().scale_changes >= 3 &&
                controller.metrics().overload_samples >= 2 &&
                controller.metrics().headroom_samples > 0,
            "Dynamic resolution did not recover when GPU headroom returned");
    std::cout << "Gloom temporal render-feature tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Temporal render-feature test failure: " << error.what() << '\n';
    return 1;
}
