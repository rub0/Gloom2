#pragma once
#include <gloom/backends/match_reader_grants.hpp>
#include <chrono>
#include <functional>
#include <memory>

namespace gloom::backends {
struct IdentityHttpResponse { unsigned status{0}; std::string body; };
// POST form data to a configured HTTPS endpoint, bounded response, no redirects.
using IdentityHttpPost = std::function<std::expected<IdentityHttpResponse, std::string>(
    std::string_view url, std::string_view form)>;
[[nodiscard]] IdentityHttpPost make_winhttp_identity_post();

struct KeycloakClientSettings {
    std::string issuer;
    std::string client_id;
    bool game_access{false};
};
struct KeycloakVerifierSettings {
    std::string issuer;
    std::string client_id; // Confidential introspection client, host only.
    std::string client_secret;
    std::string audience;
    std::string authorized_client; // Public device client allowed to obtain tokens.
    std::string required_scope{"gloom.discovery"};
};
[[nodiscard]] MatchIdentityVerifier make_keycloak_identity_verifier(
    KeycloakVerifierSettings settings, IdentityHttpPost post = make_winhttp_identity_post());

struct KeycloakDeviceChallenge {
    std::string verification_uri;
    std::string user_code;
};
// Owns device/access/refresh credentials only in memory. Methods are serialized;
// a refresh failure requires explicit new sign-in, not a password fallback.
class KeycloakSignIn final {
public:
    using Clock = std::chrono::steady_clock;
    using Now = std::function<Clock::time_point()>;
    KeycloakSignIn(KeycloakClientSettings settings, IdentityHttpPost post = make_winhttp_identity_post(),
                   Now now = [] { return Clock::now(); });
    ~KeycloakSignIn();
    [[nodiscard]] std::expected<KeycloakDeviceChallenge, std::string> begin();
    // False means pending (including waiting for the provider's polling interval).
    [[nodiscard]] std::expected<bool, std::string> poll();
    [[nodiscard]] std::expected<std::string, std::string> access_token();
    void cancel();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace gloom::backends
