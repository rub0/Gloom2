#include <gloom/backends/match_reader_grants.hpp>
#include <gloom/backends/match_request_budget.hpp>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace gloom::backends {
namespace {
bool valid_principal(std::string_view id) {
    return !id.empty() && id.size() <= 64 && std::ranges::all_of(id, [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '-' || c == '_';
    });
}
std::string hex(const unsigned char* bytes, std::size_t size) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (std::size_t i = 0; i < size; ++i) {
        result += digits[bytes[i] >> 4U]; result += digits[bytes[i] & 15U];
    }
    return result;
}
}
bool valid_match_credential(std::string_view value) {
    return value.size() >= 16 && value.size() <= 512 &&
        std::ranges::all_of(value, [](unsigned char c) { return c > 32 && c < 127; });
}
bool valid_match_identity_credential(std::string_view value) {
    return value.size() >= 16 && value.size() <= 4096 &&
        std::ranges::all_of(value, [](unsigned char c) { return c > 32 && c < 127; });
}
std::string match_credential_digest(std::string_view credential) {
    std::array<unsigned char, 32> digest{};
    unsigned size = 0;
    if (EVP_Digest(credential.data(), credential.size(), digest.data(), &size, EVP_sha256(), nullptr) != 1 ||
        size != digest.size()) throw std::runtime_error{"Credential digest failed"};
    return hex(digest.data(), digest.size());
}
MatchIdentityVerifier make_file_match_identity_verifier(std::filesystem::path path) {
    return [path = std::move(path)](std::string_view credential, std::uint64_t now)
        -> std::expected<MatchVerifiedPrincipal, std::string> {
        if (!valid_match_credential(credential)) return std::unexpected{"Identity rejected"};
        std::ifstream input{path, std::ios::binary};
        if (!input) return std::unexpected{"Identity registry unavailable"};
        // Read at most the limit + 1 even if the file grows during the read.
        std::string bytes(160 * 1024 + 1, '\0');
        input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        const auto count = input.gcount();
        if (input.bad() || count > 160 * 1024) return std::unexpected{"Identity registry invalid"};
        bytes.resize(static_cast<std::size_t>(count));
        std::istringstream lines{bytes};
        std::string line;
        if (!std::getline(lines, line)) return std::unexpected{"Identity registry invalid"};
        if (line.ends_with('\r')) line.pop_back();
        if (line != "GLOOM_MATCH_IDENTITIES_V1")
            return std::unexpected{"Identity registry invalid"};
        const auto digest = match_credential_digest(credential);
        std::set<std::string> principals, digests;
        std::optional<MatchVerifiedPrincipal> result;
        while (std::getline(lines, line)) {
            if (line.ends_with('\r')) line.pop_back();
            if (line.empty()) continue;
            const auto first = line.find('\t'), last = line.rfind('\t');
            if (first == std::string::npos || first == last) return std::unexpected{"Identity registry invalid"};
            const auto id = line.substr(0, first), hash = line.substr(first + 1, last - first - 1);
            std::uint64_t expiry = 0;
            const auto parsed = std::from_chars(line.data() + last + 1, line.data() + line.size(), expiry);
            if (!valid_principal(id) || hash.size() != 64 || !std::ranges::all_of(hash, [](char c) {
                    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                }) || parsed.ec != std::errc{} || parsed.ptr != line.data() + line.size() || !expiry ||
                !principals.insert(id).second || !digests.insert(hash).second || principals.size() > 1024)
                return std::unexpected{"Identity registry invalid"};
            if (CRYPTO_memcmp(digest.data(), hash.data(), 64) == 0 && expiry > now)
                result = MatchVerifiedPrincipal{id, expiry};
        }
        if (!result) return std::unexpected{"Identity rejected"};
        return *result;
    };
}

