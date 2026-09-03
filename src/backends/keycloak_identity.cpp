#include <gloom/backends/keycloak_identity.hpp>
#include <simdjson.h>
#include <algorithm>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gloom::backends {
namespace {
constexpr auto scope = "openid gloom.discovery";
bool printable(std::string_view value, std::size_t limit) {
    return !value.empty() && value.size() <= limit &&
        std::ranges::all_of(value, [](unsigned char c) { return c > 32 && c < 127; });
}
bool https_url(std::string_view value) {
    return printable(value, 1800) && value.starts_with("https://") && value.size() > 8 &&
        value.find_first_of("@?#\\") == std::string_view::npos && value[8] != '/';
}
std::string origin(std::string_view value) { return std::string{value.substr(0, value.find('/', 8))}; }
std::string form(std::string_view value) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') result += static_cast<char>(c);
        else { result += '%'; result += hex[c >> 4U]; result += hex[c & 15U]; }
    }
    return result;
}
void check_client(const KeycloakClientSettings& settings) {
    if (!https_url(settings.issuer) || settings.issuer.ends_with('/') || !printable(settings.client_id, 128))
        throw std::invalid_argument{"Invalid Keycloak issuer or client id"};
}
simdjson::dom::object object(simdjson::dom::parser& parser, const std::string& body) {
    if (body.empty() || body.size() > 64 * 1024) throw std::runtime_error{"Invalid identity response"};
    auto result = parser.parse(body).get_object().value();
    std::set<std::string_view> keys;
    for (auto field : result)
        if (!keys.insert(field.key).second || keys.size() > 128) throw std::runtime_error{"Ambiguous identity response"};
    return result;
}
std::string string(simdjson::dom::object obj, std::string_view key, std::size_t limit = 4096) {
    const auto value = obj.at_key(key).get_string().value();
    if (!printable(value, limit)) throw std::runtime_error{"Invalid identity field"};
    return std::string{value};
}
std::uint64_t number(simdjson::dom::object obj, std::string_view key, std::uint64_t limit) {
    const auto value = obj.at_key(key).get_uint64().value();
    if (!value || value > limit) throw std::runtime_error{"Invalid identity lifetime"};
    return value;
}
bool audience(simdjson::dom::object obj, std::string_view expected) {
    const auto value = obj.at_key("aud").value();
    if (value.is_string()) return value.get_string().value() == expected;
    if (!value.is_array()) return false;
    bool found = false;
    for (auto item : value.get_array().value()) {
        if (!item.is_string()) return false;
        found |= item.get_string().value() == expected;
    }
    return found;
}
bool has_scope(simdjson::dom::object obj, std::string_view required) {
    const auto value = obj.at_key("scope").get_string().value();
    if (value.size() > 2048) return false;
    std::istringstream fields{std::string{value}};
    for (std::string field; fields >> field;) if (field == required) return true;
    return false;
}
}

