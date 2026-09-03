#include <gloom/network/session.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace gloom::network {
namespace {

constexpr std::uint8_t session_event_subtype = 0x80;
constexpr std::size_t hello_fixed_size = sizeof(std::uint64_t) * 2 + sizeof(std::uint16_t);
constexpr std::size_t welcome_payload_size = sizeof(std::uint64_t) * 5;
constexpr std::size_t clock_request_payload_size = sizeof(std::uint64_t) * 2;
constexpr std::size_t clock_response_payload_size = sizeof(std::uint64_t) * 4;

template <typename Integer>
void append_integer(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
}

void append_double(std::vector<std::byte>& output, const double value) {
    append_integer(output, std::bit_cast<std::uint64_t>(value));
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

[[nodiscard]] double read_double(const std::span<const std::byte> input,
                                 std::size_t& offset) {
    return std::bit_cast<double>(read_integer<std::uint64_t>(input, offset));
}

[[nodiscard]] bool valid_time(const double value) noexcept {
    return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] ProtocolMessage encode_session_event(const std::uint32_t sequence,
                                                   const std::span<const std::byte> payload) {
    ProtocolMessage message;
    message.kind = MessageKind::event;
    message.sequence = sequence;
    message.payload.reserve(payload.size() + 1);
    message.payload.push_back(static_cast<std::byte>(session_event_subtype));
    message.payload.insert(message.payload.end(), payload.begin(), payload.end());
    return message;
}

} // namespace

ProtocolMessage encode_client_hello(const ClientHello& hello) {
    if (hello.client_nonce == 0 || hello.credential.size() >
                                       std::numeric_limits<std::uint16_t>::max()) {
        throw std::invalid_argument{"Client hello is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::client_hello;
    message.payload.reserve(hello_fixed_size + hello.credential.size());
    append_integer(message.payload, hello.client_nonce);
    append_integer(message.payload, hello.resume_token);
    append_integer(message.payload, static_cast<std::uint16_t>(hello.credential.size()));
    for (const char character : hello.credential) {
        message.payload.push_back(static_cast<std::byte>(character));
    }
    return message;
}

std::expected<ClientHello, std::string>
decode_client_hello(const ProtocolMessage& message) {
    if (message.kind != MessageKind::client_hello || message.payload.size() < hello_fixed_size) {
        return std::unexpected{"Message is not a client hello"};
    }
    std::size_t offset = 0;
    ClientHello hello;
    hello.client_nonce = read_integer<std::uint64_t>(message.payload, offset);
    hello.resume_token = read_integer<std::uint64_t>(message.payload, offset);
    const std::size_t credential_size =
        read_integer<std::uint16_t>(message.payload, offset);
    if (hello.client_nonce == 0 || credential_size != message.payload.size() - offset) {
        return std::unexpected{"Client hello fields are invalid"};
    }
    hello.credential.reserve(credential_size);
    for (; offset < message.payload.size(); ++offset) {
        hello.credential.push_back(static_cast<char>(message.payload[offset]));
    }
    return hello;
}

ProtocolMessage encode_server_welcome(const ServerWelcome& welcome) {
    if (welcome.client_nonce == 0 || welcome.session == invalid_session ||
        welcome.controlled_entity == 0 || welcome.resume_token == 0 ||
        !valid_time(welcome.server_time_seconds)) {
        throw std::invalid_argument{"Server welcome is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::server_welcome;
    message.simulation_tick = welcome.server_tick;
    message.payload.reserve(welcome_payload_size);
    append_integer(message.payload, welcome.client_nonce);
    append_integer(message.payload, welcome.session);
    append_integer(message.payload, welcome.controlled_entity);
    append_integer(message.payload, welcome.resume_token);
    append_double(message.payload, welcome.server_time_seconds);
    return message;
}

std::expected<ServerWelcome, std::string>
decode_server_welcome(const ProtocolMessage& message) {
    if (message.kind != MessageKind::server_welcome ||
        message.payload.size() != welcome_payload_size) {
        return std::unexpected{"Message is not a server welcome"};
    }
    std::size_t offset = 0;
    ServerWelcome welcome{
        .client_nonce = read_integer<std::uint64_t>(message.payload, offset),
        .session = read_integer<std::uint64_t>(message.payload, offset),
        .controlled_entity = read_integer<std::uint64_t>(message.payload, offset),
        .resume_token = read_integer<std::uint64_t>(message.payload, offset),
        .server_tick = message.simulation_tick,
        .server_time_seconds = read_double(message.payload, offset),
    };
    if (welcome.client_nonce == 0 || welcome.session == invalid_session ||
        welcome.controlled_entity == 0 || welcome.resume_token == 0 ||
        !valid_time(welcome.server_time_seconds)) {
        return std::unexpected{"Server welcome fields are invalid"};
    }
    return welcome;
}

ProtocolMessage encode_clock_request(const ClockRequest& request) {
    if (request.nonce == 0 || !valid_time(request.client_send_seconds)) {
        throw std::invalid_argument{"Clock request is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::clock_request;
    message.payload.reserve(clock_request_payload_size);
    append_integer(message.payload, request.nonce);
    append_double(message.payload, request.client_send_seconds);
    return message;
}

std::expected<ClockRequest, std::string>
decode_clock_request(const ProtocolMessage& message) {
    if (message.kind != MessageKind::clock_request ||
        message.payload.size() != clock_request_payload_size) {
        return std::unexpected{"Message is not a clock request"};
    }
    std::size_t offset = 0;
    ClockRequest request{.nonce = read_integer<std::uint64_t>(message.payload, offset),
                         .client_send_seconds = read_double(message.payload, offset)};
    if (request.nonce == 0 || !valid_time(request.client_send_seconds)) {
        return std::unexpected{"Clock request fields are invalid"};
    }
    return request;
}

ProtocolMessage encode_clock_response(const ClockResponse& response) {
    if (response.nonce == 0 || !valid_time(response.client_send_seconds) ||
        !valid_time(response.server_receive_seconds) ||
        !valid_time(response.server_send_seconds) ||
        response.server_send_seconds < response.server_receive_seconds) {
        throw std::invalid_argument{"Clock response is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::clock_response;
    message.payload.reserve(clock_response_payload_size);
    append_integer(message.payload, response.nonce);
    append_double(message.payload, response.client_send_seconds);
    append_double(message.payload, response.server_receive_seconds);
    append_double(message.payload, response.server_send_seconds);
    return message;
}

std::expected<ClockResponse, std::string>
decode_clock_response(const ProtocolMessage& message) {
    if (message.kind != MessageKind::clock_response ||
        message.payload.size() != clock_response_payload_size) {
        return std::unexpected{"Message is not a clock response"};
    }
    std::size_t offset = 0;
    ClockResponse response{
        .nonce = read_integer<std::uint64_t>(message.payload, offset),
        .client_send_seconds = read_double(message.payload, offset),
        .server_receive_seconds = read_double(message.payload, offset),
        .server_send_seconds = read_double(message.payload, offset),
    };
    if (response.nonce == 0 || !valid_time(response.client_send_seconds) ||
        !valid_time(response.server_receive_seconds) ||
        !valid_time(response.server_send_seconds) ||
        response.server_send_seconds < response.server_receive_seconds) {
        return std::unexpected{"Clock response fields are invalid"};
    }
    return response;
}

ServerSessionManager::ServerSessionManager(SessionSettings settings)
    : settings_{std::move(settings)} {
    if (settings_.maximum_clients == 0 || settings_.maximum_credential_bytes == 0 ||
        !settings_.authenticate || !settings_.generate_resume_token) {
        throw std::invalid_argument{"Session settings are invalid"};
    }
}

void ServerSessionManager::connected(const ConnectionId connection) {
    if (connection == invalid_connection || pending_connections_.contains(connection) ||
        find_active(connection) != nullptr) {
        throw std::invalid_argument{"Connection is invalid or already registered"};
    }
    if (pending_connections_.size() < settings_.maximum_clients * 4)
        pending_connections_.insert(connection);
}

bool ServerSessionManager::pending(const ConnectionId connection) const noexcept {
    return pending_connections_.contains(connection);
}

std::expected<ServerWelcome, std::string>
ServerSessionManager::admit(const ConnectionId connection,
                            const ClientHello& hello,
                            const std::uint64_t server_tick,
                            const double server_time_seconds, std::string authenticated_identity) {
    if (!pending_connections_.contains(connection) || hello.client_nonce == 0 ||
        hello.credential.size() > settings_.maximum_credential_bytes ||
        !valid_time(server_time_seconds)) {
        ++metrics_.rejected_protocol;
        return std::unexpected{"Invalid session admission request"};
    }
    if (!settings_.authenticate(hello.credential)) {
        ++metrics_.rejected_authentication;
        return std::unexpected{"Session credential was rejected"};
    }

    Record* resumed = nullptr;
    if (hello.resume_token != 0) {
        for (auto& [id, record] : sessions_) {
            static_cast<void>(id);
            if (record.state == State::dormant && record.resume_token == hello.resume_token &&
                server_tick <= record.expires_at_tick) {
                resumed = &record;
                break;
            }
        }
        if (resumed == nullptr) {
            ++metrics_.rejected_protocol;
            return std::unexpected{"Resume token is invalid or expired"};
        }
    }

    if (resumed == nullptr && sessions_.size() >= settings_.maximum_clients) {
        ++metrics_.rejected_capacity;
        return std::unexpected{"Session capacity has been reached"};
    }

    if ((resumed && resumed->authenticated_identity != authenticated_identity) ||
        (!resumed && !authenticated_identity.empty() && std::ranges::any_of(sessions_, [&](const auto& item) {
            return item.second.authenticated_identity == authenticated_identity;
        }))) {
        ++metrics_.rejected_authentication;
        return std::unexpected{"Session identity does not own this admission"};
    }

    const auto next_token = issue_resume_token();
    pending_connections_.erase(connection);
    Record* record = resumed;
    if (record != nullptr) {
        record->connection = connection;
        record->state = State::active;
        record->expires_at_tick = 0;
        record->resume_token = next_token;
        ++metrics_.resumed;
    } else {
        const SessionId session = next_session_++;
        Record created{
            .session = session,
            .connection = connection,
            .controlled_entity = next_entity_++,
            .resume_token = next_token,
            .authenticated_identity = std::move(authenticated_identity),
        };
        record = &sessions_.emplace(session, created).first->second;
        ++metrics_.admitted;
    }
    return ServerWelcome{
        .client_nonce = hello.client_nonce,
        .session = record->session,
        .controlled_entity = record->controlled_entity,
        .resume_token = record->resume_token,
        .server_tick = server_tick,
        .server_time_seconds = server_time_seconds,
    };
}

void ServerSessionManager::disconnected(const ConnectionId connection,
                                        const std::uint64_t server_tick) {
    pending_connections_.erase(connection);
    if (Record* record = find_active(connection)) {
        record->connection = invalid_connection;
        record->state = State::dormant;
        record->expires_at_tick = server_tick + settings_.reconnect_grace_ticks;
        ++metrics_.disconnected;
    }
}

void ServerSessionManager::expire(const std::uint64_t server_tick) {
    for (auto iterator = sessions_.begin(); iterator != sessions_.end();) {
        if (iterator->second.state == State::dormant &&
            server_tick > iterator->second.expires_at_tick) {
            iterator = sessions_.erase(iterator);
            ++metrics_.expired;
        } else {
            ++iterator;
        }
    }
}

std::expected<NetworkEntityId, std::string>
ServerSessionManager::authorize_input(const ConnectionId connection,
                                      const ProtocolMessage& message) {
    Record* record = find_active(connection);
    if (record == nullptr || message.kind != MessageKind::input_command) {
        ++metrics_.unauthorized_messages;
        return std::unexpected{"Connection does not own an input entity"};
    }
    if (message.acknowledged_sequence != 0 &&
        (!record->replication.has_acknowledged_snapshot ||
         sequence_more_recent(message.acknowledged_sequence,
                              record->replication.acknowledged_snapshot))) {
        record->replication.acknowledged_snapshot = message.acknowledged_sequence;
        record->replication.has_acknowledged_snapshot = true;
    }
    ++record->replication.received_input_batches;
    return record->controlled_entity;
}

bool ServerSessionManager::authorize_fire(const ConnectionId connection,
                                          const FireCommand& command) {
    const Record* record = find_active(connection);
    const bool authorized = record != nullptr && command.shooter == record->controlled_entity;
    if (!authorized) {
        ++metrics_.unauthorized_messages;
    }
    return authorized;
}

std::optional<NetworkEntityId>
ServerSessionManager::controlled_entity(const ConnectionId connection) const noexcept {
    const Record* record = find_active(connection);
    return record == nullptr ? std::nullopt
                             : std::optional<NetworkEntityId>{record->controlled_entity};
}

const SessionReplicationState*
ServerSessionManager::replication_state(const ConnectionId connection) const noexcept {
    const Record* record = find_active(connection);
    return record == nullptr ? nullptr : &record->replication;
}

std::size_t ServerSessionManager::active_sessions() const noexcept {
    return std::ranges::count_if(sessions_, [](const auto& item) {
        return item.second.state == State::active;
    });
}

std::size_t ServerSessionManager::dormant_sessions() const noexcept {
    return std::ranges::count_if(sessions_, [](const auto& item) {
        return item.second.state == State::dormant;
    });
}

const SessionMetrics& ServerSessionManager::metrics() const noexcept {
    return metrics_;
}

ServerSessionManager::Record*
ServerSessionManager::find_active(const ConnectionId connection) noexcept {
    for (auto& [id, record] : sessions_) {
        static_cast<void>(id);
        if (record.state == State::active && record.connection == connection) {
            return &record;
        }
    }
    return nullptr;
}

const ServerSessionManager::Record*
ServerSessionManager::find_active(const ConnectionId connection) const noexcept {
    for (const auto& [id, record] : sessions_) {
        static_cast<void>(id);
        if (record.state == State::active && record.connection == connection) {
            return &record;
        }
    }
    return nullptr;
}

std::uint64_t ServerSessionManager::issue_resume_token() {
    for (std::size_t attempt = 0; attempt < 32; ++attempt) {
        const std::uint64_t token = settings_.generate_resume_token();
        const bool collision = token == 0 || std::ranges::any_of(sessions_, [&](const auto& item) {
            return item.second.resume_token == token;
        });
        if (!collision) {
            return token;
        }
    }
    throw std::runtime_error{"Resume token generator did not produce a unique token"};
}

ProtocolMessage ClientSession::begin(std::string credential,
                                     const std::uint64_t client_nonce) {
    session_ = invalid_session;
    controlled_entity_ = 0;
    resume_token_ = 0;
    clock_.reset();
    pending_clock_requests_.clear();
    expected_client_nonce_ = client_nonce;
    return encode_client_hello({.client_nonce = client_nonce,
                                .credential = std::move(credential)});
}

ProtocolMessage ClientSession::reconnect(std::string credential,
                                         const std::uint64_t client_nonce) {
    if (resume_token_ == 0) {
        throw std::logic_error{"Client has no resumable session"};
    }
    session_ = invalid_session;
    controlled_entity_ = 0;
    clock_.reset();
    pending_clock_requests_.clear();
    expected_client_nonce_ = client_nonce;
    return encode_client_hello({.client_nonce = client_nonce,
                                .resume_token = resume_token_,
                                .credential = std::move(credential)});
}

void ClientSession::accept(const ProtocolMessage& message) {
    const auto welcome = decode_server_welcome(message);
    if (!welcome || welcome->client_nonce != expected_client_nonce_) {
        throw std::invalid_argument{"Server welcome does not match this client attempt"};
    }
    session_ = welcome->session;
    controlled_entity_ = welcome->controlled_entity;
    resume_token_ = welcome->resume_token;
}

ProtocolMessage ClientSession::create_clock_request(const double client_time_seconds) {
    if (!active() || !valid_time(client_time_seconds)) {
        throw std::logic_error{"Clock request requires an active client session"};
    }
    const std::uint64_t nonce = next_clock_nonce_++;
    pending_clock_requests_.emplace(nonce, client_time_seconds);
    return encode_clock_request({.nonce = nonce,
                                 .client_send_seconds = client_time_seconds});
}

void ClientSession::receive_clock_response(const ProtocolMessage& message,
                                           const double client_receive_seconds) {
    const auto response = decode_clock_response(message);
    if (!response || !valid_time(client_receive_seconds)) {
        throw std::invalid_argument{"Clock response is malformed"};
    }
    const auto request = pending_clock_requests_.find(response->nonce);
    if (request == pending_clock_requests_.end() ||
        request->second != response->client_send_seconds) {
        throw std::invalid_argument{"Clock response is unknown or replayed"};
    }
    clock_.observe({
        .nonce = response->nonce,
        .client_send_seconds = response->client_send_seconds,
        .server_receive_seconds = response->server_receive_seconds,
        .server_send_seconds = response->server_send_seconds,
        .client_receive_seconds = client_receive_seconds,
    });
    pending_clock_requests_.erase(request);
}

bool ClientSession::active() const noexcept {
    return session_ != invalid_session && controlled_entity_ != 0;
}

SessionId ClientSession::session() const noexcept {
    return session_;
}

NetworkEntityId ClientSession::controlled_entity() const noexcept {
    return controlled_entity_;
}

std::uint64_t ClientSession::resume_token() const noexcept {
    return resume_token_;
}

const ClockSynchronizer& ClientSession::clock() const noexcept {
    return clock_;
}

ReliableEventSender::ReliableEventSender(ReliableEventSettings settings)
    : settings_{settings} {
    if (!std::isfinite(settings_.retry_interval_seconds) ||
        settings_.retry_interval_seconds <= 0.0 || !std::isfinite(settings_.lifetime_seconds) ||
        settings_.lifetime_seconds < settings_.retry_interval_seconds ||
        settings_.maximum_pending == 0 || settings_.duplicate_window == 0) {
        throw std::invalid_argument{"Reliable event settings are invalid"};
    }
}

std::uint32_t ReliableEventSender::queue(const std::span<const std::byte> payload,
                                         const double now_seconds) {
    if (payload.empty() || payload.size() + 1 > maximum_protocol_payload ||
        !valid_time(now_seconds)) {
        throw std::invalid_argument{"Reliable event is invalid"};
    }
    if (pending_.size() >= settings_.maximum_pending) {
        throw std::length_error{"Reliable event queue is full"};
    }
    std::uint32_t sequence = next_sequence_++;
    while (sequence == 0 ||
           std::ranges::any_of(pending_, [sequence](const PendingEvent& event) {
               return event.sequence == sequence;
           })) {
        sequence = next_sequence_++;
    }
    pending_.push_back({.sequence = sequence,
                        .payload = {payload.begin(), payload.end()},
                        .queued_seconds = now_seconds});
    ++metrics_.queued;
    return sequence;
}

std::vector<ProtocolMessage> ReliableEventSender::poll(const double now_seconds) {
    if (!valid_time(now_seconds)) {
        throw std::invalid_argument{"Reliable event poll time is invalid"};
    }
    std::vector<ProtocolMessage> messages;
    for (auto iterator = pending_.begin(); iterator != pending_.end();) {
        if (now_seconds - iterator->queued_seconds > settings_.lifetime_seconds) {
            iterator = pending_.erase(iterator);
            ++metrics_.expired;
            continue;
        }
        const bool due = iterator->transmissions == 0 ||
                         now_seconds - iterator->last_send_seconds >=
                             settings_.retry_interval_seconds;
        if (due) {
            messages.push_back(encode_session_event(iterator->sequence, iterator->payload));
            if (iterator->transmissions > 0) {
                ++metrics_.retransmissions;
            }
            ++iterator->transmissions;
            ++metrics_.transmissions;
            iterator->last_send_seconds = now_seconds;
        }
        ++iterator;
    }
    return messages;
}

bool ReliableEventSender::acknowledge(const ProtocolMessage& message) {
    if (message.kind != MessageKind::event_acknowledgement || !message.payload.empty() ||
        message.acknowledged_sequence == 0) {
        return false;
    }
    const auto found = std::ranges::find(pending_, message.acknowledged_sequence,
                                        &PendingEvent::sequence);
    if (found == pending_.end()) {
        return false;
    }
    pending_.erase(found);
    ++metrics_.acknowledged;
    return true;
}

std::size_t ReliableEventSender::pending() const noexcept {
    return pending_.size();
}

const ReliableEventMetrics& ReliableEventSender::metrics() const noexcept {
    return metrics_;
}

ReliableEventReceiver::ReliableEventReceiver(const std::size_t duplicate_window)
    : duplicate_window_{duplicate_window} {
    if (duplicate_window_ == 0) {
        throw std::invalid_argument{"Reliable event duplicate window must not be empty"};
    }
}

std::expected<ReceivedSessionEvent, std::string>
ReliableEventReceiver::receive(const ProtocolMessage& message) {
    if (message.kind != MessageKind::event || message.sequence == 0 ||
        message.payload.size() < 2 ||
        std::to_integer<std::uint8_t>(message.payload.front()) != session_event_subtype) {
        return std::unexpected{"Message is not a reliable session event"};
    }
    if (received_.contains(message.sequence)) {
        ++metrics_.duplicates;
        return ReceivedSessionEvent{.sequence = message.sequence, .duplicate = true};
    }
    received_.insert(message.sequence);
    received_order_.push_back(message.sequence);
    while (received_order_.size() > duplicate_window_) {
        received_.erase(received_order_.front());
        received_order_.pop_front();
    }
    ++metrics_.delivered;
    return ReceivedSessionEvent{
        .sequence = message.sequence,
        .payload = {message.payload.begin() + 1, message.payload.end()},
    };
}

ProtocolMessage ReliableEventReceiver::acknowledgement(const std::uint32_t sequence) const {
    if (sequence == 0) {
        throw std::invalid_argument{"Cannot acknowledge event sequence zero"};
    }
    return {.kind = MessageKind::event_acknowledgement,
            .acknowledged_sequence = sequence};
}

const ReliableEventMetrics& ReliableEventReceiver::metrics() const noexcept {
    return metrics_;
}

} // namespace gloom::network
