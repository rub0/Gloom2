#include <gloom/gameplay/vertical_slice.hpp>
#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/component_replication.hpp>
#include <gloom/gameplay/kinematic_motion.hpp>
#include <gloom/backends/jolt_world.hpp>

#include <gloom/gameplay/components.hpp>
#include <gloom/network/combat.hpp>
#include <gloom/network/session.hpp>
#include <gloom/physics/components.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace gloom::gameplay {
namespace {

constexpr std::uint32_t cooldown_ticks = 30;
constexpr float maximum_weapon_range = 600.0F * legacy_gameplay_scale;
constexpr std::uint32_t respawn_ticks = 240;
constexpr std::uint32_t bite_duration_ticks = 30;
constexpr std::uint32_t bite_cooldown_ticks = 1'500;
constexpr std::uint32_t guard_duration_ticks = 60;
constexpr std::uint32_t guard_cooldown_ticks = 900;
constexpr std::uint32_t hit_marker_ticks = 12;
constexpr network::NetworkEntityId factory_lift_network_entity = 100;
constexpr network::ConnectionId player_connection = 101;
constexpr network::ConnectionId opponent_connection = 102;

[[nodiscard]] std::vector<ArenaBox> slice_arena();
[[nodiscard]] std::vector<ArenaSurface> slice_surfaces();

[[nodiscard]] constexpr std::uint32_t ability_cooldown_ticks(
    const SliceAbility ability) noexcept {
    switch (ability) {
        case SliceAbility::bite: return bite_cooldown_ticks;
        case SliceAbility::guard: return guard_cooldown_ticks;
        case SliceAbility::none: return 0;
    }
    return 0;
}

[[nodiscard]] network::ProtocolMessage wire_round_trip(const network::ProtocolMessage& message) {
    const auto decoded = network::decode_message(network::encode_message(message));
    if (!decoded) {
        throw std::runtime_error{"Vertical-slice protocol round trip failed: " + decoded.error()};
    }
    return *decoded;
}

[[nodiscard]] network::ReplicationSettings slice_movement_settings() {
    network::ReplicationSettings settings{
        .tick_rate = VerticalSliceSimulation::tick_rate,
        .snapshot_rate = 20,
        .movement_speed = 5.0F,
        .interpolation_delay_seconds = 0.1,
        .jump_speed = 5.5F,
        .gravity = -15.0F,
        .character_radius = 0.4F,
        .character_collision_height = 1.8F,
        .resolve_entity_collisions = true,
    };
    for (const auto& box : slice_arena()) {
        if (!box.solid) {
            continue;
        }
        settings.static_obstacles.push_back({.minimum_x = box.center_x - box.half_x,
                                             .maximum_x = box.center_x + box.half_x,
                                             .minimum_z = box.center_z - box.half_z,
                                             .maximum_z = box.center_z + box.half_z,
                                             .top_y = box.center_y + box.half_y});
    }
    return settings;
}

[[nodiscard]] std::vector<ArenaBox> slice_arena() {
    // A compact, data-only blockout of the legacy Factory map: long combat
    // lanes, a central furnace, opposing service blocks, overhead pipework
    // and the characteristic lava channel. The floor is deliberately split
    // around the channel; lava itself is a one-sided surface, not a box.
    // of the gameplay seam and can be replaced by authored assets later.
    return {{.center_y = -0.5F,
             .center_z = -1.175F,
             .half_x = 12.0F,
             .half_y = 0.5F,
             .half_z = 7.825F,
             .material = ArenaMaterial::floor},
            {.center_y = -0.5F,
             .center_z = 8.575F,
             .half_x = 12.0F,
             .half_y = 0.5F,
             .half_z = 0.425F,
             .material = ArenaMaterial::floor},
            {.center_y = 0.75F,
             .half_x = 1.1F,
             .half_y = 0.75F,
             .half_z = 1.0F,
             .material = ArenaMaterial::industrial,
             .solid = true},
            {.center_x = -6.0F,
             .center_y = 1.2F,
             .center_z = 4.2F,
             .half_x = 1.1F,
             .half_y = 1.2F,
             .half_z = 1.6F,
             .material = ArenaMaterial::industrial,
             .solid = true},
            {.center_x = 6.0F,
             .center_y = 1.2F,
             .center_z = -4.2F,
             .half_x = 1.1F,
             .half_y = 1.2F,
             .half_z = 1.6F,
             .material = ArenaMaterial::industrial,
             .solid = true},
            {.center_x = -5.0F,
             .center_y = 3.4F,
             .half_x = 5.2F,
             .half_y = 0.18F,
             .half_z = 0.28F,
             .material = ArenaMaterial::trim},
            {.center_x = 5.0F,
             .center_y = 3.4F,
             .half_x = 5.2F,
             .half_y = 0.18F,
             .half_z = 0.28F,
             .material = ArenaMaterial::trim},
            {.center_x = -10.8F,
             .center_y = 1.5F,
             .half_x = 0.25F,
             .half_y = 1.5F,
             .half_z = 8.0F,
             .material = ArenaMaterial::trim,
             .solid = true},
            {.center_x = 10.8F,
             .center_y = 1.5F,
             .half_x = 0.25F,
             .half_y = 1.5F,
             .half_z = 8.0F,
             .material = ArenaMaterial::trim,
             .solid = true},
            {.center_y = 1.5F,
             .center_z = -8.75F,
             .half_x = 11.5F,
             .half_y = 1.5F,
             .half_z = 0.25F,
             .material = ArenaMaterial::trim,
             .solid = true},
            {.center_y = 1.5F,
             .center_z = 8.75F,
             .half_x = 11.5F,
             .half_y = 1.5F,
             .half_z = 0.25F,
             .material = ArenaMaterial::trim,
             .solid = true}};
}

[[nodiscard]] std::vector<ArenaSurface> slice_surfaces() {
    return {{.center_y = -0.62F,
             .center_z = 7.4F,
             .half_x = 10.5F,
             .half_z = 0.75F,
             .material = ArenaMaterial::lava}};
}

} // namespace

struct VerticalSliceSimulation::Impl {
    struct Projectile {std::uint32_t id{};network::NetworkEntityId owner{};SliceWeapon weapon{};float x{},y{},z{},dx{},dy{},dz{},speed{},radius{},damage{},explosion{};std::uint16_t life{600};bool returning{};};
    struct Combatant {
        core::EntityId logical_entity;
        TransformComponent* transform{nullptr};
        CharacterPhysicsComponent* physics{nullptr};
        HealthComponent* health{nullptr};
        ShieldComponent* shield{nullptr};
        CharacterMovementComponent* movement{nullptr};
        WeaponComponent* weapon{nullptr};
        AbilityComponent* ability{nullptr};
        CharacterLoadoutComponent* loadout{nullptr};
        AuthorityComponent* authority{nullptr};
        ReplicationComponent* replication{nullptr};
        CharacterPresentationComponent* presentation{nullptr};
        ScoreComponent* score{nullptr};

        [[nodiscard]] network::NetworkEntityId entity() const noexcept {
            return authority->network_entity;
        }
    };

