# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

CC = g++
INC = -I${CEMALLOC_INCLUDE_PATH}
LIBS = -L${CEMALLOC_PATH} -lcemalloc

all : implicit_example

implicit_example: implicit_example.cc
	$(CC) -o implicit_example implicit_example.cc $(INC) $(LIBS)
clean:
	rm -f implicit_example
