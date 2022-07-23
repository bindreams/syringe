#pragma once
#include <concepts>
#include <filesystem>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

/// Compute a relative path without "." and "..". Return empty path if p is not a sub-path of base.
std::filesystem::path relative_canonical(const std::filesystem::path& p, const std::filesystem::path& base) {
	namespace fs = std::filesystem;

	fs::path canonical_p = fs::canonical(p);
	fs::path canonical_base = fs::canonical(base);

	if (canonical_p.native().starts_with(canonical_base.native())) return fs::relative(canonical_p, canonical_base);

	return {};
}

// Remove clang-format off in LLVM 15
// clang-format off
template<std::ranges::input_range R>
requires std::convertible_to<std::ranges::range_value_t<R>, std::string_view>
	std::string join(R&& range, std::string_view separator) {
	std::ostringstream result;

	std::string_view sep = "";
	for (auto& value : range) {
		result << sep << value;
		sep = separator;
	}

	return result.str();
}
// clang-format on