    [[nodiscard]] Combatant compose_character(const network::NetworkEntityId network_entity,
                                               const network::ConnectionId connection,
                                               const float spawn_x) {
        const auto entity = compose_slice_character(
            logical_entities, {.network_entity = network_entity,
                               .connection = connection,
                               .spawn_x = spawn_x});
        if (!has_complete_character_composition(logical_entities, entity)) {
            throw std::logic_error{"Incomplete slice character composition"};
        }
        return {.logical_entity = entity,
                .transform = logical_entities.get<TransformComponent>(entity),
                .physics = logical_entities.get<CharacterPhysicsComponent>(entity),
                .health = logical_entities.get<HealthComponent>(entity),
                .shield = logical_entities.get<ShieldComponent>(entity),
                .movement = logical_entities.get<CharacterMovementComponent>(entity),
                .weapon = logical_entities.get<WeaponComponent>(entity),
                .ability = logical_entities.get<AbilityComponent>(entity),
                .loadout = logical_entities.get<CharacterLoadoutComponent>(entity),
                .authority = logical_entities.get<AuthorityComponent>(entity),
                .replication = logical_entities.get<ReplicationComponent>(entity),
                .presentation = logical_entities.get<CharacterPresentationComponent>(entity),
                .score = logical_entities.get<ScoreComponent>(entity)};
    }

    explicit Impl(const VerticalSliceSettings settings)
        : movement_settings{settings.original_factory ? factory_movement_settings() : slice_movement_settings()},
          arena_boxes{settings.original_factory ? std::vector<ArenaBox>{} : slice_arena()},
          arena_surfaces{settings.original_factory ? std::vector<ArenaSurface>{{original_factory().lava_center.x, original_factory().lava_center.y, original_factory().lava_center.z, original_factory().lava_half_width, original_factory().lava_half_width}} : slice_surfaces()},
          sessions({.maximum_clients = 2,
                    .reconnect_grace_ticks = 600,
                    .authenticate = [](const std::string_view credential) {
                        return credential == "slice-player" || credential == "slice-opponent";
                    },
                    .generate_resume_token = [token = std::uint64_t{0x600d0000}]() mutable {
                        return ++token;
                    }}),
          combat({.history_ticks = 120,
                  .future_tolerance_ticks = 2,
                  .hit_radius = 0.55F,
                  .character_center_height = 0.9F,
                  .hit_half_height = 0.45F,
                  .maximum_range = maximum_weapon_range,
                  .static_obstacles = movement_settings.static_obstacles,
                  .static_mesh = settings.original_factory ? original_factory().collision : nullptr}),
          opponent_ai_enabled{settings.opponent_ai_enabled},
          authoritative_physics{settings.authoritative_physics},
          use_original_factory{settings.original_factory} {
        if (authoritative_physics == nullptr) {
            owned_authoritative_physics = std::make_unique<backends::JoltWorld>();
            owned_authoritative_physics->start();
            authoritative_physics = owned_authoritative_physics.get();
        }
        admit_client(player_connection, player_session, "slice-player", 0x1001);
        admit_client(opponent_connection, opponent_session, "slice-opponent", 0x1002);
        player = compose_character(player_session.controlled_entity(),
                                   player_connection, -5.0F);
        opponent = compose_character(opponent_session.controlled_entity(),
                                     opponent_connection, 5.0F);

        if (use_original_factory) {
            pickups.reset(original_factory().pickups);
            movement_settings=factory_movement_settings([this](network::NetworkEntityId entity){
                return entity==player.entity()?player.loadout->selection.character:opponent.loadout->selection.character;
            });
            const auto set_spawn = [](Combatant& character, const FactorySpawn& spawn) {
                character.movement->spawn_x = spawn.position.x;
                character.movement->spawn_y = spawn.position.y;
                character.movement->spawn_z = spawn.position.z;
            };
            set_spawn(player, original_factory().spawns[0]);
            set_spawn(opponent, original_factory().spawns[1]);
        }
        const network::MovementState player_initial{.entity = player.entity(),
                                                     .position_x = player.movement->spawn_x, .position_y = player.movement->spawn_y, .position_z = player.movement->spawn_z};
        const network::MovementState opponent_initial{.entity = opponent.entity(),
                                                       .position_x = opponent.movement->spawn_x, .position_y = opponent.movement->spawn_y, .position_z = opponent.movement->spawn_z};
        movement_server = std::make_unique<network::AuthoritativeMovementServer>(
            movement_settings, player.entity());
        movement_server->register_client(opponent.entity());
        movement_server->add_entity(player_initial);
        movement_server->add_entity(opponent_initial);
        player_prediction = std::make_unique<network::PredictedMovementClient>(
            movement_settings, player.entity(), player_initial);
        opponent_prediction = std::make_unique<network::PredictedMovementClient>(
            movement_settings, opponent.entity(), opponent_initial);
        synchronize_transform(player, player_initial);
        synchronize_transform(opponent, opponent_initial);
        combat.record(component_world_snapshot());
        synchronize_clock(player_session, 0.0);
        synchronize_clock(opponent_session, 0.0);
        initialize_factory_physics();
        refresh_snapshot();
    }

    void initialize_factory_physics() {
        if (authoritative_physics == nullptr) {
            return;
        }
        if (authoritative_physics->state() != core::SubsystemState::running) {
            throw std::logic_error{"Factory authoritative physics must be running"};
        }
        for (const auto& box : arena_boxes) {
            if (!box.solid && box.material != ArenaMaterial::floor) {
                continue;
            }
            const auto entity = logical_entities.create();
            logical_entities.emplace<physics::StaticBodyComponent>(
                entity, *authoritative_physics, entity,
                physics::BodyDesc{
                    .shape = {.type = physics::ShapeType::box,
                              .half_extent = {box.half_x, box.half_y, box.half_z}},
                    .transform = {.position = {box.center_x, box.center_y, box.center_z}},
                    .friction = 0.6F});
        }
        if (use_original_factory) {
            const auto entity = logical_entities.create();
            logical_entities.emplace<physics::StaticBodyComponent>(entity, *authoritative_physics, entity,
                physics::BodyDesc{.shape = {.type = physics::ShapeType::triangle_mesh, .triangle_mesh = original_factory().collision}, .friction = 0.6F});
        }
        lava_entity = logical_entities.create();
        logical_entities.emplace<DamageVolumeComponent>(
            lava_entity, DamageVolumeComponent{.damage_per_second = 90.0F});
        const auto& lava = arena_surfaces.front();
        logical_entities.emplace<physics::TriggerComponent>(
            lava_entity, *authoritative_physics, lava_entity,
            physics::BodyDesc{
                .shape = {.type = physics::ShapeType::box,
                          .half_extent = {lava.half_x, use_original_factory ? 128.0F : 0.75F, lava.half_z}},
                .transform = {.position = {lava.center_x, use_original_factory ? lava.center_y - 128.0F : -0.25F, lava.center_z}}});

        if (!use_original_factory) {
        factory_lift_entity = logical_entities.create();
        constexpr physics::Vec3 lift_start{-8.0F, 0.25F, 0.0F};
        logical_entities.emplace<TransformComponent>(
            factory_lift_entity, TransformComponent{.position_x = lift_start.x,
                                                     .position_y = lift_start.y,
                                                     .position_z = lift_start.z});
        logical_entities.emplace<KinematicMotionComponent>(
            factory_lift_entity,
            KinematicMotionComponent{.start = lift_start,
                                     .end = {-8.0F, 2.25F, 0.0F},
                                     .travel_ticks = 180,
                                     .dwell_ticks = 60});
        logical_entities.emplace<AuthorityComponent>(
            factory_lift_entity,
            AuthorityComponent{.network_entity = factory_lift_network_entity,
                               .mode = ComponentAuthority::server});
        logical_entities.emplace<ReplicationComponent>(factory_lift_entity);
        logical_entities.emplace<physics::KinematicBodyComponent>(
            factory_lift_entity, *authoritative_physics, factory_lift_entity,
            physics::BodyDesc{
                .shape = {.type = physics::ShapeType::box,
                          .half_extent = {1.0F, 0.25F, 1.0F}},
                .transform = {.position = lift_start},
                .friction = 0.8F});

        }
        attach_character_physics(player);
        attach_character_physics(opponent);
    }

