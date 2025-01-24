set(tinyxml2_BUILD_TESTING OFF)

include(FetchContent)

# Set CMP0079 policy to NEW to allow linking targets from other directories
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

# Set up TinyXML2 as a FetchContent dependency
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        321ea883b7190d4e85cae5512a12e5eaa8f8731f
)

FetchContent_MakeAvailable(tinyxml2)

message(STATUS "Setting tinyxml2 include directory: ${tinyxml2_SOURCE_DIR}")

include_directories(PUBLIC ${tinyxml2_SOURCE_DIR})
