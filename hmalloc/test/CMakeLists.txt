#
# Copyright (c) 2024 SK hynix, Inc.
#
# SPDX-License-Identifier: BSD 2-Clause
#

find_library(HMALLOC hmalloc HINTS ${PROJECT_SOURCE_DIR})

add_compile_options(-Wall -Wextra -pedantic)

if(HMALLOC_PG_BUILD)
  add_compile_options(-pg)
endif()

# cmake-lint: disable=C0301
add_custom_target(
  catch2 ALL
  COMMAND
    curl -s -k -R -C - -o ${CMAKE_CURRENT_SOURCE_DIR}/catch.hpp
    https://raw.githubusercontent.com/catchorg/Catch2/v2.13.10/single_include/catch2/catch.hpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Fetch catch.hpp")

set(HMALLOC_TEST hmalloc_test)
add_executable(${HMALLOC_TEST} hmalloc_test.cpp main.cpp)
add_dependencies(${HMALLOC_TEST} catch2)

set(CMAKE_INSTALL_RPATH "..")
set(CMAKE_BUILD_WITH_INSTALL_RPATH True)

target_link_libraries(hmalloc_test PUBLIC ${HMALLOC} ${NUMA})
