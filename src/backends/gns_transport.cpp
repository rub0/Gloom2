#include <gloom/backends/gns_transport.hpp>

#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

#include <atomic>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace gloom::backends {
namespace {

std::mutex runtime_mutex;
std::uint32_t runtime_users = 0;
std::atomic<std::uint32_t> next_ephemeral_port{49'152};

void acquire_runtime() {
    const std::scoped_lock lock{runtime_mutex};
    if (runtime_users++ == 0) {
        SteamDatagramErrMsg error_message;
        if (!GameNetworkingSockets_Init(nullptr, error_message)) {
            --runtime_users;
            throw std::runtime_error{"GameNetworkingSockets initialization failed: " +
                                     std::string{error_message}};
        }
    }
}

void release_runtime() noexcept {
    const std::scoped_lock lock{runtime_mutex};
    if (--runtime_users == 0) {
        GameNetworkingSockets_Kill();
    }
}

[[nodiscard]] SteamNetworkingIPAddr parse_address(const std::string_view endpoint) {
    const std::string text{endpoint};
    SteamNetworkingIPAddr address;
    address.Clear();
    if (text.empty() || !address.ParseString(text.c_str())) {
        throw std::invalid_argument{"Invalid network endpoint: " + text};
    }
    return address;
}

[[nodiscard]] std::string address_to_string(const SteamNetworkingIPAddr& address) {
    char buffer[SteamNetworkingIPAddr::k_cchMaxString]{};
    address.ToString(buffer, sizeof(buffer), true);
    return buffer;
}

} // namespace

struct GnsTransport::Impl {
    struct ConnectionRecord {
        network::ConnectionId id{network::invalid_connection};
        bool incoming{false};
        bool connected{false};
    };

    static inline std::mutex registry_mutex;
    static inline std::unordered_map<HSteamNetConnection, Impl*> connection_owners;
    static inline std::unordered_map<HSteamListenSocket, Impl*> listener_owners;
    static inline std::atomic<network::ConnectionId> next_connection_id{1};

    static void status_changed(SteamNetConnectionStatusChangedCallback_t* information) {
        Impl* owner = nullptr;
        {
            const std::scoped_lock lock{registry_mutex};
            if (const auto connection = connection_owners.find(information->m_hConn);
                connection != connection_owners.end()) {
                owner = connection->second;
            } else if (const auto listener = listener_owners.find(information->m_info.m_hListenSocket);
                       listener != listener_owners.end()) {
                owner = listener->second;
            }
        }
        if (owner != nullptr) {
            owner->handle_status_change(*information);
        }
    }

    void register_connection(const HSteamNetConnection handle, const ConnectionRecord record) {
        connections.emplace(handle, record);
        connection_handles.emplace(record.id, handle);
        const std::scoped_lock lock{registry_mutex};
        connection_owners[handle] = this;
    }

    void unregister_connection(const HSteamNetConnection handle) {
        if (const auto record = connections.find(handle); record != connections.end()) {
            connection_handles.erase(record->second.id);
            connections.erase(record);
        }
        const std::scoped_lock lock{registry_mutex};
        connection_owners.erase(handle);
    }

    [[nodiscard]] network::ConnectionId allocate_id() {
        return next_connection_id.fetch_add(1, std::memory_order_relaxed);
    }

