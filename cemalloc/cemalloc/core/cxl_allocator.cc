/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "cxl_allocator.h"

#include <cstring> // strerror
#include <dlfcn.h>
#include <sys/mman.h>

#include "jemalloc.h"

#include "alloc_attr.h"
#include "allocator.h"
#include "allocator_types.h"
#include "cemalloc.h"
#include "env_parser.h"
#include "local_allocator.h"
#include "logging.h"
#include "syscall_define.h"
#include "utils.h"

#ifdef JE_PREFIX
#define PREFIX JE_PREFIX
#endif

#define CXL_FUNC_I(x, y) x##y
#define CXL_FUNC(x, y) CXL_FUNC_I(x, y)

malloc_func cxl_malloc_func = (malloc_func)CXL_FUNC(PREFIX, _je_malloc);
calloc_func cxl_calloc_func = (calloc_func)CXL_FUNC(PREFIX, _je_calloc);
realloc_func cxl_realloc_func = (realloc_func)CXL_FUNC(PREFIX, _je_realloc);
posix_memalign_func cxl_posix_memalign_func =
    (posix_memalign_func)CXL_FUNC(PREFIX, _je_posix_memalign);
memalign_func cxl_memalign_func = (memalign_func)CXL_FUNC(PREFIX, _je_memalign);
valloc_func cxl_valloc_func = (valloc_func)CXL_FUNC(PREFIX, _je_valloc);
aligned_alloc_func cxl_aligned_alloc_func =
    (aligned_alloc_func)CXL_FUNC(PREFIX, _je_aligned_alloc);
free_func cxl_free_func = (free_func)CXL_FUNC(PREFIX, _je_free);
malloc_usable_size_func cxl_malloc_usable_size_func =
    (malloc_usable_size_func)CXL_FUNC(PREFIX, _je_malloc_usable_size);

bool set_cxl_allocator(void) {
    bool ret = false;
    AllocatorTypes allocator{};

    allocator.malloc_new = cxl_malloc;
    allocator.mmap_new = cxl_mmap;
    allocator.calloc_new = cxl_calloc;
    allocator.realloc_new = cxl_realloc;
    allocator.posix_memalign_new = cxl_posix_memalign;
    allocator.memalign_new = cxl_memalign;
    allocator.valloc_new = cxl_valloc;
    allocator.aligned_alloc_new = cxl_aligned_alloc;
    allocator.free_new = cxl_free;
    allocator.malloc_usable_size_new = cxl_malloc_usable_size;

    ret = set_ce_allocator(allocator);

    return ret;
}

static void *cxl_mmap_impl(void *addr, size_t length, int prot, int flags,
                           int fd, off_t offset) {
    void *mmap_addr = nullptr;
    long mbind_ret = 0;
    long mrange_ret = 0;
    uint64_t max_node = GetMaxNode() + 1;
    int mode = 0;
    AllocAttr attr = AllocAttrHandler::GetAllocAttr();
    CeNodeMask nodemask = attr.interleave_node;

    mmap_addr = local_mmap(addr, length, prot, flags, fd, offset);
    if (unlikely(mmap_addr == MAP_FAILED))
        return mmap_addr;

    switch (attr.alloc) {
    case CE_ALLOC_HOST:
        return mmap_addr;
    case CE_ALLOC_CXL:
        mode = MPOL_PREFERRED;
        break;
    case CE_ALLOC_BWAWARE:
        mode = MPOL_INTERLEAVE_WEIGHT | MPOL_F_AUTO_WEIGHT;
        break;
    case CE_ALLOC_USERDEFINED:
        mode = MPOL_INTERLEAVE_WEIGHT;
        break;
    }

    mbind_ret = ce_mbind(mmap_addr, length, mode, &nodemask, max_node, 0);
    if (unlikely(mbind_ret != 0))
        CE_LOG_ERROR("ce_mbind failed.\n");

    if (attr.alloc == CE_ALLOC_USERDEFINED) {
        mrange_ret = mrange_node_weight(
            mmap_addr, length, attr.interleave_node_weight, max_node, 0);
        if (unlikely(mrange_ret != 0))
            CE_LOG_ERROR("mrange_node_weight failed\n");
    }

    return mmap_addr;
}

void *cxl_malloc(size_t size) {
    CE_ASSERT(cxl_malloc_func != nullptr,
              "cxl_malloc should have been initialized\n");

    return cxl_malloc_func(size);
}

void *cxl_mmap(void *addr, size_t length, int prot, int flags, int fd,
               off_t offset) {
    return cxl_mmap_impl(addr, length, prot, flags, fd, offset);
}

void *cxl_calloc(size_t nmemb, size_t size) {
    CE_ASSERT(cxl_calloc_func != nullptr,
              "cxl_calloc should have been initialized\n");

    return cxl_calloc_func(nmemb, size);
}

void *cxl_realloc(void *ptr, size_t size) {
    CE_ASSERT(cxl_realloc_func != nullptr,
              "cxl_realloc should have been initialized\n");

    return cxl_realloc_func(ptr, size);
}

int cxl_posix_memalign(void **memptr, size_t alignment, size_t size) {
    CE_ASSERT(cxl_posix_memalign_func != nullptr,
              "cxl_posix_memalign should have been initialized\n");

    return cxl_posix_memalign_func(memptr, alignment, size);
}

void *cxl_memalign(size_t alignment, size_t size) {
    CE_ASSERT(cxl_memalign_func != nullptr,
              "cxl_memalign should have been initialized\n");

    return cxl_memalign_func(alignment, size);
}

void *cxl_valloc(size_t size) {
    CE_ASSERT(cxl_valloc_func != nullptr,
              "cxl_valloc should have been initialized\n");

    return cxl_valloc_func(size);
}

void *cxl_aligned_alloc(size_t alignment, size_t size) {
    CE_ASSERT(cxl_aligned_alloc_func != nullptr,
              "cxl_aligned_alloc should have been initialized\n");

    return cxl_aligned_alloc_func(alignment, size);
}

void cxl_free(void *ptr) {
    CE_ASSERT(cxl_free_func != nullptr,
              "cxl_free should have been initialized\n");
    CE_LOG_VERBOSE("call cxl_free: %16p\n", ptr);
    cxl_free_func(ptr);
}

size_t cxl_malloc_usable_size(void *ptr) {
    CE_ASSERT(cxl_malloc_usable_size_func != nullptr,
              "cxl_malloc_usable_size should have been initialized\n");

    return cxl_malloc_usable_size_func(ptr);
}
