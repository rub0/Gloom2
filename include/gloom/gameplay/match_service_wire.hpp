#pragma once
#include <gloom/gameplay/match_discovery.hpp>

namespace gloom::gameplay {
inline constexpr std::size_t maximum_match_record_bytes = 2048;
[[nodiscard]] std::expected<void, std::string> validate_stored_match(const SliceStoredMatch& match);
[[nodiscard]] std::string encode_stored_match(const SliceStoredMatch& match);
[[nodiscard]] std::expected<SliceStoredMatch, std::string> decode_stored_match(std::string_view line);
} // namespace gloom::gameplay
