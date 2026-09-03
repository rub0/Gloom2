#pragma once
#include <gloom/backends/match_reader_grants.hpp>
#include <gloom/gameplay/match_lobby.hpp>

namespace gloom::backends {
// Ed25519 authority signs a short-lived capability for one server incarnation.
class GameTicketIssuer final {
public:
    explicit GameTicketIssuer(const std::filesystem::path& private_key);
    ~GameTicketIssuer();
    [[nodiscard]] std::string issue(const MatchVerifiedPrincipal& identity,
        std::string_view instance, std::uint64_t now_ms) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
[[nodiscard]] std::shared_ptr<const gameplay::SliceIdentityProvider> make_game_ticket_verifier(
    const std::filesystem::path& public_key, std::string instance,
    std::function<std::uint64_t()> now_ms);
[[nodiscard]] std::uint64_t secure_session_token();
[[nodiscard]] std::string secure_server_instance();
} // namespace gloom::backends
