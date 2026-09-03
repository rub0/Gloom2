#include <gloom/backends/match_https_host.hpp>
#include <gloom/backends/keycloak_identity.hpp>
#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
static_assert(std::atomic_bool::is_always_lock_free);
std::atomic_bool stopped{false};
void stop(int) { stopped.store(true, std::memory_order_relaxed); }
unsigned number(std::string_view value) {
    unsigned result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument{"Invalid numeric service argument"};
    return result;
}
std::string credential(const char* name, bool required = true) {
    std::string token;
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0)
        throw std::runtime_error{"Could not read service credential"};
    if (value) { token = value; std::free(value); }
#else
    if (const auto* value = std::getenv(name)) token = value;
#endif
    if (required && token.empty()) throw std::invalid_argument{std::string{name} + " is required"};
    return token;
}
}

int main(int argc, const char* const* argv) try {
    if (argc != 6 && argc != 8) throw std::invalid_argument{
        "Usage: gloom_match_service ADDRESS PORT CERTIFICATE.pem PRIVATE_KEY.pem STORE [--run-for-ms N]"};
    const auto port = number(argv[2]);
    if (port > 65535) throw std::invalid_argument{"Invalid service port"};
    unsigned run_ms = 0;
    if (argc == 8) {
        if (std::string_view{argv[6]} != "--run-for-ms" || (run_ms = number(argv[7])) == 0)
            throw std::invalid_argument{"Invalid service time limit"};
    }
    const auto publisher = credential("GLOOM_MATCH_PUBLISHER_TOKEN");
    if (!gloom::backends::valid_match_credential(publisher))
        throw std::invalid_argument{"Invalid publisher credential (16-512 printable non-space ASCII characters required)"};
    const auto provider = credential("GLOOM_MATCH_IDENTITY_PROVIDER", false);
    gloom::backends::MatchIdentityVerifier verifier;
    gloom::backends::MatchIdentityVerifier game_verifier;
    const auto signing_key = credential("GLOOM_GAME_TICKET_PRIVATE_KEY", false);
    std::shared_ptr<gloom::backends::GameTicketIssuer> tickets;
    if (!signing_key.empty()) tickets = std::make_shared<gloom::backends::GameTicketIssuer>(signing_key);
    if (provider == "keycloak") {
        verifier = gloom::backends::make_keycloak_identity_verifier({
            .issuer = credential("GLOOM_KEYCLOAK_ISSUER"),
            .client_id = credential("GLOOM_KEYCLOAK_INTROSPECTION_CLIENT_ID"),
            .client_secret = credential("GLOOM_KEYCLOAK_INTROSPECTION_CLIENT_SECRET"),
            .audience = credential("GLOOM_KEYCLOAK_AUDIENCE"),
            .authorized_client = credential("GLOOM_KEYCLOAK_CLIENT_ID")});
        if (tickets) game_verifier = gloom::backends::make_keycloak_identity_verifier({
            .issuer = credential("GLOOM_KEYCLOAK_ISSUER"),
            .client_id = credential("GLOOM_KEYCLOAK_INTROSPECTION_CLIENT_ID"),
            .client_secret = credential("GLOOM_KEYCLOAK_INTROSPECTION_CLIENT_SECRET"),
            .audience = credential("GLOOM_KEYCLOAK_AUDIENCE"),
            .authorized_client = credential("GLOOM_KEYCLOAK_CLIENT_ID"),
            .required_scope = "gloom.play"});
    } else if (provider.empty() || provider == "registry") {
        verifier = gloom::backends::make_file_match_identity_verifier(credential("GLOOM_MATCH_IDENTITIES_FILE"));
        const auto check = verifier("startup-validation-probe", 0);
        if (!check && check.error() != "Identity rejected") throw std::runtime_error{"Invalid or unavailable identity registry"};
        if (tickets) game_verifier = verifier;
    } else throw std::invalid_argument{"Unknown matchmaking identity provider"};
    auto service = gloom::gameplay::make_durable_match_service(argv[5]);
    if (!service) throw std::runtime_error{service.error()};
    gloom::backends::MatchHttpsHost host{*service, argv[3], argv[4], std::move(verifier), publisher,
                                        std::string{argv[5]} + ".revocations", std::move(tickets), std::move(game_verifier)};
    const auto bound = host.bind(argv[1], static_cast<int>(port));
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    const auto started = std::chrono::steady_clock::now();
    std::jthread shutdown([&](std::stop_token cancellation) {
        while (!cancellation.stop_requested()) {
            if (stopped.load(std::memory_order_relaxed) || (run_ms && std::chrono::steady_clock::now() - started >=
                                       std::chrono::milliseconds{run_ms})) { host.stop(); return; }
            std::this_thread::sleep_for(std::chrono::milliseconds{20});
        }
    });
    std::cout << "Match HTTPS listener bound on " << argv[1] << ':' << bound << std::endl;
    const auto clean = host.run();
    shutdown.request_stop();
    std::cout << "Match HTTPS listener stopped; pending writes drained." << std::endl;
    return clean ? 0 : 1;
} catch (const std::exception& error) {
    std::cerr << "Match service startup/run failed: " << error.what() << '\n';
    return 1;
}
