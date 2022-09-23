# Syringe 💉
A cmake-based tool for crossplatform file embedding into C++ sources. A constexpr interface allows for zero-overhead file retrieval from a simple map-like interface.

## Installation
Pre-built binaries of Syringe are available on the "Releases" page. Copy the binary file to a directory where it can be found by CMake. We recommend placing it in `~/.local/bin` on all systems, creating the directory and adding it to PATH, if necessary. Alternatively, you can build and install the binary manually.

### Manual build
Prerequesites:
 - A C++ compiler with support for C++20, such as [Clang](https://clang.llvm.org/);
 - An up-to-date version of [CMake](https://cmake.org/) (minimum 3.17, recommended 3.23);
 - [Ninja](https://ninja-build.org/) build system is recommended.

Clone or download this project. For your convenience, Syringe provides a CMakepresets file, which already specifies the settings for building the project. Open the root folder in terminal, conifure, build and install the project by running these commands:
```sh
cmake --preset default
cmake --build build     # Add "--config Release" for Visual Studio.
cmake --install build   # Add "--prefix <dir>" for a custom directory.
```
On Windows, add the install directory to PATH.

## Usage
Although the syringe binary has a simple interface that can be use from the command line, if your project uses CMake, we recommend the CMake interface.

### CMake interface
Copy the `syringe.cmake` file into your project, and include it in your CMakeLists:
```cmake
include(syringe.cmake)
```
This file provides two functions: `inject_files` and `target_inject_files`. Either one can be used depending on which one is more convenient for your project.

```
target_inject_files(<target>
    FILES [files...]
	OUTPUT <output>
	[VARIABLE <variable>]
	[PREFIX <prefix>]
	[RELATIVE <relative>]	
)
```
`target_inject_files` adds a pre-build step for `<target>`. Files specified in `<files>` are embedded into a header file specified by `<output>`. The output file is automatically added as a source dependency of the target, and can be included by the path that was specified in the parameter (including any directories, if they were specified).

Optional parameter `<variable>` overrides the default variable name for the compile-time map that stores the embedded files (default is `resources`). This parameter can be a nested name (e.g. `my_namespace::assets`), in which the necessary namespaces will be created. `<prefix>` and `<relative>` add and remove common prefixes from embedded files\` names.


See the `examples` folder for example usage of this function.
```
inject_files(
    FILES [files...]
	OUTPUT <output>
	[VARIABLE <variable>]
	[PREFIX <prefix>]
	[RELATIVE <relative>]	
)
```
`inject_files` has an interface almost exactly the same as `target_inject_files`, but the generated embedding command does not automatically bind to any target. This has the following implications:
1. Since no target depends on the existence of the output, this command may run after the build step of your target. Add the output as a dependency using [`target_sources`](https://cmake.org/cmake/help/latest/command/target_sources.html).
2. For the same reason the user is responsible for adding the output file to the search path of the compiler. This can be done using [`target_include_directories`](https://cmake.org/cmake/help/latest/command/target_include_directories.html).
3. Unlike `target_inject_files` which places files in a predetermined directory, `inject_files` accepts an absolute path to the output. A relative path will be interpreted as relative to [`CMAKE_CURRENT_BINARY_DIR`](https://cmake.org/cmake/help/latest/variable/CMAKE_CURRENT_BINARY_DIR.html).

In summary, `inject_files` is the "raw" version of `target_inject_files`, which gives greater control of the targets to the user.

### Commandline interface
When not using CMake, you can directly call the `syringe` binary to create the header file. The options are similar to CMake, and can be inspected with `syringe --help`.

## C++ interface
After embedding the files, a header file with their binary contents is created. To use these files you need to include the header from anywhere in your project and access the object you specified during injecting (default is `resources`). This object has a map-like interface. You can iterate over it, call `size()`, and retrieve file contents with `operator[]`.
```c++
#include <resources.hpp>

constexpr span<const uint8_t> pic_of_a_dog = resources["dog.jpg"];
```
A `span<const uint8_t>` can be easily transformed to `const unsigned char*`, `std::byte` span or any other format of binary data that you prefer.
