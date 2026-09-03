#include <gloom/backends/keycloak_identity.hpp>
#include <gloom/backends/match_https_host.hpp>
#include <gloom/gameplay/match_discovery.hpp>
#include <httplib.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <thread>

namespace {
using namespace gloom::backends;
void expect(bool value, const char* message) { if (!value) throw std::runtime_error{message}; }
std::string claims(const std::string& issuer, std::string_view subject = "stable-subject") {
    return "{\"active\":true,\"iss\":\"" + issuer + "\",\"sub\":\"" + std::string{subject} +
        "\",\"aud\":[\"account\",\"gloom-match\"],\"client_id\":\"gloom-desktop\","
        "\"token_type\":\"Bearer\",\"scope\":\"openid gloom.discovery\",\"exp\":2000,\"nbf\":1}";
}
std::string tokens(std::string_view access, std::string_view refresh = "refresh-credential-first") {
    return "{\"token_type\":\"Bearer\",\"access_token\":\"" + std::string{access} +
        "\",\"refresh_token\":\"" + std::string{refresh} + "\",\"expires_in\":10,\"refresh_expires_in\":600}";
}
std::string challenge(std::string_view issuer, bool interval = true) {
    return "{\"verification_uri\":\"" + std::string{issuer} + "/device\",\"user_code\":\"ABCD-1234\","
        "\"device_code\":\"private-device-code\",\"expires_in\":600" + (interval ? std::string{",\"interval\":1}"} : "}");
}
KeycloakVerifierSettings settings(std::string issuer = "https://accounts.test/realms/gloom") {
    return {std::move(issuer), "gloom-verifier", "confidential-host-secret", "gloom-match", "gloom-desktop"};
}
void replace(std::string& text, std::string_view from, std::string_view to) {
    const auto at = text.find(from);
    expect(at != std::string::npos, "Invalid test mutation"); text.replace(at, from.size(), to);
}

void deterministic() {
    using Clock = KeycloakSignIn::Clock;
    auto now = Clock::time_point{};
    const auto issuer = settings().issuer;
    std::string response = claims(issuer);
    unsigned http_status = 200;
    unsigned introspections = 0;
    const auto verifier = make_keycloak_identity_verifier(settings(), [&](std::string_view url, std::string_view body) {
        ++introspections;
        expect(url == issuer + "/protocol/openid-connect/token/introspect", "Wrong introspection endpoint");
        expect(body.find("client_secret=confidential-host-secret") != body.npos &&
               body.find("token_type_hint=access_token") != body.npos, "Introspection omitted authentication/hint");
        return std::expected<IdentityHttpResponse, std::string>{{http_status, response}};
    });
    const std::string long_access(2048, 'a');
    const auto first = verifier(long_access, 1000);
    expect(first && first->id.size() == 64 && first->expires_at_ms == 2'000'000, "Trusted introspection failed");
    expect(verifier("rotated-access-credential", 1000)->id == first->id, "Credential rotation changed principal");
    for (const auto& [from, to] : {std::pair{"true", "false"}, {"true", "\"true\""},
         {"accounts.test", "attacker.test"}, {"gloom-match", "another-resource"},
         {"gloom-desktop", "another-client"}, {"Bearer", "Refresh"}, {"gloom.discovery", "gloom.discovery.extra"},
         {"\"exp\":2000", "\"exp\":1"}, {"\"exp\":2000", "\"exp\":-1"},
         {"\"exp\":2000", "\"exp\":2.5"}, {"\"exp\":2000", "\"exp\":18446744073709551615"},
         {"\"nbf\":1", "\"nbf\":100"}, {"\"sub\":\"stable-subject\",", ""}}) {
        response = claims(issuer); replace(response, from, to);
        expect(!verifier(long_access, 1000), "Invalid identity claims accepted");
    }
    response = claims(issuer); response.insert(1, "\"active\":true,");
    expect(!verifier(long_access, 1000), "Duplicate security claim accepted");
    response = "{"; expect(!verifier(long_access, 1000), "Malformed JSON accepted");
    response = std::string(65537, 'x'); expect(!verifier(long_access, 1000), "Oversized JSON accepted");
    response = claims(issuer);
    auto game_settings = settings(); game_settings.required_scope = "gloom.play";
    const auto game_verifier = make_keycloak_identity_verifier(game_settings, [&](auto, auto) {
        return std::expected<IdentityHttpResponse, std::string>{{200, response}};
    });
    expect(!game_verifier(long_access, 1000), "Discovery scope granted gameplay access");
    replace(response, "openid gloom.discovery", "openid gloom.discovery gloom.play");
    expect(game_verifier(long_access, 1000).has_value(), "Gameplay scope rejected");
    response = claims(issuer);
    const auto before_invalid = introspections;
    expect(!verifier(std::string(4097, 'a'), 1000) && introspections == before_invalid, "Oversized token contacted provider");
    MatchReaderGrants grants{verifier};
    auto one = grants.exchange(long_access, 1000, now);
    auto two = grants.exchange("rotated-access-credential", 1000, now);
    expect(one && two, "Provider did not integrate with grants");
    for (int i = 0; i < 20; ++i) expect(grants.consume(one->token, now) == MatchGrantAccess::allowed, "Grant denied");
    expect(grants.consume(two->token, now) == MatchGrantAccess::throttled, "Identity rotation reset quota");
    expect(grants.revoke(first->id).has_value() && !grants.exchange(long_access, 1000, now), "Identity rotation bypassed revocation");
    http_status = 503;
    const auto failed = grants.exchange(long_access, 1000, now);
    expect(!failed && failed.error() == 503, "Provider outage was not surfaced");

    unsigned requests = 0, refreshes = 0;
    bool renewal_failure = false;
    KeycloakSignIn sign_in{{issuer, "gloom-desktop"}, [&](std::string_view url, std::string_view body)
        -> std::expected<IdentityHttpResponse, std::string> {
        expect(body.find("client_secret") == body.npos, "Public client sent a secret");
        if (url.ends_with("auth/device")) {
            expect(body == "client_id=gloom-desktop&scope=openid%20gloom.discovery", "Wrong device authorization form");
            return IdentityHttpResponse{200, challenge(issuer, false)};
        }
        if (body.find("grant_type=refresh_token") != body.npos) {
            ++refreshes;
            expect(body.find(refreshes == 1 ? "refresh-credential-first" : "refresh-credential-rotated") != body.npos,
                   "Rotated refresh credential not used");
            if (renewal_failure) return IdentityHttpResponse{400, "{\"error\":\"invalid_grant\"}"};
            return IdentityHttpResponse{200, tokens("new-access-credential", "refresh-credential-rotated")};
        }
        expect(body.find("grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code") != body.npos,
               "Wrong device token form");
        ++requests;
        if (requests == 1) return IdentityHttpResponse{400, "{\"error\":\"authorization_pending\"}"};
        if (requests == 2) return IdentityHttpResponse{400, "{\"error\":\"slow_down\"}"};
        return IdentityHttpResponse{200, tokens(long_access)};
    }, [&] { return now; }};
    const auto prompt = sign_in.begin(); expect(prompt && prompt->user_code == "ABCD-1234", "Device prompt failed");
    expect(!sign_in.poll().value() && requests == 0, "Polled before default interval");
    now += std::chrono::seconds{5}; expect(!sign_in.poll().value() && requests == 1, "Pending handling failed");
    now += std::chrono::seconds{5}; expect(!sign_in.poll().value() && requests == 2, "Slow-down handling failed");
    now += std::chrono::seconds{9}; expect(!sign_in.poll().value() && requests == 2, "Ignored slow-down interval");
    now += std::chrono::seconds{1}; expect(sign_in.poll().value(), "Authorized device did not complete");
    expect(sign_in.access_token().value() == long_access, "Wrong access token");
    now += std::chrono::milliseconds{8999}; expect(sign_in.access_token().value() == long_access && !refreshes, "Early refresh");
    now += std::chrono::milliseconds{1};
    {
        std::vector<std::jthread> workers;
        std::atomic_uint successes{0};
        for (int i = 0; i < 8; ++i) workers.emplace_back([&] { if (sign_in.access_token()) ++successes; });
        workers.clear(); expect(successes == 8 && refreshes == 1, "Concurrent refresh replayed rotation");
    }
    now += std::chrono::seconds{9}; renewal_failure = true;
    expect(!sign_in.access_token() && refreshes == 2 && !sign_in.access_token() && refreshes == 2,
           "Failed refresh reused/replayed credentials");
    sign_in.cancel(); expect(!sign_in.poll() && !sign_in.access_token(), "Cancellation retained credentials");

    for (const auto& error : {"access_denied", "expired_token"}) {
        KeycloakSignIn denied{{issuer, "gloom-desktop"}, [&](std::string_view url, std::string_view) {
            return std::expected<IdentityHttpResponse, std::string>{{url.ends_with("auth/device") ? 200U : 400U,
                url.ends_with("auth/device") ? challenge(issuer) : "{\"error\":\"" + std::string{error} + "\"}"}};
        }, [&] { return now; }};
        expect(denied.begin().has_value(), "Begin failed"); now += std::chrono::seconds{1};
        expect(!denied.poll() && !denied.poll(), "Terminal OAuth error kept polling");
    }
    unsigned polls = 0;
    KeycloakSignIn timeout{{issuer, "gloom-desktop"}, [&](std::string_view url, std::string_view)
        -> std::expected<IdentityHttpResponse, std::string> {
        if (url.ends_with("auth/device")) return IdentityHttpResponse{200, challenge(issuer)};
        ++polls; return std::unexpected{"network timeout"};
    }, [&] { return now; }};
    expect(timeout.begin().has_value(), "Begin failed"); now += std::chrono::seconds{1};
    expect(!timeout.poll().value() && polls == 1, "Timeout handling failed"); now += std::chrono::seconds{1};
    expect(!timeout.poll().value() && polls == 1, "Timeout did not double interval"); now += std::chrono::seconds{1};
    expect(!timeout.poll().value() && polls == 2, "Timeout backoff did not recover"); now += std::chrono::seconds{600};
    expect(!timeout.poll() && polls == 2, "Expired device continued polling");
    KeycloakSignIn malicious{{issuer, "gloom-desktop"}, [](std::string_view, std::string_view) {
        return std::expected<IdentityHttpResponse, std::string>{{200, challenge("https://attacker.test")}};
    }};
    expect(!malicious.begin(), "Cross-origin verification prompt accepted");
    bool refused = false;
    try { KeycloakSignIn invalid{{"http://accounts.test/realm", "public"}}; } catch (...) { refused = true; }
    expect(refused, "Plain HTTP issuer accepted");
}

void certificate(const std::filesystem::path& root) {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key{EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", 2048), EVP_PKEY_free};
    std::unique_ptr<X509, decltype(&X509_free)> cert{X509_new(), X509_free};
    expect(key && cert, "TLS fixture generation failed");
    X509_set_version(cert.get(), 2); ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1);
    X509_gmtime_adj(X509_getm_notBefore(cert.get()), -60); X509_gmtime_adj(X509_getm_notAfter(cert.get()), 3600);
    X509_set_pubkey(cert.get(), key.get());
    auto* name = X509_get_subject_name(cert.get());
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);
    X509_set_issuer_name(cert.get(), name);
    X509V3_CTX context{}; X509V3_set_ctx(&context, cert.get(), cert.get(), nullptr, nullptr, 0);
    auto* ext = X509V3_EXT_conf_nid(nullptr, &context, NID_subject_alt_name, "DNS:localhost,IP:127.0.0.1");
    expect(ext != nullptr, "TLS extension failed"); X509_add_ext(cert.get(), ext, -1); X509_EXTENSION_free(ext);
    expect(X509_sign(cert.get(), key.get(), EVP_sha256()) > 0, "TLS signing failed");
    auto* output = BIO_new_file((root / "cert.pem").string().c_str(), "w");
    expect(output && PEM_write_bio_X509(output, cert.get()) == 1, "TLS cert write failed"); BIO_free(output);
    output = BIO_new_file((root / "key.pem").string().c_str(), "w");
    expect(output && PEM_write_bio_PrivateKey(output, key.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1, "TLS key write failed");
    BIO_free(output);
}
void https_integration() {
    const auto root = std::filesystem::temp_directory_path() /
        ("gloom-keycloak-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root); certificate(root);
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> game_key{EVP_PKEY_Q_keygen(nullptr, nullptr, "ED25519"), EVP_PKEY_free};
    auto* key_output = BIO_new_file((root / "game-private.pem").string().c_str(), "w");
    expect(game_key && key_output && PEM_write_bio_PrivateKey(key_output, game_key.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1, "Game key write failed");
    BIO_free(key_output); key_output = BIO_new_file((root / "game-public.pem").string().c_str(), "w");
    expect(key_output && PEM_write_bio_PUBKEY(key_output, game_key.get()) == 1, "Game public key write failed"); BIO_free(key_output);
    httplib::SSLServer provider{(root / "cert.pem").string().c_str(), (root / "key.pem").string().c_str()};
    const auto port = provider.bind_to_any_port("127.0.0.1"); expect(port > 0, "Provider bind failed");
    const auto base = "https://127.0.0.1:" + std::to_string(port);
    const auto issuer = base + "/realms/gloom";
    const std::string access(2048, 'a');
    std::atomic_uint refresh_count{0};
    std::atomic_bool play_scope{true};
    provider.Post("/realms/gloom/protocol/openid-connect/auth/device", [&](const auto& request, auto& response) {
        if (request.get_param_value("client_id") != "gloom-desktop") { response.status = 400; return; }
        response.set_content(challenge(issuer), "application/json");
    });
    provider.Post("/realms/gloom/protocol/openid-connect/token", [&](const auto& request, auto& response) {
        if (request.has_param("client_secret")) { response.status = 400; return; }
        if (request.get_param_value("grant_type") == "refresh_token") ++refresh_count;
        response.set_content(tokens(access), "application/json");
    });
    provider.Post("/realms/gloom/protocol/openid-connect/token/introspect", [&](const auto& request, auto& response) {
        if (request.get_param_value("client_secret") != "confidential-host-secret" || request.get_param_value("token") != access) {
            response.set_content("{\"active\":false}", "application/json"); return;
        }
        auto body = claims(issuer);
        if (play_scope) replace(body, "openid gloom.discovery", "openid gloom.discovery gloom.play");
        const auto utc = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        replace(body, "\"exp\":2000", "\"exp\":" + std::to_string(utc + 120));
        response.set_content(body, "application/json");
    });
    std::jthread provider_thread{[&] { provider.listen_after_bind(); }};
    struct Stop { httplib::SSLServer& server; ~Stop() { server.stop(); } } stop{provider};
    IdentityHttpPost post = [&](std::string_view url, std::string_view body) -> std::expected<IdentityHttpResponse, std::string> {
        expect(url.starts_with(base + "/"), "Test credential sent off configured origin");
        httplib::SSLClient client{"127.0.0.1", port}; client.set_ca_cert_path((root / "cert.pem").string());
        const auto response = client.Post(std::string{url.substr(base.size())}, std::string{body}, "application/x-www-form-urlencoded");
        if (!response) return std::unexpected{"TLS fixture request failed"};
        return IdentityHttpResponse{static_cast<unsigned>(response->status), response->body};
    };
    expect(!make_winhttp_identity_post()(issuer + "/protocol/openid-connect/auth/device", "client_id=gloom-desktop"),
           "WinHTTP trusted an untrusted certificate");
    auto time = KeycloakSignIn::Clock::time_point{};
    KeycloakSignIn sign_in{{issuer, "gloom-desktop"}, post, [&] { return time; }};
    expect(sign_in.begin().has_value(), "Real HTTPS device authorization failed"); time += std::chrono::seconds{1};
    expect(sign_in.poll().value() && sign_in.access_token().value() == access, "Real HTTPS device exchange failed");
    time += std::chrono::seconds{9};
    expect(sign_in.access_token().value() == access && refresh_count == 1, "Real HTTPS refresh failed");
    auto verifier = make_keycloak_identity_verifier(settings(issuer), post);
    auto store = gloom::gameplay::make_durable_match_service(root / "matches.store"); expect(store.has_value(), "Store failed");
    const auto utc_ms = [] { return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()); };
    auto game_settings = settings(issuer); game_settings.required_scope = "gloom.play";
    auto ticket_verifier = make_game_ticket_verifier(root / "game-public.pem", "verified-instance", utc_ms);
    expect((*store)->store({.advertisement = {.match_id = "verified-match", .instance_id = "verified-instance",
        .display_name = "Verified match", .endpoint = "127.0.0.1:27020"}, .expires_at_ms = utc_ms() + 60000,
        .mutation_id = "ticket-fixture"}, utc_ms()).has_value(), "Ticket match publication failed");
    {
        MatchHttpsHost host{*store, (root / "cert.pem").string(), (root / "key.pem").string(), verifier,
            "publisher-test-credential", root / "revocations", std::make_shared<GameTicketIssuer>(root / "game-private.pem"),
            make_keycloak_identity_verifier(game_settings, post)};
        const auto host_port = host.bind("127.0.0.1", 0);
        std::jthread host_thread{[&] { static_cast<void>(host.run()); }};
        struct StopHost { MatchHttpsHost& host; ~StopHost() { host.stop(); } } stop_host{host};
        httplib::SSLClient client{"127.0.0.1", host_port}; client.set_ca_cert_path((root / "cert.pem").string());
        auto response = client.Post("/reader-grants/exchange", {{"Authorization", "Bearer " + access}}, "", "text/plain");
        expect(response && response->status == 200, "Provider identity did not obtain grant over HTTPS");
        const httplib::Headers grant{{"Authorization", "Bearer " + response->body.substr(0, response->body.find('\t'))}};
        response = client.Get("/matches", grant); expect(response && response->status == 200, "Grant discovery failed");
        response = client.Get("/ready", grant); expect(response && response->status == 403, "Provider grant gained publisher scope");
        const auto principal = match_credential_digest(issuer + "\nstable-subject");
        response = client.Post("/game-tickets/verified-match", grant, "", "text/plain");
        expect(response && response->status == 401, "Discovery grant gained gameplay access");
        response = client.Post("/game-tickets/verified-match", {{"Authorization", "Bearer publisher-test-credential"}}, "", "text/plain");
        expect(response && response->status == 401, "Publisher token gained gameplay access");
        play_scope = false;
        response = client.Post("/game-tickets/verified-match", {{"Authorization", "Bearer " + access}}, "", "text/plain");
        expect(response && response->status == 401, "Discovery-only account gained gameplay access"); play_scope = true;
        response = client.Post("/game-tickets/verified-match", {{"Authorization", "Bearer " + access}}, "", "text/plain");
        expect(response && response->status == 200 && response->body.starts_with("127.0.0.1:27020\tverified-instance\tgt1."), "HTTPS ticket exchange failed");
        const auto ticket = response->body.substr(response->body.rfind('\t') + 1);
        const auto identity = ticket_verifier->verify(ticket);
        expect(identity.has_value() && !ticket_verifier->verify(ticket), "HTTPS-issued ticket not verifiable or replayable");
        response = client.Get("/matches", {{"Authorization", "Bearer " + ticket}});
        expect(response && response->status == 401, "Game ticket gained discovery access");
        response = client.Post("/game-tickets/missing", {{"Authorization", "Bearer " + access}}, "", "text/plain");
        expect(response && response->status == 404, "Missing match issued ticket");
        response = client.Delete("/reader-grants/" + principal, httplib::Headers{{"Authorization", "Bearer publisher-test-credential"}});
        expect(response && response->status == 204, "Provider principal revocation failed");
        response = client.Post("/reader-grants/exchange", {{"Authorization", "Bearer " + access}}, "", "text/plain");
        expect(response && response->status == 401, "Provider identity bypassed suspension");
        response = client.Post("/game-tickets/verified-match", {{"Authorization", "Bearer " + access}}, "", "text/plain");
        expect(response && response->status == 401, "Suspended identity obtained game ticket");
    }
    store = std::unexpected{"closed"};
    provider.stop(); provider_thread.join();
    std::filesystem::remove_all(root); // Uniquely created test directory, no system trust changes.
}
}
int main() try {
    deterministic(); https_integration(); std::cout << "Keycloak identity integration passed\n"; return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
