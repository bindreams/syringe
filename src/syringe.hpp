#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "deps/CLI11.hpp"
#include "deps/fmt.hpp"
#include "deps/mincemeat.hpp"

#include "templates.hpp"
#include "unicode.hpp"
#include "util.hpp"
#include "validators.hpp"

struct Configuration {
	std::unordered_map<std::string, std::string> paths;
	std::string namespace_name;
	std::string variable_name;
};

Configuration parse_cli(int argc, const char* const* argv) {
	CLI::App app("Inject files into C++ source code.", "syringe");

	std::vector<std::string> paths;
	std::optional<std::string> relative_to;
	std::optional<std::string> prefix;
	std::string variable;

	// clang-format off
	app.add_option("paths", paths, "One or more path to files for injecting")
		->required()
		->check(ExistingFile);
	app.add_option("-r,--relative", relative_to, "Make paths relative to a directory")
		->check(ExistingDirectory);
	app.add_option("-p,--prefix", prefix, "Prefix resulting paths with a string");
	app.add_option("--variable", variable, "Variable name for resources, e.g. \"data\" or \"my_namespace::assets\"")
		->default_val("resources");
	// clang-format on

	try {
		app.parse(argc, argv);
		Configuration config;

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
					throw CLI::ValidationError(fmt::format("{} is not a base dir of {}", *relative_to, path));
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

std::string file_hash(std::string_view path) {
	namespace mm = mincemeat;
	std::ifstream ifs(widen(path), std::ios::binary);

	std::array<uint8_t, 10240> buffer;
	mm::sha256_stream hasher;

	do {
		ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		std::span data(buffer.begin(), ifs.gcount());

		hasher << data;
	} while (ifs);

	return mm::to_string(hasher.finish());
}

/// Produce a file definition string for injecting into the template.
std::string file_definition(std::string_view path, std::string_view hash) {
	std::ifstream ifs(widen(path), std::ios::binary);

	std::array<uint8_t, 10240> buffer;
	std::stringstream cpp_data_stream;
	cpp_data_stream << std::hex << std::showbase;
	std::size_t size = 0;

	std::string_view sep = "";
	do {
		ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		size += ifs.gcount();
		std::span data(buffer.begin(), ifs.gcount());

		for (std::uint8_t byte : data) {
			cpp_data_stream << sep << fmt::format("{:#04x}", int(byte));
			sep = ", ";
		}
	} while (ifs);

	std::string cpp_data = cpp_data_stream.str();
	return fmt::format(template_file_definition, cpp_data, size, hash);
}

/// Produce a file usage string for injecting into the template.
std::string file_usage(std::string_view display_path, std::string_view hash) {
	return fmt::format(template_file_usage, display_path, hash);
}

std::string syringe(const Configuration& config) {
	std::vector<std::string> definitions;
	std::vector<std::string> usages;
	std::unordered_set<std::string> hashes;

	for (auto& [path, display_path] : config.paths) {
		std::string hash = file_hash(path);
		auto [_, is_new] = hashes.insert(hash);

		if (is_new) definitions.push_back(file_definition(path, hash));
		usages.push_back(file_usage(display_path, hash));
	}

	return fmt::format(
		template_file,
		config.namespace_name.empty() ? "" : fmt::format("namespace {} {{\n\n", config.namespace_name),
		config.namespace_name.empty() ? "" : fmt::format("\n\n}}  // namespace {}", config.namespace_name),
		config.variable_name,
		config.paths.size(),
		join(definitions, "\n"),
		join(usages, "\n"),
		cxmap
	);
}

std::string syringe(int argc, const char* const* argv) {
	return syringe(parse_cli(argc, argv));
}
