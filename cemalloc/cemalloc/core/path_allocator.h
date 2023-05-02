/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_PATH_ALLOCATOR_H_
#define _CEMALLOC_CORE_PATH_ALLOCATOR_H_

#include "allocator_types.h"

/**
 * @brief Set allocator to path_xx.
 * @return On success, true is returned. On error, false is returned.
 */
bool set_path_allocator(void);

/**
 * @brief Alias of posix malloc. Depending on CeAllocPath, call local_malloc or
 * cxl_malloc.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note Cxl memory allocation address is managed by an AddressMap.\n
 * Whenever cxl memory is allocated, the address is stored in the AddressMap.
 */
void *path_malloc(size_t size);

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
void *path_mmap(void *addr, size_t length, int prot, int flags, int fd,
                off_t offset);

/**
 * @brief Alias of posix calloc. Depending on CeAllocPath, call local_calloc or
 * cxl_calloc.
 * @param nmemb The number of elements to allocate.
 * @param size The size of an element.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *path_calloc(size_t nmemb, size_t size);

/**
 * @brief Alias of posix realloc. Depending on CeAllocPath, call local_realloc
 * or cxl_realloc.
 * @param ptr The pointer to the memory block to be resized.
 * @param size The size to be changed.
 * @return The pointer to the newly allocated memory. On error, return NULL and
 * original block is left untouched.
 * @note If size is equal to zero, this function is equal to free().
 */
void *path_realloc(void *ptr, size_t size);

/**
 * @brief Alias of posix posix_memalign. Depending on CeAllocPath, call
 * local_posix_memalign or cxl_posix_memalign.
 * @param memptr The address of the allocated memory is placed.
 * @param alignment The address of the allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return 0 on success, error values on error.
 */
int path_posix_memalign(void **memptr, size_t alignment, size_t size);

/**
 * @brief Alias of posix memalign. Depending on CeAllocPath, call local_memalign
 * or cxl_memalign.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param sz Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *path_memalign(size_t alignment, size_t size);

/**
 * @brief Alias of posix valloc. Depending on CeAllocPath, call local_valloc or
 * cxl_valloc.
 * @param sz Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *path_valloc(size_t size);

/**
 * @brief Alias of aligned_alloc. Depending on CeAllocPath, call
 * local_aligned_alloc or cxl_aligned_alloc.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param sz Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 */
void *path_aligned_alloc(size_t alignment, size_t size);

/**
 * @brief Alias of posix free.\n
 *        Depending on whether the memory address pointed by ptr exists in the
 *        AddressMap, call local_free or cxl_free.
 * @param ptr The memory pointer to free.
 */
void path_free(void *ptr);

/**
 * @brief Alias of posix malloc_usable_size.\n
 *        Depending on whether the memory address pointed by ptr exists in the
 *        AddressMap, call local_malloc_usable_size or cxl_malloc_usable_size.
 * @param ptr The pointer to the allocated memory.
 * @return The number of usable bytes in the block pointed by ptr.
 */
size_t path_malloc_usable_size(void *ptr);

#endif
