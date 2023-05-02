/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "path_allocator.h"

#include <cstring>
#include <dlfcn.h>

#include "cemalloc.h"
#include "logging.h"
#include "utils.h"

#include "address_map.h"
#include "allocator.h"
#include "cxl_allocator.h"
#include "local_allocator.h"
#include "operation_mode.h"

bool set_path_allocator(void) {
    bool ret = false;
    AllocatorTypes allocator{};

    allocator.malloc_new = path_malloc;
    allocator.mmap_new = path_mmap;
    allocator.calloc_new = path_calloc;
    allocator.realloc_new = path_realloc;
    allocator.posix_memalign_new = path_posix_memalign;
    allocator.memalign_new = path_memalign;
    allocator.valloc_new = path_valloc;
    allocator.aligned_alloc_new = path_aligned_alloc;
    allocator.free_new = path_free;
    allocator.malloc_usable_size_new = path_malloc_usable_size;

    ret = set_ce_allocator(allocator);

    return ret;
}

void *path_malloc(size_t size) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("call path_malloc: %d\n", size);
    if (CeModeHandler::IsLibcPath()) {
        addr = local_malloc(size);
    } else {
        addr = cxl_malloc(size);
        if (addr != nullptr) {
            AddressMap::Push((uintptr_t)addr);
        }
    }

    return addr;
}

void *path_mmap(void *addr, size_t length, int prot, int flags, int fd,
                off_t offset) {
    CE_LOG_VERBOSE("call path_mmap: %d\n", length);
    if (CeModeHandler::IsLibcPath()) {
        return local_mmap(addr, length, prot, flags, fd, offset);
    }
    return cxl_mmap(addr, length, prot, flags, fd, offset);
}

void *path_calloc(size_t nmemb, size_t size) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("call path_calloc: %d, %d\n", nmemb, size);
    if (CeModeHandler::IsLibcPath()) {
        addr = local_calloc(nmemb, size);
    } else {
        addr = cxl_calloc(nmemb, size);
        if (addr != nullptr) {
            AddressMap::Push((uintptr_t)addr);
        }
    }

    return addr;
}

/**
 * @brief Check the each value of CeAllocPath at the time realloc is called
 * (current) and ptr is allocated (past).\n
 * If it's different, new memory is allocated according to the current
 * CeAllocPath.
 * @param old_ptr The pointer to the memory block to be resized.
 * @param size The size to be changed.
 * @param new_device Memory type according to CeAllocPath.
 * @return The pointer to the newly allocated memory.
 */
static inline void *do_path_realloc(void *old_ptr, size_t size,
                                    CeAllocPath new_device) {
    void *new_ptr = nullptr;
    CeAllocPath old_device = (AddressMap::Find((uintptr_t)old_ptr))
                                 ? CeAllocPath::kJePath
                                 : CeAllocPath::kLibcPath;

    if (new_device == old_device) {
        if (new_device == CeAllocPath::kLibcPath) {
            new_ptr = local_realloc(old_ptr, size);
        } else {
            new_ptr = cxl_realloc(old_ptr, size);
            if (new_ptr != nullptr && new_ptr != old_ptr) {
                if (AddressMap::Pop((uintptr_t)old_ptr)) {
                    AddressMap::Push((uintptr_t)new_ptr);
                } else {
                    CE_LOG_ERROR("old_ptr(%16p) does not exist in AddressMap.");
                }
            }
        }
    } else {
        size_t old_size = 0;
        size_t new_size = 0;
        if (old_device == CeAllocPath::kLibcPath) {
            old_size = local_malloc_usable_size(old_ptr);
        } else {
            old_size = cxl_malloc_usable_size(old_ptr);
        }

        new_ptr = path_malloc(size);
        new_size = std::min(old_size, size);
        std::memcpy(new_ptr, old_ptr, new_size);
        path_free(old_ptr);
    }

    return new_ptr;
}

void *path_realloc(void *ptr, size_t size) {
    void *new_ptr = nullptr;
    CeAllocPath new_device = CeModeHandler::GetAllocPath();

    CE_LOG_VERBOSE("call path_realloc: %d\n", size);
    if (ptr == nullptr) {
        if (new_device == CeAllocPath::kLibcPath) {
            new_ptr = local_realloc(nullptr, size);
        } else {
            new_ptr = cxl_realloc(nullptr, size);
            if (new_ptr != nullptr) {
                AddressMap::Push((uintptr_t)new_ptr);
            }
        }
        return new_ptr;
    }

    if (size == 0) {
        if (AddressMap::Pop((uintptr_t)ptr)) {
            new_ptr = cxl_realloc(ptr, 0);
        } else {
            new_ptr = local_realloc(ptr, 0);
        }
        // If the size of realloc is zero, the return value must be NULL.
        CE_ASSERT(new_ptr == nullptr, "return from realloc should be NULL\n");
        return nullptr;
    }

    return do_path_realloc(ptr, size, new_device);
}

int path_posix_memalign(void **memptr, size_t alignment, size_t size) {
    int result = 0;

    CE_LOG_VERBOSE("call path_posix_memalign: %d\n", size);
    if (CeModeHandler::IsLibcPath()) {
        result = local_posix_memalign(memptr, alignment, size);
    } else {
        result = cxl_posix_memalign(memptr, alignment, size);
        if (result == 0) {
            AddressMap::Push((uintptr_t)*memptr);
        }
    }

    return result;
}

void *path_memalign(size_t alignment, size_t size) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("call path_memalign: %d\n", size);
    if (CeModeHandler::IsLibcPath()) {
        addr = local_memalign(alignment, size);
    } else {
        addr = cxl_memalign(alignment, size);
        if (addr != nullptr) {
            AddressMap::Push((uintptr_t)addr);
        }
    }

    return addr;
}

void *path_valloc(size_t size) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("call path_valloc: %d\n", size);
    if (CeModeHandler::IsLibcPath()) {
        addr = local_valloc(size);
    } else {
        addr = cxl_valloc(size);
        if (addr != nullptr) {
            AddressMap::Push((uintptr_t)addr);
        }
    }

    return addr;
}

void *path_aligned_alloc(size_t alignment, size_t size) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("call path_aligned_alloc: %d\n", size);
    if (CeModeHandler::IsLibcPath()) {
        addr = local_aligned_alloc(alignment, size);
    } else {
        addr = cxl_aligned_alloc(alignment, size);
        if (addr != nullptr) {
            AddressMap::Push((uintptr_t)addr);
        }
    }

    return addr;
}

void path_free(void *ptr) {
    CE_LOG_VERBOSE("call path_free: %16p\n", ptr);
    if (AddressMap::Pop((uintptr_t)ptr)) {
        cxl_free(ptr);
    } else {
        local_free(ptr);
    }
}

size_t path_malloc_usable_size(void *ptr) {
    size_t size = 0;

    CE_LOG_VERBOSE("ce_malloc_usable_size called\n");
    if (AddressMap::Find((uintptr_t)ptr)) {
        size = cxl_malloc_usable_size(ptr);
    } else {
        size = local_malloc_usable_size(ptr);
    }

    return size;
}
