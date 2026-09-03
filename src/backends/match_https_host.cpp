#include <gloom/backends/match_https_host.hpp>
#include <gloom/backends/match_request_budget.hpp>
#include <gloom/gameplay/match_service_wire.hpp>
#include <httplib.h>
#include <openssl/crypto.h>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace gloom::backends {
namespace {
std::uint64_t utc_ms() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}
}

struct MatchHttpsHost::Impl {
    std::shared_ptr<gameplay::SliceMatchService> service;
    httplib::SSLServer server;
    std::string publisher_authorization;
    MatchReaderGrants grants;
    std::shared_ptr<GameTicketIssuer> tickets;
    MatchIdentityVerifier game_verifier;
    MatchRequestBudget readers{60, 20}, publishers{30, 10}, rejected{20, 2};
    MatchRequestBudget exchanges{20, 2};
    std::atomic_bool draining{false};
    std::atomic_bool storage_ok{true};
    std::atomic_bool revocations_ok{true};

    Impl(std::shared_ptr<gameplay::SliceMatchService> store, const std::string& cert,
         const std::string& key, MatchIdentityVerifier verifier, std::string publisher,
         std::filesystem::path revocations, std::shared_ptr<GameTicketIssuer> ticket_issuer,
         MatchIdentityVerifier game_identity)
        : service{std::move(store)}, server{cert.c_str(), key.c_str()},
          publisher_authorization{"Bearer " + publisher}, grants{std::move(verifier), std::move(revocations)},
          tickets{std::move(ticket_issuer)}, game_verifier{std::move(game_identity)} {
        if (static_cast<bool>(tickets) != static_cast<bool>(game_verifier))
            throw std::invalid_argument{"Game ticket issuer and verifier must be configured together"};
        if (!service || !server.is_valid() || !valid_match_credential(publisher))
            throw std::invalid_argument{"Invalid HTTPS service, TLS files or publisher token (16-512 characters required)"};
        server.new_task_queue = [] { return new httplib::ThreadPool(4, 4, 32); };
        server.set_payload_max_length(gameplay::maximum_match_record_bytes);
        server.set_read_timeout(3, 0);
        server.set_write_timeout(3, 0);
        server.set_keep_alive_max_count(10);
        server.set_pre_routing_handler([this](const auto& request, auto& response) {
            response.set_header("Cache-Control", "no-store");
            response.set_header("X-Gloom-Match-Schema", "1");
            const auto supplied = request.get_header_value("Authorization");
            const auto matches = [&](const std::string& expected) {
                return supplied.size() == expected.size() &&
                    CRYPTO_memcmp(supplied.data(), expected.data(), expected.size()) == 0;
            };
            const bool single = request.get_header_value_count("Authorization") == 1;
            const bool publisher = matches(publisher_authorization) && single;
            const auto throttle = [&] {
                response.status = 429;
                response.set_header("Retry-After", "1");
            };
            if (draining) {
                response.status = 503;
                return httplib::Server::HandlerResponse::Handled;
            }
            const bool bearer = single && supplied.starts_with("Bearer ");
            if (request.method == "POST" && (request.path == "/reader-grants/exchange" || request.path.starts_with("/game-tickets/"))) {
                if (!exchanges.consume()) { throttle(); return httplib::Server::HandlerResponse::Handled; }
                if (!bearer || publisher) { response.status = 401; return httplib::Server::HandlerResponse::Handled; }
                // cpp-httplib has not read the request body at this stage.
                return httplib::Server::HandlerResponse::Unhandled;
            }
            const auto access = bearer && !publisher ? grants.consume(std::string_view{supplied}.substr(7)) :
                MatchGrantAccess::invalid;
            if (access == MatchGrantAccess::throttled) {
                throttle(); return httplib::Server::HandlerResponse::Handled;
            }
            const bool reader = access == MatchGrantAccess::allowed;
            auto& budget = publisher ? publishers : (reader ? readers : rejected);
            if (!budget.consume()) {
                response.status = 429;
                response.set_header("Retry-After", "1");
                return httplib::Server::HandlerResponse::Handled;
            }
            if (!reader && !publisher) {
                response.status = 401;
                response.set_header("WWW-Authenticate", "Bearer");
                return httplib::Server::HandlerResponse::Handled;
            }
            const bool discovery_read = (request.method == "GET" || request.method == "HEAD") &&
                (request.path == "/health" || request.path == "/matches" || request.path.starts_with("/matches/"));
            if (!publisher && !discovery_read) {
                response.status = 403;
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });
        server.Post(R"(/game-tickets/(.+))", [this](const auto& request, auto& response) {
            if (!tickets) { response.status = 503; return; }
            if (!request.body.empty() || !request.params.empty()) { response.status = 400; return; }
            const auto authorization = request.get_header_value("Authorization");
            const auto credential = std::string_view{authorization}.substr(7);
            if (!valid_match_identity_credential(credential)) { response.status = 401; return; }
            const auto now = utc_ms();
            std::expected<MatchVerifiedPrincipal, std::string> identity = std::unexpected{"Rejected"};
            try { identity = game_verifier(credential, now); } catch (...) { response.status = 503; return; }
            if (!identity) { response.status = 401; return; }
            const auto allowed = grants.authorize_game_identity(*identity, now);
            if (!allowed) {
                response.status = allowed.error();
                if (response.status == 429) response.set_header("Retry-After", "1");
                return;
            }
            const auto match = service->get(request.matches[1].str());
            if (!match || match->expires_at_ms <= now ||
                match->advertisement.protocol_version != network::protocol_version ||
                match->advertisement.phase == gameplay::SliceMatchPhase::completed) {
                response.status = 404; return;
            }
            // Active/full matches still issue tickets: only the game host can authorize a resume.
            auto bounded = *identity;
            bounded.expires_at_ms = std::min(bounded.expires_at_ms, match->expires_at_ms);
            response.set_content(match->advertisement.endpoint + '\t' + match->advertisement.instance_id + '\t' +
                tickets->issue(bounded, match->advertisement.instance_id, now), "text/plain");
        });
        server.Post("/reader-grants/exchange", [this](const auto& request, auto& response) {
            if (!request.body.empty() || !request.params.empty()) { response.status = 400; return; }
            const auto supplied = request.get_header_value("Authorization");
            auto grant = grants.exchange(std::string_view{supplied}.substr(7), utc_ms());
            if (!grant) {
                response.status = grant.error();
                if (response.status == 401) response.set_header("WWW-Authenticate", "Bearer");
                if (response.status == 429) response.set_header("Retry-After", "1");
            } else {
                response.set_content(grant->token + '\t' + std::to_string(grant->lifetime_ms), "text/plain");
            }
        });
        server.Delete(R"(/reader-grants/(.+))", [this](const auto& request, auto& response) {
            const auto result = grants.revoke(request.matches[1].str());
            if (result || result.error() == 503) revocations_ok = result.has_value();
            response.status = result ? 204 : result.error();
        });
        server.Get("/health", [](const auto&, auto& response) {
            response.set_content("alive", "text/plain");
        });
        server.Get("/ready", [this](const auto&, auto& response) {
            const bool ready = storage_ok && revocations_ok;
            response.status = ready ? 200 : 503;
            response.set_content(ready ? "ready" : "storage unavailable", "text/plain");
        });
        server.Get("/matches", [this](const auto&, auto& response) {
            const auto items = service->list();
            if (!items) { response.status = 503; return; }
            std::string body;
            const auto now = utc_ms();
            for (const auto& item : *items) {
                if (item.expires_at_ms > now) body += gameplay::encode_stored_match(item) + '\n';
            }
            response.set_content(std::move(body), "text/plain; charset=utf-8");
        });
        server.Get(R"(/matches/(.+))", [this](const auto& request, auto& response) {
            const auto item = service->get(request.matches[1].str());
            if (!item || item->expires_at_ms <= utc_ms()) { response.status = 404; return; }
            response.set_content(gameplay::encode_stored_match(*item), "text/plain; charset=utf-8");
        });
        server.Put(R"(/matches/(.+))", [this](const auto& request, auto& response) {
            auto item = gameplay::decode_stored_match(request.body);
            if (!item || item->advertisement.match_id != request.matches[1].str()) {
                response.status = 400; return;
            }
            const auto now = utc_ms(); // Never trust now_ms supplied by the publisher for ownership.
            if (item->expires_at_ms <= now || item->expires_at_ms - now > 60'000) {
                response.status = 400; return;
            }
            auto result = service->store(std::move(*item), now);
            if (!result) {
                const auto& error = result.error();
                const bool conflict = error == "Mutation id payload changed" ||
                    error == "Match advertisement is owned by another instance" ||
                    error == "Match advertisement revision is stale" ||
                    error == "Match store capacity reached";
                response.status = conflict ? 409 : 503;
                if (!conflict) storage_ok = false;
                return;
            }
            storage_ok = true;
            response.status = 204;
        });
        server.Delete(R"(/matches/(.+))", [this](const auto& request, auto& response) {
            if (request.get_param_value_count("instance_id") != 1) { response.status = 400; return; }
            auto result = service->erase(request.matches[1].str(), request.get_param_value("instance_id"));
            if (!result) { storage_ok = false; response.status = 503; return; }
            storage_ok = true;
            response.status = *result ? 204 : 404;
        });
        server.set_exception_handler([](const auto&, auto& response, std::exception_ptr) {
            response.status = 500; // No exception text, paths or credentials over the wire.
        });
    }
};

MatchHttpsHost::MatchHttpsHost(std::shared_ptr<gameplay::SliceMatchService> service,
                             std::string cert, std::string key, MatchIdentityVerifier verifier, std::string publisher,
                             std::filesystem::path revocations, std::shared_ptr<GameTicketIssuer> tickets,
                             MatchIdentityVerifier game_verifier)
    : impl_{std::make_unique<Impl>(std::move(service), cert, key, std::move(verifier), std::move(publisher),
                                  std::move(revocations), std::move(tickets), std::move(game_verifier))} {}
MatchHttpsHost::~MatchHttpsHost() = default;
int MatchHttpsHost::bind(const std::string_view address, const int port) {
    const std::string host{address};
    if (port == 0) {
        const auto bound = impl_->server.bind_to_any_port(host);
        if (bound <= 0) throw std::runtime_error{"Could not bind match HTTPS listener"};
        return bound;
    }
    if (port < 1 || port > 65535 || !impl_->server.bind_to_port(host, port))
        throw std::runtime_error{"Could not bind match HTTPS listener"};
    return port;
}
bool MatchHttpsHost::run() { return impl_->server.listen_after_bind(); }
void MatchHttpsHost::stop() { impl_->draining = true; impl_->server.stop(); }
} // namespace gloom::backends
