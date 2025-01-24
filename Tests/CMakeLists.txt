cmake_minimum_required(VERSION 3.27.1)

project(qualisys_cpp_sdk_tests LANGUAGES CXX)
    
include(TinyXml2)
include(Doctest)

set(SOURCE_LIST
    ${PROJECT_SOURCE_DIR}/Main.cpp
    ${PROJECT_SOURCE_DIR}/RTProtocolXmlTests.cpp
    ${PROJECT_SOURCE_DIR}/RTProtocolSkeletonSettingsXmlTests.cpp
    ${PROJECT_SOURCE_DIR}/GazeVectorParameterXmlTests.cpp
    ${PROJECT_SOURCE_DIR}/CalibrationXmlTests.cpp

)

add_executable(
    ${PROJECT_NAME}
    ${SOURCE_LIST}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        qualisys_cpp_sdk
        doctest::doctest
        tinyxml2::tinyxml2
)
    
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

doctest_discover_tests(${PROJECT_NAME})
