cmake_minimum_required(VERSION 3.17.0)
project(syringe VERSION 0.1.0)

option(SYRINGE_TESTS "Build tests for syringe" OFF)
option(SYRINGE_EXAMPLES "Build examples for syringe" OFF)

include(cmake/warnings.cmake)

add_executable(syringe "src/main.cpp")
target_include_directories(syringe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(syringe PRIVATE cxx_std_20)
target_compile_warnings(syringe treat_as_errors gnu_all gnu_extra ms_4)
target_compile_definitions(syringe PRIVATE "WIN32_LEAN_AND_MEAN")
set_target_properties(syringe PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS syringe)

if(SYRINGE_TESTS)
	add_executable(syringe_tests "tests/main.cpp")
	target_include_directories(syringe_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_compile_features(syringe_tests PRIVATE cxx_std_20)
	target_compile_warnings(syringe_tests treat_as_errors gnu_all gnu_extra ms_4)

	install(TARGETS syringe_tests)
	install(DIRECTORY "tests/data" DESTINATION ".")
endif()

if(SYRINGE_EXAMPLES)
	# Usually it's expected that syringe will be properly installed before usage.
	# For the example, the workaround is to set SYRINGE_EXECUTABLE cache variable to the target name.
	# When syringe.cmake is included, it will use this built target instead of searching for an install executable.
	set(SYRINGE_EXECUTABLE syringe CACHE STRING "Syringe executable target")
	add_subdirectory(example)
	add_dependencies(syringe_example syringe)
endif()
