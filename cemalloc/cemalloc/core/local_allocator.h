/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_LOCAL_ALLOCATOR_H_
#define _CEMALLOC_CORE_LOCAL_ALLOCATOR_H_

#include "allocator_types.h"

extern malloc_func local_malloc_func;
extern mmap_func local_mmap_func;
extern calloc_func local_calloc_func;
extern realloc_func local_realloc_func;
extern posix_memalign_func local_posix_memalign_func;
extern memalign_func local_memalign_func;
extern valloc_func local_valloc_func;
extern aligned_alloc_func local_aligned_alloc_func;
extern free_func local_free_func;
extern malloc_usable_size_func local_malloc_usable_size_func;

/**
 * @brief Initialize function pointers used in local_xx.
 */
void init_local_func(void);

/**
 * @brief Set allocator to local_xx.
 * @return On success, true is returned. On error, false is returned.
 */
bool set_local_allocator(void);

/**
 * @brief Allocate memory from host.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix malloc.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 * ex> void* ptr = local_malloc(SIZE);
 */
void *local_malloc(size_t size);

/**
 * @brief Create a new mapping in the virtual address space from host.
 * @param addr The starting address for the new mapping.\n
 * If addr is NULL, then the kernel choose the address at which to create the
 * mapping.
 * @param length The length of the mapping.
 * @param prot Desired memory protection of the mapping.
 * @param flags Determine whether updates to the mapping are visible to other
 * process.
 * @param fd The file descriptor that is memory mapped.
 * @param offset The starting address in file.
 * @return The pointer to the mapped area. On error, return MAP_FAILED.
 * @note This function follows the interface of posix mmap.\n
 *       This function guarantees the mapped memory be placed on the host
 * device.
 */
void *local_mmap(void *addr, size_t length, int prot, int flags, int fd,
                 off_t offset);

/**
 * @brief Allocate memory from host.
 * @param nmemb The number of elements to allocate.
 * @param size The size of an element.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix calloc.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 */
void *local_calloc(size_t nmemb, size_t size);

/**
 * @brief Change the size of the memory block from host.
 * @param ptr The pointer to the memory block to be resized.
 * @param size The size to be changed.
 * @return The pointer to the newly allocated memory. On error, return NULL and
 * original block is left untouched.
 * @note This function follows the interface of posix realloc.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 */
void *local_realloc(void *ptr, size_t size);

/**
 * @brief Allocate memory from host.
 * @param memptr The address of the allocated memory is placed.
 * @param alignment The address of the allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return 0 on success, error values on error.
 * @note This function follows the interface of posix posix_memalign.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 */
int local_posix_memalign(void **memptr, size_t alignment, size_t size);

/**
 * @brief Allocate memory from host.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix memalign.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 */
void *local_memalign(size_t alignment, size_t size);

/**
 * @brief Obsolete function which allocates memory from host.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix valloc.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 */
void *local_valloc(size_t size);

/**
 * @brief Allocate memory from host.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix aligned_alloc.\n
 * This function guarantees the allocated memory be placed on the host device.\n
 */
void *local_aligned_alloc(size_t alignment, size_t size);

/**
 * @brief Free the memory space from host.
 * @param ptr The memory pointer to free.
 * @note This function follows the interface of posix free.\n
 *       To deallocate the memory allocated by "host_" functions in this file,
 *       this function MUST be called.\n
 *       To call this function with an area allocated by "cxl_" malloc is not
 *       defined.
 */
void local_free(void *ptr);

/**
 * @brief Get the number of usable bytes from host memory.
 * @param ptr The pointer to the allocated memory.
 * @return The number of usable bytes in the block pointed by ptr.
 * @note This function follows the interface of posix malloc_usable_size.\n
 * To call this function with an area allocated by "cxl_" malloc is not
 * defined.\n
 */
size_t local_malloc_usable_size(void *ptr);

#endif
