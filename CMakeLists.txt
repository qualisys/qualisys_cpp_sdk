cmake_minimum_required(VERSION 3.5)
include(GNUInstallDirs)

option(BUILD_EXAMPLES "Build examples" OFF)

project(qualisys_cpp_sdk)

add_library(qualisys_cpp_sdk
  Markup.cpp
  Network.cpp
  RTPacket.cpp
  RTProtocol.cpp
)
target_include_directories( ${PROJECT_NAME} 
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries( ${PROJECT_NAME}
    PRIVATE $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">:ws2_32.lib>
)
# Enable C++11
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
set_target_properties(${PROJECT_NAME} 
    PROPERTIES 
    	CXX_STANDARD_REQUIRED ON
    	CXX_EXTENSIONS OFF
        DEBUG_POSTFIX "-d"
)

# ----------- INSTALL & EXPORT -----------

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-targets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR} # This is for Windows
)
# Copy along headers
set(INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
install(
    CODE "file( GLOB HEADERS \"${CMAKE_CURRENT_SOURCE_DIR}/*.h*\" )"
    CODE "file( INSTALL \${HEADERS} DESTINATION \"${INSTALL_INCLUDE_DIR}\" )"
)

# example: RigidBodyStreaming
if(BUILD_EXAMPLES)
    add_executable(RigidBodyStreaming
        RigidBodyStreaming/RigidBodyStreaming.cpp
    )
    target_link_libraries(RigidBodyStreaming
        qualisys_cpp_sdk
    )
endif()
# # example: RTClientExample
# add_executable(RTClientExample
#   RTClientExample/Input.cpp
#   RTClientExample/Operations.cpp
#   RTClientExample/Output.cpp
#   RTClientExample/OutputSettings.cpp
# )
# target_link_libraries(RTClientExample
#   qualisys_cpp_sdk
# )
