# Qualisys Realtime SDK

C++ package with SDK and examples

## Build with Visual Studio

Build RTClientSDK solution in Visual Studio 2017.

## Build with CMake (Windows & Linux)

* Tested with GCC 7.
* Tested with VS 2017.

### Cloning the Repository

This project uses Git submodules. Ensure submodules are initialized after cloning:

`git clone --recursive <repository-url>`

If already cloned, initialize and update submodules:

`git submodule update --init --recursive`

### Build Examples
```
cmake -S . -B build -Dqualisys_cpp_sdk_BUILD_EXAMPLES=ON
cmake --build build
```

### Build & Run Tests
```cmake
cmake -S . -B build -Dqualisys_cpp_sdk_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## Usage

Include the Qualisys SDK in your CMake application:

```cmake
find_package(qualisys_cpp_sdk REQUIRED)

target_link_libraries(myapplication PRIVATE qualisys_cpp_sdk)
```