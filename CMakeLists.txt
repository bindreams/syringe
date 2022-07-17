cmake_minimum_required(VERSION 3.17.0)
project(syringe VERSION 0.1.0)

include(cmake/warnings.cmake)

add_executable(syringe "src/syringe.cpp")
target_include_directories(syringe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(syringe PRIVATE cxx_std_20)
target_compile_warnings(syringe treat_as_errors gnu_all gnu_extra ms_4)
set_target_properties(syringe PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

install(TARGETS syringe)
