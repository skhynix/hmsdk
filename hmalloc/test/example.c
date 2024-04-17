/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <hmalloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KiB (1024UL)
#define MiB (1024UL * KiB)

char *p;
char *hp;

int main() {
    size_t hsz = 512;
    size_t sz = 256;

    p = malloc(sz * MiB);
    memset(p, 'x', sz * MiB);

    hp = hmalloc(hsz * MiB);
    memset(hp, 'x', hsz * MiB);

    printf("%ld MiB is allocated by malloc().\n", sz);
    printf("%ld MiB is allocated by hmalloc().\n", hsz);
    printf("Press enter to stop.\n");
    getchar();

    hfree(hp);
    free(p);

    return 0;
}
