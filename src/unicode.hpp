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

std::string narrow(std::wstring_view str) {
	std::string result;
	int size = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), nullptr, 0, nullptr, nullptr);
	result.resize(size);

	bool ok = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.size(), result.data(), size, nullptr, nullptr);
	if (not ok) {
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
				throw std::runtime_error("unknown win32 error");
		}
	}

	return result;
}

std::wstring widen(std::string_view str) {
	std::wstring result;
	int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
	result.resize(size);

	bool ok = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), result.data(), size);
	if (not ok) {
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
				throw std::runtime_error("unknown win32 error");
		}
	}

	return result;
}

std::vector<std::string> _args;
std::vector<const char*> _args_view;

std::span<const char* const> init_argv(int argc, char** argv) {
	if (_args.size() == static_cast<size_t>(argc)) return _args_view;  // Already initialized
	_args.reserve(argc);
	_args_view.reserve(argc);

#ifdef _WIN32
	(void)argv;

	int argc_ = 0;
	auto deleter = [](wchar_t** ptr) { LocalFree(ptr); };
	auto wargv = std::unique_ptr<wchar_t*[], decltype(deleter)>(CommandLineToArgvW(GetCommandLineW(), &argc_), deleter);
	assert(argc == argc_ && "init_argv: win32 reported different argument count");
	assert(wargv != nullptr && "init_argv: CommandLineToArgvW returned nullptr");

	for (int i = 0; i < argc; ++i) {
		_args.push_back(narrow(wargv[i]));
		_args_view.emplace_back(_args.back().c_str());
	}

#else
	for (int i = 0; i < argc; ++i) {
		_args.push_back(argv[i]);
		_args_view.push_back(_args.back().c_str());
	}

#endif  // _WIN32

	return _args_view;
}

std::span<const char* const> args() {
	return _args_view;
}

int argc() {
	return _args.size();
}

const char* const* argv() {
	return _args_view.data();
}
