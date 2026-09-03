#pragma once

#include <gloom/gameplay/match_discovery.hpp>

#include <memory>

namespace gloom::backends {

struct GameJoinTicket { std::string endpoint; std::string instance; std::string credential; };
[[nodiscard]] std::expected<GameJoinTicket, std::string> request_game_ticket(
    std::string_view service_url, std::string_view identity, std::string_view match_id);

// Concrete HTTPS connector for the Gloom match-service REST contract.
[[nodiscard]] std::unique_ptr<gameplay::SliceMatchServiceConnector>
make_winhttp_match_service_connector();

// Exchanges a trusted individual identity credential for short-lived discovery
// grants, renewing before subsequent reads. Publisher connector stays static.
[[nodiscard]] std::unique_ptr<gameplay::SliceMatchServiceConnector>
make_winhttp_match_reader_connector(std::function<std::expected<std::string, std::string>()> acquire_identity = {});

} // namespace gloom::backends
