# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

CC = g++
INC = -I${CEMALLOC_INCLUDE_PATH}
LIBS = -L${CEMALLOC_PATH} -lcemalloc

all : explicit_example

explicit_example: explicit_example.cc
	$(CC) -o explicit_example explicit_example.cc $(INC) $(LIBS)

clean:
	rm -f explicit_example
