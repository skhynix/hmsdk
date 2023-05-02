#!/usr/bin/env bash
# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

# replace below CEMALLOC_PATH, CEMALLOC_LIBRARY_PATH with your path
export CEMALLOC_PATH=/path/to/cemalloc/package
export LD_LIBRARY_PATH=${CEMALLOC_PATH}

# set JAVA_HOME and CLASSPATH
export JAVA_HOME=/path/to/java
export CLASSPATH=$(dirname $(realpath $0)):${CEMALLOC_PATH}/cemalloc.jar

export CE_MODE=CE_EXPLICIT_INDICATOR

# enable among (CXL, BWAWARE, USERDEFINED) according to your usage
# CXL
#export CE_CXL_NODE=2
#export CE_ALLOC=CE_ALLOC_CXL

# BWAWARE
# export CE_CXL_NODE=2
# export CE_ALLOC=CE_ALLOC_BWAWARE

# USERDEFINED
# export CE_ALLOC=CE_ALLOC_USERDEFINED
# export CE_INTERLEAVE_NODE=2,3
