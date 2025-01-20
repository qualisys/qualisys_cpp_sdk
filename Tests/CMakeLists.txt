cmake_minimum_required(VERSION 3.27.1)

option(BUILD_TESTS "Build tests" OFF)

# if (BUILD_TESTS)
    project(qualisys_cpp_sdk_tests LANGUAGES CXX)
    
    include(TinyXml2)
    include(Doctest)

    set(SOURCE_LIST
       ${PROJECT_SOURCE_DIR}/Main.cpp
       ${PROJECT_SOURCE_DIR}/RTProtocolXmlTests.cpp
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

    enable_testing()
    doctest_discover_tests(${PROJECT_NAME})
# endif()
