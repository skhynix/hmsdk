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

option(HMALLOC_MANUAL "hmalloc: man" OFF)

option(HMALLOC_TEST "hmalloc: test" OFF)

option(HMALLOC_PG_BUILD "hmalloc: -pg" OFF)
if(HMALLOC_PG_BUILD)
  add_compile_options(-pg)
endif()

option(HMALLOC_ASAN_BUILD "hmalloc: -fsanitize=address" OFF)
if(HMALLOC_ASAN_BUILD)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HMCTL hmctl)
set(HMCTL_SOURCES src/hmctl.c)
add_executable(${HMCTL} ${HMCTL_SOURCES})

set(HMALLOC hmalloc)
set(HMALLOC_SOURCES src/hmalloc.c src/env.c)

find_library(JEMALLOC jemalloc)
if(NOT JEMALLOC)
  message(FATAL_ERROR "jemalloc library not found!")
endif()

find_library(NUMA numa)
if(NOT NUMA)
  message(FATAL_ERROR "numa library not found!")
endif()

add_library(${HMALLOC} SHARED ${HMALLOC_SOURCES})

target_include_directories(
  hmalloc
  PUBLIC include
  PRIVATE src)

target_link_libraries(${HMALLOC} PRIVATE ${JEMALLOC} ${NUMA})

if(HMALLOC_TEST)
  add_subdirectory(test)
endif()

if(HMALLOC_MANUAL)
  add_custom_target(
    man ALL
    COMMAND pandoc -s ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmctl.md -t man -o
            ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmctl.8
    COMMAND pandoc -s ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmalloc.md -t man -o
            ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmalloc.3
    COMMAND pandoc -s ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmalloc_usable_size.md -t
            man -o ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmalloc_usable_size.3
    COMMAND pandoc -s ${CMAKE_CURRENT_SOURCE_DIR}/doc/hposix_memalign.md -t man
            -o ${CMAKE_CURRENT_SOURCE_DIR}/doc/hposix_memalign.3
    COMMAND pandoc -s ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmmap.md -t man -o
            ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmmap.3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating man page")
endif()

install(TARGETS ${HMCTL} DESTINATION bin)
install(TARGETS ${HMALLOC} DESTINATION lib)
install(FILES include/hmalloc.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmctl.8
        DESTINATION share/man/man8)
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmalloc.3
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmalloc_usable_size.3
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/hposix_memalign.3
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/hmmap.3
  DESTINATION share/man/man3)
