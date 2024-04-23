/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "env.h"

#include <assert.h>
#include <errno.h>
#include <jemalloc/jemalloc.h>
#include <numa.h>
#include <numaif.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define __unused __attribute__((unused))

#define is_pow2(val) (((val) & ((val)-1)) == 0)

/* global variables set by environment variables */
static bool use_jemalloc;
static unsigned long nodemask;
static int mpol_mode;

static unsigned arena_index;
static extent_hooks_t *hooks;

static int maxnode;

void *extent_alloc(extent_hooks_t *extent_hooks __unused, void *new_addr, size_t size,
                   size_t alignment __unused, bool *zero __unused, bool *commit __unused,
                   unsigned arena_ind __unused) {
    new_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
    if (unlikely(new_addr == MAP_FAILED))
        return MAP_FAILED;

    if (nodemask > 0) {
        long ret = mbind(new_addr, size, mpol_mode, &nodemask, maxnode, 0);
        if (unlikely(ret)) {
            int mbind_errno = errno;
            munmap(new_addr, size);
            errno = mbind_errno;
            return NULL;
        }
    }
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

void update_env(void) {
    use_jemalloc = getenv_jemalloc();
    nodemask = getenv_nodemask();
    mpol_mode = getenv_mpol_mode();
}

__attribute__((constructor)) void hmalloc_init(void) {
    int err __unused;
    size_t unsigned_size = sizeof(unsigned);

    update_env();

    if (use_jemalloc) {
        maxnode = numa_max_node() + 2;
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

void *hcalloc(size_t nmemb, size_t size) {
    void *ptr = hmalloc(nmemb * size);

    if (likely(ptr))
        memset(ptr, 0, nmemb * size);
    return ptr;
}

void *hrealloc(void *ptr, size_t size) {
    if (!use_jemalloc)
        return realloc(ptr, size);

    if (ptr == NULL)
        return hmalloc(size);

    if (size == 0) {
        hfree(ptr);
        return NULL;
    }
    return rallocx(ptr, size, MALLOCX_ARENA(arena_index) | MALLOCX_TCACHE_NONE);
}

void *haligned_alloc(size_t alignment, size_t size) {
    if (!use_jemalloc)
        return aligned_alloc(alignment, size);

    if (unlikely(!alignment || !size || !is_pow2(alignment) || (size % alignment))) {
        errno = EINVAL;
        return NULL;
    }

    return mallocx(size,
                   MALLOCX_ALIGN(alignment) | MALLOCX_ARENA(arena_index) | MALLOCX_TCACHE_NONE);
}

int hposix_memalign(void **memptr, size_t alignment, size_t size) {
    int old_errno;

    if (!use_jemalloc)
        return posix_memalign(memptr, alignment, size);

    old_errno = errno;

    if (unlikely(!alignment || !size || !is_pow2(alignment) || (size % alignment))) {
        *memptr = NULL;
        return EINVAL;
    }

    *memptr =
        mallocx(size, MALLOCX_ALIGN(alignment) | MALLOCX_ARENA(arena_index) | MALLOCX_TCACHE_NONE);

    if (unlikely(*memptr == NULL)) {
        int ret = errno;
        errno = old_errno;
        return ret;
    }
    return 0;
}

size_t hmalloc_usable_size(void *ptr) {
    if (!use_jemalloc)
        return malloc_usable_size(ptr);

    if (unlikely(ptr == NULL))
        return 0;
    return sallocx(ptr, 0);
}
