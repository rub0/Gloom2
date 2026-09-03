#include <gloom/backends/game_ticket.hpp>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <mutex>
#include <stdexcept>

namespace gloom::backends {
namespace {
using Key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
Key load_key(const std::filesystem::path& path, bool secret) {
    std::unique_ptr<BIO, decltype(&BIO_free)> input{BIO_new_file(path.string().c_str(), "r"), BIO_free};
    Key key{input ? (secret ? PEM_read_bio_PrivateKey(input.get(), nullptr, [](char*, int, int, void*) { return 0; }, nullptr) :
                               PEM_read_bio_PUBKEY(input.get(), nullptr, nullptr, nullptr)) : nullptr, EVP_PKEY_free};
    if (!key || !EVP_PKEY_is_a(key.get(), "ED25519")) throw std::invalid_argument{"Ed25519 game ticket key required"};
    return key;
}
std::string hex(const unsigned char* data, std::size_t count) {
    constexpr auto digits = "0123456789abcdef";
    std::string out;
    for (std::size_t i = 0; i < count; ++i) { out += digits[data[i] >> 4U]; out += digits[data[i] & 15U]; }
    return out;
}
bool identifier(std::string_view text, std::size_t max) {
    return !text.empty() && text.size() <= max && std::ranges::all_of(text, [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    });
}
bool is_hex(std::string_view text) {
    return std::ranges::all_of(text, [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}
std::uint64_t integer(std::string_view text, int base = 10) {
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) throw std::runtime_error{"Invalid ticket number"};
    return value;
}
class Verifier final : public gameplay::SliceIdentityProvider {
public:
    Verifier(const std::filesystem::path& path, std::string instance, std::function<std::uint64_t()> now)
        : key_{load_key(path, false)}, instance_{std::move(instance)}, now_{std::move(now)} {
        if (!identifier(instance_, 64) || !now_) throw std::invalid_argument{"Invalid ticket verifier configuration"};
    }
    std::expected<gameplay::SlicePlayerIdentity, std::string> verify(std::string_view ticket) const override try {
        if (ticket.size() > 512 || !ticket.starts_with("gt1.")) return std::unexpected{"Game ticket rejected"};
        std::array<std::string_view, 7> fields;
        std::size_t offset = 0;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            const auto end = ticket.find('.', offset);
            if ((i + 1 < fields.size()) == (end == std::string_view::npos)) return std::unexpected{"Game ticket rejected"};
            fields[i] = ticket.substr(offset, end == std::string_view::npos ? ticket.size() - offset : end - offset);
            offset = end + 1;
        }
        // gt1.principal.instance.issued.expires.nonce.signature
        if (fields[1].size() != 64 || !is_hex(fields[1]) || fields[2] != instance_ ||
            fields[5].size() != 32 || !is_hex(fields[5]) || fields[6].size() != 128 || !is_hex(fields[6]))
            return std::unexpected{"Game ticket rejected"};
        std::array<unsigned char, 64> signature{};
        for (std::size_t i = 0; i < signature.size(); ++i)
            signature[i] = static_cast<unsigned char>(integer(fields[6].substr(i * 2, 2), 16));
        const auto body = ticket.substr(0, ticket.size() - 129);
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{EVP_MD_CTX_new(), EVP_MD_CTX_free};
        if (!context || EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, key_.get()) != 1 ||
            EVP_DigestVerify(context.get(), signature.data(), signature.size(),
                reinterpret_cast<const unsigned char*>(body.data()), body.size()) != 1)
            return std::unexpected{"Game ticket rejected"};
        const auto now = now_(), issued = integer(fields[3]), expiry = integer(fields[4]);
        if (issued > now || expiry <= now || expiry <= issued || expiry - issued > 30'000)
            return std::unexpected{"Game ticket expired or invalid"};
        // Retain the full principal binding despite the existing compact lobby id.
        auto account = integer(fields[1].substr(0, 16), 16);
        if (!account) account = 1;
        std::lock_guard lock{mutex_};
        std::erase_if(used_, [&](const auto& record) { return record.second <= now; });
        if (used_.contains(std::string{fields[5]}) || used_.size() >= 4096)
            return std::unexpected{"Game ticket replay or capacity limit"};
        const auto existing = principals_.find(account);
        if ((existing != principals_.end() && existing->second != fields[1]) ||
            (existing == principals_.end() && principals_.size() >= 1024))
            return std::unexpected{"Game identity collision or capacity limit"};
        principals_.try_emplace(account, fields[1]);
        used_.emplace(fields[5], expiry);
        return gameplay::SlicePlayerIdentity{account, "P_" + std::string{fields[1].substr(0, 16)}};
    } catch (...) { return std::unexpected{"Game ticket rejected"}; }
private:
    Key key_;
    std::string instance_;
    std::function<std::uint64_t()> now_;
    mutable std::mutex mutex_;
    mutable std::map<std::uint64_t, std::string> principals_;
    mutable std::map<std::string, std::uint64_t> used_;
};
}
struct GameTicketIssuer::Impl { Key key{nullptr, EVP_PKEY_free}; };
GameTicketIssuer::GameTicketIssuer(const std::filesystem::path& key) : impl_{std::make_unique<Impl>()} {
    impl_->key = load_key(key, true);
}
GameTicketIssuer::~GameTicketIssuer() = default;
std::string GameTicketIssuer::issue(const MatchVerifiedPrincipal& identity, std::string_view instance, std::uint64_t now) const {
    if (!identifier(identity.id, 64) || !identifier(instance, 64) || identity.expires_at_ms <= now)
        throw std::invalid_argument{"Invalid game ticket identity or target"};
    // Normalize registry identities too; Keycloak already supplies its stable digest.
    const auto principal = identity.id.size() == 64 && is_hex(identity.id) ? identity.id : match_credential_digest(identity.id);
    const auto expiry = now + std::min<std::uint64_t>(30'000, identity.expires_at_ms - now);
    std::array<unsigned char, 16> nonce{};
    if (RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1) throw std::runtime_error{"Ticket randomness failed"};
    const auto body = "gt1." + principal + '.' + std::string{instance} + '.' + std::to_string(now) + '.' +
        std::to_string(expiry) + '.' + hex(nonce.data(), nonce.size());
    std::array<unsigned char, 64> signature{};
    std::size_t count = signature.size();
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{EVP_MD_CTX_new(), EVP_MD_CTX_free};
    if (!context || EVP_DigestSignInit(context.get(), nullptr, nullptr, nullptr, impl_->key.get()) != 1 ||
        EVP_DigestSign(context.get(), signature.data(), &count,
            reinterpret_cast<const unsigned char*>(body.data()), body.size()) != 1 || count != signature.size())
        throw std::runtime_error{"Ticket signing failed"};
    return body + '.' + hex(signature.data(), count);
}
std::shared_ptr<const gameplay::SliceIdentityProvider> make_game_ticket_verifier(
    const std::filesystem::path& key, std::string instance, std::function<std::uint64_t()> now) {
    return std::make_shared<Verifier>(key, std::move(instance), std::move(now));
}
std::uint64_t secure_session_token() {
    std::uint64_t value = 0;
    do {
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1)
            throw std::runtime_error{"Session randomness failed"};
    } while (!value);
    return value;
}
std::string secure_server_instance() {
    std::array<unsigned char, 16> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) throw std::runtime_error{"Instance randomness failed"};
    return hex(bytes.data(), bytes.size());
}
} // namespace gloom::backends
