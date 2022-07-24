#include "syringe.hpp"

int main(int argc, char** argv) {
	try {
		fmt::print("{}", syringe(argc, argv));
	} catch (std::exception& e) {
		fmt::print(stderr, "{}", e.what());
		return 1;
	}
}
