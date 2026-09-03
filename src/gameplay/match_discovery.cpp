#include <gloom/gameplay/match_discovery.hpp>
#include <gloom/gameplay/match_service_wire.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cctype>
#include <map>
#include <limits>
#include <mutex>
#include <utility>

namespace gloom::gameplay {
namespace {

[[nodiscard]] bool valid_text(const std::string_view value, const std::size_t limit) {
    if (value.empty() || value.size() > limit) {
        return false;
    }
    return std::ranges::all_of(value, [](const unsigned char character) {
        return character >= 0x20U && character != 0x7fU;
    });
}

[[nodiscard]] bool valid_endpoint(const std::string_view endpoint) {
    if (!valid_text(endpoint, 255)) {
        return false;
    }
    if (std::ranges::any_of(endpoint, [](const unsigned char character) {
            return std::isspace(character) != 0;
        })) {
        return false;
    }

    const auto separator = endpoint.rfind(':');
    if (separator == std::string_view::npos || separator == 0 ||
        separator + 1 >= endpoint.size()) {
        return false;
    }
    const auto port = endpoint.substr(separator + 1);
    if (!std::ranges::all_of(port, [](const unsigned char character) {
            return std::isdigit(character) != 0;
        })) {
        return false;
    }
    std::uint32_t port_number = 0;
    const auto [end, error] = std::from_chars(port.data(), port.data() + port.size(), port_number);
    return error == std::errc{} && end == port.data() + port.size() &&
           port_number > 0 && port_number <= 65'535;
}

[[nodiscard]] std::expected<void, std::string>
validate(const SliceMatchAdvertisement& advertisement) {
    if (!valid_text(advertisement.match_id, 96)) {
        return std::unexpected{"Match id must contain 1-96 printable characters"};
    }
    if (!valid_text(advertisement.instance_id, 96)) {
        return std::unexpected{"Match instance id must contain 1-96 printable characters"};
    }
    if (!valid_text(advertisement.display_name, 96)) {
        return std::unexpected{"Match display name must contain 1-96 printable characters"};
    }
    if (!valid_endpoint(advertisement.endpoint)) {
        return std::unexpected{"Match endpoint must be a bounded host:port value"};
    }
    if (advertisement.capacity == 0 || advertisement.player_count > advertisement.capacity) {
        return std::unexpected{"Match player counts are invalid"};
    }
    if (advertisement.protocol_version == 0 || advertisement.revision == 0) {
        return std::unexpected{"Match protocol and revision must be non-zero"};
    }
    return {};
}

[[nodiscard]] bool joinable(const SliceMatchAdvertisement& advertisement,
                            const std::uint16_t client_protocol) {
    return advertisement.protocol_version == client_protocol &&
           advertisement.phase == SliceMatchPhase::waiting &&
           advertisement.player_count < advertisement.capacity;
}

class InMemoryMatchDirectory final : public SliceMatchDirectory {
public:
    [[nodiscard]] std::expected<void, std::string>
    advertise(SliceMatchAdvertisement advertisement, const std::uint64_t now_ms,
              const std::uint64_t lease_duration_ms) override {
        if (auto result = validate(advertisement); !result) {
            return result;
        }

        if (lease_duration_ms == 0 ||
            now_ms > std::numeric_limits<std::uint64_t>::max() - lease_duration_ms) {
            return std::unexpected{"Match lease is invalid"};
        }
        std::scoped_lock lock{mutex_};
        const auto current = matches_.find(advertisement.match_id);
        if (current != matches_.end() && current->second.expires_at_ms > now_ms) {
            if (advertisement.instance_id != current->second.advertisement.instance_id) {
                return std::unexpected{"Match advertisement is owned by another instance"};
            }
            if (advertisement.revision <= current->second.advertisement.revision) {
                return std::unexpected{"Match advertisement revision is stale"};
            }
        }
        const auto id = advertisement.match_id;
        matches_.insert_or_assign(id, Entry{std::move(advertisement), now_ms + lease_duration_ms});
        return {};
    }