    void attach_character_physics(const Combatant& combatant) {
        combatant.physics->attach(*authoritative_physics, combatant.logical_entity,
                                  {combatant.movement->spawn_x, combatant.movement->spawn_y,
                                   combatant.movement->spawn_z});
    }

    void update_factory_physics() {
        if (authoritative_physics == nullptr) {
            return;
        }
        const float fixed_delta = 1.0F / static_cast<float>(tick_rate);
        auto* lift_transform = logical_entities.get<TransformComponent>(factory_lift_entity);
        auto* lift_motion = logical_entities.get<KinematicMotionComponent>(factory_lift_entity);
        const auto* lift_body = logical_entities.get<physics::KinematicBodyComponent>(
            factory_lift_entity);
        if (lift_transform != nullptr && lift_motion != nullptr && lift_body != nullptr) {
            static_cast<void>(advance_kinematic_motion(
                *authoritative_physics, *lift_body, *lift_transform, *lift_motion, fixed_delta));
            logical_entities.get<ReplicationComponent>(factory_lift_entity)->simulation_tick =
                movement_server->simulation_tick();
        }
        const auto sync = [&](const Combatant& combatant) {
            if (combatant.health->respawn_remaining != 0) {
                if (combatant.physics->attached()) combatant.physics->reset();
                return;
            }
            if (!combatant.physics->attached()) {
                return;
            }
            authoritative_physics->set_character_position(
                combatant.physics->character(), {combatant.transform->position_x,
                                                  combatant.transform->position_y,
                                                  combatant.transform->position_z});
            authoritative_physics->set_character_horizontal_velocity(
                combatant.physics->character(), {combatant.transform->velocity_x, 0.0F,
                                                  combatant.transform->velocity_z});
        };
        sync(player);
        sync(opponent);
        authoritative_physics->simulate(1.0 / static_cast<double>(tick_rate));
        for (const auto& event : authoritative_physics->take_trigger_events()) {
            if (event.trigger != lava_entity) {
                continue;
            }
            const bool inside = event.type != physics::TriggerEventType::exited;
            if (event.other == player.logical_entity) {
                player_in_lava = inside;
            } else if (event.other == opponent.logical_entity) {
                opponent_in_lava = inside;
            }
        }
        for (const auto& event : authoritative_physics->take_character_contact_events()) {
            if (event.other != lava_entity) continue;
            const bool inside = event.type != physics::CharacterContactEventType::exited;
            if (event.character == player.logical_entity) player_in_lava = inside;
            else if (event.character == opponent.logical_entity) opponent_in_lava = inside;
        }
        const auto* damage = logical_entities.get<DamageVolumeComponent>(lava_entity);
        if (damage != nullptr) {
            if (player_in_lava) {
                apply_environment_damage(player, player.health->life+player.shield->value+1.F);
            }
            if (opponent_in_lava) {
                apply_environment_damage(opponent, opponent.health->life+opponent.shield->value+1.F);
            }
        }
    }

    static void synchronize_transform(Combatant& combatant,
                                      const network::MovementState& movement) {
        *combatant.transform = {.position_x = movement.position_x,
                                .position_y = movement.position_y,
                                .position_z = movement.position_z,
                                .velocity_x = movement.velocity_x,
                                .velocity_y = movement.velocity_y,
                                .velocity_z = movement.velocity_z};
        combatant.replication->simulation_tick = movement.simulation_tick;
        combatant.replication->grounded = movement.grounded;
    }

    [[nodiscard]] network::MovementState component_movement(const Combatant& combatant) const {
        return movement_state_from_components(logical_entities, combatant.logical_entity);
    }

    [[nodiscard]] network::WorldSnapshot component_world_snapshot() const {
        std::array<core::EntityId, 2> entities{};
        std::size_t count = 0;
        if (player.health->respawn_remaining == 0) entities[count++] = player.logical_entity;
        if (opponent.health->respawn_remaining == 0) entities[count++] = opponent.logical_entity;
        return capture_component_snapshot(logical_entities,
                                          std::span{entities.data(), count},
                                          movement_server->simulation_tick(),
                                          movement_server->acknowledged_input());
    }

    [[nodiscard]] KinematicMechanismView factory_lift_view() const {
        const auto* transform = logical_entities.get<TransformComponent>(factory_lift_entity);
        const auto* authority = logical_entities.get<AuthorityComponent>(factory_lift_entity);
        if (transform == nullptr || authority == nullptr) return {};
        return {.entity = authority->network_entity,
                .position_x = transform->position_x,
                .position_y = transform->position_y,
                .position_z = transform->position_z,
                .velocity_x = transform->velocity_x,
                .velocity_y = transform->velocity_y,
                .velocity_z = transform->velocity_z};
    }

    void admit_client(const network::ConnectionId connection,
                      network::ClientSession& client,
                      const std::string_view credential,
                      const std::uint64_t nonce) {
        sessions.connected(connection);
        const auto hello_message = wire_round_trip(client.begin(std::string{credential}, nonce));
        const auto hello = network::decode_client_hello(hello_message);
        if (!hello) {
            throw std::runtime_error{"Vertical-slice client hello was malformed"};
        }
        const auto welcome = sessions.admit(connection, *hello, 0, 0.0);
        if (!welcome) {
            throw std::runtime_error{"Vertical-slice session admission failed: " + welcome.error()};
        }
        client.accept(wire_round_trip(network::encode_server_welcome(*welcome)));
    }

    static void synchronize_clock(network::ClientSession& client, const double now) {
        const auto request_message = wire_round_trip(client.create_clock_request(now));
        const auto request = network::decode_clock_request(request_message);
        if (!request) {
            throw std::runtime_error{"Vertical-slice clock request was malformed"};
        }
        client.receive_clock_response(
            wire_round_trip(network::encode_clock_response({
                .nonce = request->nonce,
                .client_send_seconds = request->client_send_seconds,
                .server_receive_seconds = now + 0.001,
                .server_send_seconds = now + 0.001,
            })),
            now + 0.002);
    }

