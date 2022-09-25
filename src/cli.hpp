#pragma once
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>

#include <filesystem>
#include "deps/CLI11.hpp"
#include "unicode.hpp"
#include "util.hpp"

// Validators ==========================================================================================================
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

// Processing of argc/argv =============================================================================================
std::vector<std::string> _args;
std::vector<const char*> _args_view;

inline std::span<const char* const> init_argv(int argc, const char* const* argv) {
	if (_args.size() == static_cast<size_t>(argc)) return _args_view;  // Already initialized
	_args.reserve(argc);
	_args_view.reserve(argc);

#ifdef _WIN32
	(void)argv;

	int argc_ = 0;
	auto deleter = [](wchar_t** ptr) { LocalFree(ptr); };
	auto wargv = std::unique_ptr<wchar_t*[], decltype(deleter)>(CommandLineToArgvW(GetCommandLineW(), &argc_), deleter);
	assert(argc == argc_ && "init_argv: WinAPI reported different argument count");
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

inline std::span<const char* const> args() {
	return _args_view;
}

inline int argc() {
	return _args.size();
}

inline const char* const* argv() {
	return _args_view.data();
}

// Parsing CLI =========================================================================================================
struct InputConfig {
	std::unordered_map<std::string, std::string> paths;
	std::string namespace_name;
	std::string variable_name;
};

struct Config : InputConfig {
	std::string output_path;
};

inline Config parse_cli(int argc, const char* const* argv) {
	CLI::App app("Inject files into C++ source code.", "syringe");

	std::vector<std::string> paths;
	std::optional<std::string> output_path;
	std::optional<std::string> relative_to;
	std::optional<std::string> prefix;
	std::string variable;

	// clang-format off
	app.add_option("paths", paths, "One or more path to files for injecting")
		->required()
		->check(ExistingFile);
	app.add_option("-o,--output", output_path, "Path for the output file (omit to use stdout)");
	app.add_option("-r,--relative", relative_to, "Make paths relative to a directory")
		->check(ExistingDirectory);
	app.add_option("-p,--prefix", prefix, "Prefix resulting paths with a string");
	app.add_option("--variable", variable, "Variable name for resources, e.g. \"data\" or \"my_namespace::assets\"")
		->default_val("resources");
	// clang-format on

	try {
		app.parse(argc, argv);
		Config config;

		config.output_path = output_path.value_or("");
		if (config.output_path == "-") {
			config.output_path = "";
		}

		// Compute variable and namespace name -------------------------------------------------------------------------
		std::size_t split_pos = variable.rfind("::");
		if (split_pos == std::string::npos) {
			config.variable_name = std::move(variable);
		} else {
			config.namespace_name = variable.substr(0, split_pos);
			config.variable_name = variable.substr(split_pos + 2);
		}

		// Compute paths -----------------------------------------------------------------------------------------------
		std::string final_prefix = prefix.value_or("");

		for (const std::string& path : paths) {
			std::string final_path = final_prefix;

			if (relative_to.has_value()) {
				std::string normalized_path = narrow(relative_canonical(widen(path), *relative_to).native());
				if (normalized_path.empty()) {
					throw CLI::ValidationError(fmt::format("\"{}\" is not a base dir of \"{}\"", *relative_to, path));
				}

				final_path += normalized_path;
			} else {
				final_path += path;
			}

			std::ranges::replace(final_path, '\\', '/');
			config.paths[path] = final_path;
		}

		return config;
	} catch (const CLI::ParseError& e) {
		std::exit(app.exit(e));
	}
}
