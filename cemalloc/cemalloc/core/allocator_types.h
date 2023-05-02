/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_ALLOCATOR_TYPES_H_
#define _CEMALLOC_CORE_ALLOCATOR_TYPES_H_

#include <stddef.h>
#include <sys/types.h>

typedef void *(*mmap_func)(void *addr, size_t length, int prot, int flags,
                           int fd, off_t offset);

typedef void *(*malloc_func)(size_t size);
typedef void *(*calloc_func)(size_t nmemb, size_t size);
typedef void *(*realloc_func)(void *ptr, size_t size);
typedef int (*posix_memalign_func)(void **memptr, size_t alignment,
                                   size_t size);
typedef void *(*memalign_func)(size_t alignment, size_t size);
typedef void *(*valloc_func)(size_t size);
typedef void *(*aligned_alloc_func)(size_t alignment, size_t size);
typedef void (*free_func)(void *ptr);
typedef size_t (*malloc_usable_size_func)(void *ptr);
/*
 * We might need to consider adding malloc_stats()
 * and malloc_usable_size(). Or do we need more?
 */

/**
 * @struct AllocatorTypes
 * @brief allocator function pointer.
 */
struct AllocatorTypes {
    /// malloc_new malloc function pointer
    malloc_func malloc_new;
    /// mmap_new mmap function pointer
    mmap_func mmap_new;
    /// calloc_new calloc function pointer
    calloc_func calloc_new;
    /// realloc_new realloc function pointer
    realloc_func realloc_new;
    /// posix_memalign_new posix_memalign function pointer
    posix_memalign_func posix_memalign_new;
    /// memalign_new memalign function pointer
    memalign_func memalign_new;
    /// valloc_new valloc function pointer
    valloc_func valloc_new;
    /// aligned_alloc_new aligned_alloc function pointer
    aligned_alloc_func aligned_alloc_new;
    /// free_new free function pointer
    free_func free_new;
    /// malloc_usable_size_new malloc_usable_size function pointer
    malloc_usable_size_func malloc_usable_size_new;
};

#endif
