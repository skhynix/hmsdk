/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "syscall_define.h"

#include <sys/syscall.h> /* syscall */
#include <unistd.h>      /* syscall */

#include "logging.h"

#define __NR_mrange_node_weight (452)

long mrange_node_weight(void *start, unsigned long len,
                        const unsigned int *weights, unsigned int weight_count,
                        unsigned long flags) {
    CE_LOG_VERBOSE("mrange_node_weight called\n");
    return syscall(__NR_mrange_node_weight, (long)start, len, weights,
                   weight_count, flags);
}

long ce_mbind(void *start, unsigned long len, int mode,
              const unsigned long *nmask, unsigned long maxnode,
              unsigned flags) {
    CE_LOG_VERBOSE("ce_mbind called\n");
    return syscall(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode,
                   flags);
}
