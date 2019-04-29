cmake_minimum_required(VERSION 3.5)

project(qualisys_cpp_sdk)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
)

add_library(qualisys_cpp_sdk
  Markup.cpp
  Network.cpp
  RTPacket.cpp
  RTProtocol.cpp
)

# example: RigidBodyStreaming
add_executable(RigidBodyStreaming
  RigidBodyStreaming/RigidBodyStreaming.cpp
)
target_link_libraries(RigidBodyStreaming
  qualisys_cpp_sdk
)

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
