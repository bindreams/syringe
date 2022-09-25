#pragma once
#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on
#endif  // _WIN32

#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

void throw_winapi_error() {
	switch (GetLastError()) {
		case ERROR_INSUFFICIENT_BUFFER:
			throw std::runtime_error("insufficient buffer");
		case ERROR_INVALID_FLAGS:
			throw std::runtime_error("invalid flags");
		case ERROR_INVALID_PARAMETER:
			throw std::runtime_error("invalid parameter");
		case ERROR_NO_UNICODE_TRANSLATION:
			throw std::invalid_argument("no unicode translation");
		default:
			throw std::runtime_error("unknown WinAPI error");
	}
}

std::string narrow(std::wstring_view str) {
	std::string result;
	int size = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), nullptr, 0, nullptr, nullptr);
	result.resize(size);

	bool ok = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), result.data(), size, nullptr, nullptr);
	if (not ok) throw_winapi_error();

	return result;
}

std::wstring widen(std::string_view str) {
	std::wstring result;
	int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
	result.resize(size);

	bool ok = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), result.data(), size);
	if (not ok) throw_winapi_error();

	return result;
}
