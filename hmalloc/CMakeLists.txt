#
# Copyright (c) 2024 SK hynix, Inc.
#
# SPDX-License-Identifier: BSD 2-Clause
#

cmake_minimum_required(VERSION 3.14)

project(hmalloc)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "RelWithDebInfo")
endif()

add_compile_options(-Wall -Wextra -pedantic)

option(HMALLOC_TEST "hmalloc: test" OFF)

option(HMALLOC_PG_BUILD "hmalloc: -pg" OFF)
if(HMALLOC_PG_BUILD)
  add_compile_options(-pg)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HMALLOC hmalloc)
set(HMALLOC_SOURCES src/hmalloc.c)

add_library(${HMALLOC} SHARED ${HMALLOC_SOURCES})

target_include_directories(
  hmalloc
  PUBLIC include
  PRIVATE src)

target_link_libraries(${HMALLOC} PRIVATE)

if(HMALLOC_TEST)
  add_subdirectory(test)
endif()