    [[nodiscard]] std::expected<bool, std::string>
    withdraw(const std::string_view match_id,
             const std::string_view instance_id) override {
        std::scoped_lock lock{mutex_};
        const auto found = matches_.find(std::string{match_id});
        if (found == matches_.end() ||
            found->second.advertisement.instance_id != instance_id) {
            return false;
        }
        matches_.erase(found);
        return true;
    }

    [[nodiscard]] std::expected<std::vector<SliceMatchAdvertisement>, std::string>
    list_joinable(const std::uint64_t now_ms,
                  const std::uint16_t client_protocol) const override {
        std::scoped_lock lock{mutex_};
        expire(now_ms);
        std::vector<SliceMatchAdvertisement> result;
        for (const auto& [id, entry] : matches_) {
            static_cast<void>(id);
            if (joinable(entry.advertisement, client_protocol)) {
                result.push_back(entry.advertisement);
            }
        }
        return result;
    }

    [[nodiscard]] std::expected<SliceJoinResolution, std::string>
    resolve(const std::string_view match_id, const std::uint64_t now_ms,
            const std::uint16_t client_protocol) const override {
        std::scoped_lock lock{mutex_};
        expire(now_ms);
        const auto found = matches_.find(std::string{match_id});
        if (found == matches_.end()) {
            return std::unexpected{"Match is not advertised"};
        }
        const auto& advertisement = found->second.advertisement;
        if (advertisement.protocol_version != client_protocol) {
            return std::unexpected{"Match protocol is incompatible"};
        }
        if (advertisement.phase != SliceMatchPhase::waiting) {
            return std::unexpected{"Match is no longer waiting for players"};
        }
        if (advertisement.player_count >= advertisement.capacity) {
            return std::unexpected{"Match is full"};
        }
        return SliceJoinResolution{advertisement.match_id, advertisement.endpoint};
    }

private:
    struct Entry {
        SliceMatchAdvertisement advertisement;
        std::uint64_t expires_at_ms{0};
    };

    void expire(const std::uint64_t now_ms) const {
        std::erase_if(matches_, [now_ms](const auto& item) {
            return item.second.expires_at_ms <= now_ms;
        });
    }

    mutable std::mutex mutex_;
    mutable std::map<std::string, Entry, std::less<>> matches_;
};

} // namespace

std::unique_ptr<SliceMatchDirectory> make_in_memory_match_directory() {
    return std::make_unique<InMemoryMatchDirectory>();
}

namespace {

class ServiceMatchDirectory final : public SliceMatchDirectory {
public:
    explicit ServiceMatchDirectory(std::shared_ptr<SliceMatchService> service)
        : service_{std::move(service)} {}

    [[nodiscard]] std::expected<void, std::string>
    advertise(SliceMatchAdvertisement advertisement, const std::uint64_t now_ms,
              const std::uint64_t lease_duration_ms) override {
        if (!service_) {
            return std::unexpected{"Match service is unavailable"};
        }
        if (auto result = validate(advertisement); !result) {
            return result;
        }
        if (lease_duration_ms == 0 ||
            now_ms > std::numeric_limits<std::uint64_t>::max() - lease_duration_ms) {
            return std::unexpected{"Match lease is invalid"};
        }
        const std::string mutation = advertisement.instance_id + ":" +
                                     std::to_string(advertisement.revision);
        return service_->store({std::move(advertisement), now_ms + lease_duration_ms, mutation}, now_ms);
    }

    [[nodiscard]] std::expected<bool, std::string>
    withdraw(const std::string_view match_id,
             const std::string_view instance_id) override {
        if (!service_) {
            return std::unexpected{"Match service is unavailable"};
        }
        return service_->erase(match_id, instance_id);
    }

