#pragma once

#include <gloom/network/transport.hpp>

#include <memory>

namespace gloom::backends {

class GnsTransport final : public network::Transport {
public:
    GnsTransport();
    ~GnsTransport() override;

    GnsTransport(const GnsTransport&) = delete;
    GnsTransport& operator=(const GnsTransport&) = delete;
    GnsTransport(GnsTransport&&) = delete;
    GnsTransport& operator=(GnsTransport&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] core::SubsystemState state() const noexcept override;
    void start() override;
    void tick(double delta_seconds) override;
    void stop() noexcept override;

    [[nodiscard]] std::string listen(std::string_view endpoint) override;
    void stop_listening() override;
    [[nodiscard]] network::ConnectionId connect(std::string_view endpoint) override;
    void disconnect(network::ConnectionId connection) override;
    void send(network::ConnectionId connection, network::PacketView packet) override;
    [[nodiscard]] std::optional<network::ConnectionEvent> poll_event() override;
    [[nodiscard]] std::optional<network::ReceivedPacket> receive() override;
    [[nodiscard]] network::TransportMetrics metrics() const noexcept override;

private:
    struct Impl;

    void require_running() const;

    std::unique_ptr<Impl> impl_;
    core::SubsystemState state_{core::SubsystemState::stopped};
};

} // namespace gloom::backends
