#
# Copyright (c) 2022-2024 SK hynix, Inc.
#
# SPDX-License-Identifier: BSD 2-Clause
#

list(APPEND CEMALLOC_FLAGS "-Wall")
list(APPEND CEMALLOC_FLAGS "-Wextra")
list(APPEND CEMALLOC_FLAGS "-Wsign-compare")
if(${DISTRIBUTION})
  list(APPEND CEMALLOC_FLAGS "-fvisibility=hidden")
endif()
list(JOIN CEMALLOC_FLAGS " " CEMALLOC_FLAGS)
message(STATUS "FLAGS: " ${CEMALLOC_FLAGS})
set(CMAKE_CXX_FLAGS ${CEMALLOC_FLAGS})

set(CEMALLOC_SOURCES
    constructor.cc
    allocator.cc
    local_allocator.cc
    cxl_allocator.cc
    path_allocator.cc
    address_map.cc
    logging.cc
    utils.cc
    operation_mode.cc
    env_parser.cc
    alloc_attr.cc
    explicit_indicator.cc
    syscall_define.cc)

include_directories(${CMAKE_SOURCE_DIR}/cemalloc/include)
include_directories(${CMAKE_SOURCE_DIR}/externals/jemalloc/include/jemalloc)

add_library(cemalloc SHARED ${CEMALLOC_SOURCES})
set_target_properties(cemalloc PROPERTIES VERSION ${CEMALLOC_VERSION})

if(NOT DEFINED JEMALLOC_PREFIX)
  set(JEMALLOC_PREFIX ce)
endif(NOT DEFINED JEMALLOC_PREFIX)
add_definitions(-DJE_PREFIX=${JEMALLOC_PREFIX})

set(JEMALLOC_LIBRARY
    ${CMAKE_SOURCE_DIR}/out/lib/lib${JEMALLOC_PREFIX}malloc_pic.a)
target_link_libraries(cemalloc ${JEMALLOC_LIBRARY} dl stdc++ pthread)
