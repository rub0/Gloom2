#pragma once

#include <gloom/gameplay/match_lobby.hpp>
#include <gloom/network/protocol.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <future>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace gloom::gameplay {

struct SliceMatchAdvertisement {
    std::string match_id;
    std::string instance_id;
    std::string display_name;
    std::string endpoint;
    std::uint16_t protocol_version{network::protocol_version};
    std::size_t player_count{0};
    std::size_t capacity{2};
    SliceMatchPhase phase{SliceMatchPhase::waiting};
    std::uint64_t revision{1};

    [[nodiscard]] bool operator==(const SliceMatchAdvertisement&) const noexcept = default;
};

struct SliceJoinResolution {
    std::string match_id;
    std::string endpoint;
};

// Backend-neutral directory boundary. A service-backed implementation may replace
// the in-memory implementation without exposing its SDK to gameplay or transport.
class SliceMatchDirectory {
public:
    virtual ~SliceMatchDirectory() = default;

    [[nodiscard]] virtual std::expected<void, std::string>
    advertise(SliceMatchAdvertisement advertisement, std::uint64_t now_ms,
              std::uint64_t lease_duration_ms) = 0;
    [[nodiscard]] virtual std::expected<bool, std::string>
    withdraw(std::string_view match_id, std::string_view instance_id) = 0;
    [[nodiscard]] virtual std::expected<std::vector<SliceMatchAdvertisement>, std::string>
    list_joinable(std::uint64_t now_ms,
                  std::uint16_t client_protocol = network::protocol_version) const = 0;
    [[nodiscard]] virtual std::expected<SliceJoinResolution, std::string>
    resolve(std::string_view match_id, std::uint64_t now_ms,
            std::uint16_t client_protocol = network::protocol_version) const = 0;
};

[[nodiscard]] std::unique_ptr<SliceMatchDirectory> make_in_memory_match_directory();

struct SliceStoredMatch {
    SliceMatchAdvertisement advertisement;
    std::uint64_t expires_at_ms{0};
    std::string mutation_id;
    [[nodiscard]] bool operator==(const SliceStoredMatch&) const noexcept = default;
};

// Provider-facing persistence contract. Implementations make store/erase atomic
// per match id and enforce live instance ownership plus monotonic revisions.
class SliceMatchService {
public:
    virtual ~SliceMatchService() = default;
    [[nodiscard]] virtual std::expected<void, std::string>
    store(SliceStoredMatch match, std::uint64_t now_ms) = 0;
    [[nodiscard]] virtual std::expected<bool, std::string>
    erase(std::string_view match_id, std::string_view instance_id) = 0;
    [[nodiscard]] virtual std::expected<std::vector<SliceStoredMatch>, std::string>
    list() const = 0;
    [[nodiscard]] virtual std::expected<SliceStoredMatch, std::string>
    get(std::string_view match_id) const = 0;
};

class SliceMatchServiceConnector {
public:
    virtual ~SliceMatchServiceConnector() = default;
    [[nodiscard]] virtual std::expected<std::shared_ptr<SliceMatchService>, std::string>
    connect(std::string_view service_url, std::string_view bearer_token) = 0;
};

