#pragma once
#include <filesystem>
#include "deps/CLI11.hpp"
#include "unicode.hpp"

enum class path_type { nonexistent, file, directory };

inline path_type check_path(std::string_view file) noexcept {
	std::error_code ec;
	auto stat = std::filesystem::status(widen(file), ec);
	if (ec) return path_type::nonexistent;
	switch (stat.type()) {
		case std::filesystem::file_type::none:
		case std::filesystem::file_type::not_found:
			return path_type::nonexistent;
		case std::filesystem::file_type::directory:
			return path_type::directory;
		case std::filesystem::file_type::symlink:
		case std::filesystem::file_type::block:
		case std::filesystem::file_type::character:
		case std::filesystem::file_type::fifo:
		case std::filesystem::file_type::socket:
		case std::filesystem::file_type::regular:
		case std::filesystem::file_type::unknown:
		default:
			return path_type::file;
	}
}

class ExistingFileValidator : public CLI::Validator {
public:
	ExistingFileValidator() : Validator("FILE") {
		func_ = [](std::string& filename) {
			auto path_result = check_path(filename);
			if (path_result == path_type::nonexistent) {
				return "File does not exist: " + filename;
			}
			if (path_result == path_type::directory) {
				return "File is actually a directory: " + filename;
			}
			return std::string();
		};
	}
};

class ExistingDirectoryValidator : public CLI::Validator {
public:
	ExistingDirectoryValidator() : Validator("DIR") {
		func_ = [](std::string& filename) {
			auto path_result = check_path(filename);
			if (path_result == path_type::nonexistent) {
				return "Directory does not exist: " + filename;
			}
			if (path_result == path_type::file) {
				return "Directory is actually a file: " + filename;
			}
			return std::string();
		};
	}
};

ExistingFileValidator ExistingFile;
ExistingDirectoryValidator ExistingDirectory;