struct MatchReaderGrants::Impl {
    struct Grant { std::string digest; Clock::time_point expires; };
    struct Principal {
        explicit Principal(Clock::time_point now) : reads{20, 5, now}, exchanges{4, 1, now} {}
        MatchRequestBudget reads, exchanges;
        std::vector<Grant> grants;
        bool revoked{false};
    };
    MatchIdentityVerifier verify;
    std::mutex mutex;
    // Never evict quota/suspension records: renewal and churn cannot reset them.
    std::map<std::string, Principal, std::less<>> principals;
    std::filesystem::path revocations;
    bool save() const {
        if (revocations.empty()) return true; // In-memory embedding/tests only.
        auto temporary = revocations; temporary += ".tmp";
        {
            std::ofstream output{temporary, std::ios::trunc | std::ios::binary};
            output << "GLOOM_MATCH_REVOCATIONS_V1\n";
            for (const auto& [id, principal] : principals)
                if (principal.revoked) output << id << '\n';
            output.flush();
            if (!output) return false;
            output.close();
            if (!output) return false;
        }
#ifdef _WIN32
        const auto pending = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (pending == INVALID_HANDLE_VALUE) return false;
        const auto flushed = FlushFileBuffers(pending);
        CloseHandle(pending);
        return flushed && MoveFileExW(temporary.c_str(), revocations.c_str(),
                                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
        std::error_code error;
        std::filesystem::rename(temporary, revocations, error);
        return !error;
#endif
    }
    void load() {
        if (revocations.empty()) return;
        if (std::filesystem::exists(revocations)) {
            std::ifstream input{revocations, std::ios::binary};
            std::string bytes(70 * 1024 + 1, '\0');
            input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            const auto count = input.gcount();
            if (input.bad() || count > 70 * 1024) throw std::runtime_error{"Invalid revocation store"};
            bytes.resize(static_cast<std::size_t>(count));
            std::istringstream lines{bytes};
            std::string id;
            if (!std::getline(lines, id) || id != "GLOOM_MATCH_REVOCATIONS_V1")
                throw std::runtime_error{"Invalid revocation store"};
            while (std::getline(lines, id)) {
                if (!valid_principal(id) || principals.size() >= 1024 || principals.contains(id))
                    throw std::runtime_error{"Invalid revocation store"};
                principals.try_emplace(id, Clock::now()).first->second.revoked = true;
            }
        }
        if (!save()) throw std::runtime_error{"Revocation store unavailable"};
    }
};
MatchReaderGrants::MatchReaderGrants(MatchIdentityVerifier verifier, std::filesystem::path revocations)
    : impl_{std::make_unique<Impl>()} {
    if (!verifier) throw std::invalid_argument{"Identity verifier is required"};
    impl_->verify = std::move(verifier);
    impl_->revocations = std::move(revocations);
    impl_->load();
}
MatchReaderGrants::~MatchReaderGrants() = default;
std::expected<void, int> MatchReaderGrants::authorize_game_identity(
    const MatchVerifiedPrincipal& identity, std::uint64_t utc, Clock::time_point now) {
    if (!valid_principal(identity.id) || identity.expires_at_ms <= utc) return std::unexpected{401};
    std::lock_guard lock{impl_->mutex};
    auto found = impl_->principals.find(identity.id);
    if (found == impl_->principals.end()) {
        if (impl_->principals.size() >= 1024) return std::unexpected{503};
        found = impl_->principals.try_emplace(identity.id, now).first;
    }
    if (found->second.revoked) return std::unexpected{401};
    if (!found->second.exchanges.consume(now)) return std::unexpected{429};
    return {};
}
std::expected<MatchReaderGrant, int> MatchReaderGrants::exchange(
    std::string_view credential, std::uint64_t utc, Clock::time_point now) {
    if (!valid_match_identity_credential(credential)) return std::unexpected{401};
    std::expected<MatchVerifiedPrincipal, std::string> identity = std::unexpected{"Identity rejected"};
    try { identity = impl_->verify(credential, utc); } catch (...) { return std::unexpected{503}; }
    if (!identity || !valid_principal(identity->id) || identity->expires_at_ms <= utc)
        return std::unexpected{401};
    std::lock_guard lock{impl_->mutex};
    auto entry = impl_->principals.find(identity->id);
    if (entry == impl_->principals.end()) {
        if (impl_->principals.size() >= 1024) return std::unexpected{503};
        entry = impl_->principals.try_emplace(identity->id, now).first;
    }
    auto& principal = entry->second;
    if (principal.revoked) return std::unexpected{401};
    if (!principal.exchanges.consume(now)) return std::unexpected{429};
    std::erase_if(principal.grants, [&](const auto& grant) { return grant.expires <= now; });
    if (principal.grants.size() >= 4) return std::unexpected{429};
    std::array<unsigned char, 32> random{};
    if (RAND_bytes(random.data(), static_cast<int>(random.size())) != 1) return std::unexpected{503};
    MatchReaderGrant grant{"gr1_" + hex(random.data(), random.size()),
                           std::min<std::uint64_t>(60'000, identity->expires_at_ms - utc)};
    principal.grants.push_back({match_credential_digest(grant.token), now + std::chrono::milliseconds{grant.lifetime_ms}});
    return grant;
}
MatchGrantAccess MatchReaderGrants::consume(std::string_view token, Clock::time_point now) {
    if (token.size() != 68 || !token.starts_with("gr1_")) return MatchGrantAccess::invalid;
    const auto digest = match_credential_digest(token);
    std::lock_guard lock{impl_->mutex};
    for (auto& [id, principal] : impl_->principals) {
        for (const auto& grant : principal.grants) {
            if (CRYPTO_memcmp(digest.data(), grant.digest.data(), digest.size()) == 0 &&
                grant.expires > now && !principal.revoked)
                return principal.reads.consume(now) ? MatchGrantAccess::allowed : MatchGrantAccess::throttled;
        }
    }
    return MatchGrantAccess::invalid;
}
std::expected<void, int> MatchReaderGrants::revoke(std::string_view principal) {
    if (!valid_principal(principal)) return std::unexpected{400};
    std::lock_guard lock{impl_->mutex};
    auto entry = impl_->principals.find(principal);
    if (entry == impl_->principals.end()) {
        if (impl_->principals.size() >= 1024) return std::unexpected{503};
        entry = impl_->principals.try_emplace(std::string{principal}, Clock::now()).first;
    }
    entry->second.revoked = true;
    entry->second.grants.clear();
    // Fail closed in memory even if persistence fails; caller gets 503 and may
    // retry this idempotent operation. Never acknowledge a non-durable revocation.
    if (!impl_->save()) return std::unexpected{503};
    return {};
}
} // namespace gloom::backends
