/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_LOGGING_H_
#define _CEMALLOC_CORE_LOGGING_H_

#include "cemalloc.h"
#include <cassert>
#include <stdlib.h>

/// @brief Log level.
#define LOG_ERROR 0
#define LOG_WARN 1
#define LOG_INFO 2
#define LOG_DEBUG 3
#define LOG_VERBOSE 4

/// @brief Macro of print ERROR log.
#if LOG_LEVEL >= LOG_ERROR
#define CE_LOG_ERROR(format, arg...)                                           \
    do {                                                                       \
        CeLog(format, ##arg);                                                  \
        exit(EXIT_FAILURE);                                                    \
    } while (0)
#else
#define CE_LOG_ERROR(format, arg...)
#endif

/// @brief Macro of print WARN log.
#if LOG_LEVEL >= LOG_WARN
#define CE_LOG_WARN(format, arg...)                                            \
    do {                                                                       \
        CeLog(format, ##arg);                                                  \
    } while (0)
#else
#define CE_LOG_WARN(format, arg...)
#endif

/// @brief Macro of print INFO log.
#if LOG_LEVEL >= LOG_INFO
#define CE_LOG_INFO(format, arg...)                                            \
    do {                                                                       \
        CeLog(format, ##arg);                                                  \
    } while (0)
#else
#define CE_LOG_INFO(format, arg...)
#endif

/// @brief Macro of print DEBUG log.
#if LOG_LEVEL >= LOG_DEBUG
#define CE_LOG_DEBUG(format, arg...)                                           \
    do {                                                                       \
        CeLog(format, ##arg);                                                  \
    } while (0)
#else
#define CE_LOG_DEBUG(format, arg...)
#endif

/// @brief Macro of print VERBOSE log.
#if LOG_LEVEL >= LOG_VERBOSE
#define CE_LOG_VERBOSE(format, arg...)                                         \
    do {                                                                       \
        CeLog(format, ##arg);                                                  \
    } while (0)
#else
#define CE_LOG_VERBOSE(format, arg...)
#endif

/// @brief Macro of assert log.
#ifdef CEASSERT
#define CE_ASSERT(cond, format, arg...) CeAssert(cond, format, ##arg);
#else
#define CE_ASSERT(cond, format, arg...)
#endif

/**
 * @brief Print assert error log and exit process if condition is false.
 * @param condition Whether it is true or not.
 * @param format User's error message.
 */
void CeAssert(bool cond, const char *format, ...);

/**
 * @brief Print log according to the log level.
 * @param log_level Define 5 log levels.
 * @param format Log message.
 * @note Default log level is LOG_WARN=1.
 */
void CeLog(const char *format, ...);

#endif
