#include <gloom/gameplay/match_discovery.hpp>
#include <gloom/gameplay/match_service_wire.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>

namespace {

class FakeMatchService final : public gloom::gameplay::SliceMatchService {
public:
    std::expected<void, std::string>
    store(gloom::gameplay::SliceStoredMatch match, const std::uint64_t now_ms) override {
        const auto found = records.find(match.advertisement.match_id);
        if (found != records.end() && found->second.expires_at_ms > now_ms) {
            if (found->second.advertisement.instance_id != match.advertisement.instance_id) {
                return std::unexpected{"owned"};
            }
            if (found->second.advertisement.revision >= match.advertisement.revision) {
                return std::unexpected{"stale"};
            }
        }
        records.insert_or_assign(match.advertisement.match_id, std::move(match));
        return {};
    }

    std::expected<bool, std::string>
    erase(const std::string_view match_id, const std::string_view instance_id) override {
        const auto found = records.find(std::string{match_id});
        if (found == records.end() || found->second.advertisement.instance_id != instance_id) {
            return false;
        }
        records.erase(found);
        return true;
    }

    std::expected<std::vector<gloom::gameplay::SliceStoredMatch>, std::string>
    list() const override {
        std::vector<gloom::gameplay::SliceStoredMatch> result;
        for (const auto& [id, record] : records) {
            static_cast<void>(id);
            result.push_back(record);
        }
        return result;
    }

    std::expected<gloom::gameplay::SliceStoredMatch, std::string>
    get(const std::string_view match_id) const override {
        const auto found = records.find(std::string{match_id});
        if (found == records.end()) {
            return std::unexpected{"not found"};
        }
        return found->second;
    }

    std::map<std::string, gloom::gameplay::SliceStoredMatch, std::less<>> records;
};

class FakeServiceConnector final : public gloom::gameplay::SliceMatchServiceConnector {
public:
    explicit FakeServiceConnector(std::shared_ptr<gloom::gameplay::SliceMatchService> service)
        : service_{std::move(service)} {}

    std::expected<std::shared_ptr<gloom::gameplay::SliceMatchService>, std::string>
    connect(const std::string_view service_url,
            const std::string_view bearer_token) override {
        ++attempts;
        observed_url = service_url;
        observed_token = bearer_token;
        if (attempts <= failures_before_success) {
            return std::unexpected{"temporary outage"};
        }
        return service_;
    }

    std::shared_ptr<gloom::gameplay::SliceMatchService> service_;
    std::size_t failures_before_success{2};
    std::size_t attempts{0};
    std::string observed_url;
    std::string observed_token;
};

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

gloom::gameplay::SliceMatchAdvertisement match(
    std::string id, std::string endpoint, const std::uint64_t revision = 1) {
    return {.match_id = std::move(id),
            .instance_id = "server-a",
            .display_name = "Factory duel",
            .endpoint = std::move(endpoint),
            .revision = revision};
}

} // namespace

