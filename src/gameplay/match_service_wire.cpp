#include <gloom/gameplay/match_service_wire.hpp>
#include <algorithm>
#include <array>
#include <charconv>

namespace gloom::gameplay {
namespace {
bool text_field(const std::string_view s, const std::size_t maximum) {
    return !s.empty() && s.size() <= maximum && std::ranges::all_of(s, [](unsigned char c) {
        return c >= 32 && c < 127;
    });
}
}

std::expected<void, std::string> validate_stored_match(const SliceStoredMatch& item) {
    const auto& a = item.advertisement;
    if (!text_field(a.match_id, 96) || !text_field(a.instance_id, 96) ||
        !text_field(a.display_name, 96) || !text_field(a.endpoint, 255) ||
        !text_field(item.mutation_id, 128)) return std::unexpected{"Invalid match text field"};
    const auto colon = a.endpoint.rfind(':');
    if (colon == std::string::npos || colon == 0 || a.endpoint.find(' ') != std::string::npos)
        return std::unexpected{"Invalid match endpoint"};
    unsigned port = 0;
    const auto first = a.endpoint.data() + colon + 1;
    const auto last = a.endpoint.data() + a.endpoint.size();
    const auto parsed = std::from_chars(first, last, port);
    if (parsed.ec != std::errc{} || parsed.ptr != last || port == 0 || port > 65535)
        return std::unexpected{"Invalid match port"};
    if (a.protocol_version == 0 || a.capacity == 0 || a.capacity > 64 ||
        a.player_count > a.capacity || static_cast<unsigned>(a.phase) > 2 ||
        a.revision == 0 || item.expires_at_ms == 0)
        return std::unexpected{"Invalid match numeric field"};
    return {};
}

std::string encode_stored_match(const SliceStoredMatch& item) {
    const auto& a = item.advertisement;
    return a.match_id + '\t' + a.instance_id + '\t' + a.display_name + '\t' + a.endpoint + '\t' +
        std::to_string(a.protocol_version) + '\t' + std::to_string(a.player_count) + '\t' +
        std::to_string(a.capacity) + '\t' + std::to_string(static_cast<unsigned>(a.phase)) + '\t' +
        std::to_string(a.revision) + '\t' + std::to_string(item.expires_at_ms) + '\t' + item.mutation_id;
}

std::expected<SliceStoredMatch, std::string> decode_stored_match(std::string_view line) {
    if (line.size() > maximum_match_record_bytes) return std::unexpected{"Match record too large"};
    std::array<std::string_view, 11> fields;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto tab = line.find('\t');
        if ((i == fields.size() - 1) != (tab == std::string_view::npos))
            return std::unexpected{"Incorrect match field count"};
        fields[i] = line.substr(0, tab);
        if (tab != std::string_view::npos) line.remove_prefix(tab + 1);
    }
    std::array<std::uint64_t, 6> numbers{};
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        const auto f = fields[i + 4];
        const auto p = std::from_chars(f.data(), f.data() + f.size(), numbers[i]);
        if (p.ec != std::errc{} || p.ptr != f.data() + f.size())
            return std::unexpected{"Invalid match number"};
    }
    if (numbers[0] > 65535 || numbers[1] > 64 || numbers[2] > 64 || numbers[3] > 2)
        return std::unexpected{"Match number out of range"};
    SliceStoredMatch item{{std::string{fields[0]}, std::string{fields[1]}, std::string{fields[2]},
        std::string{fields[3]}, static_cast<std::uint16_t>(numbers[0]),
        static_cast<std::size_t>(numbers[1]), static_cast<std::size_t>(numbers[2]),
        static_cast<SliceMatchPhase>(numbers[3]), numbers[4]}, numbers[5], std::string{fields[10]}};
    if (auto result = validate_stored_match(item); !result) return std::unexpected{result.error()};
    return item;
}
} // namespace gloom::gameplay