    void handle_status_change(const SteamNetConnectionStatusChangedCallback_t& change) {
        const auto state = change.m_info.m_eState;
        if (state == k_ESteamNetworkingConnectionState_Connecting) {
            if (!connections.contains(change.m_hConn) &&
                change.m_info.m_hListenSocket != k_HSteamListenSocket_Invalid) {
                const network::ConnectionId id = allocate_id();
                register_connection(change.m_hConn, {.id = id, .incoming = true});
                if (sockets->SetConnectionPollGroup(change.m_hConn, poll_group) == false ||
                    sockets->AcceptConnection(change.m_hConn) != k_EResultOK) {
                    events.push_back({id,
                                      network::ConnectionState::failed,
                                      true,
                                      static_cast<std::int32_t>(change.m_info.m_eEndReason),
                                      "Failed to accept incoming connection"});
                    sockets->CloseConnection(change.m_hConn, 1000, "Accept failed", false);
                    unregister_connection(change.m_hConn);
                    return;
                }
                events.push_back({id, network::ConnectionState::connecting, true, 0, {}});
            }
            return;
        }

        const auto found = connections.find(change.m_hConn);
        if (found == connections.end()) {
            return;
        }
        ConnectionRecord& record = found->second;
        if (state == k_ESteamNetworkingConnectionState_Connected) {
            if (!record.connected) {
                record.connected = true;
                ++transport_metrics.opened_connections;
                events.push_back(
                    {record.id, network::ConnectionState::connected, record.incoming, 0, {}});
            }
            return;
        }

        if (state == k_ESteamNetworkingConnectionState_ClosedByPeer ||
            state == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
            const bool failed = state == k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
            const network::ConnectionId id = record.id;
            const bool incoming = record.incoming;
            events.push_back({id,
                              failed ? network::ConnectionState::failed
                                     : network::ConnectionState::disconnected,
                              incoming,
                              static_cast<std::int32_t>(change.m_info.m_eEndReason),
                              change.m_info.m_szEndDebug});
            sockets->CloseConnection(change.m_hConn, 0, nullptr, false);
            unregister_connection(change.m_hConn);
        }
    }

    void drain_messages() {
        SteamNetworkingMessage_t* messages[32]{};
        while (true) {
            const int count = sockets->ReceiveMessagesOnPollGroup(poll_group, messages, 32);
            if (count < 0) {
                throw std::runtime_error{"GameNetworkingSockets failed while receiving messages"};
            }
            if (count == 0) {
                return;
            }
            for (int index = 0; index < count; ++index) {
                SteamNetworkingMessage_t* message = messages[index];
                if (const auto connection = connections.find(message->m_conn);
                    connection != connections.end()) {
                    network::ReceivedPacket packet;
                    packet.connection = connection->second.id;
                    packet.payload.resize(static_cast<std::size_t>(message->m_cbSize));
                    if (!packet.payload.empty()) {
                        std::memcpy(packet.payload.data(), message->m_pData, packet.payload.size());
                    }
                    packet.delivery = (message->m_nFlags & k_nSteamNetworkingSend_Reliable) != 0
                                          ? network::Delivery::reliable
                                          : network::Delivery::unreliable;
                    packet.sequence = message->m_nMessageNumber;
                    ++transport_metrics.received_packets;
                    transport_metrics.received_bytes += packet.payload.size();
                    packets.push_back(std::move(packet));
                }
                message->Release();
            }
        }
    }