MatchIdentityVerifier make_keycloak_identity_verifier(KeycloakVerifierSettings settings, IdentityHttpPost post) {
    check_client({settings.issuer, settings.client_id});
    if (!post || !printable(settings.client_secret, 512) || !printable(settings.audience, 128) ||
        !printable(settings.authorized_client, 128) || !printable(settings.required_scope, 128))
        throw std::invalid_argument{"Invalid Keycloak verifier configuration"};
    return [settings = std::move(settings), post = std::move(post)](std::string_view credential, std::uint64_t now)
        -> std::expected<MatchVerifiedPrincipal, std::string> {
        if (!valid_match_identity_credential(credential)) return std::unexpected{"Identity rejected"};
        const auto response = post(settings.issuer + "/protocol/openid-connect/token/introspect",
            "client_id=" + form(settings.client_id) + "&client_secret=" + form(settings.client_secret) +
            "&token_type_hint=access_token&token=" + form(credential));
        if (!response || response->status != 200)
            throw std::runtime_error{"Identity provider unavailable"}; // Host maps to 503, no provider body.
        try {
            simdjson::dom::parser parser;
            const auto obj = object(parser, response->body);
            if (!obj.at_key("active").get_bool().value() || string(obj, "iss", 1800) != settings.issuer ||
                !audience(obj, settings.audience) || string(obj, "client_id", 128) != settings.authorized_client ||
                string(obj, "token_type", 32) != "Bearer" || !has_scope(obj, settings.required_scope))
                return std::unexpected{"Identity rejected"};
            const auto expiry = number(obj, "exp", std::numeric_limits<std::uint64_t>::max() / 1000) * 1000;
            if (expiry <= now) return std::unexpected{"Identity rejected"};
            const auto nbf = obj.at_key("nbf");
            if (nbf.error() != simdjson::NO_SUCH_FIELD && nbf.get_uint64().value() > now / 1000)
                return std::unexpected{"Identity rejected"};
            const auto subject = string(obj, "sub", 256);
            // Namespace by issuer. Never use mutable username/email/display claims.
            return MatchVerifiedPrincipal{match_credential_digest(settings.issuer + "\n" + subject), expiry};
        } catch (...) { return std::unexpected{"Identity rejected"}; }
    };
}

