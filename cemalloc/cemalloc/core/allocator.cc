/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "allocator.h"

#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include <mutex>
#include <new>

#include "cemalloc.h"
#include "logging.h"
#include "utils.h"

#include "address_map.h"
#include "constructor.h"
#include "cxl_allocator.h"
#include "local_allocator.h"
#include "operation_mode.h"

#define BUFFER_SIZE_FOR_DLSYM 50
static uint8_t buffer[BUFFER_SIZE_FOR_DLSYM];
static __thread bool hook_guard = false;

static std::mutex allocator_mtx;
static malloc_func ce_malloc_func = nullptr;
static mmap_func ce_mmap_func = nullptr;
static calloc_func ce_calloc_func = nullptr;
static realloc_func ce_realloc_func = nullptr;
static posix_memalign_func ce_posix_memalign_func = nullptr;
static memalign_func ce_memalign_func = nullptr;
static valloc_func ce_valloc_func = nullptr;
static aligned_alloc_func ce_aligned_alloc_func = nullptr;
static free_func ce_free_func = nullptr;
static malloc_usable_size_func ce_malloc_usable_size_func = nullptr;

bool set_ce_allocator(const AllocatorTypes &allocator) {
    std::lock_guard<std::mutex> lg(allocator_mtx);

    if (unlikely(!allocator.malloc_new || !allocator.mmap_new ||
                 !allocator.calloc_new || !allocator.realloc_new ||
                 !allocator.posix_memalign_new || !allocator.memalign_new ||
                 !allocator.valloc_new || !allocator.aligned_alloc_new ||
                 !allocator.free_new || !allocator.malloc_usable_size_new)) {
        CE_LOG_WARN("incomplete allocator is passed\n");
        return false;
    }

    ce_malloc_func = allocator.malloc_new;
    ce_mmap_func = allocator.mmap_new;
    ce_calloc_func = allocator.calloc_new;
    ce_realloc_func = allocator.realloc_new;
    ce_posix_memalign_func = allocator.posix_memalign_new;
    ce_memalign_func = allocator.memalign_new;
    ce_valloc_func = allocator.valloc_new;
    ce_aligned_alloc_func = allocator.aligned_alloc_new;
    ce_free_func = allocator.free_new;
    ce_malloc_usable_size_func = allocator.malloc_usable_size_new;

    return true;
}

extern "C" {

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
    CEMALLOC_ALIAS(ce_mmap);
void *mmap64(void *addr, size_t length, int prot, int flags, int fd,
             off_t offset) CEMALLOC_ALIAS(ce_mmap);
void *malloc(size_t size) CEMALLOC_ALIAS(ce_malloc);
void free(void *ptr) CEMALLOC_ALIAS(ce_free);
void *calloc(size_t nmemb, size_t size) CEMALLOC_ALIAS(ce_calloc);
void *realloc(void *ptr, size_t size) CEMALLOC_ALIAS(ce_realloc);
int posix_memalign(void **memptr, size_t alignment, size_t size)
    CEMALLOC_ALIAS(ce_posix_memalign);
void *memalign(size_t alignment, size_t size) CEMALLOC_ALIAS(ce_memalign);
void *valloc(size_t size) CEMALLOC_ALIAS(ce_valloc);
void *aligned_alloc(size_t alignment, size_t size)
    CEMALLOC_ALIAS(ce_aligned_alloc);
size_t malloc_usable_size(void *ptr) CEMALLOC_ALIAS(ce_malloc_usable_size);

/**
 * @brief Alias of posix mmap. Depending on CeAllocPath, call local_mmap or
 * cxl_mmap.
 * @param addr The starting address for the new mapping.\n
 * If addr is NULL, then the kernel choose the address at which to create the
 * mapping.
 * @param length the length of the mapping.
 * @param prot Desired memory protection of the mapping.
 * @param flags Determine whether updates to the mapping are visible to other
 * process.
 * @param fd The file descriptor that is memory mapped.
 * @param offset The starting address in file.
 * @return The pointer to the mapped area. On error, return MAP_FAILED.
 */
void *ce_mmap(void *addr, size_t length, int prot, int flags, int fd,
              off_t offset) {
    CE_LOG_VERBOSE("ce_mmap called\n");
    if (unlikely(!IsInitialized())) {
        CE_LOG_INFO("mmap is not initialized\n");
        CemallocInit();
    }

    return ce_mmap_func(addr, length, prot, flags, fd, offset);
}

/**
 * @brief Alias of posix malloc. Depending on CeAllocPath, call local_malloc or
 * cxl_malloc.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note Cxl memory allocation address is managed by an AddressMap.\n
 * Whenever cxl memory is allocated, the address is stored in the AddressMap.
 */
void *ce_malloc(size_t sz) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("ce_malloc called\n");
    if (unlikely(!IsInitialized())) {
        CE_LOG_INFO("malloc is not initialized\n");
        CemallocInit();
    }

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }
    hook_guard = true;
    addr = ce_malloc_func(sz);
    hook_guard = false;

    return addr;
}

