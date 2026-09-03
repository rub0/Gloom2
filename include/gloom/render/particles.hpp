#pragma once
#include <gloom/render/scene.hpp>
#include <filesystem>
#include <string>

namespace gloom::render {
struct ParticleRecipe {
    std::string name;
    std::uint32_t material_slot{0}, burst{0};
    float rate{0}, life{1}, speed{0}, spread{0}, gravity{0};
    float size_start{.1F},size_end{.2F}, trail_seconds{0},distortion{0};
    Color color_start,color_end{1,1,1,0};
};
[[nodiscard]] std::vector<ParticleRecipe> load_particle_recipes(const std::filesystem::path& path);
[[nodiscard]] bool valid_particle_recipe(const ParticleRecipe& recipe) noexcept;

struct ParticleMetrics {std::uint64_t spawned{0},expired{0},dropped{0};};
struct Particle {
    std::uint64_t owner{0};
    std::uint32_t recipe{0};
    double birth{0};
    Vec3 position,velocity;
    float rotation{0};
    bool view_model{false};
};
class ParticleSystem {
public:
    explicit ParticleSystem(std::vector<ParticleRecipe> recipes, std::size_t capacity=2048);
    // Upsert a continuous emitter. Emitter IDs are stable attachment IDs, owners
    // are entity lifetimes. A missing refresh ends the emitter after this step.
    void emitter(std::uint64_t id,std::uint64_t owner,std::string_view recipe,Vec3 position,Vec3 direction={0,1,0});
    void burst(std::uint64_t owner,std::string_view recipe,Vec3 position,Vec3 direction,
               std::uint64_t seed,bool view_model=false);
    void advance(double seconds);
    void cancel(std::uint64_t owner);
    void clear() noexcept;
    [[nodiscard]] std::vector<RenderInstance> render(const Camera& camera,std::span<const RenderInstance> materials) const;
    [[nodiscard]] std::span<const Particle> particles() const noexcept {return particles_;}
    [[nodiscard]] const ParticleMetrics& metrics() const noexcept {return metrics_;}
    [[nodiscard]] double time() const noexcept {return time_;}
private:
    struct Emitter {std::uint64_t id,owner,serial{0};std::uint32_t recipe;Vec3 previous,position,direction;double next{0};bool refreshed{true};};
    std::uint32_t find(std::string_view name) const;
    void spawn(std::uint64_t owner,std::uint32_t recipe,Vec3 position,Vec3 direction,std::uint64_t seed,double birth,bool view_model);
    std::vector<ParticleRecipe> recipes_;
    std::vector<Particle> particles_;
    std::vector<Emitter> emitters_;
    std::size_t capacity_;
    double time_{0};
    ParticleMetrics metrics_;
};
} // namespace gloom::render
