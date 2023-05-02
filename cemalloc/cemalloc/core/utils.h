/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_UTILS_H_
#define _CEMALLOC_CORE_UTILS_H_

#include <stdint.h>

#include <regex>
#include <string>
#include <vector>

#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) !!(x)
#define unlikely(x) !!(x)
#endif

/**
 * @brief Maximum number of system numa memory node that cemalloc can support.
 * @note For x86 systems, we believe 64 is an enough number for the number of
 *       nodes.\n
 *       Thus, the type of AllocAttr.interleave_node is defined as CeNodeMask.
 */
#define MAX_NUMNODES 64
typedef uint64_t CeNodeMask;

/// @brief Set the number of system numa memory node.
void SetMaxNode();

/**
 * @brief Check whether 'node' is valid value or not.
 * @param node Memory node number.
 * @return Whether 'node' is valid value or not.
 */
bool MaxNodeCheck(int node);

/**
 * @brief Get the number of system memory node.
 * @return The number of system memory node.
 */
int GetMaxNode();

/**
 * @brief Set bit mask of node.
 * @param nodemask Reference to CeNodeMask.
 * @param node Memory node which would be set to bit mask.
 */
void SetNode(CeNodeMask &nodemask, uint32_t node);

/**
 * @brief Unset bit mask of node.
 * @param nodemask Reference to CeNodeMask.
 * @param node Memory node which would be unset to bit mask.
 */
void UnSetNode(CeNodeMask &nodemask, uint32_t node);

/**
 * @brief Initialize bit mask of nodes.
 * @param nodemask Reference to CeNodeMask.
 */
void InitNodeMask(CeNodeMask &nodemask);

/**
 * @brief Parser of $CE_INTERLEAVE_NODE.
 * @param interleave_node Node mask to store the parsed node value.
 * @param interleave_node_weight Weight array to store the parsed weight value.
 * @param s The value of $CE_INTERLEAVE_NODE.
 * @param max_node The number of system numa memory node.
 * @return Whether $CE_INTERLEAVE_NODE value is valid or not.
 * @note It parses a list of integer*integer. If second integer is omitted, it
 *       would be set to 1.\n
 *  ex> 0*2,1*1\n
 *  ex> 0*2,1\n
 *  => interleave_node[0]=11b=3\n
 *     interleave_node_weight[0]=2, interleave_node_weight[1]=1
 */
bool ParseWeightString(CeNodeMask &interleave_node,
                       unsigned int *interleave_node_weight, const char *s,
                       int max_node);
/**
 * @brief Check whether parameter value is a number.
 * @param str String which need to check if it is a number.
 * @return Whether 'str' is a number or not.
 */
bool IsNumber(const char *str);

/**
 * @brief Check if BW-Aware is enabled.
 * @return On success, zero is returned. On error, -errno is returned.
 */
int32_t CheckBWAwareEnabled();

#endif
