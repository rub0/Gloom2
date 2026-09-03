#pragma once

#include <gloom/core/subsystem.hpp>
#include <gloom/platform/input.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace gloom::platform {

struct WindowDesc {
    std::string title{"Gloom"};
    std::uint32_t width{1280};
    std::uint32_t height{720};
    bool resizable{true};
};

// SDL3 will implement this interface without leaking SDL types to the engine.
class Window : public core::Subsystem {
public:
    [[nodiscard]] virtual bool poll_events() = 0;
    [[nodiscard]] virtual void* native_handle() noexcept = 0;
    [[nodiscard]] virtual std::pair<std::uint32_t, std::uint32_t> drawable_size() const noexcept = 0;
    [[nodiscard]] virtual InputState input_state() const noexcept = 0;
    virtual void set_minimized(bool minimized) = 0;
    virtual void set_relative_mouse_mode(bool enabled) = 0;
    virtual void set_title(std::string title) = 0;
};

} // namespace gloom::platform
