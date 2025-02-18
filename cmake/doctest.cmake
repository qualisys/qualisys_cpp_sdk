add_definitions(-DENABLE_DOCTEST_IN_LIBRARY)

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY "https://github.com/onqtam/doctest"
    GIT_TAG "ae7a13539fb71f270b87eb2e874fbac80bc8dda2"
)

FetchContent_MakeAvailable(doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

message(STATUS "Setting doctest include directory: ${doctest_SOURCE_DIR}")

include_directories(PUBLIC ${doctest_SOURCE_DIR})

