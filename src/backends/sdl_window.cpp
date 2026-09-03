#include <gloom/backends/sdl_window.hpp>

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace gloom::backends {

struct SdlWindow::Impl {
    SDL_Window* window{nullptr};
    platform::InputState input;
};

SdlWindow::SdlWindow(platform::WindowDesc description)
    : description_{std::move(description)}, impl_{std::make_unique<Impl>()} {}

SdlWindow::~SdlWindow() {
    stop();
}

std::string_view SdlWindow::name() const noexcept {
    return "platform.sdl3";
}

core::SubsystemState SdlWindow::state() const noexcept {
    return state_;
}

void SdlWindow::start() {
    if (state_ == core::SubsystemState::running) {
        throw std::logic_error{"SDL window is already running"};
    }

    impl_->input = {};
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error{"SDL video initialization failed: " + std::string{SDL_GetError()}};
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (description_.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    impl_->window = SDL_CreateWindow(
        description_.title.c_str(),
        static_cast<int>(description_.width),
        static_cast<int>(description_.height),
        flags);

    if (impl_->window == nullptr) {
        const std::string error = SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw std::runtime_error{"SDL window creation failed: " + error};
    }

    state_ = core::SubsystemState::running;
}

void SdlWindow::tick([[maybe_unused]] const double delta_seconds) {}

void SdlWindow::stop() noexcept {
    if (impl_->window != nullptr) {
        SDL_DestroyWindow(impl_->window);
        impl_->window = nullptr;
    }
    if (state_ == core::SubsystemState::running) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    impl_->input = {};
    state_ = core::SubsystemState::stopped;
}

bool SdlWindow::poll_events() {
    SDL_Event event;
    float look_delta_x = 0.0F;
    float look_delta_y = 0.0F;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            return false;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            look_delta_x += event.motion.xrel;
            look_delta_y += event.motion.yrel;
        }
    }
    int key_count = 0;
    const bool* keys = SDL_GetKeyboardState(&key_count);
    const auto pressed = [keys, key_count](const SDL_Scancode key) {
        return static_cast<int>(key) < key_count && keys[key];
    };
    const SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(nullptr, nullptr);
    impl_->input = {
        .move_left = pressed(SDL_SCANCODE_A) || pressed(SDL_SCANCODE_LEFT),
        .move_right = pressed(SDL_SCANCODE_D) || pressed(SDL_SCANCODE_RIGHT),
        .move_forward = pressed(SDL_SCANCODE_W) || pressed(SDL_SCANCODE_UP),
        .move_backward = pressed(SDL_SCANCODE_S) || pressed(SDL_SCANCODE_DOWN),
        .jump = pressed(SDL_SCANCODE_SPACE),
        .fire_primary = (mouse_buttons & SDL_BUTTON_LMASK) != 0,
        .use_primary_ability = pressed(SDL_SCANCODE_Q) ||
                               (mouse_buttons & SDL_BUTTON_RMASK) != 0,
        .menu_previous = pressed(SDL_SCANCODE_LEFT),
        .menu_next = pressed(SDL_SCANCODE_RIGHT),
        .menu_confirm = pressed(SDL_SCANCODE_RETURN) ||
                        pressed(SDL_SCANCODE_KP_ENTER),
        .look_delta_x = look_delta_x,
        .look_delta_y = look_delta_y,
    };
    return true;
}

void* SdlWindow::native_handle() noexcept {
#if defined(SDL_PLATFORM_WINDOWS)
    if (impl_->window != nullptr) {
        const auto properties = SDL_GetWindowProperties(impl_->window);
        return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    }
#endif
    return nullptr;
}

std::pair<std::uint32_t, std::uint32_t> SdlWindow::drawable_size() const noexcept {
    int width = 0;
    int height = 0;
    if (impl_->window != nullptr) {
        SDL_GetWindowSizeInPixels(impl_->window, &width, &height);
    }
    return {
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
    };
}

platform::InputState SdlWindow::input_state() const noexcept {
    return impl_->input;
}

void SdlWindow::set_title(std::string title) {
    description_.title = std::move(title);
    if (impl_->window != nullptr && !SDL_SetWindowTitle(impl_->window, description_.title.c_str())) {
        throw std::runtime_error{"SDL window title update failed: " +
                                 std::string{SDL_GetError()}};
    }
}

void SdlWindow::set_minimized(const bool minimized) {
    if (impl_->window == nullptr) {
        throw std::logic_error{"SDL window must be running before changing its minimized state"};
    }

    const bool succeeded = minimized ? SDL_MinimizeWindow(impl_->window) : SDL_RestoreWindow(impl_->window);
    if (!succeeded) {
        throw std::runtime_error{"SDL failed to change the window state: " + std::string{SDL_GetError()}};
    }
}

void SdlWindow::set_relative_mouse_mode(const bool enabled) {
    if (impl_->window == nullptr) {
        throw std::logic_error{"SDL window must be running before changing mouse mode"};
    }
    if (!SDL_SetWindowRelativeMouseMode(impl_->window, enabled)) {
        throw std::runtime_error{"SDL failed to change relative mouse mode: " +
                                 std::string{SDL_GetError()}};
    }
}

} // namespace gloom::backends
