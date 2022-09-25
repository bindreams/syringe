#include "deps/fmt.hpp"
#include "syringe.hpp"

int main(int raw_argc, char** raw_argv) {
	init_argv(raw_argc, raw_argv);

	try {
		syringe(argc(), argv());
	} catch (const std::exception& e) {
		fmt::print(stderr, "{}", e.what());
		return 1;
	}
}
