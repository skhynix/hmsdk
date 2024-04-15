/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <stddef.h>
#include <stdlib.h>

void *hmalloc(size_t size) {
    return malloc(size);
}

void hfree(void *ptr) {
    free(ptr);
}
