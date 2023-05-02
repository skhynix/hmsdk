/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_INCLUDE_CEMALLOC_TYPES_H_
#define _CEMALLOC_INCLUDE_CEMALLOC_TYPES_H_

/// @brief Define CEMALLOC_EXPORT as this API is public outside.
#define CEMALLOC_EXPORT __attribute__((visibility("default")))

/// @brief Define the device and method of memory allocation.
typedef enum {
    CE_ALLOC_HOST = 0,
    CE_ALLOC_CXL = 1,
    CE_ALLOC_USERDEFINED = 2,
    CE_ALLOC_BWAWARE = 3
} CeAlloc;

#endif // _CEMALLOC_INCLUDE_CEMALLOC_TYPES_H_
