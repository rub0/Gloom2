#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/component_replication.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <deque>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace gloom::gameplay {
namespace {

constexpr std::size_t combatant_payload_size = 96;
constexpr std::size_t hud_payload_size = 28;
constexpr std::size_t network_payload_size = 48;
constexpr std::size_t mechanism_payload_size = 32;
constexpr std::size_t slice_payload_size = combatant_payload_size * 2 +
                                           mechanism_payload_size + hud_payload_size +
                                           network_payload_size + 4;
constexpr std::size_t maximum_pending_remote_inputs = 256;

template <typename Integer>
void append_integer(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
}

void append_float(std::vector<std::byte>& output, const float value) {
    append_integer(output, std::bit_cast<std::uint32_t>(value));
}

template <typename Integer>
[[nodiscard]] Integer read_integer(const std::span<const std::byte> input,
                                   std::size_t& offset) {
    static_assert(std::is_unsigned_v<Integer>);
    Integer value = 0;
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        value |= static_cast<Integer>(std::to_integer<unsigned int>(input[offset++]))
                 << (index * 8);
    }
    return value;
}

[[nodiscard]] float read_float(const std::span<const std::byte> input,
                               std::size_t& offset) {
    return std::bit_cast<float>(read_integer<std::uint32_t>(input, offset));
}

void append_combatant(std::vector<std::byte>& output, const CombatantView& view) {
    append_integer(output, view.entity);
    append_float(output, view.position_x);
    append_float(output, view.position_y);
    append_float(output, view.position_z);
    append_float(output, view.velocity_x);
    append_float(output, view.velocity_y);
    append_float(output, view.velocity_z);
    append_float(output, view.life);
    append_float(output, view.shield);
    append_float(output, view.respawn_remaining_seconds);
    append_float(output, view.facing_x);
    append_float(output, view.facing_z);
    append_integer(output, view.kills);
    append_integer(output, view.deaths);
    append_integer(output, static_cast<std::uint8_t>(view.character));
    append_integer(output, static_cast<std::uint8_t>(view.weapon));
    append_integer(output, static_cast<std::uint8_t>(view.ability));
    append_integer(output, static_cast<std::uint8_t>(view.primary_ability_active));
    append_integer(output, static_cast<std::uint8_t>(view.alive));
    append_integer(output, static_cast<std::uint8_t>(view.grounded));
    append_float(output,view.aim_pitch);
    append_integer(output,view.shot_sequence);append_integer(output,view.shot_tick);
    for (float x:view.shot_impact) append_float(output,x);
    append_integer(output,static_cast<std::uint8_t>(view.shot_hit));
    append_integer(output,static_cast<std::uint8_t>(view.shot_contact));
}

[[nodiscard]] CombatantView read_combatant(const std::span<const std::byte> input,
                                           std::size_t& offset) {
    CombatantView view;
    view.entity = read_integer<std::uint64_t>(input, offset);
    view.position_x = read_float(input, offset);
    view.position_y = read_float(input, offset);
    view.position_z = read_float(input, offset);
    view.velocity_x = read_float(input, offset);
    view.velocity_y = read_float(input, offset);
    view.velocity_z = read_float(input, offset);
    view.life = read_float(input, offset);
    view.shield = read_float(input, offset);
    view.respawn_remaining_seconds = read_float(input, offset);
    view.facing_x = read_float(input, offset);
    view.facing_z = read_float(input, offset);
    view.kills = read_integer<std::uint32_t>(input, offset);
    view.deaths = read_integer<std::uint32_t>(input, offset);
    view.character = static_cast<SliceCharacter>(read_integer<std::uint8_t>(input, offset));
    view.weapon = static_cast<SliceWeapon>(read_integer<std::uint8_t>(input, offset));
    view.ability = static_cast<SliceAbility>(read_integer<std::uint8_t>(input, offset));
    view.primary_ability_active = read_integer<std::uint8_t>(input, offset) != 0;
    view.alive = read_integer<std::uint8_t>(input, offset) != 0;
    view.grounded = read_integer<std::uint8_t>(input, offset) != 0;
    view.aim_pitch=read_float(input,offset);
    view.shot_sequence=read_integer<std::uint32_t>(input,offset);view.shot_tick=read_integer<std::uint64_t>(input,offset);
    for (auto& x:view.shot_impact) x=read_float(input,offset);
    view.shot_hit=read_integer<std::uint8_t>(input,offset)!=0;
    view.shot_contact=read_integer<std::uint8_t>(input,offset)!=0;
    return view;
}

[[nodiscard]] bool valid_combatant(const CombatantView& view) noexcept {
    return view.entity != 0 && std::isfinite(view.aim_pitch) && std::abs(view.aim_pitch)<=1.571F &&
           std::ranges::all_of(view.shot_impact,[](float x){return std::isfinite(x);}) &&
           (!view.shot_hit || view.shot_contact) && std::isfinite(view.position_x) &&
           std::isfinite(view.position_y) && std::isfinite(view.position_z) &&
           std::isfinite(view.velocity_x) && std::isfinite(view.velocity_y) &&
           std::isfinite(view.velocity_z) &&
           std::isfinite(view.life) && std::isfinite(view.shield) &&
           std::isfinite(view.respawn_remaining_seconds) &&
           std::isfinite(view.facing_x) && std::isfinite(view.facing_z) &&
           valid_slice_selection({.character = view.character,
                                  .weapon = view.weapon,
                                  .ability = view.ability}) &&
           view.life >= 0.0F &&
           view.life <= legacy_maximum_life && view.shield >= 0.0F &&
           view.shield <= legacy_maximum_shield && view.respawn_remaining_seconds >= 0.0F;
}

[[nodiscard]] bool valid_mechanism(const KinematicMechanismView& view) noexcept {
    return view.entity != 0 && std::isfinite(view.position_x) &&
           std::isfinite(view.position_y) && std::isfinite(view.position_z) &&
           std::isfinite(view.velocity_x) && std::isfinite(view.velocity_y) &&
           std::isfinite(view.velocity_z);
}

