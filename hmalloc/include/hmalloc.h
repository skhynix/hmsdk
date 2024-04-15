/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef HMALLOC_H
#define HMALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *hmalloc(size_t size);
void hfree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
