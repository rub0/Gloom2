#pragma once
#include <gloom/gameplay/match_discovery.hpp>
#include <gloom/backends/match_reader_grants.hpp>
#include <gloom/backends/game_ticket.hpp>
#include <memory>

namespace gloom::backends {
class MatchHttpsHost final {
public:
    MatchHttpsHost(std::shared_ptr<gameplay::SliceMatchService> service,
                   std::string certificate_file, std::string private_key_file,
                   MatchIdentityVerifier identity_verifier, std::string publisher_token,
                   std::filesystem::path revocations,
                   std::shared_ptr<GameTicketIssuer> tickets = {}, MatchIdentityVerifier game_verifier = {});
    ~MatchHttpsHost();
    MatchHttpsHost(const MatchHttpsHost&) = delete;
    MatchHttpsHost& operator=(const MatchHttpsHost&) = delete;
    // Port zero requests an ephemeral port. A failed bind throws.
    [[nodiscard]] int bind(std::string_view address, int port);
    // Blocks until stop; active requests drain before returning.
    [[nodiscard]] bool run();
    void stop();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace gloom::backends
