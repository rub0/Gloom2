#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace gloom::render {

// Slots: base, metallic/roughness, normal, emission, AO, specular weight,
// specular color, anisotropy, detail color, lightmap.
inline constexpr std::size_t material_texture_count = 10;
struct TextureMapping {
    std::array<float, 2> scale{1.0F, 1.0F};
    std::array<float, 2> offset{};
    float rotation{0.0F};
    std::uint32_t uv_set{0};
};

struct MaterialSurface {
    std::array<float, 3> specular_color{1.0F, 1.0F, 1.0F};
    float specular_factor{1.0F};
    float normal_scale{1.0F};
    float occlusion_strength{1.0F};
    float anisotropy_strength{0.0F};
    float anisotropy_rotation{0.0F};
    std::array<float, 2> uv_scroll{};
    float lava_wave{0.0F};
    float alpha_cutoff{0.5F};
    std::uint32_t alpha_mode{0}; // opaque, mask, blend, additive
    bool double_sided{false};
    std::array<TextureMapping, material_texture_count> mapping{};
};

inline bool valid_material_surface(const MaterialSurface& value) noexcept {
    for (const auto number : value.specular_color)
        if (!std::isfinite(number) || number < 0.0F || number > 1.0F) return false;
    for (const auto number : {value.specular_factor, value.normal_scale,
                             value.occlusion_strength, value.anisotropy_strength,
                             value.anisotropy_rotation, value.uv_scroll[0],
                             value.uv_scroll[1], value.lava_wave, value.alpha_cutoff})
        if (!std::isfinite(number)) return false;
    if (value.specular_factor < 0 || value.specular_factor > 1 ||
        value.occlusion_strength < 0 || value.occlusion_strength > 1 ||
        value.anisotropy_strength < 0 || value.anisotropy_strength > 1 ||
        value.normal_scale < 0 || value.lava_wave < 0 || value.alpha_mode > 3 ||
        value.alpha_cutoff < 0 || value.alpha_cutoff > 1) return false;
    for (const auto& map : value.mapping) {
        if (map.uv_set > 1) return false;
        for (const auto number : {map.scale[0], map.scale[1], map.offset[0],
                                 map.offset[1], map.rotation})
            if (!std::isfinite(number)) return false;
    }
    return true;
}
} // namespace gloom::render