/**
 * @brief Alias of posix free.\n
 *        Depending on whether the memory address pointed by ptr exists in the
 *        AddressMap, call local_free or cxl_free.
 * @param ptr The memory pointer to free.
 */
void ce_free(void *ptr) {
    CE_LOG_VERBOSE("call ce_free: %16p\n", ptr);
    CE_ASSERT(IsInitialized(), "ce_free is not initialized\n");

    if (unlikely(ptr == nullptr)) {
        CE_LOG_INFO("ptr is NULL\n");
    }

    ce_free_func(ptr);
}

/**
 * @brief Alias of posix calloc. Depending on CeAllocPath, call local_calloc or
 * cxl_calloc.
 * @param nmemb The number of elements to allocate.
 * @param size The size of an element.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *ce_calloc(size_t nmemb, size_t sz) {
    void *addr = nullptr;

    /* calloc() is called by dlsym, which makes it essential to have small
     * pre-allocated memory space for dlsym.
     * Unlike other allocation APIs, this function return a static buffer array
     * instead of calling CemallocInit() when those function pointers are not
     * initialized. CemallocInit() is going to be called anyway.
     */
    if (unlikely(!IsInitialized())) {
        CE_LOG_INFO("calloc is not initialized\n");
        return buffer;
    }

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }
    hook_guard = true;
    addr = ce_calloc_func(nmemb, sz);
    hook_guard = false;

    return addr;
}

/**
 * @brief Alias of posix realloc. Depending on CeAllocPath, call local_realloc
 * or cxl_realloc.
 * @param ptr The pointer to the memory block to be resized.
 * @param size The size to be changed.
 * @return The pointer to the newly allocated memory. On error, return NULL and
 * original block is left untouched.
 * @note If size is equal to zero, this function is equal to free().
 */
void *ce_realloc(void *ptr, size_t size) {
    void *new_ptr = nullptr;

    CE_LOG_VERBOSE("ce_realloc called\n");
    CE_ASSERT(IsInitialized(), "ce_realloc is not initialized\n");

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }

    hook_guard = true;
    new_ptr = ce_realloc_func(ptr, size);
    hook_guard = false;

    return new_ptr;
}

/**
 * @brief Alias of posix posix_memalign. Depending on CeAllocPath, call
 * local_posix_memalign or cxl_posix_memalign.
 * @param memptr The address of the allocated memory is placed.
 * @param alignment The address of the allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return 0 on success, error values on error.
 */
int ce_posix_memalign(void **memptr, size_t alignment, size_t sz) {
    int result = 0;

    CE_LOG_VERBOSE("ce_posix_memalign called\n");
    CE_ASSERT(IsInitialized(), "ce_posix_memalign is not initialized\n");

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }
    hook_guard = true;
    result = ce_posix_memalign_func(memptr, alignment, sz);
    hook_guard = false;

    return result;
}

