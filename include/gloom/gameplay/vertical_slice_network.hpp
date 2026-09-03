#pragma once

#include <gloom/gameplay/match_lobby.hpp>
#include <gloom/gameplay/vertical_slice.hpp>
#include <gloom/network/session.hpp>
#include <gloom/network/transport.hpp>

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gloom::gameplay {

struct SliceRemoteHostSettings {
    bool opponent_ai_enabled{false};
    std::size_t required_players{2};
    std::uint64_t reconnect_grace_ticks{600};
    std::shared_ptr<const SliceIdentityProvider> identity_provider;
    std::function<bool(const SlicePlayerIdentity&)> validate_identity;
    physics::World* authoritative_physics{nullptr};
    std::function<std::uint64_t()> generate_resume_token;
    bool original_factory{false};
};

struct SliceWireMessage {
    network::ProtocolMessage message;
    network::Delivery delivery{network::Delivery::unreliable};
};

struct SliceHostMessage : SliceWireMessage {
    network::ConnectionId connection{network::invalid_connection};
};

[[nodiscard]] network::ProtocolMessage
encode_slice_snapshot(const SliceSnapshot& snapshot, std::uint32_t acknowledged_input,
                      std::uint32_t sequence);
[[nodiscard]] std::expected<SliceSnapshot, std::string>
decode_slice_snapshot(const network::ProtocolMessage& message);

// Transport-independent authoritative host adapter. A socket backend only moves
// SliceHostMessage bytes; admission, ownership and command routing stay here.
class VerticalSliceRemoteHost final {
public:
    explicit VerticalSliceRemoteHost(bool opponent_ai_enabled = true);
    explicit VerticalSliceRemoteHost(SliceRemoteHostSettings settings);
    ~VerticalSliceRemoteHost();
    VerticalSliceRemoteHost(const VerticalSliceRemoteHost&) = delete;
    VerticalSliceRemoteHost& operator=(const VerticalSliceRemoteHost&) = delete;

    void connected(network::ConnectionId connection);
    void disconnected(network::ConnectionId connection);
    [[nodiscard]] std::vector<SliceHostMessage>
    receive(network::ConnectionId connection, const network::ProtocolMessage& message,
            double now_seconds);
    [[nodiscard]] std::optional<SliceHostMessage> tick();
    [[nodiscard]] std::vector<SliceHostMessage> tick_clients();

    [[nodiscard]] const SliceSnapshot& snapshot() const noexcept;
    [[nodiscard]] std::span<const ArenaBox> arena() const noexcept;
    [[nodiscard]] std::span<const ArenaSurface> surfaces() const noexcept;
    [[nodiscard]] std::size_t active_clients() const noexcept;
    [[nodiscard]] const network::SessionMetrics& session_metrics() const noexcept;
    [[nodiscard]] const SliceLobbyState& lobby() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Transport-independent remote player adapter. It predicts the controlled
// entity immediately and reconciles against authoritative gameplay snapshots.
class VerticalSliceRemoteClient final {
public:
    VerticalSliceRemoteClient();
    explicit VerticalSliceRemoteClient(SlicePlayerIdentity identity);
    VerticalSliceRemoteClient(SlicePlayerSelection selection,
                              bool automatic_selection);
    VerticalSliceRemoteClient(SlicePlayerIdentity identity,
                              SlicePlayerSelection selection);
    VerticalSliceRemoteClient(SlicePlayerIdentity identity,
                              SlicePlayerSelection selection,
                              bool automatic_selection);
    ~VerticalSliceRemoteClient();
    VerticalSliceRemoteClient(const VerticalSliceRemoteClient&) = delete;
    VerticalSliceRemoteClient& operator=(const VerticalSliceRemoteClient&) = delete;

    [[nodiscard]] SliceWireMessage begin(std::string credential = "gloom-slice");
    [[nodiscard]] SliceWireMessage reconnect(std::string credential = "gloom-slice");
    [[nodiscard]] SliceWireMessage begin_with_credential(std::string credential);
    [[nodiscard]] SliceWireMessage reconnect_with_credential(std::string credential);
    [[nodiscard]] std::optional<SliceWireMessage>
    receive(const network::ProtocolMessage& message, double now_seconds);
    [[nodiscard]] std::vector<SliceWireMessage> create_input(const SliceInput& input);
    [[nodiscard]] bool set_desired_selection(SlicePlayerSelection selection) noexcept;
    [[nodiscard]] std::optional<SliceWireMessage> confirm_selection();
    [[nodiscard]] SlicePlayerSelection desired_selection() const noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] bool has_snapshot() const noexcept;
    [[nodiscard]] const SliceSnapshot& snapshot() const noexcept;
    [[nodiscard]] const network::ClientSession& session() const noexcept;
    [[nodiscard]] const network::ReconciliationMetrics&
    reconciliation_metrics() const noexcept;
    [[nodiscard]] const SlicePlayerIdentity& identity() const noexcept;
    [[nodiscard]] const SliceLobbyState& lobby() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gloom::gameplay