    void route_input(const network::ConnectionId connection,
                     network::ClientSession& session,
                     network::PredictedMovementClient& prediction,
                     const float axis_x,
                     const float axis_z,
                     const bool jump, const bool dodge) {
        const auto message = wire_round_trip(prediction.create_input(axis_x, axis_z, jump, dodge));
        const auto owner = sessions.authorize_input(connection, message);
        if (!owner || *owner != session.controlled_entity()) {
            ++rejected_commands;
            return;
        }
        movement_server->receive(*owner, message);
        ++authorized_input_batches;
    }

    bool respawn_if_ready(Combatant& combatant) {
        if (combatant.health->respawn_remaining == 0) {
            return false;
        }
        --combatant.health->respawn_remaining;
        if (combatant.health->respawn_remaining != 0) {
            return false;
        }
        combatant.health->life = legacy_default_life;
        combatant.shield->value = 0.0F;
        combatant.weapon->cooldown_remaining = 0;
        combatant.weapon->arsenal.clear_modifiers();
        combatant.ability->cooldown_remaining = 0;
        combatant.ability->active_remaining = 0;
        combatant.ability->hit_consumed = false;
        if (use_original_factory) {
            const auto& spawns=original_factory().spawns;
            const auto& spawn=spawns[(combatant.health->deaths*2+combatant.entity()-1)%spawns.size()];
            combatant.movement->spawn_x=spawn.position.x;
            combatant.movement->spawn_y=spawn.position.y;
            combatant.movement->spawn_z=spawn.position.z;
        }
        movement_server->teleport_entity(combatant.entity(), combatant.movement->spawn_x,
                                         combatant.movement->spawn_y, combatant.movement->spawn_z);
        if (authoritative_physics != nullptr && !combatant.physics->attached()) {
            combatant.physics->attach(*authoritative_physics, combatant.logical_entity,
                                      {combatant.movement->spawn_x, combatant.movement->spawn_y,
                                       combatant.movement->spawn_z});
        }
        return true;
    }

    static bool apply_damage(Combatant& attacker, Combatant& target, const float damage) {
        const float absorbed =
            std::min(target.shield->value, damage * legacy_shield_absorption);
        target.shield->value -= absorbed;
        target.health->life -= damage - absorbed;
        if (target.health->life >= 1.0F) {
            return false;
        }
        target.health->life = 0.0F;
        target.health->respawn_remaining = respawn_ticks;
        ++target.health->deaths;
        ++attacker.score->kills;
        return true;
    }

    static bool apply_environment_damage(Combatant& target, const float damage) {
        if (target.health->respawn_remaining != 0) {
            return false;
        }
        const float absorbed =
            std::min(target.shield->value, damage * legacy_shield_absorption);
        target.shield->value -= absorbed;
        target.health->life -= damage - absorbed;
        if (target.health->life >= 1.0F) {
            return false;
        }
        target.health->life = 0.0F;
        target.health->respawn_remaining = respawn_ticks;
        ++target.health->deaths;
        return true;
    }

    bool route_fire(const network::ConnectionId connection,
                    Combatant& shooter,
                    Combatant& target,
                    float aim_x,
                    float aim_y,
                    float aim_z,
                    const float damage,
                    const float maximum_distance) {
        if (shooter.health->respawn_remaining != 0 ||
            target.health->respawn_remaining != 0) {
            return false;
        }
        const float length = std::sqrt(aim_x * aim_x + aim_y * aim_y + aim_z * aim_z);
        if (length < 1.0e-5F) {
            return false;
        }
        aim_x /= length;
        aim_y /= length;
        aim_z /= length;
        const network::FireCommand command{
            .sequence = shooter.weapon->next_fire_sequence++,
            .shooter = shooter.entity(),
            .estimated_server_tick = movement_server->simulation_tick(),
            .aim_x = aim_x,
            .aim_y = aim_y,
            .aim_z = aim_z,
            .maximum_distance = maximum_distance,
        };
        const auto decoded = network::decode_fire_command(
            wire_round_trip(network::encode_fire_command(command)));
        if (!decoded || !sessions.authorize_fire(connection, *decoded)) {
            ++rejected_commands;
            return false;
        }
        ++authorized_fire_commands;
        const auto result = network::decode_fire_result(
            wire_round_trip(network::encode_fire_result(combat.validate(*decoded))));
        if (result && (result->status==network::FireValidationStatus::hit || result->status==network::FireValidationStatus::miss)) {
            shooter.weapon->shot_sequence=command.sequence;
            shooter.weapon->shot_tick=movement_server->simulation_tick();
            const auto position=component_movement(shooter);
            shooter.weapon->shot_impact={position.position_x+aim_x*result->distance,
                position.position_y+0.9F+aim_y*result->distance,position.position_z+aim_z*result->distance};
            shooter.weapon->shot_hit=result->status==network::FireValidationStatus::hit;
            shooter.weapon->shot_contact=shooter.weapon->shot_hit || result->distance<maximum_distance;
        }
        if (!result || result->status != network::FireValidationStatus::hit ||
            result->target != target.entity()) {
            return false;
        }
        static_cast<void>(apply_damage(shooter, target, damage));
        return true;
    }

    PickupActor pickup_actor(Combatant& c, bool hold_pull) {
        const auto p=component_movement(c);
        return {static_cast<std::uint8_t>(c.entity()),{p.position_x,p.position_y,p.position_z},
            c.health->respawn_remaining==0,hold_pull,&c.weapon->arsenal,&c.health->life,&c.shield->value};
    }

