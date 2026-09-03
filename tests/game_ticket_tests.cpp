#include <gloom/backends/game_ticket.hpp>
#include <gloom/backends/gns_transport.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <openssl/pem.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace {
using namespace gloom;
void expect(bool ok, const char* message) { if (!ok) throw std::runtime_error{message}; }
void keys(const std::filesystem::path& root) {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key{EVP_PKEY_Q_keygen(nullptr, nullptr, "ED25519"), EVP_PKEY_free};
    expect(key != nullptr, "Key generation failed");
    auto* out = BIO_new_file((root / "private.pem").string().c_str(), "w");
    expect(out && PEM_write_bio_PrivateKey(out, key.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1, "Private key write failed");
    BIO_free(out); out = BIO_new_file((root / "public.pem").string().c_str(), "w");
    expect(out && PEM_write_bio_PUBKEY(out, key.get()) == 1, "Public key write failed"); BIO_free(out);
}
void deterministic(const std::filesystem::path& root) {
    backends::GameTicketIssuer issuer{root / "private.pem"};
    std::uint64_t now = 1000;
    auto verifier = backends::make_game_ticket_verifier(root / "public.pem", "instance", [&] { return now; });
    const backends::MatchVerifiedPrincipal alice{"alice", 100000}, bob{"bob", 100000};
    const auto ticket = issuer.issue(alice, "instance", now);
    auto tampered = ticket; tampered[5] = tampered[5] == 'a' ? 'b' : 'a';
    expect(!verifier->verify(tampered), "Tampered ticket accepted");
    expect(!verifier->verify("gloom-slice:1:alice") && !verifier->verify("reader-grant-credential"), "Development/discovery credential accepted");
    expect(!verifier->verify(issuer.issue(alice, "different", now)), "Cross-instance ticket accepted");
    const auto first = verifier->verify(ticket); expect(first.has_value(), "Valid ticket rejected");
    expect(!verifier->verify(ticket), "Ticket replay accepted");
    expect(verifier->verify(issuer.issue(alice, "instance", now))->account_id == first->account_id, "Identity changed on renewal");
    const auto expired = issuer.issue(alice, "instance", now); now += 30000;
    expect(!verifier->verify(expired), "Expired ticket accepted");
    expect(!verifier->verify(issuer.issue(alice, "instance", now + 1)), "Future ticket accepted");
    const auto limited = issuer.issue({"alice", now + 1}, "instance", now); ++now;
    expect(!verifier->verify(limited), "Identity expiry was extended");
    std::string principal(64, 'a'), collision = principal; collision.back() = 'b';
    expect(verifier->verify(issuer.issue({principal, 100000}, "instance", now)).has_value(), "Full principal failed");
    expect(!verifier->verify(issuer.issue({collision, 100000}, "instance", now)), "Compact account collision impersonated owner");
    auto other_root = root / "other"; std::filesystem::create_directory(other_root); keys(other_root);
    backends::GameTicketIssuer other{other_root / "private.pem"};
    expect(!verifier->verify(other.issue(alice, "instance", now)), "Wrong signing key accepted");
    for (const auto& malformed : {std::string{}, ticket + ".extra", std::string(513, 'x'), ticket.substr(0, ticket.size()-1)})
        expect(!verifier->verify(malformed), "Malformed ticket accepted");

    gameplay::VerticalSliceRemoteHost host{gameplay::SliceRemoteHostSettings{
        .identity_provider = verifier, .generate_resume_token = backends::secure_session_token}};
    const auto hello = [&](std::uint64_t connection, const backends::MatchVerifiedPrincipal& identity, std::uint64_t resume = 0) {
        host.connected(connection);
        return host.receive(connection, network::encode_client_hello({connection, resume, issuer.issue(identity, "instance", now)}), 1.0);
    };
    const auto admitted = hello(1, alice); expect(admitted.size() == 1, "Verified host admission failed");
    const auto welcome = network::decode_server_welcome(admitted.front().message).value();
    expect(hello(2, alice).empty() && host.active_clients() == 1, "Duplicate account consumed a slot");
    expect(hello(3, bob).size() == 1 && host.active_clients() == 2, "Second account could not join");
    host.disconnected(1); host.disconnected(3);
    expect(hello(4, bob, welcome.resume_token).empty(), "Another verified account stole dormant session");
    const auto resumed = hello(5, alice, welcome.resume_token);
    expect(resumed.size() == 1, "Failed attack consumed rightful resume token");
    const auto next = network::decode_server_welcome(resumed.front().message).value();
    expect(next.session == welcome.session && next.controlled_entity == welcome.controlled_entity &&
           next.resume_token != welcome.resume_token, "Reconnect changed ownership or failed to rotate token");
    host.disconnected(5);
    expect(hello(6, alice, welcome.resume_token).empty(), "Old resume token accepted");
    expect(hello(7, alice, next.resume_token).size() == 1, "Owner could not resume after stale-token attempt");
    expect(backends::secure_session_token() != 0 && backends::secure_server_instance() != backends::secure_server_instance(), "Randomness failed");
}
void send(network::Transport& transport, network::ConnectionId connection, const gameplay::SliceWireMessage& message) {
    const auto bytes = network::encode_message(message.message);
    transport.send(connection, {.payload = bytes, .delivery = message.delivery});
}
void transport(const std::filesystem::path& root) {
    backends::GameTicketIssuer issuer{root / "private.pem"};
    auto verifier = backends::make_game_ticket_verifier(root / "public.pem", "transport-instance", [] { return 1000; });
    gameplay::VerticalSliceRemoteHost host{gameplay::SliceRemoteHostSettings{
        .required_players = 1, .identity_provider = verifier, .generate_resume_token = backends::secure_session_token}};
    gameplay::VerticalSliceRemoteClient client;
    backends::GnsTransport server, remote; server.start(); remote.start();
    const auto endpoint = server.listen("127.0.0.1:0");
    auto connection = remote.connect(endpoint);
    bool resumed = false, disconnect_seen = false;
    const auto pump = [&](bool reconnect) {
        for (unsigned i = 0; i < 4000; ++i) {
            server.tick(1.0 / 60.0); remote.tick(1.0 / 60.0);
            while (auto event = server.poll_event()) {
                if (event->state == network::ConnectionState::connected) host.connected(event->connection);
                else if (event->state == network::ConnectionState::disconnected || event->state == network::ConnectionState::failed) {
                    host.disconnected(event->connection); disconnect_seen = true;
                }
            }
            while (auto event = remote.poll_event()) {
                if (event->state == network::ConnectionState::connected) {
                    auto credential = issuer.issue({"transport-player", 100000}, "transport-instance", 1000);
                    send(remote, connection, reconnect ? client.reconnect_with_credential(credential) : client.begin_with_credential(credential));
                }
            }
            while (auto packet = server.receive()) {
                const auto message = network::decode_message(packet->payload).value();
                for (const auto& response : host.receive(packet->connection, message, 1.0)) send(server, response.connection, response);
            }
            while (auto packet = remote.receive()) {
                const auto message = network::decode_message(packet->payload).value();
                if (reconnect && message.kind == network::MessageKind::server_welcome) resumed = true;
                if (auto response = client.receive(message, 1.0)) send(remote, connection, *response);
            }
            for (const auto& response : host.tick_clients()) send(server, response.connection, response);
            if (reconnect ? resumed : client.has_snapshot()) return;
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        throw std::runtime_error{"Real GNS ticket handshake timed out"};
    };
    pump(false);
    expect(client.identity().account_id == host.lobby().players.front().identity.account_id,
           "Client retained self-asserted identity after authoritative admission");
    const auto session = client.session().session(), token = client.session().resume_token();
    remote.disconnect(connection);
    for (unsigned i = 0; i < 1000 && !disconnect_seen; ++i) {
        server.tick(0.001); remote.tick(0.001);
        while (auto event = server.poll_event()) if (event->state == network::ConnectionState::disconnected) {
            host.disconnected(event->connection); disconnect_seen = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    expect(disconnect_seen, "Disconnect was not observed");
    connection = remote.connect(endpoint); pump(true);
    expect(client.session().session() == session && client.session().resume_token() != token && host.session_metrics().resumed == 1,
           "Real GNS reconnect lost identity/session");
    remote.stop(); server.stop();
}
}
int main() try {
    const auto root = std::filesystem::temp_directory_path() / ("gloom-tickets-" + gloom::backends::secure_server_instance());
    std::filesystem::create_directory(root); keys(root); deterministic(root); transport(root);
    std::filesystem::remove_all(root);
    std::cout << "Game ticket and verified transport tests passed\n"; return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