    ISteamNetworkingSockets* sockets{nullptr};
    HSteamNetPollGroup poll_group{k_HSteamNetPollGroup_Invalid};
    HSteamListenSocket listener{k_HSteamListenSocket_Invalid};
    std::unordered_map<HSteamNetConnection, ConnectionRecord> connections;
    std::unordered_map<network::ConnectionId, HSteamNetConnection> connection_handles;
    std::deque<network::ConnectionEvent> events;
    std::deque<network::ReceivedPacket> packets;
    network::TransportMetrics transport_metrics;
    bool runtime_acquired{false};
};

GnsTransport::GnsTransport() : impl_{std::make_unique<Impl>()} {}

GnsTransport::~GnsTransport() {
    stop();
}

std::string_view GnsTransport::name() const noexcept {
    return "network.gamenetworkingsockets";
}

core::SubsystemState GnsTransport::state() const noexcept {
    return state_;
}

void GnsTransport::start() {
    if (state_ == core::SubsystemState::running) {
        throw std::logic_error{"GameNetworkingSockets transport is already running"};
    }
    acquire_runtime();
    impl_->runtime_acquired = true;
    try {
        impl_->sockets = SteamNetworkingSockets();
        if (impl_->sockets == nullptr) {
            throw std::runtime_error{"GameNetworkingSockets did not provide its socket interface"};
        }
        impl_->poll_group = impl_->sockets->CreatePollGroup();
        if (impl_->poll_group == k_HSteamNetPollGroup_Invalid) {
            throw std::runtime_error{"GameNetworkingSockets could not create a poll group"};
        }
        impl_->transport_metrics = {};
        state_ = core::SubsystemState::running;
    } catch (...) {
        stop();
        throw;
    }
}

void GnsTransport::tick([[maybe_unused]] const double delta_seconds) {
    require_running();
    impl_->sockets->RunCallbacks();
    impl_->drain_messages();
}

void GnsTransport::stop() noexcept {
    if (impl_->sockets != nullptr) {
        stop_listening();
        while (!impl_->connections.empty()) {
            const HSteamNetConnection handle = impl_->connections.begin()->first;
            {
                const std::scoped_lock lock{Impl::registry_mutex};
                Impl::connection_owners.erase(handle);
            }
            impl_->sockets->CloseConnection(handle, 1000, "Gloom transport stopped", false);
            impl_->connection_handles.erase(impl_->connections.begin()->second.id);
            impl_->connections.erase(impl_->connections.begin());
        }
        if (impl_->poll_group != k_HSteamNetPollGroup_Invalid) {
            impl_->sockets->DestroyPollGroup(impl_->poll_group);
            impl_->poll_group = k_HSteamNetPollGroup_Invalid;
        }
    }
    impl_->events.clear();
    impl_->packets.clear();
    impl_->sockets = nullptr;
    if (impl_->runtime_acquired) {
        release_runtime();
        impl_->runtime_acquired = false;
    }
    state_ = core::SubsystemState::stopped;
}

std::string GnsTransport::listen(const std::string_view endpoint) {
    require_running();
    if (impl_->listener != k_HSteamListenSocket_Invalid) {
        throw std::logic_error{"Transport is already listening"};
    }
    SteamNetworkingIPAddr address = parse_address(endpoint);
    SteamNetworkingConfigValue_t callback;
    callback.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                    reinterpret_cast<void*>(Impl::status_changed));
    if (address.m_port == 0) {
        // GNS rejects port zero instead of asking the OS for an ephemeral port. Keep that
        // useful transport-level convention by probing the IANA dynamic/private range.
        constexpr std::uint32_t first_dynamic_port = 49'152;
        constexpr std::uint32_t dynamic_port_count = 65'536 - first_dynamic_port;
        const std::uint32_t first_attempt =
            next_ephemeral_port.fetch_add(1, std::memory_order_relaxed) - first_dynamic_port;
        for (std::uint32_t attempt = 0;
             attempt < dynamic_port_count && impl_->listener == k_HSteamListenSocket_Invalid;
             ++attempt) {
            address.m_port = static_cast<std::uint16_t>(
                first_dynamic_port + ((first_attempt + attempt) % dynamic_port_count));
            impl_->listener = impl_->sockets->CreateListenSocketIP(address, 1, &callback);
        }
    } else {
        impl_->listener = impl_->sockets->CreateListenSocketIP(address, 1, &callback);
    }
    if (impl_->listener == k_HSteamListenSocket_Invalid) {
        throw std::runtime_error{"GameNetworkingSockets could not listen on " +
                                 std::string{endpoint}};
    }
    {
        const std::scoped_lock lock{Impl::registry_mutex};
        Impl::listener_owners[impl_->listener] = impl_.get();
    }
    SteamNetworkingIPAddr bound_address;
    if (!impl_->sockets->GetListenSocketAddress(impl_->listener, &bound_address)) {
        stop_listening();
        throw std::runtime_error{"GameNetworkingSockets could not query the listening address"};
    }
    return address_to_string(bound_address);
}

