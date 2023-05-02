# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

cmake_minimum_required(VERSION 3.14)

project(
  CEMALLOC_JAVA
  VERSION 1.1
  LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT DEFINED ENV{JAVA_HOME})
  message(FATAL_ERROR "You must set $JAVA_HOME")
endif()

set(JAVA_HOME $ENV{JAVA_HOME})
set(CEMALLOC_ROOT ${CMAKE_SOURCE_DIR}/../../../)
set(JNI_SOURCE cemalloc_jni.cc)

include_directories(${JAVA_HOME}/include)
include_directories(${JAVA_HOME}/include/linux)
include_directories(${CEMALLOC_ROOT}/cemalloc/core)
include_directories(${CEMALLOC_ROOT}/cemalloc/include)
link_directories(${CEMALLOC_ROOT}/out/cemalloc/core/)

add_library(cemallocjava SHARED ${JNI_SOURCE})
target_link_libraries(cemallocjava cemalloc)