    [[nodiscard]] std::expected<std::vector<SliceMatchAdvertisement>, std::string>
    list_joinable(const std::uint64_t now_ms,
                  const std::uint16_t client_protocol) const override {
        if (!service_) {
            return std::unexpected{"Match service is unavailable"};
        }
        const auto stored = service_->list();
        if (!stored) {
            return std::unexpected{stored.error()};
        }
        std::vector<SliceMatchAdvertisement> result;
        for (const auto& item : *stored) {
            if (item.expires_at_ms > now_ms && joinable(item.advertisement, client_protocol)) {
                result.push_back(item.advertisement);
            }
        }
        std::ranges::sort(result, {}, &SliceMatchAdvertisement::match_id);
        return result;
    }

    [[nodiscard]] std::expected<SliceJoinResolution, std::string>
    resolve(const std::string_view match_id, const std::uint64_t now_ms,
            const std::uint16_t client_protocol) const override {
        if (!service_) {
            return std::unexpected{"Match service is unavailable"};
        }
        const auto stored = service_->get(match_id);
        if (!stored) {
            return std::unexpected{stored.error()};
        }
        if (stored->expires_at_ms <= now_ms) {
            return std::unexpected{"Match advertisement lease expired"};
        }
        const auto& advertisement = stored->advertisement;
        if (advertisement.protocol_version != client_protocol) {
            return std::unexpected{"Match protocol is incompatible"};
        }
        if (advertisement.phase != SliceMatchPhase::waiting) {
            return std::unexpected{"Match is no longer waiting for players"};
        }
        if (advertisement.player_count >= advertisement.capacity) {
            return std::unexpected{"Match is full"};
        }
        return SliceJoinResolution{advertisement.match_id, advertisement.endpoint};
    }

private:
    std::shared_ptr<SliceMatchService> service_;
};

} // namespace

std::unique_ptr<SliceMatchDirectory>
make_service_match_directory(std::shared_ptr<SliceMatchService> service) {
    return std::make_unique<ServiceMatchDirectory>(std::move(service));
}

namespace {
class MatchServiceCore final : public SliceMatchService {
public:
    std::expected<void, std::string> store(SliceStoredMatch match,
                                            const std::uint64_t now_ms) override {
        if (auto valid = validate_stored_match(match); !valid) return valid;
        std::scoped_lock lock{mutex_};
        const auto found = records_.find(match.advertisement.match_id);
        if (found != records_.end() && found->second.mutation_id == match.mutation_id) {
            return found->second == match ? std::expected<void, std::string>{}
                                          : std::unexpected{"Mutation id payload changed"};
        }
        if (found != records_.end() && found->second.expires_at_ms > now_ms) {
            if (found->second.advertisement.instance_id != match.advertisement.instance_id)
                return std::unexpected{"Match advertisement is owned by another instance"};
            if (found->second.advertisement.revision >= match.advertisement.revision)
                return std::unexpected{"Match advertisement revision is stale"};
        }
        records_.insert_or_assign(match.advertisement.match_id, std::move(match));
        return {};
    }
    std::expected<bool, std::string> erase(const std::string_view id,
                                            const std::string_view instance) override {
        std::scoped_lock lock{mutex_};
        const auto found = records_.find(std::string{id});
        if (found == records_.end() || found->second.advertisement.instance_id != instance)
            return false;
        records_.erase(found); return true;
    }
    std::expected<std::vector<SliceStoredMatch>, std::string> list() const override {
        std::scoped_lock lock{mutex_}; std::vector<SliceStoredMatch> result;
        for (const auto& [id, record] : records_) { static_cast<void>(id); result.push_back(record); }
        return result;
    }
    std::expected<SliceStoredMatch, std::string> get(const std::string_view id) const override {
        std::scoped_lock lock{mutex_}; const auto found = records_.find(std::string{id});
        if (found == records_.end()) return std::unexpected{"Match is not stored"};
        return found->second;
    }
private:
    mutable std::mutex mutex_;
    std::map<std::string, SliceStoredMatch, std::less<>> records_;
};
} // namespace

std::shared_ptr<SliceMatchService> make_match_service_core() {
    return std::make_shared<MatchServiceCore>();
}

std::expected<std::unique_ptr<SliceMatchDirectory>, std::string>
connect_match_directory(SliceMatchServiceConnector& connector,
                        const SliceMatchServiceConnectionSettings& settings) {
    if (settings.service_url.empty()) {
        return std::unexpected{"Match service URL is empty"};
    }
    if (settings.bearer_token.empty() && !settings.acquire_bearer_token) {
        return std::unexpected{"Match service bearer token is empty"};
    }
    if (settings.maximum_attempts == 0 || settings.initial_backoff_ms == 0 ||
        settings.maximum_backoff_ms < settings.initial_backoff_ms) {
        return std::unexpected{"Match service retry policy is invalid"};
    }

    std::string last_error;
    std::uint64_t delay = settings.initial_backoff_ms;
    for (std::size_t attempt = 0; attempt < settings.maximum_attempts; ++attempt) {
        std::string token = settings.bearer_token;
        bool may_connect = true;
        if (settings.acquire_bearer_token) {
            auto acquired = settings.acquire_bearer_token(attempt != 0);
            if (!acquired) { last_error = acquired.error(); may_connect = false; }
            else token = std::move(*acquired);
        }
        if (may_connect) {
            if (token.empty()) return std::unexpected{"Match service credential provider returned an empty token"};
            auto service = connector.connect(settings.service_url, token);
            if (service) {
                if (!*service) {
                    return std::unexpected{"Match service connector returned no service"};
                }
                return make_service_match_directory(std::move(*service));
            }
            last_error = service.error();
        }
        if (attempt + 1 < settings.maximum_attempts && settings.wait) {
            settings.wait(delay);
        }
        if (delay <= settings.maximum_backoff_ms / 2) {
            delay *= 2;
        } else {
            delay = settings.maximum_backoff_ms;
        }
    }
    return std::unexpected{"Could not connect to match service after " +
                           std::to_string(settings.maximum_attempts) +
                           " attempts: " + last_error};
}

std::expected<SliceJoinResolution, std::string>
resolve_slice_join_target(const std::string_view endpoint_or_match,
                          const SliceMatchDirectory* directory,
                          const std::uint64_t now_ms,
                          const std::uint16_t client_protocol) {
    constexpr std::string_view prefix{"match:"};
    if (endpoint_or_match.starts_with(prefix)) {
        if (directory == nullptr) {
            return std::unexpected{"Match directory is not configured"};
        }
        return directory->resolve(endpoint_or_match.substr(prefix.size()), now_ms,
                                  client_protocol);
    }
    if (!valid_endpoint(endpoint_or_match)) {
        return std::unexpected{"Join target must be an endpoint or match:<id>"};
    }
    return SliceJoinResolution{"", std::string{endpoint_or_match}};
}

std::expected<void, std::string>
SliceMatchSelection::refresh(const SliceMatchDirectory& directory,
                             const std::uint64_t now_ms,
                             const std::uint16_t client_protocol) {
    auto listed = directory.list_joinable(now_ms, client_protocol);
    if (!listed) {
        return std::unexpected{listed.error()};
    }
    matches_ = std::move(*listed);
    selected_index_ = 0;
    return {};
}

bool SliceMatchSelection::select(const std::size_t index) noexcept {
    if (index >= matches_.size()) {
        return false;
    }
    selected_index_ = index;
    return true;
}

std::span<const SliceMatchAdvertisement> SliceMatchSelection::matches() const noexcept {
    return matches_;
}

const SliceMatchAdvertisement* SliceMatchSelection::selected() const noexcept {
    return selected_index_ < matches_.size() ? &matches_[selected_index_] : nullptr;
}

std::expected<SliceJoinResolution, std::string>
SliceMatchSelection::resolve(const SliceMatchDirectory& directory,
                             const std::uint64_t now_ms,
                             const std::uint16_t client_protocol) const {
    const auto* choice = selected();
    if (choice == nullptr) {
        return std::unexpected{"No match is selected"};
    }
    return directory.resolve(choice->match_id, now_ms, client_protocol);
}

bool AsyncSliceMatchBrowser::begin_refresh(const SliceMatchDirectory& directory,
                                           const std::uint64_t now_ms,
                                           const std::uint16_t protocol) {
    if (refreshing_) return false;
    error_.clear();
    pending_ = std::async(std::launch::async, [&directory, now_ms, protocol] {
        return directory.list_joinable(now_ms, protocol);
    });
    refreshing_ = true; return true;
}

bool AsyncSliceMatchBrowser::poll() {
    if (!refreshing_ || pending_.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
        return false;
    auto result = pending_.get(); refreshing_ = false;
    if (result) matches_ = std::move(*result);
    else { matches_.clear(); error_ = result.error(); }
    return true;
}

bool AsyncSliceMatchBrowser::refreshing() const noexcept { return refreshing_; }
std::span<const SliceMatchAdvertisement> AsyncSliceMatchBrowser::matches() const noexcept { return matches_; }
std::string_view AsyncSliceMatchBrowser::error() const noexcept { return error_; }

SliceMatchAdvertisement make_slice_match_advertisement(
    std::string match_id, std::string instance_id, std::string display_name,
    std::string endpoint,
    const SliceLobbyState& lobby, const std::size_t capacity,
    const std::uint64_t revision) {
    const auto connected = std::ranges::count_if(lobby.players, [](const SliceLobbyPlayer& player) {
        return player.connected;
    });
    return SliceMatchAdvertisement{
        std::move(match_id), std::move(instance_id), std::move(display_name), std::move(endpoint),
        network::protocol_version, static_cast<std::size_t>(connected), capacity,
        lobby.phase, revision};
}

SliceMatchPublication::SliceMatchPublication(
    SliceMatchDirectory& directory, SliceMatchPublicationSettings settings)
    : directory_{directory}, settings_{std::move(settings)} {}

SliceMatchPublication::~SliceMatchPublication() { static_cast<void>(stop()); }

std::expected<void, std::string>
SliceMatchPublication::start(const SliceLobbyState& lobby, const std::uint64_t now_ms) {
    if (active_) {
        return std::unexpected{"Match publication is already active"};
    }
    if (settings_.refresh_interval_ms == 0 ||
        settings_.refresh_interval_ms >= settings_.lease_duration_ms) {
        return std::unexpected{"Match refresh interval must be shorter than its lease"};
    }
    return publish(lobby, now_ms);
}

std::expected<void, std::string>
SliceMatchPublication::update(const SliceLobbyState& lobby, const std::uint64_t now_ms) {
    if (!active_) {
        return std::unexpected{"Match publication is not active"};
    }
    if (lobby.revision == lobby_revision_ && now_ms < refresh_at_ms_) {
        return {};
    }
    return publish(lobby, now_ms);
}

bool SliceMatchPublication::stop() {
    if (!active_) {
        return false;
    }
    const auto withdrawn = directory_.withdraw(settings_.match_id, settings_.instance_id);
    active_ = false;
    return withdrawn && *withdrawn;
}

bool SliceMatchPublication::active() const noexcept { return active_; }

std::expected<void, std::string>
SliceMatchPublication::publish(const SliceLobbyState& lobby, const std::uint64_t now_ms) {
    auto advertisement = make_slice_match_advertisement(
        settings_.match_id, settings_.instance_id, settings_.display_name,
        settings_.endpoint, lobby, settings_.capacity, advertisement_revision_ + 1);
    auto result = directory_.advertise(std::move(advertisement), now_ms,
                                       settings_.lease_duration_ms);
    if (!result) {
        return result;
    }
    ++advertisement_revision_;
    lobby_revision_ = lobby.revision;
    refresh_at_ms_ = now_ms + settings_.refresh_interval_ms;
    active_ = true;
    return {};
}

} // namespace gloom::gameplay
