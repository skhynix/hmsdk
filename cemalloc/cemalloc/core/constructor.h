/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_CONSTRUCTOR_H_
#define _CEMALLOC_CORE_CONSTRUCTOR_H_

#include <stdint.h>

/**
 * @brief Check whether cemalloc constructor is called.
 * @return Whether constructor is called or not.
 */
bool IsInitialized(void);

/**
 * @brief Define overriding functions of memory allocation and free for local
 *        memory.\n
 *        Also, call some essential functions for cemalloc operation.
 * @note Refer cxl_allocator.cc for functions regarding cxl memory
 */
void CemallocInit(void) __attribute__((constructor(101)));

/**
 * @brief Read environment variables and system configuration for cemalloc
 *        internal operation.
 */
void CemallocInit2(void) __attribute__((constructor(102)));

/**
 * @brief Clean before exit
 */
void CemallocDeinit(void) __attribute__((destructor));

#endif
