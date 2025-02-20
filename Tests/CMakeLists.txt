cmake_minimum_required(VERSION 3.8)

project(qualisys_cpp_sdk_tests LANGUAGES CXX)

include(doctest)

set(SOURCE_LIST
    ${CMAKE_SOURCE_DIR}/External/tinyxml2/tinyxml2.cpp
    ${PROJECT_SOURCE_DIR}/Main.cpp
    ${PROJECT_SOURCE_DIR}/CalibrationParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/EyeTrackerParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/GazeVectorParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/GeneralParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/ImageParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/SkeletonParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/3dParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/6dParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/AnalogParametersTests.cpp
    ${PROJECT_SOURCE_DIR}/ForceParametersTests.cpp
)

add_executable(
    ${PROJECT_NAME}
    ${SOURCE_LIST}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/External/tinyxml2
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        qualisys_cpp_sdk
        doctest::doctest
)

# Ensure if shared library, that it's found at runtime
if(qualisys_cpp_sdk_BUILD_SHARED OR qualisys_cpp_sdk_BUILD_SHARED_VERSIONED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QUALISYS_SDK_SHARED)

    if (UNIX)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "${CMAKE_BINARY_DIR}"
            INSTALL_RPATH "$ORIGIN"
        )
    elseif(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:qualisys_cpp_sdk>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

doctest_discover_tests(${PROJECT_NAME})
