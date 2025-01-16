add_definitions(-DENABLE_DOCTEST_IN_LIBRARY)

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY "https://github.com/onqtam/doctest"
    GIT_TAG "932a2ca50666138256dae56fbb16db3b1cae133a"
)

FetchContent_MakeAvailable(doctest)

set(doctest_INCLUDE_DIR "${doctest_SOURCE_DIR}/doctest")

message(STATUS "Setting doctest include directory: ${doctest_INCLUDE_DIR}")

include_directories(${doctest_INCLUDE_DIR})