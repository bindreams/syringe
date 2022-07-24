#pragma once
#include <concepts>
#include <cstddef>
#include <string_view>
#include <vector>

// clang-format off
template<typename T>
requires std::same_as<T, std::string>
std::vector<std::string_view> split(T&& str, std::string_view separator) = delete;
// clang-format on

std::vector<std::string_view> split(std::string_view data, std::string_view separator) {
	std::vector<std::string_view> result;
	std::size_t first = 0;
	std::size_t last = 0;

	while (true) {
		last = data.find(separator, first);
		result.push_back(data.substr(first, last - first));

		if (last == data.npos) break;
		first = last + separator.size();
	}

	return result;
}
