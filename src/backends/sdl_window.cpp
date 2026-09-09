#include <gloom/backends/sdl_window.hpp>

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <algorithm>
#include <cstring>

namespace gloom::backends {

struct SdlWindow::Impl {
    SDL_Window* window{nullptr};
    platform::InputState input;
    SDL_Scancode last_move{SDL_SCANCODE_UNKNOWN};
    Uint64 last_move_ms{0};
    platform::DoubleTap space_tap;
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
    SDL_StartTextInput(impl_->window);
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
    std::array<char,256> text{};std::size_t text_size=0;
    bool backspace=false,select_all=false,dodge=false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            return false;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            look_delta_x += event.motion.xrel;
            look_delta_y += event.motion.yrel;
        }
        if(event.type==SDL_EVENT_TEXT_INPUT){
            const auto count=std::min(std::strlen(event.text.text),text.size()-1-text_size);
            std::memcpy(text.data()+text_size,event.text.text,count);text_size+=count;
        }
        if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST)impl_->last_move=SDL_SCANCODE_UNKNOWN;
        if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST)impl_->space_tap.clear();
        if(event.type==SDL_EVENT_KEY_DOWN){
            const auto key=event.key.scancode;
            if(key==SDL_SCANCODE_SPACE && !event.key.repeat && SDL_GetWindowRelativeMouseMode(impl_->window)){
                dodge|=impl_->space_tap.press(SDL_GetTicks());
            }
            if(!event.key.repeat && SDL_GetWindowRelativeMouseMode(impl_->window) &&
               (key==SDL_SCANCODE_W || key==SDL_SCANCODE_A || key==SDL_SCANCODE_S || key==SDL_SCANCODE_D)){
                const auto now=SDL_GetTicks();
                dodge|=impl_->last_move==key && now-impl_->last_move_ms<300;
                impl_->last_move=key;impl_->last_move_ms=now;
            }
            backspace|=event.key.scancode==SDL_SCANCODE_BACKSPACE;
            select_all|=event.key.scancode==SDL_SCANCODE_A && (event.key.mod&SDL_KMOD_CTRL)!=0;
            if(event.key.scancode==SDL_SCANCODE_V && (event.key.mod&SDL_KMOD_CTRL)!=0){
                if(char* clipboard=SDL_GetClipboardText()){
                    const auto count=std::min(std::strlen(clipboard),text.size()-1-text_size);
                    std::memcpy(text.data()+text_size,clipboard,count);text_size+=count;SDL_free(clipboard);
                }
            }
        }
    }
    int key_count = 0;
    const bool* keys = SDL_GetKeyboardState(&key_count);
    const auto pressed = [keys, key_count](const SDL_Scancode key) {
        return static_cast<int>(key) < key_count && keys[key];
    };
    float mouse_x=0,mouse_y=0;
    const SDL_MouseButtonFlags mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    int logical_width=1,logical_height=1;SDL_GetWindowSize(impl_->window,&logical_width,&logical_height);
    const auto [pixel_width,pixel_height]=drawable_size();
    impl_->input = {
        .move_left = pressed(SDL_SCANCODE_A) || pressed(SDL_SCANCODE_LEFT),
        .move_right = pressed(SDL_SCANCODE_D) || pressed(SDL_SCANCODE_RIGHT),
        .move_forward = pressed(SDL_SCANCODE_W) || pressed(SDL_SCANCODE_UP),
        .move_backward = pressed(SDL_SCANCODE_S) || pressed(SDL_SCANCODE_DOWN),
        .jump = pressed(SDL_SCANCODE_SPACE),
        .fire_primary = (mouse_buttons & SDL_BUTTON_LMASK) != 0,
        .fire_secondary = (mouse_buttons & SDL_BUTTON_RMASK) != 0,
        .use_primary_ability = pressed(SDL_SCANCODE_Q),
        .use_secondary_ability = pressed(SDL_SCANCODE_E),
        .menu_previous = pressed(SDL_SCANCODE_LEFT),
        .menu_next = pressed(SDL_SCANCODE_RIGHT),
        .menu_confirm = pressed(SDL_SCANCODE_RETURN) ||
                        pressed(SDL_SCANCODE_KP_ENTER),
        .look_delta_x = look_delta_x,
        .look_delta_y = look_delta_y,
        .mouse_x=mouse_x*static_cast<float>(pixel_width)/static_cast<float>(std::max(1,logical_width)),
        .mouse_y=mouse_y*static_cast<float>(pixel_height)/static_cast<float>(std::max(1,logical_height)),
        .mouse_primary=(mouse_buttons&SDL_BUTTON_LMASK)!=0,
        .menu_back=pressed(SDL_SCANCODE_ESCAPE),.menu_tab=pressed(SDL_SCANCODE_TAB),
        .menu_shift=pressed(SDL_SCANCODE_LSHIFT)||pressed(SDL_SCANCODE_RSHIFT),
        .menu_up=pressed(SDL_SCANCODE_UP),.menu_down=pressed(SDL_SCANCODE_DOWN),
        .focused=(SDL_GetWindowFlags(impl_->window)&SDL_WINDOW_INPUT_FOCUS)!=0,
        .backspace=backspace,.select_all=select_all,.text=text,.dodge=dodge,
        .weapon_selection=static_cast<std::uint8_t>(pressed(SDL_SCANCODE_1)?0:pressed(SDL_SCANCODE_2)?1:pressed(SDL_SCANCODE_3)?2:pressed(SDL_SCANCODE_4)?3:pressed(SDL_SCANCODE_5)?4:255),
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
    if(!enabled){impl_->last_move=SDL_SCANCODE_UNKNOWN;impl_->space_tap.clear();}
    if (!SDL_SetWindowRelativeMouseMode(impl_->window, enabled)) {
        throw std::runtime_error{"SDL failed to change relative mouse mode: " +
                                 std::string{SDL_GetError()}};
    }
}

} // namespace gloom::backends