    bool route_weapon_actions(const network::ConnectionId connection, Combatant& shooter,
                              Combatant& target, const SliceInput& input) {
        bool hit=false;
        if(shooter.health->respawn_remaining) {shooter.weapon->arsenal.clear_modifiers();return false;}
        for(const auto& action:shooter.weapon->arsenal.tick({input.fire_primary,input.fire_secondary})) {
            shooter.weapon->cooldown_remaining=shooter.weapon->arsenal.cooldown_remaining();
            switch(action.kind){
            case LegacyWeaponActionKind::hitscan:
            case LegacyWeaponActionKind::expansive_hitscan:
            case LegacyWeaponActionKind::charged_hitscan:
                hit=route_fire(connection,shooter,target,input.aim_x,input.aim_y,input.aim_z,
                               action.damage,action.range)||hit;break;
            case LegacyWeaponActionKind::magnetic_projectiles:
                {const auto position=component_movement(shooter);for(std::uint16_t pellet=0;pellet<action.projectile_count&&projectiles.size()<32;++pellet){const float side=(static_cast<int>(pellet%4)-1.5F)*.012F;const float up=(static_cast<int>(pellet/4)-1.F)*.012F;float dx=input.aim_x+side*input.aim_z,dy=input.aim_y+up,dz=input.aim_z-side*input.aim_x;const float l=std::sqrt(dx*dx+dy*dy+dz*dz);if(l<1e-5F)continue;projectiles.push_back({next_projectile_id++,shooter.entity(),action.weapon,position.position_x,position.position_y+.9F,position.position_z,dx/l,dy/l,dz/l,action.projectile_speed,action.projectile_radius,action.damage,0});}}break;
            case LegacyWeaponActionKind::charged_fireball:
                {const auto position=component_movement(shooter);const float l=std::sqrt(input.aim_x*input.aim_x+input.aim_y*input.aim_y+input.aim_z*input.aim_z);if(l>=1e-5F&&projectiles.size()<32)projectiles.push_back({next_projectile_id++,shooter.entity(),action.weapon,position.position_x,position.position_y+.9F,position.position_z,input.aim_x/l,input.aim_y/l,input.aim_z/l,action.projectile_speed,action.projectile_radius,action.damage,action.explosion_radius});}break;
            case LegacyWeaponActionKind::recall_projectiles:
                for(auto& p:projectiles)if(p.owner==shooter.entity()&&p.weapon==SliceWeapon::shotgun)p.returning=true;
                break;
            case LegacyWeaponActionKind::pull_item:
                static_cast<void>(pickups.pull(pickup_actor(shooter,input.fire_secondary),
                    {input.aim_x,input.aim_y,input.aim_z},action.range));
                break;
            case LegacyWeaponActionKind::steer_fireballs:
                {const float l=std::sqrt(input.aim_x*input.aim_x+input.aim_y*input.aim_y+input.aim_z*input.aim_z);if(l>=1e-5F)for(auto& p:projectiles)if(p.owner==shooter.entity()&&p.weapon==SliceWeapon::iron_hell_goat){p.dx=input.aim_x/l;p.dy=input.aim_y/l;p.dz=input.aim_z/l;}}break;
            }
        }
        shooter.weapon->cooldown_remaining=shooter.weapon->arsenal.cooldown_remaining();
        return hit;
    }

    bool advance_projectiles(Combatant& owner,Combatant& target){bool hit=false;const auto owner_pos=component_movement(owner);const auto target_pos=component_movement(target);for(auto& p:projectiles){if(p.owner!=owner.entity()||!p.life)continue;if(p.returning){const float x=owner_pos.position_x-p.x,y=owner_pos.position_y+.9F-p.y,z=owner_pos.position_z-p.z;const float l=std::sqrt(x*x+y*y+z*z);if(l<.35F){p.life=0;continue;}p.dx=x/l;p.dy=y/l;p.dz=z/l;}p.x+=p.dx*p.speed/60.F;p.y+=p.dy*p.speed/60.F;p.z+=p.dz*p.speed/60.F;--p.life;const float x=target_pos.position_x-p.x,y=target_pos.position_y+.9F-p.y,z=target_pos.position_z-p.z;const float range=p.radius+.55F;if(x*x+y*y+z*z<=range*range&&target.health->respawn_remaining==0){static_cast<void>(apply_damage(owner,target,p.damage));owner.weapon->shot_sequence=owner.weapon->next_fire_sequence++;owner.weapon->shot_tick=movement_server->simulation_tick();owner.weapon->shot_impact={p.x,p.y,p.z};owner.weapon->shot_hit=owner.weapon->shot_contact=true;p.life=0;hit=true;}}std::erase_if(projectiles,[](const Projectile& p){return !p.life;});return hit;}

    bool activate_ability(const network::ConnectionId connection,
                          Combatant& combatant,
                          const SliceInput& input) {
        if (!input.use_primary_ability || combatant.health->respawn_remaining != 0 ||
            combatant.ability->cooldown_remaining != 0 ||
            combatant.loadout->selection.ability == SliceAbility::none) {
            return false;
        }
        const auto owner = sessions.controlled_entity(connection);
        if (!owner || *owner != combatant.entity()) {
            ++rejected_commands;
            return false;
        }
        const auto selected = combatant.loadout->selection.ability;
        if (selected == SliceAbility::bite) {
            const float length = std::sqrt(input.aim_x * input.aim_x +
                                           input.aim_z * input.aim_z);
            if (!std::isfinite(length) || length < 1.0e-5F) {
                return false;
            }
            combatant.movement->facing_x = input.aim_x / length;
            combatant.movement->facing_z = input.aim_z / length;
            combatant.ability->active_remaining = bite_duration_ticks;
            combatant.ability->hit_consumed = false;
        } else if (selected == SliceAbility::guard) {
            combatant.shield->value = std::min(
                legacy_maximum_shield,
                combatant.shield->value + hound_guard_shield);
            combatant.ability->active_remaining = guard_duration_ticks;
            combatant.ability->hit_consumed = true;
        }
        combatant.ability->cooldown_remaining = ability_cooldown_ticks(selected);
        ++authorized_ability_commands;
        return true;
    }

    bool resolve_bite(Combatant& attacker, Combatant& target) {
        if (attacker.loadout->selection.ability != SliceAbility::bite ||
            attacker.ability->active_remaining == 0 || attacker.ability->hit_consumed ||
            target.health->respawn_remaining != 0) {
            return false;
        }
        const network::FireCommand probe{
            .sequence = attacker.ability->next_sequence++,
            .shooter = attacker.entity(),
            .estimated_server_tick = movement_server->simulation_tick(),
            .aim_x = attacker.movement->facing_x,
            .aim_z = attacker.movement->facing_z,
            .maximum_distance = hound_bite_range,
        };
        const auto result = combat.validate(probe);
        if (result.status != network::FireValidationStatus::hit ||
            result.target != target.entity()) {
            return false;
        }
        attacker.ability->hit_consumed = true;
        static_cast<void>(apply_damage(attacker, target, hound_bite_damage));
        attacker.health->life = std::min(legacy_maximum_life,
                                         attacker.health->life + hound_bite_life_steal);
        return true;
    }

    void tick(const SliceInput& input) {
        SliceInput opponent_input;
        const auto& player_server = movement_server->entity(player.entity());
        const auto& opponent_server = movement_server->entity(opponent.entity());
        const float difference_x = player_server.position_x - opponent_server.position_x;
        const float difference_z = player_server.position_z - opponent_server.position_z;
        const float distance = std::sqrt(difference_x * difference_x + difference_z * difference_z);
        const float inverse_distance = distance > 1.0e-5F ? 1.0F / distance : 0.0F;
        const float approach = distance > 8.0F ? 0.75F : 0.0F;
        const float strafe = (movement_server->simulation_tick() / 90U) % 2U == 0U
                                 ? 0.35F
                                 : -0.35F;
        if (opponent_ai_enabled) {
            opponent_input.axis_x = difference_x * inverse_distance * approach -
                                    difference_z * inverse_distance * strafe;
            opponent_input.axis_z = difference_z * inverse_distance * approach +
                                    difference_x * inverse_distance * strafe;
            opponent_input.aim_x = difference_x;
            opponent_input.aim_z = difference_z;
            opponent_input.fire_primary = distance <= soul_reaper_range &&
                                          movement_server->simulation_tick() % 90U == 0U;
            opponent_input.use_primary_ability = distance <= hound_bite_range;
        }
        tick(input, opponent_input);
    }

