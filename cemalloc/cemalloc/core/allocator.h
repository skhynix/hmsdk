/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_ALLOCATOR_H_
#define _CEMALLOC_CORE_ALLOCATOR_H_

#include <sys/types.h>

#include "allocator_types.h"

/// @brief Define cemalloc's alias function attribute.
#define CEMALLOC_ALIAS(_func_)                                                 \
    __attribute__((alias(#_func_), visibility("default")))

/**
 * @brief Set up ce_xx functions.
 * @param allocator allocator function pointer
 * @return On success, true is returned. On error, false is returned.
 */
bool set_ce_allocator(const AllocatorTypes &allocator);

#endif
