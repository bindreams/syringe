#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "deps/ctre.hpp"
#include "syringe.hpp"
#include "util.hpp"

using namespace std;

auto match_definition =
	ctre::match<R"#(constexpr std::array<std::uint8_t, (\d+)> _(\w+) = \{((?:0x\d\d, )*0x\d\d)\};)#">;
auto match_definition_empty = ctre::match<R"#(constexpr std::array<std::uint8_t, (\d+)> _(\w+) = \{\};)#">;
auto match_usage = ctre::match<R"#(\tresources\["([\w/. ]*)"\] = syringe::_(\w+);)#">;

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
