/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_CXL_ALLOCATOR_H_
#define _CEMALLOC_CORE_CXL_ALLOCATOR_H_

#include "allocator_types.h"

extern malloc_func cxl_malloc_func;
extern calloc_func cxl_calloc_func;
extern realloc_func cxl_realloc_func;
extern posix_memalign_func cxl_posix_memalign_func;
extern memalign_func cxl_memalign_func;
extern valloc_func cxl_valloc_func;
extern aligned_alloc_func cxl_aligned_alloc_func;
extern free_func cxl_free_func;
extern malloc_usable_size_func cxl_malloc_usable_size_func;

/**
 * @brief Set allocator to cxl_xx.
 * @return On success, true is returned. On error, false is returned.
 */
bool set_cxl_allocator(void);

#endif
