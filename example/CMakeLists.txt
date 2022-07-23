cmake_minimum_required(VERSION 3.17.0)
project(syringe_example VERSION 0.1.0)

include(syringe.cmake)

add_executable(syringe_example "main.cpp")
target_compile_features(syringe_example PRIVATE cxx_std_20)

# When adding all files from a whole directory, file(GLOB) may be used to detect files.
# ${MUSIC_FILES} will contain absolute paths to all files in "music/".
file(GLOB MUSIC_FILES CONFIGURE_DEPENDS "music/*")
message(STATUS "List of included files: ${MUSIC_FILES}")

target_inject_files(syringe_example
	FILES ${MUSIC_FILES}
	OUTPUT resources.hpp
	VARIABLE "example::resources"
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	PREFIX "assets/"  # Optionally prefix all file entries with a prefix or a pseudo-directory.
)

install(TARGETS syringe_example)
