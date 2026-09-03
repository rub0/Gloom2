#pragma once

#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/network/movement_replication.hpp>

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gloom::gameplay {

struct SlicePlayerIdentity {
    std::uint64_t account_id{0};
    std::string display_name;

    [[nodiscard]] bool operator==(const SlicePlayerIdentity&) const noexcept = default;
};

class SliceIdentityProvider {
public:
    virtual ~SliceIdentityProvider() = default;
    [[nodiscard]] virtual std::expected<SlicePlayerIdentity, std::string>
    verify(std::string_view credential) const = 0;
};

[[nodiscard]] std::shared_ptr<const SliceIdentityProvider>
make_development_identity_provider(std::string secret = "gloom-slice");

enum class SliceMatchPhase : std::uint8_t { waiting = 0, active = 1, completed = 2 };
enum class SliceMatchEndReason : std::uint8_t { none = 0, abandonment = 1 };

struct SliceLobbyPlayer {
    network::NetworkEntityId entity{0};
    SlicePlayerIdentity identity;
    SlicePlayerSelection selection;
    bool connected{false};
    bool ready{false};
};

struct SliceLobbyState {
    std::uint64_t revision{0};
    SliceMatchPhase phase{SliceMatchPhase::waiting};
    SliceMatchEndReason end_reason{SliceMatchEndReason::none};
    network::NetworkEntityId winner_entity{0};
    std::vector<SliceLobbyPlayer> players;
};

struct SliceLobbySettings {
    std::size_t required_players{2};
    std::uint64_t reconnect_grace_ticks{600};
    std::function<bool(const SlicePlayerIdentity&)> validate_identity;
};

[[nodiscard]] bool valid_development_identity(const SlicePlayerIdentity& identity) noexcept;
[[nodiscard]] std::string encode_slice_credential(std::string_view secret,
                                                  const SlicePlayerIdentity& identity);
[[nodiscard]] std::expected<SlicePlayerIdentity, std::string>
decode_slice_credential(std::string_view credential, std::string_view expected_secret);

[[nodiscard]] network::ProtocolMessage encode_lobby_ready(bool ready);
[[nodiscard]] std::expected<bool, std::string>
decode_lobby_ready(const network::ProtocolMessage& message);
[[nodiscard]] network::ProtocolMessage
encode_lobby_selection(SlicePlayerSelection selection);
[[nodiscard]] std::expected<SlicePlayerSelection, std::string>
decode_lobby_selection(const network::ProtocolMessage& message);
[[nodiscard]] network::ProtocolMessage encode_lobby_state(const SliceLobbyState& state);
[[nodiscard]] std::expected<SliceLobbyState, std::string>
decode_lobby_state(const network::ProtocolMessage& message);

class SliceMatchLobby final {
public:
    explicit SliceMatchLobby(SliceLobbySettings settings);

    [[nodiscard]] std::expected<void, std::string>
    admit(network::NetworkEntityId entity,
          const SlicePlayerIdentity& identity,
          std::uint64_t server_tick);
    [[nodiscard]] bool can_admit_identity(const SlicePlayerIdentity& identity, bool resume) const;
    [[nodiscard]] bool set_ready(network::NetworkEntityId entity, bool ready);
    [[nodiscard]] bool set_selection(network::NetworkEntityId entity,
                                     SlicePlayerSelection selection);
    void disconnected(network::NetworkEntityId entity, std::uint64_t server_tick);
    [[nodiscard]] bool tick(std::uint64_t server_tick);

    [[nodiscard]] bool accepts_gameplay(network::NetworkEntityId entity) const noexcept;
    [[nodiscard]] const SliceLobbyState& state() const noexcept;

private:
    struct Record {
        SliceLobbyPlayer player;
        std::uint64_t expires_at_tick{0};
    };

    void refresh_state();
    void try_start();

    SliceLobbySettings settings_;
    std::vector<Record> records_;
    SliceLobbyState state_;
};

} // namespace gloom::gameplay
