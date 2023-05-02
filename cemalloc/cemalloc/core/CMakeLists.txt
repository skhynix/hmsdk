# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

list(APPEND cemalloc_flags "-Wall")
list(APPEND cemalloc_flags "-Wextra")
list(APPEND cemalloc_flags "-Wsign-compare")
if(${DISTRIBUTION})
  list(APPEND cemalloc_flags "-fvisibility=hidden")
endif()
list(JOIN cemalloc_flags " " cemalloc_flags)
message(STATUS "FLAGS: " ${cemalloc_flags})
set(CMAKE_CXX_FLAGS ${cemalloc_flags})

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
set_target_properties(cemalloc PROPERTIES VERSION ${VERSION_STRING})

if(NOT DEFINED JEMALLOC_PREFIX)
  set(JEMALLOC_PREFIX ce)
endif(NOT DEFINED JEMALLOC_PREFIX)
add_definitions(-DJE_PREFIX=${JEMALLOC_PREFIX})

set(JEMALLOC_LIBRARY
    ${CMAKE_SOURCE_DIR}/out/lib/lib${JEMALLOC_PREFIX}malloc_pic.a)
target_link_libraries(cemalloc ${JEMALLOC_LIBRARY} dl stdc++ pthread)