    void tick(const SliceInput& player_input, const SliceInput& opponent_input) {
        if(player_input.weapon_selection<slice_weapon_count)static_cast<void>(player.weapon->arsenal.select(static_cast<SliceWeapon>(player_input.weapon_selection)));
        if(opponent_input.weapon_selection<slice_weapon_count)static_cast<void>(opponent.weapon->arsenal.select(static_cast<SliceWeapon>(opponent_input.weapon_selection)));
        const bool player_respawned = respawn_if_ready(player);
        const bool opponent_respawned = respawn_if_ready(opponent);
        if (player_respawned) {
            player_prediction->reset_local_state(movement_server->entity(player.entity()));
        }
        if (opponent_respawned) {
            opponent_prediction->reset_local_state(movement_server->entity(opponent.entity()));
        }
        movement_server->set_entity_collision_enabled(
            player.entity(), player.health->respawn_remaining == 0);
        movement_server->set_entity_collision_enabled(
            opponent.entity(), opponent.health->respawn_remaining == 0);
        if (opponent.health->respawn_remaining == 0) {
            const std::array collision{movement_server->entity(opponent.entity())};
            player_prediction->set_collision_entities(collision);
        } else {
            player_prediction->set_collision_entities({});
        }
        if (player.health->respawn_remaining == 0) {
            const std::array collision{movement_server->entity(player.entity())};
            opponent_prediction->set_collision_entities(collision);
        } else {
            opponent_prediction->set_collision_entities({});
        }
        if (player.ability->cooldown_remaining != 0) {
            --player.ability->cooldown_remaining;
        }
        if (opponent.ability->cooldown_remaining != 0) {
            --opponent.ability->cooldown_remaining;
        }

        const auto update_facing = [](Combatant& combatant, const SliceInput& input) {
            const float length = std::sqrt(input.aim_x * input.aim_x +
                                           input.aim_z * input.aim_z);
            if (std::isfinite(input.aim_y) && std::isfinite(length) && (length>1e-5F || std::abs(input.aim_y)>1e-5F))
                combatant.weapon->aim_pitch=std::atan2(input.aim_y,length);
            if (std::isfinite(length) && length >= 1.0e-5F) {
                combatant.movement->facing_x = input.aim_x / length;
                combatant.movement->facing_z = input.aim_z / length;
            }
        };
        update_facing(player, player_input);
        update_facing(opponent, opponent_input);
        static_cast<void>(activate_ability(player.authority->connection, player, player_input));
        static_cast<void>(activate_ability(opponent.authority->connection, opponent,
                                           opponent_input));

        const auto movement_axis_x = [](const Combatant& combatant, const SliceInput& input) {
            return combatant.loadout->selection.ability == SliceAbility::bite &&
                           combatant.ability->active_remaining != 0
                       ? combatant.movement->facing_x
                       : input.axis_x;
        };
        const auto movement_axis_z = [](const Combatant& combatant, const SliceInput& input) {
            return combatant.loadout->selection.ability == SliceAbility::bite &&
                           combatant.ability->active_remaining != 0
                       ? combatant.movement->facing_z
                       : input.axis_z;
        };

        route_input(player.authority->connection,
                    player_session,
                    *player_prediction,
                    player.health->respawn_remaining == 0
                        ? movement_axis_x(player, player_input) : 0.0F,
                    player.health->respawn_remaining == 0
                        ? movement_axis_z(player, player_input) : 0.0F,
                    player.health->respawn_remaining == 0 && player_input.jump,
                    player.health->respawn_remaining == 0 && player_input.dodge);
        route_input(opponent.authority->connection,
                    opponent_session,
                    *opponent_prediction,
                    opponent.health->respawn_remaining == 0
                        ? movement_axis_x(opponent, opponent_input) : 0.0F,
                    opponent.health->respawn_remaining == 0
                        ? movement_axis_z(opponent, opponent_input) : 0.0F,
                    opponent.health->respawn_remaining == 0 && opponent_input.jump,
                    opponent.health->respawn_remaining == 0 && opponent_input.dodge);

        if (const auto primary_snapshot = movement_server->tick()) {
            player_prediction->receive(wire_round_trip(*primary_snapshot));
        }
        if (const auto remote_snapshot = movement_server->take_snapshot(opponent.entity())) {
            opponent_prediction->receive(wire_round_trip(*remote_snapshot));
        }
        synchronize_transform(player, movement_server->entity(player.entity()));
        synchronize_transform(opponent, movement_server->entity(opponent.entity()));
        update_factory_physics();
        combat.record(component_world_snapshot());

        if (resolve_bite(player, opponent)) {
            player.presentation->hit_marker_ticks = hit_marker_ticks;
        }
        if (resolve_bite(opponent, player)) {
            opponent.presentation->hit_marker_ticks = hit_marker_ticks;
        }

        if (route_weapon_actions(player.authority->connection,player,opponent,player_input)) {
            player.presentation->hit_marker_ticks = hit_marker_ticks;
        }
        if (route_weapon_actions(opponent.authority->connection,opponent,player,opponent_input)) {
            opponent.presentation->hit_marker_ticks = hit_marker_ticks;
        }
        if(advance_projectiles(player,opponent))player.presentation->hit_marker_ticks=hit_marker_ticks;
        if(advance_projectiles(opponent,player))opponent.presentation->hit_marker_ticks=hit_marker_ticks;
        auto pickup_actors=std::array{pickup_actor(player,player_input.fire_secondary),
                                     pickup_actor(opponent,opponent_input.fire_secondary)};
        pickups.tick(pickup_actors);
        if (player.presentation->hit_marker_ticks != 0) {
            --player.presentation->hit_marker_ticks;
        }
        if (opponent.presentation->hit_marker_ticks != 0) {
            --opponent.presentation->hit_marker_ticks;
        }
        if (player.ability->active_remaining != 0) {
            --player.ability->active_remaining;
        }
        if (opponent.ability->active_remaining != 0) {
            --opponent.ability->active_remaining;
        }
        refresh_snapshot();
    }

