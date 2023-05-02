/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_ENV_PARSER_H_
#define _CEMALLOC_CORE_ENV_PARSER_H_

#include <stdint.h>

#include "operation_mode.h"

/// @brief Environment variable for cemalloc user's operation mode.
/// @note ex> export CE_MODE=CE_IMPLICIT | CE_EXPLICIT | CE_EXPLICIT_INDICATOR
#define ENV_CE_MODE "CE_MODE"
/// @brief Environment variable for allocation attribute.
/// @note ex> export CE_ALLOC=CE_ALLOC_HOST | CE_ALLOC_CXL |
///                           CE_ALLOC_USERDEFINED | CE_ALLOC_BWAWARE
#define ENV_CE_ALLOC "CE_ALLOC"
/// @brief Environment variable for cxl node number.
/// @note ex> export CE_CXL_NODE=0 (only number)
#define ENV_CE_CXL_NODE "CE_CXL_NODE"
/// @brief Environment variable for the node number and allocation ratio.
/// @note CE_INTERLEAVE_NODE is a list of node_number*weight\n
///       If the weight is omitted, it means the weight=1.\n
///   ex> export CE_INTERLEAVE_NODE=0*2,1*1\n
///   ex> export CE_INTERLEAVE_NODE=0*2,1 // weight of node 1 is set to 1.
#define ENV_CE_INTERLEAVE_NODE "CE_INTERLEAVE_NODE"

/**
 * @brief Read $CE_MODE which means cemalloc's user operation mode.
 * @return CeMode based on CE_MODE environment variable.
 * @note CE_MODE=CE_IMPLICIT | CE_EXPLICIT | CE_EXPLICIT_INDICATOR
 */
CeMode GetEnvCeMode(void);

/**
 * @brief Read environment variables, and set appropriate values to variables
 * related to operation mode and allocation attribute.
 * @return 0 on success, process exit on fail.
 */
int32_t EnvParser(void);
#endif
