#include "deps/fmt.hpp"
#include "syringe.hpp"
#include "unicode.hpp"

int main(int argc, char** argv) {
	try {
		auto args = init_argv(argc, argv);
		fmt::print("{}", syringe(args.size(), args.data()));
	} catch (const std::exception& e) {
		fmt::print(stderr, "{}", e.what());
		return 1;
	}
}
