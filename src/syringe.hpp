#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include "deps/fmt.hpp"
#include "deps/mincemeat.hpp"

#include "cli.hpp"
#include "templates.hpp"

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
	std::size_t size = 0;

	std::string_view sep = "";
	do {
		ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		size += ifs.gcount();
		std::span data(buffer.begin(), ifs.gcount());

		for (std::uint8_t byte : data) {
			cpp_data_stream << sep << static_cast<int>(byte);
			sep = ",";
		}
	} while (ifs);

	std::string cpp_data = cpp_data_stream.str();
	return fmt::format(template_file_definition, cpp_data, size, hash);
}

/// Produce a file usage string for injecting into the template.
std::string file_usage(std::string_view display_path, std::string_view hash) {
	return fmt::format(template_file_usage, display_path, hash);
}

struct syringe_impl_result_t {
	std::vector<std::string> definitions;
	std::vector<std::string> usages;
	std::unordered_set<std::string> hashes;
};

syringe_impl_result_t syringe_impl(const InputConfig& config) {
	syringe_impl_result_t r;

	for (auto& [path, display_path] : config.paths) {
		std::string hash = file_hash(path);
		auto [_, is_new] = r.hashes.insert(hash);

		if (is_new) r.definitions.push_back(file_definition(path, hash));
		r.usages.push_back(file_usage(display_path, hash));
	}

	return r;
}

void syringe(const InputConfig& config, FILE* fp) {
	auto [definitions, usages, hashes] = syringe_impl(config);

	fmt::print(
		fp,
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

[[nodiscard]] std::string syringe(const InputConfig& config) {
	auto [definitions, usages, hashes] = syringe_impl(config);

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

void syringe(int argc, const char* const* argv) {
	auto config = parse_cli(argc, argv);

	if (config.output_path.empty()) {
		syringe(config, stdout);
	} else {
		auto file_close = [](FILE* fp) { std::fclose(fp); };
		std::unique_ptr<FILE, decltype(file_close)> fp(std::fopen(config.output_path.c_str(), "w"), file_close);

		syringe(config, fp.get());
	}
}