    [[nodiscard]] CombatantView view(const Combatant& combatant,
                                     const network::MovementState& movement) const {
        return {.entity = combatant.entity(),
                .position_x = movement.position_x,
                .position_y = movement.position_y,
                .position_z = movement.position_z,
                .velocity_x = movement.velocity_x,
                .velocity_y = movement.velocity_y,
                .velocity_z = movement.velocity_z,
                .life = combatant.health->life,
                .shield = combatant.shield->value,
                 .respawn_remaining_seconds =
                     static_cast<float>(combatant.health->respawn_remaining) /
                                               static_cast<float>(tick_rate),
                 .facing_x = combatant.movement->facing_x,
                 .facing_z = combatant.movement->facing_z,
                 .kills = combatant.score->kills,
                 .deaths = combatant.health->deaths,
                 .character = combatant.loadout->selection.character,
                 .weapon = combatant.weapon->arsenal.active_weapon(),
                 .ability = combatant.loadout->selection.ability,
                 .primary_ability_active = combatant.ability->active_remaining != 0,
                .alive = combatant.health->respawn_remaining == 0,
                .grounded = movement.grounded,
                .air_dodge_available = movement.air_dodge_available,
                .aim_pitch = combatant.weapon->aim_pitch,
                .shot_sequence = combatant.weapon->shot_sequence,
                .shot_tick = combatant.weapon->shot_tick,
                .shot_impact = combatant.weapon->shot_impact,
                .shot_hit = combatant.weapon->shot_hit,
                .shot_contact = combatant.weapon->shot_contact,
                .ammunition = [&]{std::array<std::uint16_t,slice_weapon_count> value{};for(std::size_t i=0;i<value.size();++i)value[i]=combatant.weapon->arsenal.ammo(static_cast<SliceWeapon>(i));return value;}(),
                .owned_weapons = [&]{std::uint8_t value=0;for(std::size_t i=0;i<slice_weapon_count;++i)if(combatant.weapon->arsenal.owns(static_cast<SliceWeapon>(i)))value|=static_cast<std::uint8_t>(1U<<i);return value;}(),
                .weapon_charge_fraction = combatant.weapon->arsenal.charge_fraction(),
                .damage_modifier_ticks = combatant.weapon->arsenal.damage_modifier_ticks(),
                .cooldown_modifier_ticks = combatant.weapon->arsenal.cooldown_modifier_ticks()};
    }

    void refresh_snapshot() {
        if (!movement_server || !player_prediction) {
            return;
        }
        snapshot = {
            .simulation_tick = movement_server->simulation_tick(),
            .player = view(player, component_movement(player)),
            .opponent = view(opponent, component_movement(opponent)),
            .factory_lift = factory_lift_view(),
            .hud = {.life_fraction = player.health->life / legacy_maximum_life,
                    .shield_fraction = player.shield->value / legacy_maximum_shield,
                     .weapon_ready_fraction = player.weapon->arsenal.cooldown_remaining()==0?1.F:1.F-static_cast<float>(player.weapon->arsenal.cooldown_remaining())/std::max(1U,static_cast<unsigned>(legacy_weapon_rule(player.weapon->arsenal.active_weapon()).primary_cooldown_ticks)),
                     .primary_ability_ready_fraction =
                         player.loadout->selection.ability == SliceAbility::none
                             ? 0.0F
                             : 1.0F - static_cast<float>(
                                          player.ability->cooldown_remaining) /
                                          static_cast<float>(ability_cooldown_ticks(
                                              player.loadout->selection.ability)),
                     .primary_ability_active =
                         player.loadout->selection.ability != SliceAbility::none &&
                         player.ability->active_remaining != 0,
                    .hit_marker = player.presentation->hit_marker_ticks != 0,
                    .dead = player.health->respawn_remaining != 0,
                    .kills = player.score->kills,
                    .deaths = player.health->deaths,
                    .ammunition = player.weapon->arsenal.active_ammo(),
                    .maximum_ammunition = legacy_weapon_rule(player.weapon->arsenal.active_weapon()).maximum_ammo,
                    .weapon_charge_fraction = player.weapon->arsenal.charge_fraction()},
            .network = {.active_sessions = sessions.active_sessions(),
                        .authorized_input_batches = authorized_input_batches,
                        .authorized_fire_commands = authorized_fire_commands,
                        .authorized_ability_commands = authorized_ability_commands,
                        .rejected_commands = rejected_commands,
                        .reconciliation_count = player_prediction->reconciliation_count()},
            .scene_id = use_original_factory ? original_factory().scene_id : 0U,
        };
        snapshot.projectile_count=static_cast<std::uint8_t>(std::min<std::size_t>(projectiles.size(),snapshot.projectiles.size()));
        snapshot.pickup_count=static_cast<std::uint8_t>(pickups.views().size());
        std::ranges::copy(pickups.views(),snapshot.pickups.begin());
        for(std::size_t i=0;i<snapshot.projectile_count;++i){const auto& p=projectiles[i];snapshot.projectiles[i]={p.id,p.owner,p.weapon,p.x,p.y,p.z,p.radius};}
    }

    network::ReplicationSettings movement_settings;
    std::vector<ArenaBox> arena_boxes;
    std::vector<ArenaSurface> arena_surfaces;
    network::ServerSessionManager sessions;
    network::ClientSession player_session;
    network::ClientSession opponent_session;
    std::unique_ptr<network::AuthoritativeMovementServer> movement_server;
    std::unique_ptr<network::PredictedMovementClient> player_prediction;
    std::unique_ptr<network::PredictedMovementClient> opponent_prediction;
    network::LagCompensatedCombatServer combat;
    Combatant player;
    Combatant opponent;
    SliceSnapshot snapshot;
    std::uint64_t authorized_input_batches{0};
    std::uint64_t authorized_fire_commands{0};
    std::uint64_t authorized_ability_commands{0};
    std::uint64_t rejected_commands{0};
    bool opponent_ai_enabled{true};
    std::unique_ptr<backends::JoltWorld> owned_authoritative_physics;
    physics::World* authoritative_physics{nullptr};
    core::EntityRegistry logical_entities;
    core::EntityId lava_entity;
    core::EntityId factory_lift_entity;
    bool use_original_factory{false};
    bool player_in_lava{false};
    bool opponent_in_lava{false};
    std::vector<Projectile> projectiles;
    LegacyPickups pickups;
    std::uint32_t next_projectile_id{1};
};

VerticalSliceSimulation::VerticalSliceSimulation(const bool opponent_ai_enabled)
    : VerticalSliceSimulation(VerticalSliceSettings{
          .opponent_ai_enabled = opponent_ai_enabled}) {}

VerticalSliceSimulation::VerticalSliceSimulation(VerticalSliceSettings settings)
    : impl_{std::make_unique<Impl>(settings)} {}

VerticalSliceSimulation::~VerticalSliceSimulation() = default;

network::ReplicationSettings VerticalSliceSimulation::default_movement_settings() {
    return slice_movement_settings();
}

void VerticalSliceSimulation::tick(const SliceInput& input) { impl_->tick(input); }

void VerticalSliceSimulation::tick(const SliceInput& player_input,
                                   const SliceInput& opponent_input) {
    impl_->tick(player_input, opponent_input);
}

void VerticalSliceSimulation::add_shield(const network::NetworkEntityId entity,
                                         const float amount) {
    Impl::Combatant* combatant = nullptr;
    if (impl_->player.entity() == entity) {
        combatant = &impl_->player;
    } else if (impl_->opponent.entity() == entity) {
        combatant = &impl_->opponent;
    }
    if (combatant == nullptr || combatant->health->respawn_remaining != 0 ||
        amount <= 0.0F) {
        return;
    }
    combatant->shield->value =
        std::min(legacy_maximum_shield, combatant->shield->value + amount);
    impl_->refresh_snapshot();
}

