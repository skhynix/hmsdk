/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <numaif.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool getenv_jemalloc(void) {
    char *env = getenv("HMALLOC_JEMALLOC");

    if (env && !strcmp(env, "1"))
        return true;
    return false;
}

unsigned long getenv_nodemask(void) {
    char *env = getenv("HMALLOC_NODEMASK");

    if (!env)
        return 0;
    return atol(env);
}

int getenv_mpol_mode(void) {
    char *env = getenv("HMALLOC_MPOL_MODE");

    if (!env)
        return MPOL_DEFAULT;
    return atoi(env);
}
