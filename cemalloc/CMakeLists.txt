# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

cmake_minimum_required(VERSION 3.14)

project(
  CEMALLOC
  VERSION 1.1
  LANGUAGES CXX)

if(NOT DEFINED LOG_LEVEL)
  set(LOG_LEVEL 1)
endif()
if(NOT DEFINED DISTRIBUTION)
  set(DISTRIBUTION 1)
endif()

add_definitions(-DLOG_LEVEL=${LOG_LEVEL})
add_definitions(-DDISTRO=0)
if(CMAKE_BUILD_TYPE STREQUAL "debug")
  add_definitions(-DCEASSERT)
endif()

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(cemalloc/core)

set(TESTCASES ${CMAKE_CURRENT_SOURCE_DIR}/tests)
if(NOT ${DISTRIBUTION}
   AND EXISTS ${TESTCASES}
   AND IS_DIRECTORY ${TESTCASES})
  add_subdirectory(tests)
endif()
