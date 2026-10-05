#include <gloom/core/allocation_profile.hpp>
#include <gloom/render/particles.hpp>
#define SIMDJSON_EXCEPTIONS 0
#include <simdjson.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace gloom::render {
namespace {
Vec3 add(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 mul(Vec3 a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}
Vec3 mix(Vec3 a, Vec3 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
uint64 hash(uint64 x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
float random(uint64& seed) {
    seed = hash(seed + 0x9e3779b97f4a7c15ULL);
    return static_cast<float>(seed >> 40) / 16777216.0F;
}
float length(Vec3 v) {
    // Double intermediates cover every finite float without overflow/underflow of the squared sum.
    return static_cast<float>(sqrt(static_cast<double>(v.x) * v.x + static_cast<double>(v.y) * v.y + static_cast<double>(v.z) * v.z));
}
Vec3 position(const Particle& p, const ParticleRecipe& r, double time) {
    const float age = static_cast<float>(time > p.birth ? time - p.birth : 0);
    return add(p.position, add(mul(p.velocity, age), {0, .5F * r.gravity * age * age, 0}));
}
#ifndef NDEBUG
bool finite(Vec3 p) {
    return isfinite(p.x) && isfinite(p.y) && isfinite(p.z);
}
#endif
bool color(simdjson::dom::element value, const char* name, Color& result) {
    simdjson::dom::array array;
    if (value[name].get_array().get(array) || array.size() != 4)
        return false;
    float numbers[4];
    size_t index = 0;
    for (simdjson::dom::element item : array) {
        double number;
        if (item.get_double().get(number))
            return false;
        numbers[index++] = static_cast<float>(number);
    }
    result = {.red = numbers[0], .green = numbers[1], .blue = numbers[2], .alpha = numbers[3]};
    return true;
}
}
bool valid_particle_recipe(const ParticleRecipe& r) noexcept {
    const float numbers[]{r.rate, r.life, r.speed, r.spread, r.gravity, r.size_start, r.size_end, r.trail_seconds, r.distortion, r.color_start.red,
        r.color_start.green, r.color_start.blue, r.color_start.alpha, r.color_end.red, r.color_end.green, r.color_end.blue, r.color_end.alpha};
    for (float number : numbers)
        if (!isfinite(number))
            return false;
    if (!r.name[0] || !memchr(r.name, 0, sizeof(r.name)))
        return false;
    return r.material_slot < 64 && r.burst <= 512 && r.rate >= 0 && r.rate <= 500 && r.life > 0 && r.life <= 10 && r.speed >= 0 && r.speed <= 100 &&
           r.spread >= 0 && r.spread <= 20 && r.size_start > 0 && r.size_start <= 10 && r.size_end > 0 && r.size_end <= 10 && r.trail_seconds >= 0 &&
           r.trail_seconds <= r.life && r.distortion >= 0 && r.distortion <= .02F && fabsf(r.gravity) <= 100 && r.color_start.red >= 0 &&
           r.color_start.green >= 0 && r.color_start.blue >= 0 && r.color_end.red >= 0 && r.color_end.green >= 0 && r.color_end.blue >= 0 &&
           r.color_start.alpha >= 0 && r.color_start.alpha <= 1 && r.color_end.alpha >= 0 && r.color_end.alpha <= 1;
}
const char* load_particle_recipes(const char* path, Array<ParticleRecipe>& recipes) {
    assert(path);
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") || !file)
        return "Cannot open particle recipes";
    if (fseek(file, 0, SEEK_END)) {
        fclose(file);
        return "Cannot read particle recipes";
    }
    const long bytes = ftell(file);
    if (bytes < 0 || fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return "Cannot read particle recipes";
    }
    Array<char> input;
    input.reserve(static_cast<size_t>(bytes) + simdjson::SIMDJSON_PADDING);
    input.resize(static_cast<size_t>(bytes) + simdjson::SIMDJSON_PADDING);
    const size_t read = fread(input.data(), 1, bytes, file);
    fclose(file);
    if (read != static_cast<size_t>(bytes))
        return "Cannot read particle recipes";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    if (parser.parse(input.data(), bytes, false).get(doc))
        return "Invalid particle recipe JSON";
    uint64 version;
    if (doc["version"].get_uint64().get(version) || version != 1)
        return "Unsupported particle recipe version";
    simdjson::dom::array entries;
    if (doc["recipes"].get_array().get(entries) || !entries.size() || entries.size() > 64)
        return "Invalid particle recipe count";
    Array<ParticleRecipe> parsed;
    parsed.reserve(entries.size());
    for (simdjson::dom::element value : entries) {
        ParticleRecipe r;
        const char* name;
        size_t name_length;
        if (value["name"].get_c_str().get(name) || value["name"].get_string_length().get(name_length) || !name_length || name_length > 64 ||
            memchr(name, 0, name_length))
            return "Invalid particle recipe name";
        memcpy(r.name, name, name_length);
        uint64 material, burst;
        if (value["material"].get_uint64().get(material) || value["burst"].get_uint64().get(burst) || material >= 64 || burst > 512)
            return "Particle recipe index/count exceeds capacity";
        r.material_slot = static_cast<uint32>(material);
        r.burst = static_cast<uint32>(burst);
        const char* fields[]{"rate", "life", "speed", "spread", "gravity", "size_start", "size_end", "trail_seconds", "distortion"};
        float* numbers[]{&r.rate, &r.life, &r.speed, &r.spread, &r.gravity, &r.size_start, &r.size_end, &r.trail_seconds, &r.distortion};
        for (size_t index = 0; index < 9; ++index) {
            double number;
            if (value[fields[index]].get_double().get(number))
                return "Invalid particle recipe numeric field";
            *numbers[index] = static_cast<float>(number);
        }
        if (!color(value, "color_start", r.color_start) || !color(value, "color_end", r.color_end) || !valid_particle_recipe(r))
            return "Invalid particle recipe";
        // ponytail: at most 64 names; a bounded scan avoids a map and another owner.
        for (const ParticleRecipe& previous : parsed)
            if (strcmp(previous.name, r.name) == 0)
                return "Duplicate particle recipe name";
        parsed.push_back(r);
    }
    recipes = static_cast<Array<ParticleRecipe>&&>(parsed);
    return nullptr;
}
ParticleSystem::ParticleSystem(Span<const ParticleRecipe> recipes, size_t capacity) : capacity_{capacity} {
    assert(capacity && capacity <= 16384 && !recipes.empty() && recipes.size() <= 64);
    for (size_t index = 0; index < recipes.size(); ++index) {
        assert(valid_particle_recipe(recipes[index]));
        for (size_t previous = 0; previous < index; ++previous)
            assert(strcmp(recipes[previous].name, recipes[index].name) != 0);
    }
    recipes_.append(recipes);
    particles_.reserve(capacity);
    emitters_.reserve(64);
}
uint32 ParticleSystem::find(const char* name) const {
    assert(name);
    for (uint32 index = 0; index < recipes_.size(); ++index)
        if (strcmp(recipes_[index].name, name) == 0)
            return index;
    assert(false && "Unknown particle recipe");
    return 0;
}
void ParticleSystem::emitter(uint64 id, uint64 owner, const char* name, Vec3 p, Vec3 d) {
    assert(id && name && finite(p) && finite(d));
    for (Emitter& e : emitters_)
        if (e.id == id) {
            assert(e.owner == owner && strcmp(recipes_[e.recipe].name, name) == 0);
            e.position = p;
            e.direction = d;
            e.refreshed = true;
            return;
        }
    const uint32 recipe = find(name);
    assert(recipes_[recipe].rate > 0);
    if (emitters_.size() >= 64) {
        ++metrics_.dropped;
        return;
    }
    emitters_.push_back(
        {.id = id, .owner = owner, .recipe = recipe, .previous = p, .position = p, .direction = d, .next = time_ + 1.0 / recipes_[recipe].rate});
}
void ParticleSystem::spawn(uint64 owner, uint32 recipe, Vec3 p, Vec3 d, uint64 seed, double birth, bool fps) {
    if (particles_.size() >= capacity_) {
        ++metrics_.dropped;
        return;
    }
    const ParticleRecipe& r = recipes_[recipe];
    const float n = length(d);
    if (n > 1e-6F)
        d = mul(d, 1 / n);
    Vec3 noise{random(seed) * 2 - 1, random(seed) * 2 - 1, random(seed) * 2 - 1};
    particles_.push_back({.owner = owner,
        .recipe = recipe,
        .birth = birth,
        .position = p,
        .velocity = add(mul(d, r.speed), mul(noise, r.spread)),
        .rotation = random(seed) * 6.2831853F,
        .view_model = fps});
    ++metrics_.spawned;
}
void ParticleSystem::burst(uint64 owner, const char* name, Vec3 p, Vec3 d, uint64 seed, bool fps) {
    assert(finite(p) && finite(d));
    const uint32 recipe = find(name);
    for (uint32 index = 0; index < recipes_[recipe].burst; ++index)
        spawn(owner, recipe, p, d, hash(seed + index), time_, fps);
}
bool ParticleSystem::translate(uint64 owner, Vec3 d) noexcept {
    assert(finite(d));
    bool found = false;
    for (Particle& p : particles_)
        if (p.owner == owner) {
            p.position = add(p.position, d);
            found = true;
        }
    for (Emitter& e : emitters_)
        if (e.owner == owner) {
            e.previous = add(e.previous, d);
            e.position = add(e.position, d);
            found = true;
        }
    return found;
}
void ParticleSystem::expire(double time) {
    size_t count = 0;
    for (size_t index = 0; index < particles_.size(); ++index)
        if (time - particles_[index].birth < recipes_[particles_[index].recipe].life - 1e-9) {
            if (count != index)
                particles_[count] = particles_[index];
            ++count;
        }
    metrics_.expired += particles_.size() - count;
    particles_.resize(count);
}
void ParticleSystem::advance(double seconds) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::particles);
    assert(isfinite(seconds) && seconds >= 0 && seconds <= 10);
    const double end = time_ + seconds;
    size_t count = 0;
    for (const Emitter& e : emitters_)
        if (e.refreshed) {
            if (&emitters_[count] != &e)
                emitters_[count] = e;
            ++count;
        }
    emitters_.resize(count);
    for (;;) {
        size_t next = emitters_.size();
        int64 earliest = 0;
        for (size_t index = 0; index < emitters_.size(); ++index) {
            const int64 birth = llround(emitters_[index].next * 1e9);
            if (next == emitters_.size() || birth < earliest || (birth == earliest && emitters_[index].id < emitters_[next].id)) {
                next = index;
                earliest = birth;
            }
        }
        if (next == emitters_.size() || emitters_[next].next > end + 1e-9)
            break;
        Emitter& e = emitters_[next];
        const ParticleRecipe& r = recipes_[e.recipe];
        // Expire before each chronological birth, not prematurely at frame end.
        expire(e.next);
        const double fraction = seconds > 0 ? (e.next - time_) / seconds : 1;
        spawn(e.owner, e.recipe,
            mix(e.previous, e.position,
                static_cast<float>(fraction < 0   ? 0
                                   : fraction > 1 ? 1
                                                  : fraction)),
            e.direction, hash(e.id + e.serial), e.next, false);
        ++e.serial;
        e.next += 1.0 / r.rate;
    }
    expire(end);
    for (Emitter& e : emitters_) {
        e.previous = e.position;
        e.refreshed = false;
    }
    time_ = end;
}
void ParticleSystem::cancel(uint64 owner) {
    size_t count = 0;
    for (const Emitter& e : emitters_)
        if (e.owner != owner) {
            if (&emitters_[count] != &e)
                emitters_[count] = e;
            ++count;
        }
    emitters_.resize(count);
    count = 0;
    for (const Particle& p : particles_)
        if (p.owner != owner) {
            if (&particles_[count] != &p)
                particles_[count] = p;
            ++count;
        }
    particles_.resize(count);
}
void ParticleSystem::clear() noexcept {
    particles_.resize(0);
    emitters_.resize(0);
    time_ = 0;
}
void ParticleSystem::render(Array<RenderInstance>& result, const Camera& camera, Span<const RenderInstance> materials) const {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::particles);
    assert(materials.empty() || !result.data() ||
           reinterpret_cast<size_t>(materials.data()) + materials.size() * sizeof(RenderInstance) <= reinterpret_cast<size_t>(result.data()) ||
           reinterpret_cast<size_t>(materials.data()) >= reinterpret_cast<size_t>(result.data() + result.capacity()));
    result.reserve(result.size() + particles_.size());
    GLOOM_PROFILE_CAPACITY("particles", particles_.size(), particles_.capacity(), sizeof(Particle));
    GLOOM_PROFILE_CAPACITY("emitters", emitters_.size(), emitters_.capacity(), sizeof(Emitter));
    const Transform camera_basis = camera_relative_transform(camera, {}, {1, 1, 1});
    for (const Particle& p : particles_) {
        const ParticleRecipe& r = recipes_[p.recipe];
        if (r.material_slot >= materials.size())
            continue;
        const float age = static_cast<float>(time_ - p.birth), fraction = age / r.life;
        const float t = fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;
        result.append({&materials[r.material_slot], 1});
        RenderInstance& instance = result[result.size() - 1];
        const float size = r.size_start + (r.size_end - r.size_start) * t;
        instance.transform = {.position = position(p, r, time_), .rotation = camera_basis.rotation, .scale = {size, size, size}};
        if (r.trail_seconds > 0 && age > 0) {
            const Vec3 start = position(p, r, time_ - (age < r.trail_seconds ? age : r.trail_seconds));
            const Vec3 end = instance.transform.position;
            const float distance = length({end.x - start.x, end.y - start.y, end.z - start.z});
            if (distance > 1e-4F) {
                Camera ribbon{.position = mul(add(start, end), .5F), .target = camera.position, .up = {end.x - start.x, end.y - start.y, end.z - start.z}};
                const Vec3 cross{(camera.position.y - ribbon.position.y) * ribbon.up.z - (camera.position.z - ribbon.position.z) * ribbon.up.y,
                    (camera.position.z - ribbon.position.z) * ribbon.up.x - (camera.position.x - ribbon.position.x) * ribbon.up.z,
                    (camera.position.x - ribbon.position.x) * ribbon.up.y - (camera.position.y - ribbon.position.y) * ribbon.up.x};
                if (length(cross) > 1e-5F)
                    instance.transform = camera_relative_transform(ribbon, {}, {size, distance + size, size});
            }
        }
        instance.color = {.red = r.color_start.red + (r.color_end.red - r.color_start.red) * t,
            .green = r.color_start.green + (r.color_end.green - r.color_start.green) * t,
            .blue = r.color_start.blue + (r.color_end.blue - r.color_start.blue) * t,
            .alpha = r.color_start.alpha + (r.color_end.alpha - r.color_start.alpha) * t};
        instance.color.alpha *= age / .025F < 1 ? age / .025F : 1;
        instance.view_model = p.view_model;
        instance.casts_shadow = false;
        instance.lod_count = 1;
        instance.particle = true;
        instance.distortion = r.distortion;
        instance.soft_distance = p.view_model ? 0 : .15F;
    }
}
} // namespace gloom::render
