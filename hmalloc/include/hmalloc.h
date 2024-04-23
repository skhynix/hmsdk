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
void *hcalloc(size_t nmemb, size_t size);
void *hrealloc(void *ptr, size_t size);
void *haligned_alloc(size_t alignment, size_t size);
int hposix_memalign(void **memptr, size_t alignment, size_t size);
size_t hmalloc_usable_size(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
