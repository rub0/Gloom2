#include <gloom/gameplay/match_lobby.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace gloom::gameplay {
namespace {

constexpr std::size_t maximum_display_name_bytes = 20;

class DevelopmentSliceIdentityProvider final : public SliceIdentityProvider {
public:
    explicit DevelopmentSliceIdentityProvider(std::string secret)
        : secret_{std::move(secret)} {
        if (secret_.empty()) {
            throw std::invalid_argument{"Development identity secret cannot be empty"};
        }
    }

    [[nodiscard]] std::expected<SlicePlayerIdentity, std::string>
    verify(const std::string_view credential) const override {
        return decode_slice_credential(credential, secret_);
    }

private:
    std::string secret_;
};

template <typename Integer>
void append_integer(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
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

[[nodiscard]] bool valid_phase(const SliceMatchPhase phase) noexcept {
    return phase == SliceMatchPhase::waiting || phase == SliceMatchPhase::active ||
           phase == SliceMatchPhase::completed;
}

[[nodiscard]] bool valid_end_reason(const SliceMatchEndReason reason) noexcept {
    return reason == SliceMatchEndReason::none ||
           reason == SliceMatchEndReason::abandonment;
}

} // namespace

std::shared_ptr<const SliceIdentityProvider>
make_development_identity_provider(std::string secret) {
    return std::make_shared<DevelopmentSliceIdentityProvider>(std::move(secret));
}

bool valid_development_identity(const SlicePlayerIdentity& identity) noexcept {
    return identity.account_id != 0 && !identity.display_name.empty() &&
           identity.display_name.size() <= maximum_display_name_bytes &&
           std::ranges::all_of(identity.display_name, [](const unsigned char character) {
               return (character >= 'a' && character <= 'z') ||
                      (character >= 'A' && character <= 'Z') ||
                      (character >= '0' && character <= '9') || character == '-' ||
                      character == '_';
           });
}

std::string encode_slice_credential(const std::string_view secret,
                                    const SlicePlayerIdentity& identity) {
    if (secret.empty() || !valid_development_identity(identity)) {
        throw std::invalid_argument{"Slice development identity is invalid"};
    }
    return std::string{secret} + ':' + std::to_string(identity.account_id) + ':' +
           identity.display_name;
}

std::expected<SlicePlayerIdentity, std::string>
decode_slice_credential(const std::string_view credential,
                        const std::string_view expected_secret) {
    const auto first = credential.find(':');
    const auto second = first == std::string_view::npos
                            ? std::string_view::npos
                            : credential.find(':', first + 1);
    if (first == std::string_view::npos || second == std::string_view::npos ||
        credential.substr(0, first) != expected_secret) {
        return std::unexpected{"Slice credential has an invalid format or secret"};
    }
    SlicePlayerIdentity identity;
    const auto account = credential.substr(first + 1, second - first - 1);
    const auto [end, error] = std::from_chars(account.data(),
                                              account.data() + account.size(),
                                              identity.account_id);
    identity.display_name = credential.substr(second + 1);
    if (error != std::errc{} || end != account.data() + account.size() ||
        !valid_development_identity(identity)) {
        return std::unexpected{"Slice credential identity is invalid"};
    }
    return identity;
}

network::ProtocolMessage encode_lobby_ready(const bool ready) {
    return {.kind = network::MessageKind::lobby_command,
            .payload = {std::byte{0}, static_cast<std::byte>(ready)}};
}

std::expected<bool, std::string>
decode_lobby_ready(const network::ProtocolMessage& message) {
    if (message.kind != network::MessageKind::lobby_command || message.payload.size() != 2 ||
        message.payload[0] != std::byte{0}) {
        return std::unexpected{"Message is not a lobby ready command"};
    }
    const auto value = std::to_integer<std::uint8_t>(message.payload[1]);
    if (value > 1) {
        return std::unexpected{"Lobby ready value is invalid"};
    }
    return value != 0;
}

network::ProtocolMessage encode_lobby_selection(const SlicePlayerSelection selection) {
    if (!valid_slice_selection(selection)) {
        throw std::invalid_argument{"Lobby selection is invalid"};
    }
    return {.kind = network::MessageKind::lobby_command,
            .payload = {std::byte{1},
                        static_cast<std::byte>(selection.character),
                        static_cast<std::byte>(selection.weapon),
                        static_cast<std::byte>(selection.ability)}};
}

std::expected<SlicePlayerSelection, std::string>
decode_lobby_selection(const network::ProtocolMessage& message) {
    if (message.kind != network::MessageKind::lobby_command || message.payload.size() != 4 ||
        message.payload[0] != std::byte{1}) {
        return std::unexpected{"Message is not a lobby selection command"};
    }
    const SlicePlayerSelection selection{
        .character = static_cast<SliceCharacter>(
            std::to_integer<std::uint8_t>(message.payload[1])),
        .weapon = static_cast<SliceWeapon>(
            std::to_integer<std::uint8_t>(message.payload[2])),
        .ability = static_cast<SliceAbility>(
            std::to_integer<std::uint8_t>(message.payload[3])),
    };
    if (!valid_slice_selection(selection)) {
        return std::unexpected{"Lobby selection value is invalid"};
    }
    return selection;
}

network::ProtocolMessage encode_lobby_state(const SliceLobbyState& state) {
    if (!valid_phase(state.phase) || !valid_end_reason(state.end_reason) ||
        state.players.size() > 2) {
        throw std::invalid_argument{"Lobby state is invalid"};
    }
    network::ProtocolMessage message{.kind = network::MessageKind::lobby_state,
                                     .sequence = static_cast<std::uint32_t>(state.revision)};
    append_integer(message.payload, state.revision);
    append_integer(message.payload, static_cast<std::uint8_t>(state.phase));
    append_integer(message.payload, static_cast<std::uint8_t>(state.end_reason));
    append_integer(message.payload, state.winner_entity);
    append_integer(message.payload, static_cast<std::uint8_t>(state.players.size()));
    for (const auto& player : state.players) {
        if (player.entity == 0 || !valid_development_identity(player.identity) ||
            !valid_slice_selection(player.selection)) {
            throw std::invalid_argument{"Lobby player is invalid"};
        }
        append_integer(message.payload, player.entity);
        append_integer(message.payload, player.identity.account_id);
        append_integer(message.payload, static_cast<std::uint8_t>(player.selection.character));
        append_integer(message.payload, static_cast<std::uint8_t>(player.selection.weapon));
        append_integer(message.payload, static_cast<std::uint8_t>(player.selection.ability));
        append_integer(message.payload, static_cast<std::uint8_t>(player.connected));
        append_integer(message.payload, static_cast<std::uint8_t>(player.ready));
        append_integer(message.payload,
                       static_cast<std::uint8_t>(player.identity.display_name.size()));
        for (const char character : player.identity.display_name) {
            message.payload.push_back(static_cast<std::byte>(character));
        }
    }
    return message;
}

std::expected<SliceLobbyState, std::string>
decode_lobby_state(const network::ProtocolMessage& message) {
    constexpr std::size_t header_size = 19;
    if (message.kind != network::MessageKind::lobby_state ||
        message.payload.size() < header_size) {
        return std::unexpected{"Message is not a lobby state"};
    }
    std::size_t offset = 0;
    SliceLobbyState state;
    state.revision = read_integer<std::uint64_t>(message.payload, offset);
    state.phase = static_cast<SliceMatchPhase>(read_integer<std::uint8_t>(message.payload,
                                                                         offset));
    state.end_reason = static_cast<SliceMatchEndReason>(
        read_integer<std::uint8_t>(message.payload, offset));
    state.winner_entity = read_integer<std::uint64_t>(message.payload, offset);
    const auto count = read_integer<std::uint8_t>(message.payload, offset);
    if (!valid_phase(state.phase) || !valid_end_reason(state.end_reason) || count > 2 ||
        state.revision == 0) {
        return std::unexpected{"Lobby state header is invalid"};
    }
    state.players.reserve(count);
    for (std::uint8_t index = 0; index < count; ++index) {
        constexpr std::size_t fixed_player_size = 22;
        if (message.payload.size() - offset < fixed_player_size) {
            return std::unexpected{"Lobby player is truncated"};
        }
        SliceLobbyPlayer player;
        player.entity = read_integer<std::uint64_t>(message.payload, offset);
        player.identity.account_id = read_integer<std::uint64_t>(message.payload, offset);
        player.selection.character = static_cast<SliceCharacter>(
            read_integer<std::uint8_t>(message.payload, offset));
        player.selection.weapon = static_cast<SliceWeapon>(
            read_integer<std::uint8_t>(message.payload, offset));
        player.selection.ability = static_cast<SliceAbility>(
            read_integer<std::uint8_t>(message.payload, offset));
        const auto connected = read_integer<std::uint8_t>(message.payload, offset);
        const auto ready = read_integer<std::uint8_t>(message.payload, offset);
        if (connected > 1 || ready > 1) {
            return std::unexpected{"Lobby player flags are invalid"};
        }
        player.connected = connected != 0;
        player.ready = ready != 0;
        const auto name_size = read_integer<std::uint8_t>(message.payload, offset);
        if (name_size > maximum_display_name_bytes ||
            message.payload.size() - offset < name_size) {
            return std::unexpected{"Lobby player name is invalid"};
        }
        player.identity.display_name.reserve(name_size);
        for (std::uint8_t character = 0; character < name_size; ++character) {
            player.identity.display_name.push_back(
                static_cast<char>(std::to_integer<unsigned char>(message.payload[offset++])));
        }
        if (!valid_development_identity(player.identity) ||
            !valid_slice_selection(player.selection)) {
            return std::unexpected{"Lobby player identity or selection is invalid"};
        }
        state.players.push_back(std::move(player));
    }
    if (offset != message.payload.size()) {
        return std::unexpected{"Lobby state has trailing data"};
    }
    return state;
}

SliceMatchLobby::SliceMatchLobby(SliceLobbySettings settings)
    : settings_{std::move(settings)} {
    if (settings_.required_players == 0 || settings_.required_players > 2 ||
        settings_.reconnect_grace_ticks == 0 || !settings_.validate_identity) {
        throw std::invalid_argument{"Slice lobby settings are invalid"};
    }
    refresh_state();
}

std::expected<void, std::string>
SliceMatchLobby::admit(const network::NetworkEntityId entity,
                       const SlicePlayerIdentity& identity,
                       const std::uint64_t server_tick) {
    static_cast<void>(server_tick);
    if (!settings_.validate_identity(identity)) {
        return std::unexpected{"Player identity was rejected"};
    }
    auto record = std::ranges::find(records_, entity, [](const Record& item) {
        return item.player.entity;
    });
    if (record != records_.end()) {
        if (record->player.identity.account_id != identity.account_id ||
            record->player.identity.display_name != identity.display_name) {
            return std::unexpected{"Resume identity does not match the player slot"};
        }
        record->player.connected = true;
        record->expires_at_tick = 0;
        ++state_.revision;
        refresh_state();
        return {};
    }
    if (state_.phase != SliceMatchPhase::waiting ||
        records_.size() >= settings_.required_players ||
        std::ranges::any_of(records_, [&](const Record& item) {
            return item.player.identity.account_id == identity.account_id;
        })) {
        return std::unexpected{"Lobby cannot admit this player"};
    }
    records_.push_back({.player = {.entity = entity,
                                   .identity = identity,
                                   .connected = true}});
    ++state_.revision;
    refresh_state();
    return {};
}

bool SliceMatchLobby::can_admit_identity(const SlicePlayerIdentity& identity, bool resume) const {
    if (!settings_.validate_identity(identity) || state_.phase == SliceMatchPhase::completed) return false;
    const auto record = std::ranges::find(records_, identity.account_id, [](const Record& item) {
        return item.player.identity.account_id;
    });
    if (resume) return record != records_.end() && !record->player.connected &&
                       record->player.identity.display_name == identity.display_name;
    return record == records_.end() && state_.phase == SliceMatchPhase::waiting &&
           records_.size() < settings_.required_players;
}

bool SliceMatchLobby::set_ready(const network::NetworkEntityId entity, const bool ready) {
    auto record = std::ranges::find(records_, entity, [](const Record& item) {
        return item.player.entity;
    });
    if (record == records_.end() || !record->player.connected ||
        state_.phase != SliceMatchPhase::waiting || record->player.ready == ready) {
        return false;
    }
    record->player.ready = ready;
    ++state_.revision;
    try_start();
    refresh_state();
    return true;
}

bool SliceMatchLobby::set_selection(const network::NetworkEntityId entity,
                                    const SlicePlayerSelection selection) {
    auto record = std::ranges::find(records_, entity, [](const Record& item) {
        return item.player.entity;
    });
    if (record == records_.end() || !record->player.connected ||
        state_.phase != SliceMatchPhase::waiting || !valid_slice_selection(selection)) {
        return false;
    }
    if (record->player.selection == selection) {
        return true;
    }
    record->player.selection = selection;
    record->player.ready = false;
    ++state_.revision;
    refresh_state();
    return true;
}

void SliceMatchLobby::disconnected(const network::NetworkEntityId entity,
                                   const std::uint64_t server_tick) {
    auto record = std::ranges::find(records_, entity, [](const Record& item) {
        return item.player.entity;
    });
    if (record == records_.end() || !record->player.connected) {
        return;
    }
    record->player.connected = false;
    record->expires_at_tick = server_tick + settings_.reconnect_grace_ticks;
    ++state_.revision;
    refresh_state();
}

bool SliceMatchLobby::tick(const std::uint64_t server_tick) {
    bool changed = false;
    if (state_.phase == SliceMatchPhase::waiting) {
        const auto old_size = records_.size();
        std::erase_if(records_, [&](const Record& record) {
            return !record.player.connected && server_tick > record.expires_at_tick;
        });
        changed = records_.size() != old_size;
    } else if (state_.phase == SliceMatchPhase::active) {
        for (const auto& record : records_) {
            if (!record.player.connected && server_tick > record.expires_at_tick) {
                state_.phase = SliceMatchPhase::completed;
                state_.end_reason = SliceMatchEndReason::abandonment;
                const auto winner = std::ranges::find_if(records_, [](const Record& candidate) {
                    return candidate.player.connected;
                });
                state_.winner_entity = winner == records_.end() ? 0 : winner->player.entity;
                changed = true;
                break;
            }
        }
    }
    if (changed) {
        ++state_.revision;
        refresh_state();
    }
    return changed;
}

bool SliceMatchLobby::accepts_gameplay(const network::NetworkEntityId entity) const noexcept {
    return state_.phase == SliceMatchPhase::active &&
           std::ranges::any_of(records_, [&](const Record& record) {
               return record.player.entity == entity && record.player.connected &&
                      record.player.ready;
           });
}

const SliceLobbyState& SliceMatchLobby::state() const noexcept { return state_; }

void SliceMatchLobby::refresh_state() {
    state_.players.clear();
    state_.players.reserve(records_.size());
    for (const auto& record : records_) {
        state_.players.push_back(record.player);
    }
    std::ranges::sort(state_.players, {}, &SliceLobbyPlayer::entity);
}

void SliceMatchLobby::try_start() {
    if (records_.size() == settings_.required_players &&
        std::ranges::all_of(records_, [](const Record& record) {
            return record.player.connected && record.player.ready &&
                   valid_slice_selection(record.player.selection);
        })) {
        state_.phase = SliceMatchPhase::active;
    }
}

} // namespace gloom::gameplay
