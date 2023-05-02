/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#ifndef _CEMALLOC_CORE_ALLOC_ATTR_H_
#define _CEMALLOC_CORE_ALLOC_ATTR_H_

#include <mutex>
#include <stdint.h>

#include "cemalloc.h"
#include "utils.h"

/**
 * @struct AllocAttr
 * @brief Structure for allocation attribute.\n
 *        It contains device type, method for memory allocation and memory node
 *        information.
 */
struct AllocAttr {
    /// Determine the device and method for memory allocation.
    CeAlloc alloc;
    /// Bitmask of memory nodes. Used in case of
    /// CE_ALLOC_CXL/USERDEFINED/BWAWARE.
    CeNodeMask interleave_node;
    /// The ratio of allocation size for each node.
    /// Used in case of CE_ALLOC_UserDefined.
    unsigned int interleave_node_weight[MAX_NUMNODES];
};

/// @brief Define functions for setting allocation attribute to appropriate\n
///        value.
class AllocAttrExtension {
  public:
    AllocAttrExtension() : valid_(false) {}
    explicit AllocAttrExtension(bool valid) : valid_(valid) {}
    virtual ~AllocAttrExtension() = default;

    /**
     * @brief Check whether AllocAttr attr_ is set.
     * @return Whether AllocAttr attr_ is set.
     * @note attr_ is AllocAttr variable of current process or thread.
     */
    inline bool IsValid() const {
        return valid_;
    }

    /// @brief Set allocation attribute to CE_ALLOC_HOST.
    void AttrHost(void);

    /**
     * @brief Set allocation attribute to CE_ALLOC_CXL.
     * @param nodemask Bit mask of cxl node.
     */
    void AttrCxl(CeNodeMask nodemask);

    /**
     * @brief Set allocation attribute to CE_ALLOC_BWAware.
     * @param nodemask Bit mask of cxl node.
     */
    void AttrBWAware(CeNodeMask nodemask);

    /**
     * @brief Set allocation attribute to CE_ALLOC_USERDEFINED.
     * @param size The number of memory node used in USERDEFINED.
     * @param nodemask Bit mask of nodes which USERDEFINED applied.
     * @param weights The ratio of allocation size for each node. It should be
     * an integer.
     */
    void AttrUserDefined(int size, CeNodeMask nodemask,
                         const unsigned int *weights);

    /**
     * @brief Get current AllocAttr object.
     * @return Current AllocAttr object.
     */
    const AllocAttr &GetAllocAttr(void);

    /**
     * @brief Allocator of AllocAttrExtension object.
     * @param size Bytes to allocate.
     * @return The pointer to allocated memory. On failure, it throws a
     * bad_alloc exception.
     */
    void *operator new(size_t size);

    /**
     * @brief Deallocator of AllocAttrExtension object.
     * @param ptr The memory pointer to deallocate.
     */
    void operator delete(void *ptr);

  private:
    /// @brief Show whether the allocation attribute is set.
    bool valid_;
    /// @brief AllocAttr variable of current process or thread.
    AllocAttr attr_{};
};

/// @brief Define functions for initialize and get the allocation attribute of
///        current process or thread.
class AllocAttrHandler {
  public:
    /// @brief Call initialize function of each AllocAttrExtension object.
    static void Init(void);

    /// @brief Call deinitialize function of each AllocAttrExtension object.
    static void Deinit(void);

    /**
     * @brief Get process' AllocAttrExtension object.
     * @return env_alloc_attr_
     */
    static AllocAttrExtension *GetEnvAttr(void);

    /**
     * @brief Get attr_ of current process or thread.
     * @return Depending on whether threads's AllocAttrExtension is set, return
     * thread's or process' attr_.
     */
    static const AllocAttr &GetAllocAttr(void);

  private:
    /// @brief Initialize process' unique AllocAttrExtension object.
    static void InitEnvAttr(void);

    /// @brief Deinitialize process' unique AllocAttrExtension object.
    static void DeinitEnvAttr(void);

    /// @brief Pointer to process's unique AllocAttrExtension object.
    static AllocAttrExtension *env_alloc_attr_;

    /// @brief Mutex for prevent multiple threads from initializing
    /// AllocAttrExtension object at the same time.
    static std::mutex mtx_;
};

#endif
