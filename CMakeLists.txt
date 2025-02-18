cmake_minimum_required(VERSION 3.8)
project(qualisys_cpp_sdk VERSION 1.0.0)

include(GNUInstallDirs)

option(${PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)
option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)

if(NOT DEFINED ${PROJECT_NAME}_OUTPUT_TYPE)
    set(${PROJECT_NAME}_OUTPUT_TYPE "STATIC")
endif()

if(${PROJECT_NAME}_OUTPUT_TYPE STREQUAL "STATIC")
    message(STATUS "Building a static library.")
    set(LIB_TYPE "STATIC")
    set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
elseif(${PROJECT_NAME}_OUTPUT_TYPE STREQUAL "SHARED")
    message(STATUS "Building a shared library.")
    set(LIB_TYPE "SHARED")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
elseif(${PROJECT_NAME}_OUTPUT_TYPE STREQUAL "SHARED_VERSIONED")
    message(STATUS "Building a versioned shared library.")
    set(LIB_TYPE "SHARED")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    message(FATAL_ERROR "Invalid ${PROJECT_NAME}_OUTPUT_TYPE " ${${PROJECT_NAME}_OUTPUT_TYPE})
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_library(${PROJECT_NAME} ${LIB_TYPE}
        Network.cpp
        RTPacket.cpp
        RTProtocol.cpp
        Settings.cpp
        Serializer.cpp
        Deserializer.cpp
        SettingsDeserializer.cpp
        SettingsSerializer.cpp
        External/tinyxml2/tinyxml2.cpp
)

target_include_directories(${PROJECT_NAME} 
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>"
        PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/External/tinyxml2"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC 
        "$<$<STREQUAL:$<PLATFORM_ID>,Windows>:ws2_32>"
        "$<$<STREQUAL:$<PLATFORM_ID>,Windows>:iphlpapi>"
)

# Enable C++14
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
set_target_properties(${PROJECT_NAME}
        PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

if(LIB_TYPE STREQUAL "SHARED")
    if(WIN32)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    endif()

    if(NOT MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            CMAKE_CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
        )
    endif()

    if (UNIX)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endif()

# Apply versioning suffix if shared and requested
if(${PROJECT_NAME}_OUTPUT_TYPE STREQUAL "SHARED_VERSIONED")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
endif()

# ----------- INSTALL & EXPORT -----------

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

# 'make install' to the correct locations (provided by GNUInstallDirs)
install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" # This is for Windows
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
)

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
    )
endif ()

if(${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
    set(${PROJECT_NAME}_OUTPUT_TYPE ${qualisys_cpp_sdk_OUTPUT_TYPE} CACHE BOOL "qualisys_cpp_sdk build type")
endif()
