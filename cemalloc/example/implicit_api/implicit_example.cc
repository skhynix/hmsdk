/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cemalloc.h>

#define SIZE 1024 * 1024 * 1024

// Please check essential environment variable setting in implicit/env.sh
int main() {
    char *ptr;

    ptr = (char *)malloc(SIZE);
    memset(ptr, 0, SIZE);

    free(ptr);

    return 0;
}
