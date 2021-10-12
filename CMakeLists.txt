cmake_minimum_required(VERSION "3.14")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ns-box LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O2 -fsanitize=address -Wall -Wextra")

# clang-format boilerplate
find_program(
  CLANG_FORMAT_EXE
  NAMES "clang-format"
  DOC "Path to clang-format executable")

if(NOT CLANG_FORMAT_EXE)
  message(STATUS "clang-format not found.")
else()
  message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")

  file(GLOB_RECURSE FILES_TO_FORMAT src/*.cpp src/*.hpp)

  add_custom_target(
    clang-format-project-files COMMAND ${CLANG_FORMAT_EXE} -i -style=LLVM
                                       ${FILES_TO_FORMAT})
endif()

add_subdirectory(src)
