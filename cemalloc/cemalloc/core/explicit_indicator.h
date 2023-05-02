/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_EXPLICIT_INDICATOR_H_
#define _CEMALLOC_EXPLICIT_INDICATOR_H_

#include "cemalloc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable explicit indicator mode.
 * @note This function must be called before using the APIs related to
 * CE_EXPLICIT_INDICATOR.\n
 * Therefore, it would be called at the time of module import.
 */
CEMALLOC_EXPORT void EnableExplicitIndicator(void);

/**
 * @brief Enable allocation to cxl memory.
 * @return 0 on success, -1 on fail. If CeMode is not CE_EXPLICIT_INDICATOR, it
 * fails.
 * @note $CE_ALLOC has to be set.\n
 *       $CE_CXL_NODE has to be set, if
 * $CE_ALLOC=CE_ALLOC_CXL|CE_ALLOC_BWAWARE.\n $CE_INTERLEAVE_NODE has to be set,
 * if $CE_ALLOC=CE_ALLOC_USERDEFINED.
 */
CEMALLOC_EXPORT int SetCxlMemory(void);

/**
 * @brief Enable allocation to host memory.
 * @return 0 on success, -1 on fail. If CeMode is not CE_EXPLICIT_INDICATOR, it
 *         fails.
 */
CEMALLOC_EXPORT int SetHostMemory(void);

/**
 * @brief Get the current memory mode(host or cxl).
 * @return 0 if the current memory mode is host, 1 if it is cxl.
 */
CEMALLOC_EXPORT int GetMemoryMode(void);

#ifdef __cplusplus
}
#endif

#endif
