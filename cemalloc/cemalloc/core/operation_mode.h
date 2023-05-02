/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_OPERATION_MODE_H_
#define _CEMALLOC_CORE_OPERATION_MODE_H_

#include <mutex>
#include <stdint.h>

#include "alloc_attr.h"

/// @brief cemalloc user's operation mode.
enum CeMode {
    /**
     * Implicit mode which user can use cemalloc without source code
     * modification.\n ex> LD_PRELOAD=libcemalloc.so ./your_app
     */
    CE_IMPLICIT,
    /**
     * Explicit mode which user can optimize the use of cemalloc by adopting
     * explicit APIs.\n
     * ex> cxl_malloc();....; malloc() ..
     */
    CE_EXPLICIT,
    /**
     * Explicit indicator which can be used by Python and Java applications.\n
     * ex> import cemalloc\n
     *     cemalloc.SetCxlMemory()
     */
    CE_EXPLICIT_INDICATOR
};

/// @brief Define allocation path.
enum class CeAllocPath {
    kLibcPath, ///< Memory allocation using libc allocator.
    kJePath    ///< Memory allocation using jemalloc allocator.
};

/// @brief Define allocation path of each CeMode.
/// @note CE_MODE = CE_IMPLICIT | CE_EXPLICIT | CE_EXPLICIT_INDICATOR
class AllocPathImpl {
  public:
    /**
     * @brief Set allocation path of CE_IMPLICIT to kJePath.
     * @return kJePath.
     */
    static CeAllocPath ImplicitAllocPath(void);

    /**
     * @brief Set allocation path of CE_EXPLICIT to kLibcPath.
     * @return kLibcPath.
     */
    static CeAllocPath ExplicitAllocPath(void);

    /**
     * @brief Set allocation path of CE_EXPLICIT_INDICATOR to appropriate path.
     * @return Depending on the value of explicit indicator's allocation mode
     * and allocation attribute, return CeAllocPath.
     */
    static CeAllocPath ExplicitIndicatorAllocPath(void);

    /**
     * @brief Set CE_EXPLICIT_INDICATOR status.
     * @param status Whether explicit indicator's allocation mode is host memory
     *        or cxl memory.
     */
    static void SetExplicitIndicatorStatus(bool status);

    /**
     * @brief Get CE_EXPLICIT_INDICATOR status.
     * @return True if explicit indicator's allocation mode is cxl memory, false
     *         if it is host memory.
     */
    static bool GetExplicitIndicatorStatus(void);

  private:
    /**
     * @brief Get CeAllocPath using attr's CeAlloc value.
     * @param attr Current allocation attribute.
     * @return kLibcPath if CeAlloc is CE_ALLOC_HOST, else return kJePath
     */
    static CeAllocPath AttrToMode(AllocAttr attr);

    /// @brief Define whether the explicit indicator's allocation mode is\n
    ///        host memory or cxl memory.
    static __thread bool use_explicit_indicator_;
};

/// @brief Function pointer which returns CeAllocPath.
typedef CeAllocPath (*AllocPathHandler)(void);

/// @brief Handle CeMode of current process or thread.
/// @note CE_MODE = CE_IMPLICIT | CE_EXPLICIT | CE_EXPLICIT_INDICATOR
class CeModeHandler {
  public:
    /**
     * @brief Get allocation path.
     * @return Allocation path.
     */
    static CeAllocPath GetAllocPath(void);

    /**
     * @brief Check whether the allocation path is kLibcPath or kJePath.
     * @return True if it is kLibcPath, false if not.
     */
    static bool IsLibcPath(void);

    /**
     * @brief Get CeMode of current process or thread.
     * @return CeMode.
     */
    static CeMode GetCeMode(void);

    /**
     * @brief Set CeMode.
     * @param mode CeMode of current process or thread.
     * @return 0 on success.
     */
    static int32_t SetCeMode(CeMode mode);

  private:
    /// @brief CeMode of current process or thread.
    static CeMode mode_;
    /// @brief Function pointer which return CeAllocPath.
    static AllocPathHandler path_handler_;
    /// @brief Mutex for prevent multiple threads from setting CeMode\n
    ///  at the same time.
    static std::mutex mtx_;
};

#endif
