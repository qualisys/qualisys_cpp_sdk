cmake_minimum_required(VERSION 3.8)
project(qualisys_cpp_sdk)

option(BUILD_EXAMPLES "Build examples" OFF)

# Core library
add_library(${PROJECT_NAME}
        Markup.cpp
        Network.cpp
        RTPacket.cpp
        RTProtocol.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">:ws2_32.lib>
)

# Enable C++14
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
set_target_properties(${PROJECT_NAME}
        PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX "-d"
)

# ----------- INSTALL & EXPORT -----------

# Add TinyXML2
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(tinyxml2)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # For Windows
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# Install configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${ConfigPackageLocation}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    DESTINATION ${ConfigPackageLocation}
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION ${ConfigPackageLocation}
)

# ----------- EXAMPLES -----------

if (BUILD_EXAMPLES)
    add_subdirectory(RigidBodyStreaming)
    add_subdirectory(RTClientExample)
endif ()
