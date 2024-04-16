/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <assert.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define __unused __attribute__((unused))

static bool use_jemalloc;

static unsigned arena_index;
static extent_hooks_t *hooks;

void *extent_alloc(extent_hooks_t *extent_hooks __unused, void *new_addr, size_t size,
                   size_t alignment __unused, bool *zero __unused, bool *commit __unused,
                   unsigned arena_ind __unused) {
    new_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
    return new_addr;
}

static bool extent_dalloc(extent_hooks_t *extent_hooks __unused, void *addr, size_t size,
                          bool committed __unused, unsigned arena_ind __unused) {
    return munmap(addr, size);
}

static extent_hooks_t extent_hooks = {
    .alloc = extent_alloc,
    .dalloc = extent_dalloc,
};

__attribute__((constructor)) void hmalloc_init(void) {
    int err __unused;
    size_t unsigned_size = sizeof(unsigned);
    char *env_jemalloc = getenv("HMALLOC_JEMALLOC");

    if (env_jemalloc && !strcmp(env_jemalloc, "1")) {
        use_jemalloc = true;

        hooks = &extent_hooks;
        err = mallctl("arenas.create", &arena_index, &unsigned_size, (void *)&hooks,
                      sizeof(extent_hooks_t *));
        assert(!err);
    }
}

void *hmalloc(size_t size) {
    void *ptr;

    if (!use_jemalloc)
        return malloc(size);

    ptr = mallocx(size, MALLOCX_ARENA(arena_index) | MALLOCX_TCACHE_NONE);
    if (errno == ENOMEM)
        return NULL;
    return ptr;
}

void hfree(void *ptr) {
    if (unlikely(ptr == NULL))
        return;
    if (!use_jemalloc) {
        free(ptr);
        return;
    }
    dallocx(ptr, MALLOCX_ARENA(arena_index) | MALLOCX_TCACHE_NONE);
}
