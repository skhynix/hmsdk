/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "local_allocator.h"

#include <dlfcn.h>

#include "allocator.h"
#include "logging.h"
#include "utils.h"

malloc_func local_malloc_func = nullptr;
mmap_func local_mmap_func = nullptr;
calloc_func local_calloc_func = nullptr;
realloc_func local_realloc_func = nullptr;
posix_memalign_func local_posix_memalign_func = nullptr;
memalign_func local_memalign_func = nullptr;
valloc_func local_valloc_func = nullptr;
aligned_alloc_func local_aligned_alloc_func = nullptr;
free_func local_free_func = nullptr;
malloc_usable_size_func local_malloc_usable_size_func = nullptr;

void init_local_func(void) {
    local_malloc_func = (malloc_func)dlsym(RTLD_NEXT, "malloc");
    local_mmap_func = (mmap_func)dlsym(RTLD_NEXT, "mmap");
    local_calloc_func = (calloc_func)dlsym(RTLD_NEXT, "calloc");
    local_realloc_func = (realloc_func)dlsym(RTLD_NEXT, "realloc");
    local_posix_memalign_func =
        (posix_memalign_func)dlsym(RTLD_NEXT, "posix_memalign");
    local_memalign_func = (memalign_func)dlsym(RTLD_NEXT, "memalign");
    local_valloc_func = (valloc_func)dlsym(RTLD_NEXT, "valloc");
    local_aligned_alloc_func =
        (aligned_alloc_func)dlsym(RTLD_NEXT, "aligned_alloc");
    local_free_func = (free_func)dlsym(RTLD_NEXT, "free");
    local_malloc_usable_size_func =
        (malloc_usable_size_func)dlsym(RTLD_NEXT, "malloc_usable_size");
}

bool set_local_allocator(void) {
    bool ret = false;
    AllocatorTypes allocator{};

    allocator.malloc_new = local_malloc;
    allocator.mmap_new = local_mmap;
    allocator.calloc_new = local_calloc;
    allocator.realloc_new = local_realloc;
    allocator.posix_memalign_new = local_posix_memalign;
    allocator.memalign_new = local_memalign;
    allocator.valloc_new = local_valloc;
    allocator.aligned_alloc_new = local_aligned_alloc;
    allocator.free_new = local_free;
    allocator.malloc_usable_size_new = local_malloc_usable_size;

    ret = set_ce_allocator(allocator);

    return ret;
}

void *local_malloc(size_t size) {
    CE_ASSERT(local_malloc_func != nullptr,
              "local_malloc should have been initialized\n");
    CE_LOG_VERBOSE("call local_malloc: %d\n", size);

    return local_malloc_func(size);
}

void *local_mmap(void *addr, size_t length, int prot, int flags, int fd,
                 off_t offset) {
    CE_ASSERT(local_mmap_func != nullptr,
              "local_mmap should have been initialized\n");
    CE_LOG_VERBOSE("call local_mmap: %d\n", length);
    return local_mmap_func(addr, length, prot, flags, fd, offset);
}

void *local_calloc(size_t nmemb, size_t size) {
    CE_ASSERT(local_calloc_func != nullptr,
              "local_calloc should have been initialized\n");
    CE_LOG_VERBOSE("call local_calloc: %d, %d\n", nmemb, size);

    return local_calloc_func(nmemb, size);
}

void *local_realloc(void *ptr, size_t size) {
    CE_ASSERT(local_realloc_func != nullptr,
              "local_realloc should have been initialized\n");
    CE_LOG_VERBOSE("call local_realloc: %d\n", size);

    return local_realloc_func(ptr, size);
}

int local_posix_memalign(void **memptr, size_t alignment, size_t size) {
    CE_ASSERT(local_posix_memalign_func != nullptr,
              "local_posix_memalign should have been initialized\n");
    CE_LOG_VERBOSE("call local_posix_memalign: %d\n", size);

    return local_posix_memalign_func(memptr, alignment, size);
}

void *local_memalign(size_t alignment, size_t size) {
    CE_ASSERT(local_memalign_func != nullptr,
              "local_memalign should have been initialized\n");
    CE_LOG_VERBOSE("call local_memalign: %d\n", size);

    return local_memalign_func(alignment, size);
}

void *local_valloc(size_t size) {
    CE_ASSERT(local_valloc_func != nullptr,
              "local_valloc should have been initialized\n");
    CE_LOG_VERBOSE("call local_valloc: %d\n", size);

    return local_valloc_func(size);
}

void *local_aligned_alloc(size_t alignment, size_t size) {
    CE_ASSERT(local_aligned_alloc_func != nullptr,
              "local_aligned_alloc should have been initialized\n");
    CE_LOG_VERBOSE("call local_aligned_alloc: %d\n", size);

    return local_aligned_alloc_func(alignment, size);
}

void local_free(void *ptr) {
    CE_ASSERT(local_free_func != nullptr,
              "local_free should have been initialized\n");
    CE_LOG_VERBOSE("call local_free: %16p\n", ptr);
    local_free_func(ptr);
}

size_t local_malloc_usable_size(void *ptr) {
    CE_ASSERT(local_malloc_usable_size_func != nullptr,
              "local_malloc_usable_size should have been initialized\n");

    return local_malloc_usable_size_func(ptr);
}
