# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.12)
project(sssp_benchmark LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fsanitize=address -fsanitize=undefined")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(GraphBLAS REQUIRED)
find_package(LAGraph CONFIG REQUIRED)
find_package(OpenMP REQUIRED)

enable_testing()

option(BUILD_BENCHMARKS "Build benchmark targets" OFF)

add_subdirectory(src)
add_subdirectory(tests)

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
