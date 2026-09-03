#pragma once

#include <gloom/platform/window.hpp>
#include <gloom/render/renderer.hpp>

#include <memory>
#include <filesystem>

namespace gloom::backends {

class DiligentRenderer final : public render::Renderer {
public:
    explicit DiligentRenderer(platform::Window& window, render::RendererSettings settings = {});
    ~DiligentRenderer() override;

    DiligentRenderer(const DiligentRenderer&) = delete;
    DiligentRenderer& operator=(const DiligentRenderer&) = delete;
    DiligentRenderer(DiligentRenderer&&) = delete;
    DiligentRenderer& operator=(DiligentRenderer&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] core::SubsystemState state() const noexcept override;
    [[nodiscard]] render::RenderCapabilities capabilities() const noexcept override;

    void start() override;
    void tick(double delta_seconds) override;
    void stop() noexcept override;

    void resize(std::uint32_t width, std::uint32_t height) override;
    void enqueue(render::MeshUpload upload) override;
    void enqueue(render::TextureUpload upload) override;
    void enqueue(render::MaterialUpload upload) override;
    void release(render::RenderAssetId id) override;
    [[nodiscard]] render::GpuAssetState asset_state(render::RenderAssetId id) const noexcept override;
    [[nodiscard]] render::GpuResidencyMetrics residency_metrics() const noexcept override;
    [[nodiscard]] render::FrameRenderMetrics frame_metrics() const noexcept override;
    void begin_frame() override;
    void draw(const render::RenderSnapshot& snapshot) override;
    void end_frame() override;
    // Diagnostic readback after tone mapping, before presentation. Explicitly
    // stalls the GPU only on the requested frame; normal rendering never reads back.
    void capture_next_frame(std::filesystem::path ppm_path);

private:
    struct Impl;

    void create_scene_resources();
    void create_frame_resources(std::uint32_t width, std::uint32_t height);
    void process_uploads();

    platform::Window& window_;
    render::RendererSettings settings_;
    std::unique_ptr<Impl> impl_;
    core::SubsystemState state_{core::SubsystemState::stopped};
};

} // namespace gloom::backends
