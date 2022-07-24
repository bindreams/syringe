#pragma once
#include "deps/fmt.hpp"

using namespace fmt::literals;

constexpr std::string_view cxmap =
	R"(template<typename Key, typename Value, std::size_t MaxSize, std::strict_weak_order<Key, Key> Compare = std::less<>>
class cxmap {
public:
	// Element access ==================================================================================================
	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr Value& at(const K& k) {
		const auto it = find(k);
		if (it != end()) {
			return it->second;
		}

		throw std::out_of_range("cxmap::at: key not found");
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr const Value& at(const K& k) const {
		const auto it = find(k);
		if (it != end()) {
			return it->second;
		}

		throw std::out_of_range("cxmap::at: key not found");
	}

	constexpr Value& operator[](const Key& k) {
		auto it = lower_bound(k);
		if (it == end() or Compare{}(k, it->first)) {
			if (size() == max_size()) throw std::length_error("cxmap::operator[]: max size exceeded");

			++m_size;
			std::shift_right(it, end(), 1);
			*it = {k, Value{}};
		}

		return it->second;
	}

	constexpr const Value& operator[](const Key& k) const {
		return at(k);
	}

	// Iterators =======================================================================================================
	constexpr auto begin() {
		return m_data.begin();
	}
	constexpr auto begin() const {
		return m_data.begin();
	}
	constexpr auto cbegin() const {
		return m_data.begin();
	}

	constexpr auto end() {
		return std::next(begin(), m_size);
	}
	constexpr auto end() const {
		return std::next(begin(), m_size);
	}
	constexpr auto cend() const {
		return std::next(begin(), m_size);
	}

	// Capacity ========================================================================================================
	constexpr std::size_t size() const {
		return m_size;
	}
	constexpr std::size_t max_size() const {
		return MaxSize;
	}
	constexpr bool empty() const {
		return m_size == 0;
	}

	// Lookup ==========================================================================================================
	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto find(const K& k) noexcept {
		auto [left, right] = std::equal_range(begin(), end(), k, CompareToKey{});
		return left == right ? end() : left;
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto find(const K& k) const noexcept {
		auto [left, right] = std::equal_range(begin(), end(), k, CompareToKey{});
		return left == right ? end() : left;
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr bool contains(const K& k) const noexcept {
		return find(k) != end();
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto lower_bound(const K& k) noexcept {
		return std::lower_bound(begin(), end(), k, CompareToKey{});
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto lower_bound(const K& k) const noexcept {
		return std::lower_bound(begin(), end(), k, CompareToKey{});
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto upper_bound(const K& k) noexcept {
		return std::upper_bound(begin(), end(), k, CompareToKey{});
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto upper_bound(const K& k) const noexcept {
		return std::upper_bound(begin(), end(), k, CompareToKey{});
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto equal_range(const K& k) noexcept {
		return std::equal_range(begin(), end(), k, CompareToKey{});
	}

	template<typename K>
	requires std::strict_weak_order<Compare, Key, K>
	constexpr auto equal_range(const K& k) const noexcept {
		return std::equal_range(begin(), end(), k, CompareToKey{});
	}

private:
	struct CompareToKey {
		template<typename T>
		constexpr bool operator()(const std::pair<Key, Value>& lhs, T&& rhs) const {
			return Compare{}(lhs.first, std::forward<T>(rhs));
		}

		template<typename T>
		constexpr bool operator()(T&& lhs, const std::pair<Key, Value>& rhs) const {
			return Compare{}(std::forward<T>(lhs), rhs.first);
		}
	};

	std::array<std::pair<Key, Value>, MaxSize> m_data{};
	std::size_t m_size = 0;
};)";

/**
 * @brief Template for a complete resource file.
 *
 * Format arguments:
 * 0: namespace start, such as "namespace boost {" or "namespace my::nested::namespace {"
 * 1: namespace end, such as "}  // namespace boost"
 * 2: variable name
 * 3: file count
 * 4: all file variable definition strings
 * 5: all file usage strings
 * 6: cxmap string
 */
constexpr auto template_file = FMT_COMPILE(R"(#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace syringe {{

{6}

{4}

}}  // namespace syringe

{0}constexpr auto {2} = []() {{
	syringe::cxmap<std::string_view, std::span<const std::uint8_t>, {3}> resources;

{5}

	return std::as_const(resources);
}}();{1}
)");

/**
 * @brief Template for a file variable definition string.
 *
 * Format arguments:
 * 0: file contents as hex bytes separated by comma, such as "0xff, 0x12, 0x34"
 * 1: byte count
 * 2: file sha256 hex digest (lowercase)
 */
constexpr auto template_file_definition = FMT_COMPILE(R"(constexpr std::array<std::uint8_t, {1}> _{2} = {{{0}}};)");

/**
 * @brief Template for a file usage string.
 *
 * Format arguments:
 * 0: file identifier (path) such as "resource.txt" or "assets/icons/icon.png"
 * 1: file sha256 hex digest (lowercase)
 */
constexpr auto template_file_usage = FMT_COMPILE(R"(	resources["{0}"] = syringe::_{1};)");
