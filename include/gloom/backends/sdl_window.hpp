#pragma once

#include <gloom/platform/window.hpp>

#include <memory>

namespace gloom::backends {

class SdlWindow final : public platform::Window {
public:
    explicit SdlWindow(platform::WindowDesc description);
    ~SdlWindow() override;

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&&) = delete;
    SdlWindow& operator=(SdlWindow&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] core::SubsystemState state() const noexcept override;

    void start() override;
    void tick(double delta_seconds) override;
    void stop() noexcept override;

    [[nodiscard]] bool poll_events() override;
    [[nodiscard]] void* native_handle() noexcept override;
    [[nodiscard]] std::pair<std::uint32_t, std::uint32_t> drawable_size() const noexcept override;
    [[nodiscard]] platform::InputState input_state() const noexcept override;
    void set_minimized(bool minimized) override;
    void set_relative_mouse_mode(bool enabled) override;
    void set_title(std::string title) override;

private:
    struct Impl;

    platform::WindowDesc description_;
    std::unique_ptr<Impl> impl_;
    core::SubsystemState state_{core::SubsystemState::stopped};
};

} // namespace gloom::backends
