cmake_minimum_required(VERSION 3.8)
project(qualisys_cpp_sdk VERSION 1.0.0)

option(${PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)
option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)
option(${PROJECT_NAME}_BUILD_SHARED "Build shared library" OFF)
option(${PROJECT_NAME}_BUILD_SHARED_VERSIONED "Build shared library with version suffix" OFF)

# Define library type
if(${PROJECT_NAME}_BUILD_SHARED OR ${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Ensure TinyXML2 is built correctly (STATIC or SHARED)
set(tinyxml2_BUILD_TESTING OFF)
if(${PROJECT_NAME}_BUILD_SHARED OR ${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set(tinyxml2_SHARED ON)
else()
    set(tinyxml2_SHARED OFF)
endif()
add_subdirectory(external/tinyxml2)

# Enable Position Independent Code for shared libraries
if(${PROJECT_NAME}_BUILD_SHARED OR ${PROJECT_NAME}_BUILD_SHARED_VERSIONED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

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

# ----------- TESTS (Ensure Doctest is Not Broken) -----------
if(${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
