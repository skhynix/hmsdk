/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_ADDRESS_MAP_H_
#define _CEMALLOC_CORE_ADDRESS_MAP_H_

#include <stdint.h>
#include <stdlib.h>

#include "local_allocator.h"
#include "logging.h"

/// @brief Define stl map's allocator and deallocator as cemalloc's local_malloc
///        and local_free.
template <typename T>
class MapAllocator {
  public:
    using value_type = T;

    MapAllocator() = default;
    ~MapAllocator() = default;

    /**
     * @brief Set stl map's allocator to local_malloc.
     * @param object_count The number of object T.
     * @return The pointer to the allocated memory.
     */
    T *allocate(size_t object_count) {
        size_t size;
        CE_ASSERT(object_count != 0, "allocation size is zero\n");
        size = static_cast<size_t>(sizeof(T) * object_count);
        return static_cast<T *>(local_malloc(size));
    }

    /**
     * @brief Set stl map's deallocator to local_free.
     * @param ptr The memory pointer to free.
     * @param size_t The number of object T.
     */
    void deallocate(T *ptr, size_t) {
        local_free(ptr);
    }
};

/// @brief Define wrapper functions of insert/delete/find entries in the
///        address map.
class AddressMap {
  public:
    /**
     * @brief Insert a new entry of address.
     * @param addr Address of newly allocated through kJePath.
     */
    static void Push(uintptr_t addr);

    /**
     * @brief Delete an entry in the address map.
     * @param addr An Address of freed memory.
     * @return True on success, false on fail.
     */
    static bool Pop(uintptr_t addr);

    /**
     * @brief Find an entry in the address map.
     * @param addr Address to find.
     * @return True on success, false on fail.
     */
    static bool Find(uintptr_t addr);
};

#endif