int main() {
    using namespace gloom::gameplay;

    auto directory = make_in_memory_match_directory();
    expect(directory->advertise(match("bravo", "127.0.0.1:27021"), 100, 1'000).has_value(),
           "Valid match was rejected");
    expect(directory->advertise(match("alpha", "127.0.0.1:27020"), 100, 1'000).has_value(),
           "Second valid match was rejected");

    const auto listed = directory->list_joinable(100);
    expect(listed.has_value() && listed->size() == 2, "Joinable matches were not listed");
    expect((*listed)[0].match_id == "alpha" && (*listed)[1].match_id == "bravo",
           "In-memory listing is not deterministic");

    const auto resolved = directory->resolve("alpha", 100);
    expect(resolved.has_value() && resolved->endpoint == "127.0.0.1:27020",
           "Match did not resolve to its direct-IP endpoint");
    expect(!directory->resolve("missing", 100), "Missing match resolved");
    expect(!directory->resolve("alpha", 100, gloom::network::protocol_version + 1),
           "Protocol mismatch resolved");

    expect(!directory->advertise(match("alpha", "127.0.0.1:30000"), 101, 1'000),
           "Stale advertisement replaced current state");
    auto full = match("alpha", "127.0.0.1:27020", 2);
    full.player_count = full.capacity;
    expect(directory->advertise(full, 101, 1'000).has_value(), "Fresh full-match update failed");
    expect(!directory->resolve("alpha", 101), "Full match resolved");
    expect(directory->list_joinable(101)->size() == 1, "Full match remained discoverable");

    auto active = match("bravo", "127.0.0.1:27021", 2);
    active.phase = SliceMatchPhase::active;
    expect(directory->advertise(active, 101, 1'000).has_value(), "Active-match update failed");
    expect(directory->list_joinable(101)->empty(), "Active match remained discoverable");

    expect(!directory->advertise(match("bad", "no-port"), 100, 1'000),
           "Malformed endpoint was advertised");
    expect(!directory->advertise(match("bad", "127.0.0.1:70000"), 100, 1'000),
           "Out-of-range endpoint port was advertised");
    expect(!directory->advertise(match("bad id\n", "127.0.0.1:1"), 100, 1'000),
           "Control character in id was advertised");

    SliceLobbyState lobby;
    lobby.phase = SliceMatchPhase::waiting;
    lobby.players.push_back({.connected = true});
    lobby.players.push_back({.connected = false});
    const auto derived = make_slice_match_advertisement(
        "derived", "server-a", "Derived duel", "[::1]:27022", lobby, 2, 7);
    expect(derived.player_count == 1 && derived.capacity == 2 &&
               derived.phase == SliceMatchPhase::waiting && derived.revision == 7,
           "Lobby advertisement did not derive authoritative occupancy and phase");
    expect(directory->advertise(derived, 100, 1'000).has_value(), "IPv6 match was rejected");

    const auto rejected_withdrawal = directory->withdraw("derived", "server-b");
    expect(rejected_withdrawal && !*rejected_withdrawal,
           "Non-owner withdrew another instance's match");
    const auto accepted_withdrawal = directory->withdraw("derived", "server-a");
    expect(accepted_withdrawal && *accepted_withdrawal, "Owner could not withdraw match");
    expect(!directory->resolve("derived", 100), "Withdrawn match still resolved");

    auto owned = match("owned", "127.0.0.1:27023");
    expect(directory->advertise(owned, 1'000, 100).has_value(), "Owned match failed");
    auto replacement = owned;
    replacement.instance_id = "server-b";
    expect(!directory->advertise(replacement, 1'050, 100),
           "Live lease was stolen by another instance");
    expect(directory->advertise(replacement, 1'100, 100).has_value(),
           "Expired match id was not reclaimed");
    const auto old_withdrawal = directory->withdraw("owned", "server-a");
    expect(old_withdrawal && !*old_withdrawal,
           "Old instance withdrew its replacement");
    expect(directory->resolve("owned", 1'150).has_value(), "Replacement did not resolve");
    expect(!directory->resolve("owned", 1'200), "Expired lease still resolved");

    auto lifecycle_directory = make_in_memory_match_directory();
    SliceMatchPublication publication{
        *lifecycle_directory,
        {.match_id = "lifecycle",
         .instance_id = "process-42",
         .display_name = "Lifecycle duel",
         .endpoint = "127.0.0.1:27024",
         .lease_duration_ms = 300,
         .refresh_interval_ms = 100}};
    expect(publication.start(lobby, 10).has_value() && publication.active(),
           "Publication did not start with the server");
    expect(lifecycle_directory->resolve("lifecycle", 250).has_value(),
           "Initial publication lease was absent");
    expect(publication.update(lobby, 90).has_value(), "No-op lifecycle update failed");
    expect(publication.update(lobby, 110).has_value(), "Lease refresh failed");
    expect(lifecycle_directory->resolve("lifecycle", 400).has_value(),
           "Refreshed lease expired too early");
    lobby.revision = 2;
    lobby.phase = SliceMatchPhase::active;
    expect(publication.update(lobby, 120).has_value(), "Lobby transition was not published");
    expect(!lifecycle_directory->resolve("lifecycle", 120),
           "Active lifecycle remained joinable");
    expect(publication.stop() && !publication.active(),
           "Orderly publication withdrawal failed");
    expect(lifecycle_directory->list_joinable(120)->empty(),
           "Stopped publication remained listed");

    auto service = std::make_shared<FakeMatchService>();
    auto service_directory = make_service_match_directory(service);
    expect(service_directory->advertise(match("zulu", "10.0.0.2:27020"), 500, 1'000).has_value(),
           "Service adapter did not persist advertisement");
    expect(service_directory->advertise(match("echo", "10.0.0.1:27020"), 500, 1'000).has_value(),
           "Service adapter did not persist second advertisement");
    SliceMatchSelection selection;
    expect(selection.refresh(*service_directory, 500).has_value() &&
               selection.matches().size() == 2 &&
               selection.selected()->match_id == "echo",
           "Client browser did not load deterministic service results");
    expect(selection.select(1) && selection.selected()->match_id == "zulu",
           "Client browser could not select a match");
    const auto selected_join = selection.resolve(*service_directory, 500);
    expect(selected_join && selected_join->endpoint == "10.0.0.2:27020",
           "Selected service match did not resolve");
    const auto named_join = resolve_slice_join_target(
        "match:echo", service_directory.get(), 500);
    expect(named_join && named_join->endpoint == "10.0.0.1:27020",
           "Named match join did not reach service adapter");
    const auto direct_join = resolve_slice_join_target("127.0.0.1:27020", nullptr, 500);
    expect(direct_join && direct_join->match_id.empty() &&
               direct_join->endpoint == "127.0.0.1:27020",
           "Direct-IP fallback changed");
    expect(!resolve_slice_join_target("match:echo", nullptr, 500),
           "Match join succeeded without a configured directory");
    expect(!service_directory->resolve("echo", 1'500),
           "Service adapter returned an expired record");

    FakeServiceConnector connector{service};
    std::vector<std::uint64_t> waits;
    auto connected_directory = connect_match_directory(
        connector,
        {.service_url = "https://match.example.test/v1",
         .bearer_token = "opaque-service-token",
         .maximum_attempts = 4,
         .initial_backoff_ms = 20,
         .maximum_backoff_ms = 50,
         .wait = [&waits](const std::uint64_t delay) { waits.push_back(delay); }});
    expect(connected_directory.has_value() && connector.attempts == 3 &&
               connector.observed_url == "https://match.example.test/v1" &&
               connector.observed_token == "opaque-service-token",
           "Authenticated service connection did not recover");
    expect(waits.size() == 2 && waits[0] == 20 && waits[1] == 40,
           "Service connection backoff was not bounded exponential");
    expect((*connected_directory)->list_joinable(500)->size() == 2,
           "Connected directory did not expose persistent records");

    FakeServiceConnector unavailable{service};
    unavailable.failures_before_success = 10;
    waits.clear();
    const auto failed_connection = connect_match_directory(
        unavailable,
        {.service_url = "https://match.example.test/v1",
         .bearer_token = "opaque-service-token",
         .maximum_attempts = 3,
         .initial_backoff_ms = 30,
         .maximum_backoff_ms = 50,
         .wait = [&waits](const std::uint64_t delay) { waits.push_back(delay); }});
    expect(!failed_connection && unavailable.attempts == 3 &&
               waits.size() == 2 && waits[0] == 30 && waits[1] == 50,
           "Unavailable service did not stop after bounded retries");
    expect(!connect_match_directory(
                connector, {.service_url = "https://match.example.test/v1"}),
           "Missing service credential was accepted");

    auto core = make_match_service_core();
    auto core_directory = make_service_match_directory(core);
    const auto idempotent = match("idempotent", "127.0.0.1:27030");
    expect(core_directory->advertise(idempotent, 10, 100).has_value() &&
               core_directory->advertise(idempotent, 10, 100).has_value(),
           "Service core did not accept an identical mutation replay");
    auto changed_replay = idempotent;
    changed_replay.endpoint = "127.0.0.1:27031";
    expect(!core_directory->advertise(changed_replay, 10, 100),
           "Service core accepted changed payload for an idempotency key");

    FakeServiceConnector rotating_connector{service};
    rotating_connector.failures_before_success = 1;
    std::vector<bool> refresh_requests;
    const auto rotating_directory = connect_match_directory(
        rotating_connector,
        {.service_url = "https://match.example.test/v1",
         .maximum_attempts = 2,
         .initial_backoff_ms = 1,
         .maximum_backoff_ms = 1,
         .wait = [](std::uint64_t) {},
         .acquire_bearer_token = [&refresh_requests](const bool refresh) {
             refresh_requests.push_back(refresh);
             return std::string{refresh ? "rotated-token" : "initial-token"};
         }});
    expect(rotating_directory && refresh_requests.size() == 2 &&
               !refresh_requests[0] && refresh_requests[1] &&
               rotating_connector.observed_token == "rotated-token",
           "Credential provider did not rotate the token on reconnect");

    AsyncSliceMatchBrowser async_browser;
    expect(async_browser.begin_refresh(*service_directory, 500) &&
               !async_browser.begin_refresh(*service_directory, 500),
           "Async browser allowed overlapping refreshes");
    while (!async_browser.poll()) std::this_thread::yield();
    expect(!async_browser.refreshing() && async_browser.error().empty() &&
               async_browser.matches().size() == 2,
           "Async browser did not publish its completed listing");

    const auto durable_path = std::filesystem::temp_directory_path() /
        ("gloom-match-discovery-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".store");
    std::error_code cleanup_error;
    std::filesystem::remove(durable_path, cleanup_error);
    auto durable = make_durable_match_service(durable_path);
    expect(durable.has_value(), "Durable service did not open");
    expect(!make_durable_match_service(durable_path), "Concurrent store owner was accepted");
    auto durable_directory = make_service_match_directory(*durable);
    expect(durable_directory->advertise(
               match("persistent", "127.0.0.1:27040"), 1'000, 10'000).has_value(),
           "Durable service did not store a match");
    durable_directory.reset();
    durable->reset();
    auto reopened = make_durable_match_service(durable_path);
    expect(reopened.has_value(), "Durable service did not reopen");
    auto reopened_directory = make_service_match_directory(*reopened);
    const auto persisted = reopened_directory->resolve("persistent", 1'001);
    expect(persisted && persisted->endpoint == "127.0.0.1:27040",
           "Durable service lost its match across restart");
    const auto record_before = (*reopened)->get("persistent");
    const auto encoded = encode_stored_match(*record_before);
    expect(decode_stored_match(encoded) == record_before, "Shared codec failed round trip");
    expect(!decode_stored_match(encoded + "\textra") &&
               !decode_stored_match(encoded + "\n") &&
               !decode_stored_match(std::string(4096, 'x')),
           "Shared codec accepted malformed or oversized data");
    auto temporary_path = durable_path;
    temporary_path += ".tmp";
    std::filesystem::create_directory(temporary_path); // Deliberately block the pending-write file.
    auto update = record_before->advertisement;
    update.revision += 1;
    update.display_name = "This write must fail";
    expect(!reopened_directory->advertise(update, 1'002, 10'000),
           "Blocked disk write was reported successful");
    expect((*reopened)->get("persistent") == record_before,
           "Failed durable write changed in-memory state");
    std::filesystem::remove(temporary_path);
    reopened_directory.reset();
    reopened->reset();
    auto intact = make_durable_match_service(durable_path);
    expect(intact && (*intact)->get("persistent") == record_before,
           "Failed durable write changed committed file");
    intact->reset();
    {
        std::ofstream corrupt{durable_path, std::ios::trunc};
        corrupt << "gloom-match-store-v1\nmalformed-record\n";
    }
    expect(!make_durable_match_service(durable_path), "Corrupt store was silently accepted");
    std::filesystem::remove(durable_path, cleanup_error);
    auto lock_path = durable_path;
    lock_path += ".lock";
    std::filesystem::remove(lock_path, cleanup_error);

    std::cout << "Match discovery tests passed\n";
    return EXIT_SUCCESS;
}