/**
 * @brief Alias of posix valloc. Depending on CeAllocPath, call local_valloc or
 * cxl_valloc.
 * @param sz Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *ce_valloc(size_t sz) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("ce_valloc called\n");
    CE_ASSERT(IsInitialized(), "ce_valloc is not initialized\n");

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }
    hook_guard = true;
    addr = ce_valloc_func(sz);
    hook_guard = false;

    return addr;
}

/**
 * @brief Alias of posix memalign. Depending on CeAllocPath, call local_memalign
 * or cxl_memalign.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param sz Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *ce_memalign(size_t alignment, size_t sz) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("ce_memalign called\n");
    CE_ASSERT(IsInitialized(), "ce_memalign is not initialized\n");

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }
    hook_guard = true;
    addr = ce_memalign_func(alignment, sz);
    hook_guard = false;

    return addr;
}

/**
 * @brief Alias of aligned_alloc. Depending on CeAllocPath, call
 * local_aligned_alloc or cxl_aligned_alloc.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param sz Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *ce_aligned_alloc(size_t alignment, size_t sz) {
    void *addr = nullptr;

    CE_LOG_VERBOSE("ce_aligned_alloc called\n");
    CE_ASSERT(IsInitialized(), "ce_aligned_alloc is not initialized\n");

    if (unlikely(hook_guard)) {
        CE_LOG_ERROR(
            "hook_guard is on, previous malloc should have been handled\n");
    }
    hook_guard = true;
    addr = ce_aligned_alloc_func(alignment, sz);
    hook_guard = false;

    return addr;
}

/**
 * @brief Alias of posix malloc_usable_size.\n
 *        Depending on whether the memory address pointed by ptr exists in the
 *        AddressMap, call local_malloc_usable_size or cxl_malloc_usable_size.
 * @param ptr The pointer to the allocated memory.
 * @return The number of usable bytes in the block pointed by ptr.
 */
size_t ce_malloc_usable_size(void *ptr) {
    size_t size = 0;

    CE_LOG_VERBOSE("ce_malloc_usable_size called\n");
    CE_ASSERT(IsInitialized(), "ce_malloc_usable_size is not initialized\n");
    size = ce_malloc_usable_size_func(ptr);

    return size;
}
}

static void *handleOOM(std::size_t size, bool nothrow) {
    void *ptr = nullptr;

    while (ptr == nullptr) {
        std::new_handler handler = nullptr;
        // GCC-4.8 and clang 4.0 do not have std::get_new_handler.
        {
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx);

            handler = std::set_new_handler(nullptr);
            std::set_new_handler(handler);
        }
        if (handler == nullptr)
            break;

        try {
            handler();
        } catch (const std::bad_alloc &) {
            break;
        }

        ptr = ce_malloc(size);
    }

    if (ptr == nullptr && !nothrow)
        std::__throw_bad_alloc();
    return ptr;
}

template <bool IsNoExcept>
void *newImpl(std::size_t size) noexcept(IsNoExcept) {
    void *ptr = ce_malloc(size);
    if (likely(ptr != nullptr))
        return ptr;

    return handleOOM(size, IsNoExcept);
}

void *operator new(std::size_t size) {
    CE_LOG_VERBOSE("operator new called\n");
    CE_ASSERT(IsInitialized(), "cemalloc function are not initialized\n");
    return newImpl<false>(size);
}

void *operator new[](std::size_t size) {
    CE_LOG_VERBOSE("operator new[] called\n");
    CE_ASSERT(IsInitialized(), "cemalloc function are not initialized\n");
    return newImpl<false>(size);
}

void *operator new(std::size_t size,
                   const std::nothrow_t & /*unused*/) noexcept {
    CE_LOG_VERBOSE("operator new called\n");
    CE_ASSERT(IsInitialized(), "cemalloc function are not initialized\n");
    return newImpl<true>(size);
}

void *operator new[](std::size_t size,
                     const std::nothrow_t & /*unused*/) noexcept {
    CE_LOG_VERBOSE("operator new[] called\n");
    CE_ASSERT(IsInitialized(), "cemalloc function are not initialized\n");
    return newImpl<true>(size);
}

void operator delete(void *ptr) noexcept {
    CE_LOG_VERBOSE("call operator delete: %16p\n", ptr);
    CE_ASSERT(IsInitialized(), "cemalloc functions are not initialized\n");

    ce_free(ptr);
}

void operator delete[](void *ptr) noexcept {
    CE_LOG_VERBOSE("call operator delete[]: %16p\n", ptr);
    CE_ASSERT(IsInitialized(), "cemalloc functions are not initialized\n");

    ce_free(ptr);
}

void operator delete(void *ptr, const std::nothrow_t & /*unused*/) noexcept {
    CE_LOG_VERBOSE("call operator delete: %16p\n", ptr);
    CE_ASSERT(IsInitialized(), "cemalloc functions are not initialized\n");

    ce_free(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t & /*unused*/) noexcept {
    CE_LOG_VERBOSE("call operator delete[]: %16p\n", ptr);
    CE_ASSERT(IsInitialized(), "cemalloc functions are not initialized\n");

    ce_free(ptr);
}