bool VerticalSliceSimulation::set_selection(const network::NetworkEntityId entity,
                                            const SlicePlayerSelection selection) {
    if (!valid_slice_selection(selection)) {
        return false;
    }
    Impl::Combatant* combatant = nullptr;
    if (impl_->player.entity() == entity) {
        combatant = &impl_->player;
    } else if (impl_->opponent.entity() == entity) {
        combatant = &impl_->opponent;
    }
    if (combatant == nullptr) {
        return false;
    }
    combatant->loadout->selection = selection;
    combatant->weapon->arsenal.reset(selection.weapon);
    if (selection.ability == SliceAbility::none) {
        combatant->ability->cooldown_remaining = 0;
        combatant->ability->active_remaining = 0;
        combatant->ability->hit_consumed = false;
    }
    impl_->refresh_snapshot();
    return true;
}

bool VerticalSliceSimulation::acquire_weapon(const network::NetworkEntityId entity,const SliceWeapon weapon,const std::uint16_t ammunition){
    Impl::Combatant* c=impl_->player.entity()==entity?&impl_->player:impl_->opponent.entity()==entity?&impl_->opponent:nullptr;
    if(!c||c->health->respawn_remaining)return false;const bool changed=c->weapon->arsenal.acquire(weapon,ammunition);impl_->refresh_snapshot();return changed;
}
bool VerticalSliceSimulation::add_ammunition(const network::NetworkEntityId entity,const SliceWeapon weapon,const std::uint16_t ammunition){
    Impl::Combatant* c=impl_->player.entity()==entity?&impl_->player:impl_->opponent.entity()==entity?&impl_->opponent:nullptr;
    if(!c||c->health->respawn_remaining)return false;const bool changed=c->weapon->arsenal.add_ammo(weapon,ammunition);impl_->refresh_snapshot();return changed;
}
bool VerticalSliceSimulation::select_weapon(const network::NetworkEntityId entity,const SliceWeapon weapon){
    Impl::Combatant* c=impl_->player.entity()==entity?&impl_->player:impl_->opponent.entity()==entity?&impl_->opponent:nullptr;
    if(!c||c->health->respawn_remaining)return false;const bool changed=c->weapon->arsenal.select(weapon);if(changed)c->loadout->selection.weapon=weapon;impl_->refresh_snapshot();return changed;
}

const SliceSnapshot& VerticalSliceSimulation::snapshot() const noexcept {
    return impl_->snapshot;
}

SliceSnapshot VerticalSliceSimulation::snapshot_for(
    const network::NetworkEntityId controlled_entity) const {
    if (controlled_entity != impl_->player.entity() &&
        controlled_entity != impl_->opponent.entity()) {
        throw std::invalid_argument{"Controlled entity is not a slice combatant"};
    }
    const bool controls_player = controlled_entity == impl_->player.entity();
    const auto& local = controls_player ? impl_->player : impl_->opponent;
    const auto& remote = controls_player ? impl_->opponent : impl_->player;
    SliceSnapshot result{
        .simulation_tick = impl_->movement_server->simulation_tick(),
        .player = impl_->view(local, impl_->component_movement(local)),
        .opponent = impl_->view(remote, impl_->component_movement(remote)),
        .factory_lift = impl_->factory_lift_view(),
        .hud = {.life_fraction = local.health->life / legacy_maximum_life,
                .shield_fraction = local.shield->value / legacy_maximum_shield,
                .weapon_ready_fraction = local.weapon->arsenal.cooldown_remaining()==0?1.F:1.F-static_cast<float>(local.weapon->arsenal.cooldown_remaining())/std::max(1U,static_cast<unsigned>(legacy_weapon_rule(local.weapon->arsenal.active_weapon()).primary_cooldown_ticks)),
                .primary_ability_ready_fraction =
                    local.loadout->selection.ability == SliceAbility::none
                        ? 0.0F
                        : 1.0F - static_cast<float>(
                                     local.ability->cooldown_remaining) /
                                     static_cast<float>(ability_cooldown_ticks(
                                         local.loadout->selection.ability)),
                .primary_ability_active =
                    local.loadout->selection.ability != SliceAbility::none &&
                    local.ability->active_remaining != 0,
                .hit_marker = local.presentation->hit_marker_ticks != 0,
                .dead = local.health->respawn_remaining != 0,
                .kills = local.score->kills,
                .deaths = local.health->deaths,
                .ammunition = local.weapon->arsenal.active_ammo(),
                .maximum_ammunition = legacy_weapon_rule(local.weapon->arsenal.active_weapon()).maximum_ammo,
                .weapon_charge_fraction = local.weapon->arsenal.charge_fraction()},
        .network = impl_->snapshot.network,
        .projectiles = impl_->snapshot.projectiles,
        .projectile_count = impl_->snapshot.projectile_count,
    };
    result.scene_id=impl_->use_original_factory ? original_factory().scene_id : 0U;
    result.pickups=impl_->snapshot.pickups;
    result.pickup_count=impl_->snapshot.pickup_count;
    return result;
}

std::span<const ArenaBox> VerticalSliceSimulation::arena() const noexcept {
    return impl_->arena_boxes;
}

std::span<const ArenaSurface> VerticalSliceSimulation::surfaces() const noexcept {
    return impl_->arena_surfaces;
}

const network::ReplicationSettings& VerticalSliceSimulation::movement_settings() const noexcept {
    return impl_->movement_settings;
}

void SlicePresentationFeedback::observe(const SliceSnapshot& snapshot) noexcept {
    const bool reset = !initialized_ || snapshot.opponent.entity != opponent_entity_ ||
                       snapshot.simulation_tick < last_tick_;
    if (reset) {
        state_ = {};
        last_tick_ = snapshot.simulation_tick;
        opponent_entity_ = snapshot.opponent.entity;
        opponent_life_ = snapshot.opponent.life;
        opponent_alive_ = snapshot.opponent.alive;
        initialized_ = true;
        return;
    }
    if (snapshot.simulation_tick == last_tick_) {
        return;
    }

    if (snapshot.opponent.life < opponent_life_) {
        state_.opponent_damage_remaining_seconds = 0.20F;
    }
    if (opponent_alive_ && !snapshot.opponent.alive) {
        state_.opponent_death_remaining_seconds = 0.50F;
    } else if (!opponent_alive_ && snapshot.opponent.alive) {
        state_.opponent_respawn_remaining_seconds = 0.65F;
    }
    last_tick_ = snapshot.simulation_tick;
    opponent_life_ = snapshot.opponent.life;
    opponent_alive_ = snapshot.opponent.alive;
}

void SlicePresentationFeedback::advance(const double elapsed_seconds) noexcept {
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0) {
        return;
    }
    const float elapsed = static_cast<float>(elapsed_seconds);
    const auto advance_timer = [elapsed](float& timer) { timer = std::max(0.0F, timer - elapsed); };
    advance_timer(state_.opponent_damage_remaining_seconds);
    advance_timer(state_.opponent_death_remaining_seconds);
    advance_timer(state_.opponent_respawn_remaining_seconds);
}

const SlicePresentationFeedbackState& SlicePresentationFeedback::state() const noexcept {
    return state_;
}

} // namespace gloom::gameplay
