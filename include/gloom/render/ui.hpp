#pragma once
#include <gloom/render/gpu_assets.hpp>
#include <gloom/platform/input.hpp>
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace gloom::render {
struct UiVertex {float x,y,u,v,r,g,b,a;};
struct UiDrawData {RenderAssetId atlas;std::vector<UiVertex> vertices;};
struct UiRect {float x,y,w,h;};
using UiColor=std::array<float,4>;
inline constexpr UiColor ui_ink{.84F,.94F,.91F,1},ui_cyan{.22F,.85F,.81F,1},ui_muted{.48F,.64F,.63F,1};
inline constexpr RenderAssetId ui_atlas_id{0x6400640064ULL};

// Retained focus with an immediate draw list. Coordinates use a centered 1280x720
// safe area; hit testing and drawing share the same physical-pixel transform.
class UiCanvas {
public:
    explicit UiCanvas(const std::filesystem::path& directory);
    [[nodiscard]] TextureUpload atlas_upload() const;
    void begin(std::uint32_t width,std::uint32_t height,const platform::InputState& input);
    void rect(UiRect r,UiColor color);
    void ring(float x,float y,float radius,float fraction,UiColor color,float thickness=3);
    void image(std::string_view name,UiRect r,UiColor color={1,1,1,1});
    void text(float x,float y,std::string_view text,float size=22,UiColor color=ui_ink,bool heading=false);
    void paragraph(float x,float y,std::string_view text,float width,float size=20,UiColor color=ui_muted);
    void panel(UiRect r);
    void ornament(UiRect r);
    void background(std::string_view title,std::string_view subtitle);
    bool button(std::string_view id,UiRect r,std::string_view label,bool enabled=true);
    bool field(std::string_view id,UiRect r,std::string& value,std::size_t limit=128);
    [[nodiscard]] bool back() const noexcept{return escape_;}
    [[nodiscard]] bool text_focused() const noexcept{return text_focused_;}
    [[nodiscard]] const UiDrawData& data() const noexcept{return data_;}
    [[nodiscard]] std::string_view focus() const noexcept{return focus_;}
private:
    struct Sprite {float x,y,w,h,advance;};
    bool control(std::string_view id,UiRect r,bool enabled);
    std::filesystem::path directory_;
    std::unordered_map<std::string,Sprite> sprites_;
    UiDrawData data_{ui_atlas_id,{}};
    std::uint32_t atlas_width_{},atlas_height_{};
    float width_{1280},height_{720},scale_{1},left_{},top_{},mouse_x_{},mouse_y_{};
    platform::InputState previous_{},input_{};
    bool first_{true},click_{},confirm_{},escape_{},text_focused_{};
    std::string focus_;
    std::vector<std::string> controls_,previous_controls_;
};
} // namespace gloom::render