struct SliceMatchServiceConnectionSettings {
    std::string service_url;
    std::string bearer_token;
    std::size_t maximum_attempts{3};
    std::uint64_t initial_backoff_ms{250};
    std::uint64_t maximum_backoff_ms{2'000};
    std::function<void(std::uint64_t)> wait;
    std::function<std::expected<std::string, std::string>(bool refresh)> acquire_bearer_token;
};

// Authenticates through a provider connector with bounded exponential backoff,
// then exposes the resulting service through the standard directory adapter.
[[nodiscard]] std::expected<std::unique_ptr<SliceMatchDirectory>, std::string>
connect_match_directory(SliceMatchServiceConnector& connector,
                        const SliceMatchServiceConnectionSettings& settings);

[[nodiscard]] std::unique_ptr<SliceMatchDirectory>
make_service_match_directory(std::shared_ptr<SliceMatchService> service);

[[nodiscard]] std::shared_ptr<SliceMatchService> make_match_service_core();
[[nodiscard]] std::expected<std::shared_ptr<SliceMatchService>, std::string>
make_durable_match_service(const std::filesystem::path& storage_file);

[[nodiscard]] std::expected<SliceJoinResolution, std::string>
resolve_slice_join_target(std::string_view endpoint_or_match,
                          const SliceMatchDirectory* directory,
                          std::uint64_t now_ms,
                          std::uint16_t client_protocol = network::protocol_version);

class SliceMatchSelection final {
public:
    [[nodiscard]] std::expected<void, std::string>
    refresh(const SliceMatchDirectory& directory, std::uint64_t now_ms,
            std::uint16_t client_protocol = network::protocol_version);
    [[nodiscard]] bool select(std::size_t index) noexcept;
    [[nodiscard]] std::span<const SliceMatchAdvertisement> matches() const noexcept;
    [[nodiscard]] const SliceMatchAdvertisement* selected() const noexcept;
    [[nodiscard]] std::expected<SliceJoinResolution, std::string>
    resolve(const SliceMatchDirectory& directory, std::uint64_t now_ms,
            std::uint16_t client_protocol = network::protocol_version) const;

private:
    std::vector<SliceMatchAdvertisement> matches_;
    std::size_t selected_index_{0};
};

class AsyncSliceMatchBrowser final {
public:
    [[nodiscard]] bool begin_refresh(const SliceMatchDirectory& directory,
                                     std::uint64_t now_ms,
                                     std::uint16_t client_protocol = network::protocol_version);
    [[nodiscard]] bool poll();
    [[nodiscard]] bool refreshing() const noexcept;
    [[nodiscard]] std::span<const SliceMatchAdvertisement> matches() const noexcept;
    [[nodiscard]] std::string_view error() const noexcept;

private:
    std::future<std::expected<std::vector<SliceMatchAdvertisement>, std::string>> pending_;
    std::vector<SliceMatchAdvertisement> matches_;
    std::string error_;
    bool refreshing_{false};
};

[[nodiscard]] SliceMatchAdvertisement make_slice_match_advertisement(
    std::string match_id, std::string instance_id, std::string display_name, std::string endpoint,
    const SliceLobbyState& lobby, std::size_t capacity, std::uint64_t revision);

struct SliceMatchPublicationSettings {
    std::string match_id;
    std::string instance_id;
    std::string display_name;
    std::string endpoint;
    std::size_t capacity{2};
    std::uint64_t lease_duration_ms{15'000};
    std::uint64_t refresh_interval_ms{5'000};
};

// Server-owned publication lifecycle. Destruction performs a best-effort,
// owner-checked withdrawal; a crash is recovered by the directory lease.
class SliceMatchPublication final {
public:
    SliceMatchPublication(SliceMatchDirectory& directory,
                          SliceMatchPublicationSettings settings);
    ~SliceMatchPublication();
    SliceMatchPublication(const SliceMatchPublication&) = delete;
    SliceMatchPublication& operator=(const SliceMatchPublication&) = delete;

    [[nodiscard]] std::expected<void, std::string>
    start(const SliceLobbyState& lobby, std::uint64_t now_ms);
    [[nodiscard]] std::expected<void, std::string>
    update(const SliceLobbyState& lobby, std::uint64_t now_ms);
    [[nodiscard]] bool stop();
    [[nodiscard]] bool active() const noexcept;

private:
    [[nodiscard]] std::expected<void, std::string>
    publish(const SliceLobbyState& lobby, std::uint64_t now_ms);

    SliceMatchDirectory& directory_;
    SliceMatchPublicationSettings settings_;
    std::uint64_t advertisement_revision_{0};
    std::uint64_t lobby_revision_{0};
    std::uint64_t refresh_at_ms_{0};
    bool active_{false};
};

} // namespace gloom::gameplay
