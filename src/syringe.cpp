#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "deps/CLI11.hpp"
#include "deps/fmt.hpp"
#include "deps/mincemeat.hpp"

#include "templates.hpp"
#include "util.hpp"

namespace mm = mincemeat;
namespace fs = std::filesystem;

struct Configuration {
	std::unordered_map<std::string, std::string> paths;
	std::string namespace_name;
	std::string variable_name;
};

Configuration parse_cli(int argc, char** argv) {
	CLI::App app("Inject files into C++ source code.", "syringe");

	std::vector<fs::path> paths;
	std::optional<fs::path> relative_to;
	std::optional<std::string> prefix;
	std::string variable;

	// clang-format off
	app.add_option("paths", paths, "One or more path to files for injecting")
		->required()
		->check(CLI::ExistingFile);
	app.add_option("-r,--relative", relative_to, "Make paths relative to a directory")
		->check(CLI::ExistingDirectory);
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
		if (relative_to.has_value()) {
			for (auto& path : paths) {
				fs::path final_path = relative_canonical(path, *relative_to);
				if (final_path.empty()) {
					throw CLI::ValidationError(fmt::format("{} is not a base dir of {}", *relative_to, path));
				}

				config.paths[path.string()] = final_prefix + final_path.string();
			}
		} else {
			for (auto& path : paths) {
				config.paths[path.string()] = final_prefix + path.string();
			}
		}

		return config;
	} catch (const CLI::ParseError& e) {
		std::exit(app.exit(e));
	}
}

/// Produce a file definition string and a file usage string for injecting into the template.
std::pair<std::string, std::string> file_strings(fs::path path, std::string_view display_path) {
	std::ifstream ifs(path, std::ios::binary);

	std::array<uint8_t, 10240> buffer;
	mm::sha256_stream hasher;
	std::stringstream cpp_data_stream;
	cpp_data_stream << std::hex << std::showbase;
	std::size_t size = 0;

	do {
		ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		size += ifs.gcount();
		std::span data(buffer.begin(), ifs.gcount());

		std::string_view sep = "";
		for (std::uint8_t byte : data) {
			cpp_data_stream << sep << int(byte);
			sep = ", ";
		}

		hasher << data;
	} while (ifs);

	std::string cpp_data = cpp_data_stream.str();
	std::string digest = mm::to_string(hasher.finish());

	std::string file_definition = fmt::format(template_file_definition, cpp_data, size, digest);
	std::string file_usage = fmt::format(template_file_usage, display_path, digest);

	return {file_definition, file_usage};
}

int main(int argc, char** argv) {
	auto config = parse_cli(argc, argv);

	std::vector<std::string> definitions;
	std::vector<std::string> usages;

	for (auto& [path, display_path] : config.paths) {
		auto [def, usage] = file_strings(path, display_path);
		definitions.push_back(std::move(def));
		usages.push_back(std::move(usage));
	}

	fmt::print(
		template_file,
		config.namespace_name.empty() ? "" : fmt::format("namespace {} {{", config.namespace_name),
		config.namespace_name.empty() ? "" : fmt::format("}}  // namespace {}", config.namespace_name),
		config.variable_name,
		config.paths.size(),
		join(definitions, "\n"),
		join(usages, "\n"),
		cxmap
	);
}