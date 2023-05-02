/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_INCLUDE_CEMALLOC_H_
#define _CEMALLOC_INCLUDE_CEMALLOC_H_

#include <stddef.h>
#include <sys/types.h>

#include "cemalloc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate memory from cxl.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix malloc.\n
 * This function guarantees the allocated memory be placed on the CXL device.\n
 * ex> void* ptr = cxl_malloc(SIZE);
 */
CEMALLOC_EXPORT void *cxl_malloc(size_t size);

/**
 * @brief Create a new mapping in the virtual aaddress space from cxl.
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
 * This function guarantees the mapped memory be placed on the CXL device.
 */
CEMALLOC_EXPORT void *cxl_mmap(void *addr, size_t length, int prot, int flags,
                               int fd, off_t offset);

/**
 * @brief Allocate memory from cxl.
 * @param nmemb The number of elements to allocate.
 * @param size The size of an element.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix calloc.\n
 * This function guarantees the allocated memory be placed on the CXL device.
 */
CEMALLOC_EXPORT void *cxl_calloc(size_t nmemb, size_t size);

/**
 * @brief Change the size of the memory block from cxl.
 * @param ptr The pointer to the memory block to be resized.
 * @param size The size to be changed.
 * @return The pointer to the newly allocated memory. On error, return NULL and
 * original block is left untouched.
 * @note This function follows the interface of posix realloc.\n
 * This function guarantees the allocated memory be placed on the CXL device.
 */
CEMALLOC_EXPORT void *cxl_realloc(void *ptr, size_t size);

/**
 * @brief Allocate memory from cxl.
 * @param memptr The address of the allocated memory is placed.
 * @param alignment The address of the allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return 0 on success, error values on error.
 * @note This function follows the interface of posix posix_memalign.\n
 * This function guarantees the allocated memory be placed on the CXL device.
 */
CEMALLOC_EXPORT int cxl_posix_memalign(void **memptr, size_t alignment,
                                       size_t size);

/**
 * @brief Allocate memory from cxl
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix memalign.\n
 * This function guarantees the allocated memory be placed on the CXL device.
 */
CEMALLOC_EXPORT void *cxl_memalign(size_t alignment, size_t size);

/**
 * @brief Obsolete function which allocates memory from cxl.
 * @param size Bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix valloc.\n
 * This function guarantees the allocated memory be placed on the CXL device.
 */
CEMALLOC_EXPORT void *cxl_valloc(size_t size);

/**
 * @brief Allocate memory from cxl.
 * @param alignment The address of allocated memory will be multiple of
 * alignment.
 * @param size The bytes to allocate.
 * @return The pointer to the allocated memory. On error, return NULL.
 * @note This function follows the interface of posix aligned_alloc.\n
 * This function guarantees the allocated memory be placed on the CXL device.
 */
CEMALLOC_EXPORT void *cxl_aligned_alloc(size_t alignment, size_t size);

/**
 * @brief Free the memory space from cxl.
 * @param ptr The memory pointer to free.
 * @note This function follows the interface of posix free.\n
 * To deallocate the memory allocated by "cxl_" functions in this file, this
 * function MUST be called.\n
 * To call this function with an area allocated by "host_" malloc is not
 * defined.
 */
CEMALLOC_EXPORT void cxl_free(void *ptr);

/**
 * @brief Get the number of usable bytes from cxl memory.
 * @param ptr The pointer to the allocated memory.
 * @return The number of usable bytes in the block pointed by ptr.
 * @note This function follows the interface of posix malloc_usable_size.\n
 * To call this function with an area allocated by "host_" malloc is not
 * defined.
 */
CEMALLOC_EXPORT size_t cxl_malloc_usable_size(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // _CEMALLOC_INCLUDE_CEMALLOC_H_