void append_mechanism(std::vector<std::byte>& output,
                      const KinematicMechanismView& view) {
    append_integer(output, view.entity);
    append_float(output, view.position_x);
    append_float(output, view.position_y);
    append_float(output, view.position_z);
    append_float(output, view.velocity_x);
    append_float(output, view.velocity_y);
    append_float(output, view.velocity_z);
}

[[nodiscard]] KinematicMechanismView read_mechanism(
    const std::span<const std::byte> input, std::size_t& offset) {
    return {.entity = read_integer<std::uint64_t>(input, offset),
            .position_x = read_float(input, offset),
            .position_y = read_float(input, offset),
            .position_z = read_float(input, offset),
            .velocity_x = read_float(input, offset),
            .velocity_y = read_float(input, offset),
            .velocity_z = read_float(input, offset)};
}

struct AbilityCommand {
    std::uint32_t sequence{0};
    network::NetworkEntityId entity{0};
    float aim_x{0.0F};
    float aim_y{0.0F};
    float aim_z{0.0F};
};

[[nodiscard]] network::ProtocolMessage encode_ability_command(const AbilityCommand& command) {
    network::ProtocolMessage message;
    message.kind = network::MessageKind::ability_command;
    message.sequence = command.sequence;
    message.payload.reserve(20);
    append_integer(message.payload, command.entity);
    append_float(message.payload, command.aim_x);
    append_float(message.payload, command.aim_y);
    append_float(message.payload, command.aim_z);
    return message;
}

[[nodiscard]] std::optional<AbilityCommand>
decode_ability_command(const network::ProtocolMessage& message) {
    if (message.kind != network::MessageKind::ability_command || message.sequence == 0 ||
        message.payload.size() != 20) {
        return std::nullopt;
    }
    std::size_t offset = 0;
    AbilityCommand command;
    command.sequence = message.sequence;
    command.entity = read_integer<std::uint64_t>(message.payload, offset);
    command.aim_x = read_float(message.payload, offset);
    command.aim_y = read_float(message.payload, offset);
    command.aim_z = read_float(message.payload, offset);
    const float length = std::sqrt(command.aim_x * command.aim_x +
                                   command.aim_y * command.aim_y +
                                   command.aim_z * command.aim_z);
    if (command.entity == 0 || !std::isfinite(length) || length < 1.0e-5F) {
        return std::nullopt;
    }
    command.aim_x /= length;
    command.aim_y /= length;
    command.aim_z /= length;
    return command;
}

[[nodiscard]] SliceHostMessage host_message(const network::ConnectionId connection,
                                            network::ProtocolMessage message,
                                            const network::Delivery delivery) {
    SliceHostMessage output;
    output.connection = connection;
    output.message = std::move(message);
    output.delivery = delivery;
    return output;
}

