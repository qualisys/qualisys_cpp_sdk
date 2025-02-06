# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-src")
  file(MAKE_DIRECTORY "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-src")
endif()
file(MAKE_DIRECTORY
  "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-build"
  "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix"
  "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix/tmp"
  "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix/src/tinyxml2-populate-stamp"
  "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix/src"
  "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix/src/tinyxml2-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix/src/tinyxml2-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/qualisys_cpp_sdk/mingw_build/build/_deps/tinyxml2-subbuild/tinyxml2-populate-prefix/src/tinyxml2-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
