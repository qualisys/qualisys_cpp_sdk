cmake_minimum_required(VERSION 3.8)
project(qualisys_cpp_sdk VERSION 1.0.0)

option(${PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)
option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)
option(${PROJECT_NAME}_BUILD_SHARED "Build shared library" OFF)
option(${PROJECT_NAME}_BUILD_SHARED_VERSIONED "Build shared library with version suffix" OFF)

# Default to Release if not set
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build: None, Debug, Release, RelWithDebInfo, MinSizeRel.")
endif()

# Define library type
if(${PROJECT_NAME}_BUILD_SHARED OR ${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Enable Position Independent Code for shared libraries
if(${PROJECT_NAME}_BUILD_SHARED OR ${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# TinyXML2
set(tinyxml2_BUILD_TESTING OFF)
if(${PROJECT_NAME}_BUILD_SHARED OR ${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set(tinyxml2_SHARED_LIBS ON)
else()
    set(tinyxml2_SHARED_LIBS OFF)
endif()
add_subdirectory(external/tinyxml2)

add_library(${PROJECT_NAME} ${LIB_TYPE}
        Network.cpp
        RTPacket.cpp
        RTProtocol.cpp
        Settings.cpp
        Serializer.cpp
        Deserializer.cpp
        SettingsDeserializer.cpp
        SettingsSerializer.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>
)

# Windows-specific linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC 
            $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">:ws2_32.lib iphlpapi.lib>
            $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","GNU">:ws2_32 iphlpapi>
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml2::tinyxml2)

# Enable C++14
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
set_target_properties(${PROJECT_NAME}
        PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Apply versioning suffix if shared and requested
if(${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
endif()

# ----------- INSTALL & EXPORT -----------

include(GNUInstallDirs)

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${ConfigPackageLocation}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${ConfigPackageLocation}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
		DESTINATION ${ConfigPackageLocation}
)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # This is for Windows
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# Copy along headers
set(INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
install(
        CODE "file( GLOB HEADERS \"${CMAKE_CURRENT_SOURCE_DIR}/*.h*\" )"
        CODE "file( INSTALL \${HEADERS} DESTINATION \"${INSTALL_INCLUDE_DIR}\" )"
)

# ----------- EXAMPLES -----------

# example: RigidBodyStreaming
if (${PROJECT_NAME}_BUILD_EXAMPLES)
    add_executable(RigidBodyStreaming
            RigidBodyStreaming/RigidBodyStreaming.cpp
    )
    target_link_libraries(RigidBodyStreaming
            qualisys_cpp_sdk
            tinyxml2
    )
endif ()

if(${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
