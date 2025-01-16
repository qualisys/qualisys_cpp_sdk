include(FetchContent)

FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(tinyxml2)

set(tinyxml2_INCLUDE_DIR "${tinyxml2_SOURCE_DIR}")

message(STATUS "Setting TinyXML2 include directory: ${tinyxml2_INCLUDE_DIR}")

include_directories(${tinyxml2_INCLUDE_DIR})