[[nodiscard]] SlicePlayerIdentity make_development_identity() {
    static std::atomic_uint64_t counter{1};
    const auto time = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const auto suffix = counter.fetch_add(1, std::memory_order_relaxed);
    const auto account = (time ^ (suffix * 0x9e3779b97f4a7c15ULL)) | 1ULL;
    return {.account_id = account,
            .display_name = "Player-" + std::to_string(account % 10'000U)};
}

} // namespace

network::ProtocolMessage encode_slice_snapshot(const SliceSnapshot& snapshot,
                                               const std::uint32_t acknowledged_input,
                                               const std::uint32_t sequence) {
    if (!valid_combatant(snapshot.player) || !valid_combatant(snapshot.opponent) ||
        ((snapshot.scene_id != 0 and snapshot.scene_id != original_factory().scene_id) || (snapshot.scene_id == 0 && !valid_mechanism(snapshot.factory_lift)) || (snapshot.scene_id != 0 && snapshot.factory_lift.entity != 0)) ||
        !std::isfinite(snapshot.hud.life_fraction) ||
        !std::isfinite(snapshot.hud.shield_fraction) ||
        !std::isfinite(snapshot.hud.weapon_ready_fraction) ||
        !std::isfinite(snapshot.hud.primary_ability_ready_fraction)) {
        throw std::invalid_argument{"Slice snapshot contains invalid fields"};
    }
    network::ProtocolMessage message;
    message.kind = network::MessageKind::gameplay_snapshot;
    message.simulation_tick = snapshot.simulation_tick;
    message.sequence = sequence;
    message.acknowledged_sequence = acknowledged_input;
    message.payload.reserve(slice_payload_size);
    append_combatant(message.payload, snapshot.player);
    append_combatant(message.payload, snapshot.opponent);
    append_mechanism(message.payload, snapshot.factory_lift);
    append_float(message.payload, snapshot.hud.life_fraction);
    append_float(message.payload, snapshot.hud.shield_fraction);
    append_float(message.payload, snapshot.hud.weapon_ready_fraction);
    append_float(message.payload, snapshot.hud.primary_ability_ready_fraction);
    append_integer(message.payload,
                   static_cast<std::uint8_t>(snapshot.hud.primary_ability_active));
    append_integer(message.payload, static_cast<std::uint8_t>(snapshot.hud.hit_marker));
    append_integer(message.payload, static_cast<std::uint8_t>(snapshot.hud.dead));
    append_integer(message.payload, static_cast<std::uint8_t>(snapshot.player.alive));
    append_integer(message.payload, snapshot.hud.kills);
    append_integer(message.payload, snapshot.hud.deaths);
    append_integer(message.payload, snapshot.network.active_sessions);
    append_integer(message.payload, snapshot.network.authorized_input_batches);
    append_integer(message.payload, snapshot.network.authorized_fire_commands);
    append_integer(message.payload, snapshot.network.authorized_ability_commands);
    append_integer(message.payload, snapshot.network.rejected_commands);
    append_integer(message.payload, snapshot.network.reconciliation_count);
    append_integer(message.payload, snapshot.scene_id);
    return message;
}

std::expected<SliceSnapshot, std::string>
decode_slice_snapshot(const network::ProtocolMessage& message) {
    if (message.kind != network::MessageKind::gameplay_snapshot ||
        message.payload.size() != slice_payload_size || message.sequence == 0) {
        return std::unexpected{"Message is not a Gloom slice snapshot"};
    }
    std::size_t offset = 0;
    SliceSnapshot snapshot;
    snapshot.simulation_tick = message.simulation_tick;
    snapshot.player = read_combatant(message.payload, offset);
    snapshot.opponent = read_combatant(message.payload, offset);
    snapshot.factory_lift = read_mechanism(message.payload, offset);
    snapshot.hud.life_fraction = read_float(message.payload, offset);
    snapshot.hud.shield_fraction = read_float(message.payload, offset);
    snapshot.hud.weapon_ready_fraction = read_float(message.payload, offset);
    snapshot.hud.primary_ability_ready_fraction = read_float(message.payload, offset);
    snapshot.hud.primary_ability_active =
        read_integer<std::uint8_t>(message.payload, offset) != 0;
    snapshot.hud.hit_marker = read_integer<std::uint8_t>(message.payload, offset) != 0;
    snapshot.hud.dead = read_integer<std::uint8_t>(message.payload, offset) != 0;
    static_cast<void>(read_integer<std::uint8_t>(message.payload, offset));
    snapshot.hud.kills = read_integer<std::uint32_t>(message.payload, offset);
    snapshot.hud.deaths = read_integer<std::uint32_t>(message.payload, offset);
    snapshot.network.active_sessions = read_integer<std::uint64_t>(message.payload, offset);
    snapshot.network.authorized_input_batches =
        read_integer<std::uint64_t>(message.payload, offset);
    snapshot.network.authorized_fire_commands =
        read_integer<std::uint64_t>(message.payload, offset);
    snapshot.network.authorized_ability_commands =
        read_integer<std::uint64_t>(message.payload, offset);
    snapshot.network.rejected_commands = read_integer<std::uint64_t>(message.payload, offset);
    snapshot.network.reconciliation_count =
        read_integer<std::uint64_t>(message.payload, offset);
    snapshot.scene_id = read_integer<std::uint32_t>(message.payload, offset);
    if (!valid_combatant(snapshot.player) || !valid_combatant(snapshot.opponent) ||
        ((snapshot.scene_id != 0 and snapshot.scene_id != original_factory().scene_id) || (snapshot.scene_id == 0 && !valid_mechanism(snapshot.factory_lift)) || (snapshot.scene_id != 0 && snapshot.factory_lift.entity != 0)) ||
        !std::isfinite(snapshot.hud.life_fraction) ||
        !std::isfinite(snapshot.hud.shield_fraction) ||
        !std::isfinite(snapshot.hud.weapon_ready_fraction) ||
        !std::isfinite(snapshot.hud.primary_ability_ready_fraction) ||
        offset != message.payload.size()) {
        return std::unexpected{"Slice snapshot fields are invalid"};
    }
    return snapshot;
}

struct VerticalSliceRemoteHost::Impl {
    struct RemoteControl {
        network::NetworkEntityId entity{0};
        float axis_x{0.0F};
        float axis_z{0.0F};
        float aim_x{1.0F};
        float aim_y{0.0F};
        float aim_z{0.0F};
        bool jump{false};
        bool fire{false};
        bool ability{false};
        std::deque<network::MovementInput> pending_inputs;
        std::uint32_t latest_received_input{0};
        bool has_received_input{false};
        std::uint32_t acknowledged_input{0};
        std::uint32_t latest_fire_sequence{0};
        bool has_fire_sequence{false};
        std::uint32_t latest_ability_sequence{0};
        bool has_ability_sequence{false};
        std::uint32_t snapshot_sequence{0};
    };

    explicit Impl(SliceRemoteHostSettings settings)
        : identity_provider{settings.identity_provider
                                ? std::move(settings.identity_provider)
                                : make_development_identity_provider()},
          simulation{VerticalSliceSettings{
              .opponent_ai_enabled = settings.opponent_ai_enabled,
              .authoritative_physics = settings.authoritative_physics,
              .original_factory = settings.original_factory}},
          sessions({.maximum_clients = 2,
                    .reconnect_grace_ticks = settings.reconnect_grace_ticks,
                    .maximum_credential_bytes = 512,
                    .authenticate = [](const std::string_view credential) {
                        // The host verifies and resolves the principal exactly once
                        // before admitting it to the generic session manager.
                        return !credential.empty();
                    },
                    .generate_resume_token = settings.generate_resume_token ? std::move(settings.generate_resume_token) : [token = std::uint64_t{0x700d0000}]() mutable {
                        return ++token;
                    }}),
          lobby({.required_players = settings.required_players,
                 .reconnect_grace_ticks = settings.reconnect_grace_ticks,
                 .validate_identity = settings.validate_identity
                                          ? std::move(settings.validate_identity)
                                          : valid_development_identity}),
          spectator_snapshot{simulation.snapshot()} {}

    void apply_network_metrics(SliceSnapshot& snapshot) const {
        snapshot.network.active_sessions = sessions.active_sessions();
        snapshot.network.authorized_input_batches = authorized_input_batches;
        snapshot.network.authorized_fire_commands = authorized_fire_commands;
        snapshot.network.authorized_ability_commands = authorized_ability_commands;
        snapshot.network.rejected_commands = rejected_commands;
        snapshot.network.reconciliation_count = 0;
    }

    std::shared_ptr<const SliceIdentityProvider> identity_provider;
    VerticalSliceSimulation simulation;
    network::ServerSessionManager sessions;
    SliceMatchLobby lobby;
    std::unordered_map<network::ConnectionId, RemoteControl> controls;
    std::vector<SliceHostMessage> pending_messages;
    SliceSnapshot spectator_snapshot;
    std::uint64_t authorized_input_batches{0};
    std::uint64_t authorized_fire_commands{0};
    std::uint64_t authorized_ability_commands{0};
    std::uint64_t rejected_commands{0};
    std::uint64_t server_tick{0};

    void queue_lobby_state() {
        std::erase_if(pending_messages, [](const SliceHostMessage& message) {
            return message.message.kind == network::MessageKind::lobby_state;
        });
        const auto state = encode_lobby_state(lobby.state());
        for (const auto& [connection, control] : controls) {
            static_cast<void>(control);
            pending_messages.push_back(host_message(connection, state,
                                                    network::Delivery::reliable));
        }
    }
};

VerticalSliceRemoteHost::VerticalSliceRemoteHost(const bool opponent_ai_enabled)
    : VerticalSliceRemoteHost(SliceRemoteHostSettings{
          .opponent_ai_enabled = opponent_ai_enabled,
          .required_players = 1,
          .validate_identity = valid_development_identity}) {}

VerticalSliceRemoteHost::VerticalSliceRemoteHost(SliceRemoteHostSettings settings)
    : impl_{std::make_unique<Impl>(std::move(settings))} {}

VerticalSliceRemoteHost::~VerticalSliceRemoteHost() = default;

void VerticalSliceRemoteHost::connected(const network::ConnectionId connection) {
    impl_->sessions.connected(connection);
}

void VerticalSliceRemoteHost::disconnected(const network::ConnectionId connection) {
    const auto entity = impl_->sessions.controlled_entity(connection);
    impl_->sessions.disconnected(connection, impl_->server_tick);
    if (entity) {
        impl_->lobby.disconnected(*entity, impl_->server_tick);
    }
    impl_->controls.erase(connection);
    impl_->queue_lobby_state();
}

std::vector<SliceHostMessage>
VerticalSliceRemoteHost::receive(const network::ConnectionId connection,
                                 const network::ProtocolMessage& message,
                                 const double now_seconds) {
    std::vector<SliceHostMessage> output;
    if (message.kind == network::MessageKind::client_hello) {
        const auto hello = network::decode_client_hello(message);
        if (!hello || !impl_->sessions.pending(connection) || hello->client_nonce == 0 ||
            hello->credential.size() > 512 || !std::isfinite(now_seconds) || now_seconds < 0) return output;
        const auto identity = hello
                                  ? impl_->identity_provider->verify(hello->credential)
                                  : std::expected<SlicePlayerIdentity, std::string>{
                                        std::unexpected{"Malformed client hello"}};
        if (!hello || !identity) {
            return output;
        }
        if (!impl_->lobby.can_admit_identity(*identity, hello->resume_token != 0)) return output;
        const auto welcome = impl_->sessions.admit(
            connection, *hello, impl_->server_tick, now_seconds,
            std::to_string(identity->account_id) + ":" + identity->display_name);
        if (!welcome || (welcome->controlled_entity != VerticalSliceSimulation::player_entity &&
                         welcome->controlled_entity != VerticalSliceSimulation::opponent_entity)) {
            return output;
        }
        if (!impl_->lobby.admit(welcome->controlled_entity, *identity,
                                impl_->server_tick)) {
            impl_->sessions.disconnected(connection, impl_->server_tick);
            return output;
        }
        impl_->controls.insert_or_assign(
            connection, Impl::RemoteControl{.entity = welcome->controlled_entity});
        output.push_back(host_message(connection,
                                      network::encode_server_welcome(*welcome),
                                      network::Delivery::reliable));
        impl_->queue_lobby_state();
        return output;
    }
    if (message.kind == network::MessageKind::clock_request) {
        const auto request = network::decode_clock_request(message);
        if (request && impl_->sessions.controlled_entity(connection)) {
            output.push_back(host_message(
                connection,
                network::encode_clock_response({.nonce = request->nonce,
                                                .client_send_seconds =
                                                    request->client_send_seconds,
                                                .server_receive_seconds = now_seconds,
                                                .server_send_seconds = now_seconds}),
                network::Delivery::unreliable));
        }
        return output;
    }
    if (message.kind == network::MessageKind::lobby_command) {
        const auto owner = impl_->sessions.controlled_entity(connection);
        const auto selection = decode_lobby_selection(message);
        const auto ready = decode_lobby_ready(message);
        if (selection && owner && impl_->lobby.set_selection(*owner, *selection) &&
            impl_->simulation.set_selection(*owner, *selection)) {
            impl_->queue_lobby_state();
        } else if (ready && owner && impl_->lobby.set_ready(*owner, *ready)) {
            impl_->queue_lobby_state();
        } else if ((!selection && !ready) || !owner) {
            ++impl_->rejected_commands;
        }
        return output;
    }
    if (message.kind == network::MessageKind::input_command) {
        const auto owner = impl_->sessions.authorize_input(connection, message);
        const auto inputs = network::decode_movement_input_batch(message);
        auto control = impl_->controls.find(connection);
        if (control != impl_->controls.end() && owner && *owner == control->second.entity &&
            impl_->lobby.accepts_gameplay(*owner) &&
            inputs && !inputs->empty()) {
            ++impl_->authorized_input_batches;
            for (const auto& input : *inputs) {
                if ((!control->second.has_received_input ||
                     network::sequence_more_recent(input.sequence,
                                                   control->second.latest_received_input)) &&
                    control->second.pending_inputs.size() < maximum_pending_remote_inputs) {
                    control->second.pending_inputs.push_back(input);
                    control->second.latest_received_input = input.sequence;
                    control->second.has_received_input = true;
                }
            }
        } else {
            ++impl_->rejected_commands;
        }
        return output;
    }
    if (message.kind == network::MessageKind::event) {
        const auto fire = network::decode_fire_command(message);
        auto control = impl_->controls.find(connection);
        if (fire && impl_->sessions.authorize_fire(connection, *fire) &&
            control != impl_->controls.end() &&
            impl_->lobby.accepts_gameplay(control->second.entity) &&
            (!control->second.has_fire_sequence ||
             network::sequence_more_recent(fire->sequence,
                                           control->second.latest_fire_sequence))) {
            control->second.latest_fire_sequence = fire->sequence;
            control->second.has_fire_sequence = true;
            control->second.aim_x = fire->aim_x;
            control->second.aim_y = fire->aim_y;
            control->second.aim_z = fire->aim_z;
            control->second.fire = true;
            ++impl_->authorized_fire_commands;
        } else {
            ++impl_->rejected_commands;
        }
        return output;
    }
    if (message.kind == network::MessageKind::ability_command) {
        const auto ability = decode_ability_command(message);
        const auto owner = impl_->sessions.controlled_entity(connection);
        auto control = impl_->controls.find(connection);
        if (ability && owner && *owner == ability->entity &&
            control != impl_->controls.end() && control->second.entity == *owner &&
            impl_->lobby.accepts_gameplay(*owner) &&
            (!control->second.has_ability_sequence ||
             network::sequence_more_recent(ability->sequence,
                                           control->second.latest_ability_sequence))) {
            control->second.latest_ability_sequence = ability->sequence;
            control->second.has_ability_sequence = true;
            control->second.aim_x = ability->aim_x;
            control->second.aim_y = ability->aim_y;
            control->second.aim_z = ability->aim_z;
            control->second.ability = true;
            ++impl_->authorized_ability_commands;
        } else {
            ++impl_->rejected_commands;
        }
    }
    return output;
}

std::optional<SliceHostMessage> VerticalSliceRemoteHost::tick() {
    auto messages = tick_clients();
    if (messages.empty()) {
        return std::nullopt;
    }
    return std::move(messages.front());
}

std::vector<SliceHostMessage> VerticalSliceRemoteHost::tick_clients() {
    ++impl_->server_tick;
    impl_->sessions.expire(impl_->server_tick);
    std::vector<SliceHostMessage> output = std::move(impl_->pending_messages);
    impl_->pending_messages.clear();
    if (impl_->lobby.tick(impl_->server_tick)) {
        impl_->queue_lobby_state();
        output.insert(output.end(),
                      std::make_move_iterator(impl_->pending_messages.begin()),
                      std::make_move_iterator(impl_->pending_messages.end()));
        impl_->pending_messages.clear();
    }
    SliceInput player_input;
    SliceInput opponent_input;
    for (auto& [connection, control] : impl_->controls) {
        static_cast<void>(connection);
        if (!control.pending_inputs.empty()) {
            const auto input = control.pending_inputs.front();
            control.pending_inputs.pop_front();
            control.axis_x = input.axis_x;
            control.axis_z = input.axis_z;
            control.jump = control.jump || input.jump;
            control.acknowledged_input = input.sequence;
        }
        SliceInput input{.axis_x = control.axis_x,
                         .axis_z = control.axis_z,
                         .aim_x = control.aim_x,
                         .aim_y = control.aim_y,
                         .aim_z = control.aim_z,
                         .jump = std::exchange(control.jump, false),
                         .fire_primary = std::exchange(control.fire, false),
                         .use_primary_ability = std::exchange(control.ability, false)};
        if (control.entity == VerticalSliceSimulation::player_entity) {
            player_input = input;
        } else if (control.entity == VerticalSliceSimulation::opponent_entity) {
            opponent_input = input;
        }
    }
    if (impl_->lobby.state().phase == SliceMatchPhase::active) {
        impl_->simulation.tick(player_input, opponent_input);
    }
    impl_->spectator_snapshot = impl_->simulation.snapshot();
    impl_->apply_network_metrics(impl_->spectator_snapshot);
    if (impl_->lobby.state().phase != SliceMatchPhase::active ||
        impl_->simulation.snapshot().simulation_tick % 3U != 0U) {
        return output;
    }
    output.reserve(output.size() + impl_->controls.size());
    for (auto& [connection, control] : impl_->controls) {
        auto snapshot = impl_->simulation.snapshot_for(control.entity);
        impl_->apply_network_metrics(snapshot);
        output.push_back(host_message(
            connection,
            encode_slice_snapshot(snapshot,
                                  control.acknowledged_input,
                                  ++control.snapshot_sequence),
            network::Delivery::unreliable));
    }
    return output;
}

const SliceSnapshot& VerticalSliceRemoteHost::snapshot() const noexcept {
    return impl_->spectator_snapshot;
}

std::span<const ArenaBox> VerticalSliceRemoteHost::arena() const noexcept {
    return impl_->simulation.arena();
}

std::span<const ArenaSurface> VerticalSliceRemoteHost::surfaces() const noexcept {
    return impl_->simulation.surfaces();
}

std::size_t VerticalSliceRemoteHost::active_clients() const noexcept {
    return impl_->sessions.active_sessions();
}

const network::SessionMetrics& VerticalSliceRemoteHost::session_metrics() const noexcept {
    return impl_->sessions.metrics();
}

const SliceLobbyState& VerticalSliceRemoteHost::lobby() const noexcept {
    return impl_->lobby.state();
}

struct VerticalSliceRemoteClient::Impl {
    explicit Impl(SlicePlayerIdentity configured_identity,
                  const SlicePlayerSelection configured_selection,
                  const bool automatic_selection)
        : identity{std::move(configured_identity)},
          desired_selection{configured_selection},
          selection_confirmed{automatic_selection} {
        if (!valid_development_identity(identity) ||
            !valid_slice_selection(desired_selection)) {
            throw std::invalid_argument{"Remote slice client identity is invalid"};
        }
    }

    network::ClientSession session;
    SlicePlayerIdentity identity;
    SlicePlayerSelection desired_selection;
    SliceLobbyState lobby;
    std::unique_ptr<network::PredictedMovementClient> prediction;
    SliceSnapshot snapshot;
    std::uint32_t next_fire_sequence{1};
    std::uint64_t next_fire_tick{0};
    std::uint32_t next_ability_sequence{1};
    std::uint64_t ability_request_tick{0};
    bool ability_request_pending{false};
    std::uint32_t predicted_ability_remaining{0};
    float predicted_ability_x{1.0F};
    float predicted_ability_z{0.0F};
    bool received_snapshot{false};
    bool selection_sent{false};
    bool ready_sent{false};
    bool clock_ready{false};
    bool selection_confirmed{true};
    core::EntityRegistry logical_entities;
    core::EntityId local_logical;
    core::EntityId remote_logical;

    void compose_replication_entities(const network::NetworkEntityId controlled) {
        logical_entities.clear();
        const auto other = controlled == VerticalSliceSimulation::player_entity
                               ? VerticalSliceSimulation::opponent_entity
                               : VerticalSliceSimulation::player_entity;
        local_logical = compose_slice_character(
            logical_entities, {.network_entity = controlled,
                               .selection = desired_selection});
        remote_logical = compose_slice_character(
            logical_entities, {.network_entity = other});
        logical_entities.get<AuthorityComponent>(local_logical)->mode =
            ComponentAuthority::predicted_owner;
        logical_entities.get<AuthorityComponent>(remote_logical)->mode =
            ComponentAuthority::interpolated_remote;
    }

    void apply_lobby_selections(const SliceLobbyState& state) {
        for (const auto& player : state.players) {
            for (const auto entity : {local_logical, remote_logical}) {
                const auto* authority = logical_entities.get<AuthorityComponent>(entity);
                auto* loadout = logical_entities.get<CharacterLoadoutComponent>(entity);
                if (authority != nullptr && loadout != nullptr &&
                    authority->network_entity == player.entity) {
                    loadout->selection = player.selection;
                }
            }
        }
    }
};

VerticalSliceRemoteClient::VerticalSliceRemoteClient()
    : VerticalSliceRemoteClient(make_development_identity(), {}) {}

VerticalSliceRemoteClient::VerticalSliceRemoteClient(SlicePlayerIdentity identity)
    : VerticalSliceRemoteClient(std::move(identity), {}) {}

VerticalSliceRemoteClient::VerticalSliceRemoteClient(
    const SlicePlayerSelection selection, const bool automatic_selection)
    : VerticalSliceRemoteClient(make_development_identity(), selection,
                                automatic_selection) {}

VerticalSliceRemoteClient::VerticalSliceRemoteClient(
    SlicePlayerIdentity identity, const SlicePlayerSelection selection)
    : VerticalSliceRemoteClient(std::move(identity), selection, true) {}

VerticalSliceRemoteClient::VerticalSliceRemoteClient(
    SlicePlayerIdentity identity, const SlicePlayerSelection selection,
    const bool automatic_selection)
    : impl_{std::make_unique<Impl>(std::move(identity), selection,
                                  automatic_selection)} {}

VerticalSliceRemoteClient::~VerticalSliceRemoteClient() = default;

SliceWireMessage VerticalSliceRemoteClient::begin(std::string credential) {
    return begin_with_credential(encode_slice_credential(credential, impl_->identity));
}

SliceWireMessage VerticalSliceRemoteClient::begin_with_credential(std::string credential) {
    impl_->selection_sent = false;
    impl_->ready_sent = false;
    impl_->clock_ready = false;
    return {.message = impl_->session.begin(std::move(credential), 0x700d1001),
            .delivery = network::Delivery::reliable};
}

SliceWireMessage VerticalSliceRemoteClient::reconnect(std::string credential) {
    return reconnect_with_credential(
        encode_slice_credential(credential, impl_->identity));
}

SliceWireMessage VerticalSliceRemoteClient::reconnect_with_credential(
    std::string credential) {
    impl_->selection_sent = false;
    impl_->ready_sent = false;
    impl_->clock_ready = false;
    return {.message = impl_->session.reconnect(std::move(credential), 0x700d1002),
            .delivery = network::Delivery::reliable};
}

std::optional<SliceWireMessage>
VerticalSliceRemoteClient::receive(const network::ProtocolMessage& message,
                                   const double now_seconds) {
    if (message.kind == network::MessageKind::server_welcome) {
        impl_->session.accept(message);
        if (impl_->session.controlled_entity() != VerticalSliceSimulation::player_entity &&
            impl_->session.controlled_entity() != VerticalSliceSimulation::opponent_entity) {
            throw std::runtime_error{"Remote slice client received the wrong entity"};
        }
        const auto settings = impl_->received_snapshot && impl_->snapshot.scene_id != 0 ? factory_movement_settings() : VerticalSliceSimulation::default_movement_settings();
        const auto entity = impl_->session.controlled_entity();
        impl_->compose_replication_entities(entity);
        const bool resumes_known_state = impl_->received_snapshot &&
                                         impl_->snapshot.player.entity == entity;
        impl_->prediction = std::make_unique<network::PredictedMovementClient>(
            settings,
            entity,
            network::MovementState{
                .entity = entity,
                .position_x = resumes_known_state
                                  ? impl_->snapshot.player.position_x
                                  : entity == VerticalSliceSimulation::player_entity ? -5.0F
                                                                                     : 5.0F,
                .position_y = resumes_known_state ? impl_->snapshot.player.position_y : 0.0F,
                .position_z = resumes_known_state ? impl_->snapshot.player.position_z : 0.0F,
                .velocity_x = resumes_known_state ? impl_->snapshot.player.velocity_x : 0.0F,
                .velocity_y = resumes_known_state ? impl_->snapshot.player.velocity_y : 0.0F,
                .velocity_z = resumes_known_state ? impl_->snapshot.player.velocity_z : 0.0F,
                .grounded = !resumes_known_state || impl_->snapshot.player.grounded,
            });
        return SliceWireMessage{
            .message = impl_->session.create_clock_request(now_seconds),
            .delivery = network::Delivery::unreliable};
    }
    if (message.kind == network::MessageKind::clock_response) {
        impl_->session.receive_clock_response(message, now_seconds);
        impl_->clock_ready = true;
        if (!impl_->selection_confirmed) {
            return std::nullopt;
        }
        impl_->selection_sent = true;
        return SliceWireMessage{.message = encode_lobby_selection(impl_->desired_selection),
                                .delivery = network::Delivery::reliable};
    }
    if (message.kind == network::MessageKind::lobby_state) {
        const auto state = decode_lobby_state(message);
        if (state && impl_->local_logical.valid()) {
            impl_->apply_lobby_selections(*state);
        }
        if (state && state->revision > impl_->lobby.revision) {
            impl_->lobby = *state;
            const auto player = std::ranges::find(state->players, impl_->session.controlled_entity(), &SliceLobbyPlayer::entity);
            if (player != state->players.end()) impl_->identity = player->identity;
        }
        if (state && impl_->selection_sent && !impl_->ready_sent &&
            state->phase == SliceMatchPhase::waiting) {
            const auto entity = impl_->session.controlled_entity();
            const auto player = std::ranges::find(state->players, entity,
                                                  &SliceLobbyPlayer::entity);
            if (player != state->players.end() &&
                player->selection == impl_->desired_selection && !player->ready) {
                impl_->ready_sent = true;
                return SliceWireMessage{.message = encode_lobby_ready(true),
                                        .delivery = network::Delivery::reliable};
            }
        }
        return std::nullopt;
    }
    if (message.kind != network::MessageKind::gameplay_snapshot || !impl_->prediction) {
        return std::nullopt;
    }
    const auto decoded = decode_slice_snapshot(message);
    if (!decoded) {
        return std::nullopt;
    }
    if (impl_->received_snapshot && (decoded->scene_id != impl_->snapshot.scene_id ||
        decoded->simulation_tick < impl_->snapshot.simulation_tick)) return std::nullopt;
    if (impl_->ability_request_pending &&
        decoded->simulation_tick > impl_->ability_request_tick) {
        impl_->ability_request_pending = false;
    }
    network::WorldSnapshot movement{
        .simulation_tick = decoded->simulation_tick,
        .acknowledged_input = message.acknowledged_sequence,
        .entities = {{.entity = decoded->player.entity,
                      .simulation_tick = decoded->simulation_tick,
                      .position_x = decoded->player.position_x,
                      .position_y = decoded->player.position_y,
                      .position_z = decoded->player.position_z,
                      .velocity_x = decoded->player.velocity_x,
                      .velocity_y = decoded->player.velocity_y,
                      .velocity_z = decoded->player.velocity_z,
                      .grounded = decoded->player.grounded},
                     {.entity = decoded->opponent.entity,
                      .simulation_tick = decoded->simulation_tick,
                      .position_x = decoded->opponent.position_x,
                      .position_y = decoded->opponent.position_y,
                      .position_z = decoded->opponent.position_z,
                      .velocity_x = decoded->opponent.velocity_x,
                      .velocity_y = decoded->opponent.velocity_y,
                      .velocity_z = decoded->opponent.velocity_z,
                      .grounded = decoded->opponent.grounded}},
    };
    if (impl_->received_snapshot && decoded->scene_id != impl_->snapshot.scene_id) return std::nullopt;
    if (!impl_->received_snapshot && decoded->scene_id != 0) {
        impl_->prediction = std::make_unique<network::PredictedMovementClient>(factory_movement_settings(),
            decoded->player.entity, movement.entities.front());
    }
    const std::array component_entities{impl_->local_logical, impl_->remote_logical};
    static_cast<void>(apply_component_snapshot(impl_->logical_entities,
                                                component_entities,
                                                movement,
                                                true));
    const bool respawned = impl_->received_snapshot && impl_->snapshot.hud.dead &&
                           !decoded->hud.dead;
    impl_->prediction->receive(network::encode_world_snapshot(movement, message.sequence));
    if (respawned) {
        impl_->prediction->reset_local_state(movement.entities.front());
    }
    impl_->snapshot = *decoded;
    const auto& predicted = impl_->prediction->local_state();
    impl_->snapshot.player.position_x = predicted.position_x;
    impl_->snapshot.player.position_y = predicted.position_y;
    impl_->snapshot.player.position_z = predicted.position_z;
    auto* predicted_transform = impl_->logical_entities.get<TransformComponent>(
        impl_->local_logical);
    *predicted_transform = {.position_x = predicted.position_x,
                            .position_y = predicted.position_y,
                            .position_z = predicted.position_z,
                            .velocity_x = predicted.velocity_x,
                            .velocity_y = predicted.velocity_y,
                            .velocity_z = predicted.velocity_z};
    impl_->snapshot.network.reconciliation_count = impl_->prediction->reconciliation_count();
    impl_->received_snapshot = true;
    return std::nullopt;
}

bool VerticalSliceRemoteClient::set_desired_selection(
    const SlicePlayerSelection selection) noexcept {
    if (!valid_slice_selection(selection) || impl_->selection_confirmed ||
        impl_->selection_sent || impl_->lobby.phase != SliceMatchPhase::waiting) {
        return false;
    }
    impl_->desired_selection = selection;
    if (impl_->local_logical.valid()) {
        if (auto* loadout = impl_->logical_entities.get<CharacterLoadoutComponent>(
                impl_->local_logical)) {
            loadout->selection = selection;
        }
    }
    return true;
}

std::optional<SliceWireMessage> VerticalSliceRemoteClient::confirm_selection() {
    if (impl_->selection_confirmed || impl_->selection_sent ||
        impl_->lobby.phase != SliceMatchPhase::waiting) {
        return std::nullopt;
    }
    impl_->selection_confirmed = true;
    if (!impl_->clock_ready) {
        return std::nullopt;
    }
    impl_->selection_sent = true;
    return SliceWireMessage{.message = encode_lobby_selection(impl_->desired_selection),
                            .delivery = network::Delivery::reliable};
}

SlicePlayerSelection VerticalSliceRemoteClient::desired_selection() const noexcept {
    return impl_->desired_selection;
}

std::vector<SliceWireMessage>
VerticalSliceRemoteClient::create_input(const SliceInput& input) {
    std::vector<SliceWireMessage> output;
    if (!impl_->prediction || !impl_->received_snapshot || !impl_->session.active() ||
        impl_->lobby.phase != SliceMatchPhase::active) {
        return output;
    }
    const bool dead = impl_->received_snapshot && impl_->snapshot.hud.dead;
    if (impl_->received_snapshot && impl_->snapshot.opponent.alive) {
        const auto& opponent = impl_->snapshot.opponent;
        const std::array collision{network::MovementState{
            .entity = opponent.entity,
            .simulation_tick = impl_->snapshot.simulation_tick,
            .position_x = opponent.position_x,
            .position_y = opponent.position_y,
            .position_z = opponent.position_z,
            .velocity_x = opponent.velocity_x,
            .velocity_y = opponent.velocity_y,
            .velocity_z = opponent.velocity_z,
            .grounded = opponent.grounded}};
        impl_->prediction->set_collision_entities(collision);
    } else {
        impl_->prediction->set_collision_entities({});
    }
    const float aim_length = std::sqrt(input.aim_x * input.aim_x +
                                       input.aim_y * input.aim_y +
                                       input.aim_z * input.aim_z);
    const float horizontal_aim_length = std::sqrt(input.aim_x * input.aim_x +
                                                  input.aim_z * input.aim_z);
    const bool request_ability = !dead && input.use_primary_ability &&
                                 !impl_->ability_request_pending &&
                                 impl_->snapshot.player.ability != SliceAbility::none &&
                                 impl_->snapshot.hud.primary_ability_ready_fraction >= 0.999F &&
                                 std::isfinite(aim_length) && aim_length >= 1.0e-5F &&
                                 std::isfinite(horizontal_aim_length) &&
                                 horizontal_aim_length >= 1.0e-5F;
    if (request_ability && impl_->snapshot.player.ability == SliceAbility::bite) {
        impl_->predicted_ability_remaining = 30;
        impl_->predicted_ability_x = input.aim_x / horizontal_aim_length;
        impl_->predicted_ability_z = input.aim_z / horizontal_aim_length;
    }
    const bool predicting_ability = impl_->predicted_ability_remaining != 0;
    output.push_back({.message = impl_->prediction->create_input(
                          dead ? 0.0F
                               : predicting_ability ? impl_->predicted_ability_x : input.axis_x,
                          dead ? 0.0F
                               : predicting_ability ? impl_->predicted_ability_z : input.axis_z,
                          !dead && input.jump),
                      .delivery = network::Delivery::unreliable});
    if (impl_->predicted_ability_remaining != 0) {
        --impl_->predicted_ability_remaining;
    }
    if (dead || !input.fire_primary ||
        impl_->snapshot.simulation_tick < impl_->next_fire_tick) {
        // Ability traffic remains independent from weapon fire.
    } else {
        const float length = std::sqrt(input.aim_x * input.aim_x +
                                       input.aim_y * input.aim_y +
                                       input.aim_z * input.aim_z);
        if (std::isfinite(length) && length >= 1.0e-5F) {
            output.push_back({.message = network::encode_fire_command({
                                  .sequence = impl_->next_fire_sequence++,
                                  .shooter = impl_->session.controlled_entity(),
                                  .estimated_server_tick = impl_->snapshot.simulation_tick,
                                  .aim_x = input.aim_x / length,
                                  .aim_y = input.aim_y / length,
                                  .aim_z = input.aim_z / length,
                                  .maximum_distance = soul_reaper_range,
                              }),
                              .delivery = network::Delivery::unreliable});
            impl_->next_fire_tick = impl_->snapshot.simulation_tick + 30U;
        }
    }
    if (!request_ability) {
        return output;
    }
    output.push_back({.message = encode_ability_command({
                          .sequence = impl_->next_ability_sequence++,
                          .entity = impl_->session.controlled_entity(),
                          .aim_x = input.aim_x / aim_length,
                          .aim_y = input.aim_y / aim_length,
                          .aim_z = input.aim_z / aim_length,
                      }),
                      .delivery = network::Delivery::reliable});
    impl_->ability_request_pending = true;
    impl_->ability_request_tick = impl_->snapshot.simulation_tick;
    return output;
}

bool VerticalSliceRemoteClient::active() const noexcept { return impl_->session.active(); }

bool VerticalSliceRemoteClient::has_snapshot() const noexcept {
    return impl_->received_snapshot;
}

const SliceSnapshot& VerticalSliceRemoteClient::snapshot() const noexcept {
    return impl_->snapshot;
}

const network::ClientSession& VerticalSliceRemoteClient::session() const noexcept {
    return impl_->session;
}

const network::ReconciliationMetrics&
VerticalSliceRemoteClient::reconciliation_metrics() const noexcept {
    if (!impl_->prediction) {
        static const network::ReconciliationMetrics empty;
        return empty;
    }
    return impl_->prediction->reconciliation_metrics();
}

const SlicePlayerIdentity& VerticalSliceRemoteClient::identity() const noexcept {
    return impl_->identity;
}

const SliceLobbyState& VerticalSliceRemoteClient::lobby() const noexcept {
    return impl_->lobby;
}

} // namespace gloom::gameplay
