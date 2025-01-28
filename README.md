# Qualisys Realtime SDK

C++ package with SDK and examples

## Build with Visual Studio

Build RTClientSDK solution in Visual Studio 2017.

## Build with CMake (Windows & Linux)

* _Tested with GCC 7._
* _Tested with VS 2017._

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

Include the Qualisys SDK in your cmake application with:

```cmake
find_package(qualisys_cpp_sdk REQUIRED)

target_link_libraries(myapplication
        qualisys_cpp_sdk)
```
