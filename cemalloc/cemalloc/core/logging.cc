/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>

#include "utils.h"

#define BT_BUF_SIZE 100

void CeAssert(bool condition, const char *format, ...) {
    if (unlikely(!condition)) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        int32_t nptrs = 0;
        void *buffer[BT_BUF_SIZE];
        char **strings = nullptr;

        nptrs = backtrace(buffer, BT_BUF_SIZE);
        fprintf(stderr, "backtrace() returned %d addresses\n", nptrs);

        strings = backtrace_symbols(buffer, nptrs);
        if (strings == nullptr) {
            perror("backtrace_symbols");
            exit(EXIT_FAILURE);
        }

        for (int32_t i = 0; i < nptrs; i++)
            fprintf(stderr, "%s\n", strings[i]);

        /* Reason for below commented free:
         * We have to call free here although we are only to be exited.
         * However, what if we got here from ce_free?
         * We would be caught up in a recursive trap.
         * */
        // free(strings);
        exit(EXIT_FAILURE);
    }
}

void CeLog(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
