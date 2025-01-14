add_executable(RigidBodyStreaming
    RigidBodyStreaming.cpp
)

set_target_properties(RigidBodyStreaming PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(RigidBodyStreaming PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(RigidBodyStreaming
    PRIVATE qualisys_cpp_sdk tinyxml2::tinyxml2 # maybe not needed
)
