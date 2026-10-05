#pragma once
#include <gloom/render/scene.hpp>

namespace gloom::render {
struct ParticleRecipe {
    char name[65]{};
    uint32 material_slot{0}, burst{0};
    float rate{0}, life{1}, speed{0}, spread{0}, gravity{0};
    float size_start{.1F}, size_end{.2F}, trail_seconds{0}, distortion{0};
    Color color_start, color_end{1, 1, 1, 0};
};
// Null means success. Invalid external JSON/file data returns an error and leaves recipes unchanged.
[[nodiscard]] const char* load_particle_recipes(const char* path, Array<ParticleRecipe>& recipes);
[[nodiscard]] bool valid_particle_recipe(const ParticleRecipe& recipe) noexcept;
struct ParticleMetrics {
    uint64 spawned{0}, expired{0}, dropped{0};
};
struct Particle {
    uint64 owner{0};
    uint32 recipe{0};
    double birth{0};
    Vec3 position, velocity;
    float rotation{0};
    bool view_model{false};
};
class ParticleSystem {
  public:
    // Valid, unique recipes and capacity 1..16384 are programmer preconditions.
    // Copies recipes once; no input span is retained.
    explicit ParticleSystem(Span<const ParticleRecipe> recipes, size_t capacity = 2048);
    // IDs are stable attachment IDs; owner identifies the whole entity lifetime, never a recycled index.
    // Missing refresh ends an emitter after this step. Known name/rate>0, finite inputs and unchanged
    // recipe/owner on refresh are preconditions. A recipe is resolved only when creating the emitter.
    void emitter(uint64 id, uint64 owner, const char* recipe, Vec3 position, Vec3 direction = {0, 1, 0});
    void burst(uint64 owner, const char* recipe, Vec3 position, Vec3 direction, uint64 seed, bool view_model = false);
    [[nodiscard]] bool translate(uint64 owner, Vec3 offset) noexcept;
    // Finite seconds in [0,10]. Birth ordering/expiry preserve saturation at different refresh frequencies.
    void advance(double seconds);
    void cancel(uint64 owner);
    void clear() noexcept;
    // Appends to caller-owned scene storage, preserving its prefix. Materials must not overlap result;
    // their span is borrowed only during this call. Reserve scene maximum + capacity() before frames.
    // Result views expire on growth/resize/destruction; keep storage intact through visibility/end_frame.
    void render(Array<RenderInstance>& result, const Camera& camera, Span<const RenderInstance> materials) const;
    [[nodiscard]] Span<const Particle> particles() const noexcept {
        return {particles_.data(), particles_.size()};
    }
    [[nodiscard]] const ParticleMetrics& metrics() const noexcept {
        return metrics_;
    }
    [[nodiscard]] double time() const noexcept {
        return time_;
    }
    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

  private:
    struct Emitter {
        uint64 id{0}, owner{0}, serial{0};
        uint32 recipe{0};
        Vec3 previous, position, direction;
        double next{0};
        bool refreshed{true};
    };
    uint32 find(const char* name) const;
    void spawn(uint64 owner, uint32 recipe, Vec3 position, Vec3 direction, uint64 seed, double birth, bool view_model);
    void expire(double time);
    Array<ParticleRecipe> recipes_;
    Array<Particle> particles_;
    Array<Emitter> emitters_;
    size_t capacity_;
    double time_{0};
    ParticleMetrics metrics_;
};
} // namespace gloom::render
