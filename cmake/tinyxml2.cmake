set(tinyxml2_BUILD_TESTING OFF)

include(FetchContent)

FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(tinyxml2)

include_directories(PUBLIC ${tinyxml2_SOURCE_DIR})