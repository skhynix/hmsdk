/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_SYSCALL_DEFINE_H_
#define _CEMALLOC_CORE_SYSCALL_DEFINE_H_

#define MPOL_PREFERRED (1)
#define MPOL_INTERLEAVE_WEIGHT (6)
#define MPOL_F_AUTO_WEIGHT (1 << 12)

/**
 * @brief Set the interleaving weight for the memory range.
 * @param start Start address of memory range.
 * @param len The length in bytes of memory range.
 * @param weights The ratio of allocation size for memory range.
 * @param weight_count The length of weights array.
 * @param flags Do nothing for now. Preserved for the future.
 * @return 0 on success, -1 on fail. Errno is set to indicate the error.
 */
long mrange_node_weight(void *start, unsigned long len,
                        const unsigned int *weights, unsigned int weight_count,
                        unsigned long flags);

/**
 * @brief Set the NUMA memory policy.
 * @param start Start address of memory range.
 * @param len The length in bytes of memory range.
 * @param mode Specify one of MPOL_DEFAULT, MPOL_BIND, MPOL_INTERLEAVE,
 * MPOL_PREFERED, MPOL_LOCAL.
 * @param nmask Bit mask of nodes which the mode applied.
 * @param maxnode Maximum number of memory node that system can support.
 * @return 0 on success, -1 on fail. Errno is set to indicate the error.
 */
long ce_mbind(void *start, unsigned long len, int mode,
              const unsigned long *nmask, unsigned long maxnode,
              unsigned flags);

#endif
