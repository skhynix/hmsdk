/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool getenv_jemalloc(void) {
    char *env = getenv("HMALLOC_JEMALLOC");

    if (env && !strcmp(env, "1"))
        return true;
    return false;
}
