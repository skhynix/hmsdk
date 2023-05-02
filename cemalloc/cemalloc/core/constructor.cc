/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

#include "alloc_attr.h"
#include "allocator_types.h"
#include "constructor.h"
#include "cxl_allocator.h"
#include "env_parser.h"
#include "local_allocator.h"
#include "logging.h"
#include "path_allocator.h"
#include "utils.h"

#if !defined(__GLIBC__) || !__GLIBC_PREREQ(2, 11)
#error require glibc >= 2.11
#endif

static std::atomic<bool> init_check(false);

static void InitComplete(void) {
    init_check.store(true);
}

bool IsInitialized(void) {
    return init_check.load();
}

static void SetAllocator(void) {
    CeMode ce_mode = GetEnvCeMode();

    if (ce_mode == CE_IMPLICIT) {
        if (!set_cxl_allocator())
            CE_LOG_ERROR("cxl allocator is not set\n");
        CE_LOG_VERBOSE("cxl allocator is set\n");
    } else if (ce_mode == CE_EXPLICIT) {
        if (!set_local_allocator())
            CE_LOG_ERROR("local allocator is not set\n");
        CE_LOG_VERBOSE("local allocator is set\n");
    } else {
        if (!set_path_allocator())
            CE_LOG_ERROR("path allocator is not set\n");
        CE_LOG_VERBOSE("path allocator is set\n");
    }
}

void CemallocInit(void) {
    if (unlikely(IsInitialized())) {
        CE_LOG_INFO("CemallocInit is already called\n");
        return;
    }
    CE_LOG_VERBOSE("cemalloc init function\n");

    init_local_func();
    SetAllocator();
    AllocAttrHandler::Init();
    InitComplete();
}

void CemallocInit2(void) {
    SetMaxNode();
    EnvParser();
}

void CemallocDeinit(void) {
    AllocAttrHandler::Deinit();
}
