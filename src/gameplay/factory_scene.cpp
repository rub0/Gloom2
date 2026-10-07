#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/legacy_movement.hpp>
#include <gloom/backends/jolt_world.hpp>
#include <gloom/assets/asset.hpp>
#include <simdjson.h>
#include <bit>
#include <assert.h>
#include <math.h>
#include <string.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace gloom::gameplay {
namespace {
bool read_vector(simdjson::simdjson_result<simdjson::dom::element> element, physics::Vec3& result, float scale = 1.0F) {
    simdjson::dom::array array;
    if (element.get_array().get(array) || array.size() != 3)
        return false;
    float* fields[]{&result.x, &result.y, &result.z};
    size_t index = 0;
    for (simdjson::dom::element item : array) {
        double number = 0;
        if (item.get_double().get(number) || !isfinite(number) || !isfinite(static_cast<float>(number) * scale))
            return false;
        *fields[index++] = static_cast<float>(number) * scale;
    }
    return true;
}
void hash_word(uint32& hash, uint32 value) {
    for (uint32 i = 0; i < 4; ++i) {
        hash = (hash ^ (value & 255U)) * 16777619U;
        value >>= 8;
    }
}
void hash_point(uint32& hash, physics::Vec3 value) {
    hash_word(hash, std::bit_cast<uint32>(value.x));
    hash_word(hash, std::bit_cast<uint32>(value.y));
    hash_word(hash, std::bit_cast<uint32>(value.z));
}
const std::expected<FactoryScene, std::string>& factory_result() {
    static const std::expected<FactoryScene, std::string> scene = load_factory_scene(std::filesystem::path{GLOOM_SOURCE_ROOT} / "assets/legacy");
    return scene;
}
struct CollisionQuery {
    backends::JoltWorld world{{.worker_threads = 1}};
    // ponytail: one shared query lock; split worlds only if measured concurrent character queries contend.
    SRWLOCK mutex = SRWLOCK_INIT;
    CollisionQuery() {
        world.start();
        static_cast<void>(
            world.create_body({.shape = {.type = physics::ShapeType::triangle_mesh, .triangle_mesh = &original_factory().collision}, .friction = 0.6F}));
    }
};
CollisionQuery& collision_query() {
    static CollisionQuery query;
    return query;
}
network::MovementState factory_motion(void* context, network::MovementState state, const network::MovementInput& input, double delta) {
    CollisionQuery& query = collision_query();
    const FactoryCharacterResolver* character = static_cast<const FactoryCharacterResolver*>(context);
    const bool dodge = input.dodge && (input.axis_x != 0 || input.axis_z != 0) && (state.grounded || state.air_dodge_available);
    if (state.grounded)
        state.air_dodge_available = input.jump && !dodge;
    if (dodge)
        state.air_dodge_available = false;
    const LegacyMotion motion = legacy_motion({state.velocity_x, state.velocity_y, state.velocity_z}, state.grounded || dodge, input.axis_x, input.axis_z,
        input.jump, dodge, static_cast<float>(delta),
        legacy_movement_profile(character && character->function ? character->function(character->context, state.entity) : SliceCharacter::hound));
    AcquireSRWLockExclusive(&query.mutex);
    const physics::CharacterState moved = query.world.query_character_motion({.position = {state.position_x, state.position_y, state.position_z},
                                                                                 .radius = .45F,
                                                                                 .cylinder_half_height = .45F,
                                                                                 .max_slope_angle_radians = acosf(.707F),
                                                                                 .step_up_height = .075F,
                                                                                 .step_down_height = .075F},
        motion.velocity, static_cast<float>(delta), {0, legacy_gravity, 0}, .075F);
    ReleaseSRWLockExclusive(&query.mutex);
    state.position_x = moved.position.x;
    state.position_y = moved.position.y;
    state.position_z = moved.position.z;
    state.velocity_x = moved.velocity.x;
    state.velocity_y = moved.grounded ? 0.F : moved.velocity.y;
    state.velocity_z = moved.velocity.z;
    state.grounded = moved.grounded;
    state.simulation_tick = input.simulation_tick;
    if (inside_factory_jumper(moved.position)) {
        const physics::Vec3 force = original_factory().jumper.force;
        state.velocity_x += force.x * legacy_unit_scale / legacy_motion_step;
        state.velocity_y += force.y * legacy_unit_scale / legacy_motion_step;
        state.velocity_z += force.z * legacy_unit_scale / legacy_motion_step;
        state.grounded = false;
    }
    return state;
}
}

