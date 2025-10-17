#
# Copyright (c) 2025 SK hynix, Inc.
#
# SPDX-License-Identifier: BSD 2-Clause
#

cmake_minimum_required(VERSION 3.14)

project(bwprof LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "RelWithDebInfo")
endif()

add_compile_options(-Wall -Wextra -pedantic)

option(BWPROF_PG_BUILD "bwprof: -pg" OFF)
if(BWPROF_PG_BUILD)
  add_compile_options(-pg)
endif()

option(BWPROF_ASAN_BUILD "bwprof: -fsanitize=address" OFF)
if(BWPROF_ASAN_BUILD)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

add_subdirectory(pcm EXCLUDE_FROM_ALL)

add_executable(bwprof bwprof.cc)

# build and link libpcm.so(PCM_SHARED)
target_link_libraries(bwprof PRIVATE PCM_SHARED)

set_target_properties(
  bwprof PROPERTIES BUILD_RPATH "$ORIGIN/../pcm" # build time
                    INSTALL_RPATH "$ORIGIN/../pcm") # after install