void GnsTransport::stop_listening() {
    if (impl_->listener == k_HSteamListenSocket_Invalid || impl_->sockets == nullptr) {
        return;
    }
    {
        const std::scoped_lock lock{Impl::registry_mutex};
        Impl::listener_owners.erase(impl_->listener);
    }
    impl_->sockets->CloseListenSocket(impl_->listener);
    impl_->listener = k_HSteamListenSocket_Invalid;
}

network::ConnectionId GnsTransport::connect(const std::string_view endpoint) {
    require_running();
    const SteamNetworkingIPAddr address = parse_address(endpoint);
    SteamNetworkingConfigValue_t callback;
    callback.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                    reinterpret_cast<void*>(Impl::status_changed));
    const HSteamNetConnection handle = impl_->sockets->ConnectByIPAddress(address, 1, &callback);
    if (handle == k_HSteamNetConnection_Invalid) {
        throw std::runtime_error{"GameNetworkingSockets could not connect to " +
                                 std::string{endpoint}};
    }
    const network::ConnectionId id = impl_->allocate_id();
    impl_->register_connection(handle, {.id = id, .incoming = false});
    if (!impl_->sockets->SetConnectionPollGroup(handle, impl_->poll_group)) {
        impl_->sockets->CloseConnection(handle, 1000, "Poll group assignment failed", false);
        impl_->unregister_connection(handle);
        throw std::runtime_error{"GameNetworkingSockets could not assign the connection poll group"};
    }
    impl_->events.push_back({id, network::ConnectionState::connecting, false, 0, {}});
    return id;
}

void GnsTransport::disconnect(const network::ConnectionId connection) {
    require_running();
    const auto found = impl_->connection_handles.find(connection);
    if (found == impl_->connection_handles.end()) {
        throw std::invalid_argument{"Unknown network connection"};
    }
    const HSteamNetConnection handle = found->second;
    const bool incoming = impl_->connections.at(handle).incoming;
    impl_->sockets->CloseConnection(handle, 1000, "Gloom disconnect", true);
    impl_->events.push_back(
        {connection, network::ConnectionState::disconnected, incoming, 1000, "Local disconnect"});
    impl_->unregister_connection(handle);
}

void GnsTransport::send(const network::ConnectionId connection, const network::PacketView packet) {
    require_running();
    const auto found = impl_->connection_handles.find(connection);
    if (found == impl_->connection_handles.end()) {
        throw std::invalid_argument{"Unknown network connection"};
    }
    if (packet.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error{"Network packet is too large"};
    }
    const int flags = packet.delivery == network::Delivery::reliable
                          ? k_nSteamNetworkingSend_Reliable
                          : k_nSteamNetworkingSend_Unreliable;
    const EResult result = impl_->sockets->SendMessageToConnection(
        found->second,
        packet.payload.data(),
        static_cast<std::uint32_t>(packet.payload.size()),
        flags,
        nullptr);
    if (result != k_EResultOK) {
        throw std::runtime_error{"GameNetworkingSockets send failed with result " +
                                 std::to_string(static_cast<int>(result))};
    }
    ++impl_->transport_metrics.sent_packets;
    impl_->transport_metrics.sent_bytes += packet.payload.size();
}

std::optional<network::ConnectionEvent> GnsTransport::poll_event() {
    require_running();
    if (impl_->events.empty()) {
        return std::nullopt;
    }
    network::ConnectionEvent event = std::move(impl_->events.front());
    impl_->events.pop_front();
    return event;
}

std::optional<network::ReceivedPacket> GnsTransport::receive() {
    require_running();
    if (impl_->packets.empty()) {
        return std::nullopt;
    }
    network::ReceivedPacket packet = std::move(impl_->packets.front());
    impl_->packets.pop_front();
    return packet;
}

network::TransportMetrics GnsTransport::metrics() const noexcept {
    return impl_->transport_metrics;
}

void GnsTransport::require_running() const {
    if (state_ != core::SubsystemState::running) {
        throw std::logic_error{"GameNetworkingSockets transport must be running"};
    }
}

} // namespace gloom::backends
