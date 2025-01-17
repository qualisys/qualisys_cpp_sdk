include(FetchContent)

# Set CMP0079 policy to NEW to allow linking targets from other directories
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

# Set up TinyXML2 as a FetchContent dependency
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        master # Replace 'master' with a specific tag if needed
)

FetchContent_MakeAvailable(tinyxml2)
