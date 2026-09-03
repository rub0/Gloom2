#pragma once
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <string_view>

namespace gloom::backends {
// Single-owner cache, serialized with requests by the concrete reader session.
// Exchange returns the HTTPS wire body; time is injectable for expiry testing.
class MatchReaderCredential final {
public:
    using Clock = std::chrono::steady_clock;
    using Exchange = std::function<std::expected<std::string, std::string>()>;
    using Now = std::function<Clock::time_point()>;
    [[nodiscard]] std::expected<std::string, std::string> acquire(
        const Exchange& exchange, const Now& now = [] { return Clock::now(); }) {
        const auto started = now();
        if (!token_.empty() && started < expires_) return token_;
        // Never reuse the previous grant after a failed renewal.
        token_.clear();
        const auto body = exchange();
        if (!body) return std::unexpected{body.error()};
        const auto tab = body->find('\t');
        if (tab != 68 || !body->starts_with("gr1_") ||
            !std::ranges::all_of(std::string_view{*body}.substr(4, 64), [](char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
            })) return std::unexpected{"Invalid reader grant response"};
        std::uint64_t lifetime = 0;
        const auto* end = body->data() + body->size();
        const auto parsed = std::from_chars(body->data() + tab + 1, end, lifetime);
        if (parsed.ec != std::errc{} || parsed.ptr != end || lifetime == 0 || lifetime > 60'000)
            return std::unexpected{"Invalid reader grant lifetime"};
        expires_ = started + std::chrono::milliseconds{lifetime - std::min<std::uint64_t>(1000, lifetime / 10)};
        if (now() >= expires_) return std::unexpected{"Reader grant expired during exchange"};
        token_ = body->substr(0, tab);
        return token_;
    }
private:
    std::string token_;
    Clock::time_point expires_{};
};
} // namespace gloom::backends