struct KeycloakSignIn::Impl {
    KeycloakClientSettings settings;
    IdentityHttpPost post;
    Now now;
    std::mutex mutex;
    std::string device, access, refresh;
    Clock::time_point device_expiry{}, next_poll{}, access_expiry{}, refresh_expiry{};
    std::chrono::seconds interval{5};
    bool pending{false};
    void clear() { device.clear(); access.clear(); refresh.clear(); pending = false; }
    std::string endpoint(std::string_view suffix) const {
        return settings.issuer + "/protocol/openid-connect/" + std::string{suffix};
    }
    void accept_tokens(const std::string& body, Clock::time_point started) {
        simdjson::dom::parser parser;
        const auto obj = object(parser, body);
        if (string(obj, "token_type", 32) != "Bearer") throw std::runtime_error{"Unsupported token type"};
        auto new_access = string(obj, "access_token"), new_refresh = string(obj, "refresh_token");
        if (!valid_match_identity_credential(new_access) || !valid_match_identity_credential(new_refresh))
            throw std::runtime_error{"Invalid provider token"};
        const auto duration = number(obj, "expires_in", 86'400);
        const auto refresh_duration = number(obj, "refresh_expires_in", 30 * 86'400);
        const auto expiry = started + std::chrono::seconds{duration} -
            std::chrono::milliseconds{std::min<std::uint64_t>(5000, duration * 100)};
        const auto refresh_until = started + std::chrono::seconds{refresh_duration};
        if (now() >= expiry || now() >= refresh_until) throw std::runtime_error{"Provider response expired"};
        access = std::move(new_access); refresh = std::move(new_refresh);
        access_expiry = expiry; refresh_expiry = refresh_until;
        device.clear(); pending = false;
    }
};
KeycloakSignIn::KeycloakSignIn(KeycloakClientSettings settings, IdentityHttpPost post, Now now)
    : impl_{std::make_unique<Impl>()} {
    check_client(settings);
    if (!post || !now) throw std::invalid_argument{"Identity transport and clock are required"};
    impl_->settings = std::move(settings); impl_->post = std::move(post); impl_->now = std::move(now);
}
KeycloakSignIn::~KeycloakSignIn() = default;
std::expected<KeycloakDeviceChallenge, std::string> KeycloakSignIn::begin() {
    std::lock_guard lock{impl_->mutex};
    impl_->clear();
    try {
        const auto started = impl_->now();
        const auto response = impl_->post(impl_->endpoint("auth/device"),
            "client_id=" + form(impl_->settings.client_id) + "&scope=" +
                form(impl_->settings.game_access ? "openid gloom.discovery gloom.play" : scope));
        if (!response || response->status != 200) return std::unexpected{"Could not begin Keycloak sign-in"};
        simdjson::dom::parser parser;
        const auto obj = object(parser, response->body);
        KeycloakDeviceChallenge challenge{string(obj, "verification_uri", 1800), string(obj, "user_code", 32)};
        if (!https_url(challenge.verification_uri) || origin(challenge.verification_uri) != origin(impl_->settings.issuer) ||
            !std::ranges::all_of(challenge.user_code, [](unsigned char c) {
                return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
            })) return std::unexpected{"Invalid Keycloak sign-in challenge"};
        const auto duration = number(obj, "expires_in", 900);
        std::uint64_t interval = 5;
        if (obj.at_key("interval").error() != simdjson::NO_SUCH_FIELD) interval = number(obj, "interval", 60);
        impl_->device = string(obj, "device_code");
        impl_->device_expiry = started + std::chrono::seconds{duration};
        if (impl_->now() >= impl_->device_expiry) { impl_->clear(); return std::unexpected{"Sign-in challenge expired"}; }
        impl_->interval = std::chrono::seconds{interval};
        impl_->next_poll = impl_->now() + impl_->interval;
        impl_->pending = true;
        return challenge;
    } catch (...) { impl_->clear(); return std::unexpected{"Invalid Keycloak sign-in response"}; }
}
std::expected<bool, std::string> KeycloakSignIn::poll() {
    std::lock_guard lock{impl_->mutex};
    if (!impl_->pending) return std::unexpected{"No pending sign-in"};
    const auto started = impl_->now();
    if (started >= impl_->device_expiry) { impl_->clear(); return std::unexpected{"Keycloak sign-in expired"}; }
    if (started < impl_->next_poll) return false;
    try {
        const auto response = impl_->post(impl_->endpoint("token"), "client_id=" + form(impl_->settings.client_id) +
            "&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code&device_code=" + form(impl_->device));
        if (impl_->now() >= impl_->device_expiry) { impl_->clear(); return std::unexpected{"Keycloak sign-in expired"}; }
        if (!response) {
            impl_->interval = std::min(impl_->interval * 2, std::chrono::seconds{300});
            impl_->next_poll = impl_->now() + impl_->interval;
            return false;
        }
        if (response->status == 200) { impl_->accept_tokens(response->body, started); return true; }
        if (response->status == 400) {
            simdjson::dom::parser parser;
            const auto error = string(object(parser, response->body), "error", 64);
            if (error == "authorization_pending" || error == "slow_down") {
                if (error == "slow_down") impl_->interval += std::chrono::seconds{5};
                impl_->next_poll = impl_->now() + impl_->interval;
                return false;
            }
        }
    } catch (...) { /* No provider text or tokens in diagnostics. */ }
    impl_->clear();
    return std::unexpected{"Keycloak sign-in denied or failed; start sign-in again"};
}
std::expected<std::string, std::string> KeycloakSignIn::access_token() {
    std::lock_guard lock{impl_->mutex};
    const auto started = impl_->now();
    if (!impl_->access.empty() && started < impl_->access_expiry) return impl_->access;
    if (impl_->refresh.empty() || started >= impl_->refresh_expiry) {
        impl_->clear(); return std::unexpected{"Keycloak sign-in required"};
    }
    try {
        const auto response = impl_->post(impl_->endpoint("token"),
            "client_id=" + form(impl_->settings.client_id) + "&grant_type=refresh_token&refresh_token=" + form(impl_->refresh));
        if (response && response->status == 200) {
            impl_->accept_tokens(response->body, started);
            return impl_->access;
        }
    } catch (...) { /* Fail closed, never reuse an expired access token or replay refresh. */ }
    impl_->clear();
    return std::unexpected{"Keycloak renewal failed; sign in again"};
}
void KeycloakSignIn::cancel() { std::lock_guard lock{impl_->mutex}; impl_->clear(); }
} // namespace gloom::backends
