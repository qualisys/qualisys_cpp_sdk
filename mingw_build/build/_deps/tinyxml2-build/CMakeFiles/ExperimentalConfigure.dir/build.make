# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.31

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = C:\msys64\mingw64\bin\cmake.exe

# The command to remove a file.
RM = C:\msys64\mingw64\bin\cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\qualisys_cpp_sdk

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\qualisys_cpp_sdk\mingw_build\build

# Utility rule file for ExperimentalConfigure.

# Include any custom commands dependencies for this target.
include _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/compiler_depend.make

# Include the progress variables for this target.
include _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/progress.make

_deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure:
	cd /d C:\qualisys_cpp_sdk\mingw_build\build\_deps\tinyxml2-build && C:\msys64\mingw64\bin\ctest.exe -D ExperimentalConfigure

_deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/codegen:
.PHONY : _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/codegen

ExperimentalConfigure: _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure
ExperimentalConfigure: _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/build.make
.PHONY : ExperimentalConfigure

# Rule to build all files generated by this target.
_deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/build: ExperimentalConfigure
.PHONY : _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/build

_deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/clean:
	cd /d C:\qualisys_cpp_sdk\mingw_build\build\_deps\tinyxml2-build && $(CMAKE_COMMAND) -P CMakeFiles\ExperimentalConfigure.dir\cmake_clean.cmake
.PHONY : _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/clean

_deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\qualisys_cpp_sdk C:\qualisys_cpp_sdk\mingw_build\build\_deps\tinyxml2-src C:\qualisys_cpp_sdk\mingw_build\build C:\qualisys_cpp_sdk\mingw_build\build\_deps\tinyxml2-build C:\qualisys_cpp_sdk\mingw_build\build\_deps\tinyxml2-build\CMakeFiles\ExperimentalConfigure.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : _deps/tinyxml2-build/CMakeFiles/ExperimentalConfigure.dir/depend

