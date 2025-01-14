add_executable(RTClientExample
    Input.cpp
    Operations.cpp
    Output.cpp
    OutputSettings.cpp
    RTClientExample.cpp
)

set_target_properties(RTClientExample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(RTClientExample PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(RTClientExample
    PRIVATE qualisys_cpp_sdk tinyxml2::tinyxml2 # maybe not needed
)
