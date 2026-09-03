#pragma once
#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace gloom::backends {
struct MatchVerifiedPrincipal {
    std::string id;
    std::uint64_t expires_at_ms{0};
};
// Implementations verify opaque identity credentials; clients never choose the id.
using MatchIdentityVerifier = std::function<std::expected<MatchVerifiedPrincipal, std::string>(
    std::string_view credential, std::uint64_t now_ms)>;
// Reloads a bounded, operator-owned digest registry on every exchange, failing closed.
[[nodiscard]] MatchIdentityVerifier make_file_match_identity_verifier(std::filesystem::path path);
[[nodiscard]] std::string match_credential_digest(std::string_view credential);
[[nodiscard]] bool valid_match_credential(std::string_view credential);
// External access tokens can be larger than operator tokens; still bounded for HTTP headers.
[[nodiscard]] bool valid_match_identity_credential(std::string_view credential);

struct MatchReaderGrant {
    std::string token;
    std::uint64_t lifetime_ms{0};
};
enum class MatchGrantAccess { invalid, allowed, throttled };

class MatchReaderGrants final {
public:
    using Clock = std::chrono::steady_clock;
    explicit MatchReaderGrants(MatchIdentityVerifier verifier, std::filesystem::path revocations = {});
    ~MatchReaderGrants();
    // Error values are HTTP statuses; no provider errors or credentials escape.
    [[nodiscard]] std::expected<MatchReaderGrant, int> exchange(
        std::string_view credential, std::uint64_t utc_ms, Clock::time_point now = Clock::now());
    [[nodiscard]] MatchGrantAccess consume(std::string_view token, Clock::time_point now = Clock::now());
    // Trusted provider result only: shared suspension and per-principal issuance budget.
    [[nodiscard]] std::expected<void, int> authorize_game_identity(
        const MatchVerifiedPrincipal& identity, std::uint64_t utc_ms, Clock::time_point now = Clock::now());
    // Suspends the principal, clears every grant, retains quota state. A configured
    // revocation file must be protected by the host's exclusive store lock.
    [[nodiscard]] std::expected<void, int> revoke(std::string_view principal);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace gloom::backends
