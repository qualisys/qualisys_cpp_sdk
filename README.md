# Qualisys Realtime SDK

C++ package with SDK and examples

## Build with Visual Studio

Build RTClientSDK solution in Visual Studio 2017 (or later version).

## Build with CMake (Windows & Linux)

* Tested with GCC 13.3.0
* Tested with Clang 18.1.3
* Tested with VS 2017 / MSVC 19.42

### Cloning the Repository

`git clone https://github.com/qualisys/qualisys_cpp_sdk.git`

### Build As Static
```
cmake -S . -B build -Dqualisys_cpp_sdk_OUTPUT_TYPE=STATIC && cmake --build build
```

### Build As Shared/Dynamic
```
cmake -S . -B build -Dqualisys_cpp_sdk_OUTPUT_TYPE=SHARED && cmake --build build
```

### Build As Shared/Dynamic (With Versioning)
```
cmake -S . -B build -Dqualisys_cpp_sdk_OUTPUT_TYPE=SHARED_VERSIONED && cmake --build build
```

### Build Examples
```
cmake -S . -B build -Dqualisys_cpp_sdk_BUILD_EXAMPLES=ON && cmake --build build
```

### Build & Run Tests
```
cmake -S . -B build -Dqualisys_cpp_sdk_BUILD_TESTS=ON && cmake --build build
ctest --test-dir build
```

## Usage

Include the Qualisys SDK in your CMake application:

```cmake
find_package(qualisys_cpp_sdk REQUIRED)

target_link_libraries(myapplication PRIVATE qualisys_cpp_sdk)
```