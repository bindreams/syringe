#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "deps/ctre-unicode.hpp"
#include "syringe.hpp"
#include "util.hpp"

using namespace std;

auto match_definition =
	ctre::match<R"#(constexpr std::array<std::uint8_t, (\d+)> _(\w+) = \{((?:0x\d\d, )*0x\d\d)\};)#">;
auto match_definition_empty = ctre::match<R"#(constexpr std::array<std::uint8_t, (\d+)> _(\w+) = \{\};)#">;
auto match_definition_until_data = ctre::starts_with<R"#(constexpr std::array<std::uint8_t, (\d+)> _(\w+) = \{)#">;
auto match_usage = ctre::match<R"#(\tresources\["((?:[^"\\]|\\.)*)"\] = syringe::_(\w+);)#">;

TEST_CASE("Inject abc.txt") {
	string inject_file = syringe({
		.paths = {{"data/abc.txt", "abc.txt"}},
		.namespace_name = "",
		.variable_name = "resources",
	});
	vector<string_view> lines = split(inject_file, "\n");
	bool definition_found = false;
	bool usage_found = false;

	size_t i = 0;
	for (; i < lines.size(); ++i) {
		auto [match, size, digest, data] = match_definition(lines[i]);
		if (match) {
			definition_found = true;

			CHECK(size == "3");
			CHECK(digest == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

			vector<string_view> bytes = split(data.view(), ", ");
			CHECK(bytes[0] == "0x61");
			CHECK(bytes[1] == "0x62");
			CHECK(bytes[2] == "0x63");

			break;
		}
	}

	for (; i < lines.size(); ++i) {
		auto [match, filename, digest] = match_usage(lines[i]);
		if (match) {
			usage_found = true;

			CHECK(filename == "abc.txt");
			CHECK(digest == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

			break;
		}
	}

	CHECK(definition_found);
	CHECK(usage_found);
}

TEST_CASE("Inject empty.txt") {
	string inject_file = syringe({
		.paths = {{"data/empty.txt", "empty.txt"}},
		.namespace_name = "",
		.variable_name = "resources",
	});
	vector<string_view> lines = split(inject_file, "\n");
	bool definition_found = false;
	bool usage_found = false;

	size_t i = 0;
	for (; i < lines.size(); ++i) {
		auto [match, size, digest] = match_definition_empty(lines[i]);
		if (match) {
			definition_found = true;

			CHECK(size == "0");
			CHECK(digest == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

			break;
		}
	}

	for (; i < lines.size(); ++i) {
		auto [match, filename, digest] = match_usage(lines[i]);
		if (match) {
			usage_found = true;

			CHECK(filename == "empty.txt");
			CHECK(digest == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

			break;
		}
	}

	CHECK(definition_found);
	CHECK(usage_found);
}

TEST_CASE("Inject 1MiB_null.bin") {
	string inject_file = syringe({
		.paths = {{"data/1MiB_null.bin", "1MiB_null.bin"}},
		.namespace_name = "",
		.variable_name = "resources",
	});
	vector<string_view> lines = split(inject_file, "\n");
	bool definition_found = false;
	bool usage_found = false;

	size_t i = 0;
	for (; i < lines.size(); ++i) {
		string_view line = lines[i];
		auto [match, size, digest] = match_definition_until_data(line);  // CTRE crashes on full match lol
		if (match) {
			definition_found = true;

			CHECK(size == "1048576");
			CHECK(digest == "30e14955ebf1352266dc2ff8067e68104607e750abb9d3b36582b8af909fcb58");

			vector<string_view> bytes = split(line.substr(match.size(), line.size() - match.size() - 2), ", ");
			for (string_view byte : bytes) {
				CHECK(byte == "0x00");
			}

			break;
		}
	}

	for (; i < lines.size(); ++i) {
		auto [match, filename, digest] = match_usage(lines[i]);
		if (match) {
			usage_found = true;

			CHECK(filename == "1MiB_null.bin");
			CHECK(digest == "30e14955ebf1352266dc2ff8067e68104607e750abb9d3b36582b8af909fcb58");

			break;
		}
	}

	CHECK(definition_found);
	CHECK(usage_found);
}

TEST_CASE("Inject <complex filename>") {
	string filename = "data/René Magritte - Ceci n'est pas une pipe 🚬.jpg";
	string inject_file = syringe({
		.paths = {{filename, filename}},
		.namespace_name = "",
		.variable_name = "resources",
	});
	vector<string_view> lines = split(inject_file, "\n");

	for (size_t i = 0; i < lines.size(); ++i) {
		auto [match, actual_filename, digest] = match_usage(lines[i]);
		if (match) {
			CHECK(actual_filename == filename);
			CHECK(digest == "3e4d289f4f8e0669a3891494146827f6083a5807b7f47f5ac89c3f53c779343a");

			return;
		}
	}

	FAIL("Usage line was not found");
}