std::expected<FactoryScene, std::string> load_factory_scene(const std::filesystem::path& root) {
    assets::VirtualFileSystem filesystem;
    const std::expected<void, std::string> mounted = filesystem.mount("factory", root);
    if (!mounted)
        return std::unexpected{mounted.error()};
    const std::expected<std::vector<std::byte>, std::string> source = filesystem.read(*assets::VirtualPath::parse("factory:/factory_scene.json"));
    if (!source)
        return std::unexpected{source.error()};
    simdjson::dom::parser parser;
    simdjson::dom::element document;
    if (parser.parse(reinterpret_cast<const char*>(source->data()), source->size()).get(document))
        return std::unexpected{"Invalid Factory JSON"};
    uint64 version = 0;
    double unit_scale = 0;
    if (document["version"].get_uint64().get(version) || version != 1)
        return std::unexpected{"Unknown Factory scene version"};
    if (document["unit_scale"].get_double().get(unit_scale) || !isfinite(unit_scale) || fabs(unit_scale - .15) > 1e-6)
        return std::unexpected{"Factory unit policy mismatch"};
    const float scale = static_cast<float>(unit_scale);
    FactoryScene scene;
    const std::expected<std::vector<std::byte>, std::string> probe = filesystem.read(*assets::VirtualPath::parse("factory:/factory.ibl"));
    if (!probe)
        return std::unexpected{probe.error()};
    if (probe->size() < 12 || memcmp(probe->data(), "GIBL", 4))
        return std::unexpected{"Invalid Factory environment probe"};
    memcpy(&scene.environment_probe.size, probe->data() + 4, 4);
    memcpy(&scene.environment_probe.mip_levels, probe->data() + 8, 4);
    if (scene.environment_probe.size != 32 || scene.environment_probe.mip_levels != 6)
        return std::unexpected{"Invalid Factory environment probe"};
    size_t texels = 0;
    for (uint32 level = 0; level < scene.environment_probe.mip_levels; ++level)
        texels += 6 * (scene.environment_probe.size >> level) * (scene.environment_probe.size >> level);
    if (probe->size() != 12 + texels * sizeof(std::array<float, 4>))
        return std::unexpected{"Invalid Factory probe payload"};
    scene.environment_probe.radiance.resize(texels);
    memcpy(scene.environment_probe.radiance.data(), probe->data() + 12, texels * sizeof(std::array<float, 4>));
    for (const std::array<float, 4>& pixel : scene.environment_probe.radiance)
        for (float channel : pixel)
            if (!isfinite(channel) || channel < 0)
                return std::unexpected{"Invalid Factory probe radiance"};
    simdjson::dom::array vertices, indices, entities;
    if (document["collision"]["vertices"].get_array().get(vertices) || document["collision"]["indices"].get_array().get(indices) ||
        document["entities"].get_array().get(entities))
        return std::unexpected{"Invalid Factory collision or entities"};
    scene.collision.vertices.reserve(vertices.size());
    scene.collision.indices.reserve(indices.size());
    for (simdjson::dom::element vertex : vertices) {
        physics::Vec3 point;
        if (!read_vector(simdjson::simdjson_result<simdjson::dom::element>{simdjson::dom::element{vertex}}, point))
            return std::unexpected{"Invalid Factory collision vector"};
        scene.collision.vertices.push_back(point);
    }
    for (simdjson::dom::element item : indices) {
        uint64 index = 0;
        if (item.get_uint64().get(index) || index >= scene.collision.vertices.size())
            return std::unexpected{"Factory collision index out of range"};
        scene.collision.indices.push_back(static_cast<uint32>(index));
    }
    if (scene.collision.vertices.empty() || scene.collision.indices.empty() || scene.collision.indices.size() % 3)
        return std::unexpected{"Empty or invalid Factory collision"};
    scene.spawns.reserve(9);
    scene.pickups.reserve(factory_pickup_count);
    for (simdjson::dom::element entity : entities) {
        ++scene.entity_count;
        std::string_view type;
        simdjson::dom::element authored, resolved;
        if (entity["type"].get_string().get(type) || entity["source"].get(authored) || entity["resolved"].get(resolved))
            return std::unexpected{"Invalid Factory entity fields"};
        if (type == "SpawnPoint") {
            FactorySpawn spawn;
            double yaw = 0;
            if (!read_vector(authored["position"], spawn.position, scale) || authored["yaw"].get_double().get(yaw) || !isfinite(yaw) ||
                !isfinite(static_cast<float>(yaw)))
                return std::unexpected{"Invalid Factory spawn"};
            spawn.yaw = static_cast<float>(yaw);
            scene.spawns.push_back(spawn);
        } else if (type == "Lava") {
            double width = 0;
            if (!read_vector(authored["position"], scene.lava_center, scale) || authored["plane_width"].get_double().get(width) || !isfinite(width) ||
                width <= 0 || !isfinite(static_cast<float>(width) * scale * .5F))
                return std::unexpected{"Invalid Factory lava"};
            scene.lava_half_width = static_cast<float>(width) * scale * .5F;
        } else if (type == "Jumper") {
            if (!read_vector(authored["position"], scene.jumper.position, scale) || !read_vector(authored["force"], scene.jumper.force) ||
                !read_vector(resolved["physic_dimensions"], scene.jumper.half_extent, scale * .5F))
                return std::unexpected{"Invalid Factory jumper"};
        }
        double reward = 0;
        const simdjson::error_code reward_error = resolved["reward"].get_double().get(reward);
        // Generic Ammo has no recovered archetype; preserve the 14-type/73-location contract.
        if (type == "Ammo" || reward_error == simdjson::NO_SUCH_FIELD)
            continue;
        if (reward_error)
            return std::unexpected{"Invalid Factory pickup reward"};
        std::string_view id, name;
        if (resolved["id"].get_string().get(id) || entity["id"].get_string().get(name))
            return std::unexpected{"Invalid Factory pickup identity"};
        PickupKind kind;
        if (id == "orb")
            kind = PickupKind::life;
        else if (id == "armor")
            kind = PickupKind::shield;
        else if (id == "weapon")
            kind = PickupKind::weapon;
        else if (id == "ammo")
            kind = PickupKind::ammo;
        else if (id == "damageAmplifier")
            kind = PickupKind::damage;
        else if (id == "cooldownReducer")
            kind = PickupKind::cooldown;
        else
            return std::unexpected{"Unknown Factory pickup reward"};
        double weapon = 0, radius = 3, seconds = 0;
        if ((kind == PickupKind::weapon || kind == PickupKind::ammo) && resolved["weaponType"].get_double().get(weapon))
            return std::unexpected{"Invalid Factory weapon"};
        const simdjson::error_code radius_error = resolved["physic_radius"].get_double().get(radius);
        if (radius_error && radius_error != simdjson::NO_SUCH_FIELD)
            return std::unexpected{"Invalid Factory pickup radius"};
        if (resolved["respawnTime"].get_double().get(seconds))
            return std::unexpected{"Invalid Factory respawn time"};
        const double ticks = seconds * 60;
        if (!isfinite(reward) || reward < 0 || reward > 200 || !isfinite(ticks) || ticks < 1 || ticks > 7200 || !isfinite(weapon) || weapon < 0 ||
            weapon >= slice_weapon_count || !isfinite(radius) || radius <= 0 || !isfinite(static_cast<float>(radius) * scale))
            return std::unexpected{"Invalid Factory pickup rule"};
        PickupDefinition pickup{.name = std::string{name},
            .kind = kind,
            .weapon = static_cast<SliceWeapon>(static_cast<uint32>(weapon)),
            .reward = static_cast<uint16>(reward),
            .respawn_ticks = static_cast<uint16>(ticks),
            .radius = static_cast<float>(radius) * scale};
        if (!read_vector(resolved["position"], pickup.position, scale))
            return std::unexpected{"Invalid Factory pickup position"};
        scene.pickups.push_back(std::move(pickup));
    }
    if (scene.pickups.size() != factory_pickup_count || scene.spawns.size() != 9 || scene.entity_count != 97)
        return std::unexpected{"Factory entity manifest is incomplete"};
    const std::expected<std::vector<std::byte>, std::string> visuals = filesystem.read(*assets::VirtualPath::parse("factory:/factory.gltf"));
    if (!visuals)
        return std::unexpected{visuals.error()};
    simdjson::dom::parser visual_parser;
    simdjson::dom::element visual_document;
    simdjson::dom::array nodes;
    if (visual_parser.parse(reinterpret_cast<const char*>(visuals->data()), visuals->size()).get(visual_document) ||
        visual_document["nodes"].get_array().get(nodes))
        return std::unexpected{"Invalid Factory visual manifest"};
    uint32 node_index = 0;
    for (simdjson::dom::element node : nodes) {
        std::string_view name;
        if (node["name"].get_string().get(name))
            return std::unexpected{"Invalid Factory visual node name"};
        for (PickupDefinition& pickup : scene.pickups)
            if (pickup.name == name)
                pickup.source_node = node_index;
        ++node_index;
    }
    uint32 hash = 2166136261U;
    for (physics::Vec3 point : scene.collision.vertices)
        hash_point(hash, point);
    for (uint32 index : scene.collision.indices)
        hash_word(hash, index);
    for (const FactorySpawn& spawn : scene.spawns) {
        hash_point(hash, spawn.position);
        hash_word(hash, std::bit_cast<uint32>(spawn.yaw));
    }
    hash_point(hash, scene.lava_center);
    hash_word(hash, std::bit_cast<uint32>(scene.lava_half_width));
    hash_point(hash, scene.jumper.position);
    hash_point(hash, scene.jumper.force);
    hash_point(hash, scene.jumper.half_extent);
    hash_word(hash, 69);
    hash_word(hash, 67);
    for (const PickupDefinition& pickup : scene.pickups) {
        for (unsigned char character : pickup.name)
            hash_word(hash, character);
        hash_word(hash, static_cast<uint32>(pickup.kind));
        hash_word(hash, static_cast<uint32>(pickup.weapon));
        hash_point(hash, pickup.position);
        hash_word(hash, pickup.reward);
        hash_word(hash, pickup.respawn_ticks);
        hash_word(hash, std::bit_cast<uint32>(pickup.radius));
        hash_word(hash, pickup.source_node);
    }
    scene.scene_id = hash ? hash : 1;
    scene.lights.push_back(
        {.position = {-125.828F * scale, -44.0043F * scale, -72.072F * scale}, .range = 90.0F, .color = {1.0F, 0.65F, 0.07F}, .intensity = 1.2F});
    for (render::PointLight& light : scene.lights)
        light.intensity *= 1.2F;
    return scene;
}
const char* original_factory_error() {
    return factory_result() ? nullptr : factory_result().error().c_str();
}
const FactoryScene& original_factory() {
    assert(factory_result().has_value());
    return *factory_result();
}
bool inside_factory_jumper(physics::Vec3 position) noexcept {
    const FactoryJumper& jumper = original_factory().jumper;
    return fabsf(position.x - jumper.position.x) <= jumper.half_extent.x + .45F && fabsf(position.y - jumper.position.y) <= jumper.half_extent.y + .15F &&
           fabsf(position.z - jumper.position.z) <= jumper.half_extent.z + .45F;
}
network::ReplicationSettings factory_movement_settings(FactoryCharacterResolver* character) {
    static_cast<void>(collision_query());
    network::ReplicationSettings settings;
    settings.resolve_entity_collisions = true;
    settings.character_radius = .45F;
    settings.movement_speed = 1.3F * legacy_unit_scale / legacy_motion_step;
    settings.jump_speed = 1.8F * legacy_unit_scale / legacy_motion_step * normal_jump_scale;
    settings.gravity = legacy_gravity;
    settings.scene_movement = {.function = factory_motion, .context = character};
    return settings;
}
}
