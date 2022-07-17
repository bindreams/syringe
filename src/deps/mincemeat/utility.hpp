#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace mincemeat {

/// Covert a span of bytes into a hexadecimal string.
constexpr std::string to_string(std::span<const std::uint8_t> data) noexcept {
	constexpr std::string_view hex_alphabet = "0123456789abcdef";

	std::string result;
	result.reserve(2 * data.size());

	for (std::size_t i = 0; i < data.size(); i++) {
		result += hex_alphabet[(data[i] >> 4) & 15];
		result += hex_alphabet[data[i] & 15];
	}

	return result;
}

}  // namespace mincemeat