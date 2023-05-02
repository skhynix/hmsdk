/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "address_map.h"

#include <functional>
#include <map>
#include <mutex>
#include <utility>

#include "cemalloc.h"
#include "operation_mode.h"
#include "utils.h"

template <typename key, typename value, typename pred = std::less<key>>
using map_type =
    std::map<key, value, pred, MapAllocator<std::pair<key, value>>>;

/// @brief Define functions for insert/delete/find entries in the address map.
class AddressMapImpl {
  private:
    /// @brief Address map which stores <address, CeAlloPath> pair data.
    ///  @note It stores an address of newly allocated through kJePath.
    map_type<uintptr_t, CeAllocPath> address_map_;
    /// @brief Mutex for prevent multiple threads from handling address map\n
    ///    at the same time.
    std::mutex address_map__mtx;

  public:
    AddressMapImpl() = default;
    ~AddressMapImpl() = default;

    /**
     * @brief Insert a new entry of address and CeAllocPath pair.
     * @param addr An address of newly allocated through kJePath.
     */
    void Push(uintptr_t addr) {
        CE_LOG_VERBOSE("Push %16p\n", addr);
        std::lock_guard<std::mutex> lock(address_map__mtx);
        address_map_.insert(std::make_pair(addr, CeAllocPath::kJePath));
    }

    /**
     * @brief Find an entry in address_map_.
     * @param addr The address to find.
     * @return True on success, false on fail.
     */
    bool Find(uintptr_t addr) {
        bool result = false;
        auto it = address_map_.find(addr);

        if (it != address_map_.end()) {
            CE_LOG_VERBOSE("Find %16p\n", addr);
            result = true;
        } else {
            CE_LOG_VERBOSE("no Find %16p\n", addr);
            result = false;
        }
        return result;
    }

    /**
     * @brief Delete an entry in address_map_.
     * @param addr The address of freed memory.
     * @return True on success, false on fail.
     */
    bool Pop(uintptr_t addr) {
        bool result = false;
        std::lock_guard<std::mutex> lock(address_map__mtx);
        auto it = address_map_.find(addr);

        if (it != address_map_.end()) {
            CE_LOG_VERBOSE("Pop %16p\n", addr);
            address_map_.erase(it);
            result = true;
        } else {
            CE_LOG_VERBOSE("no Pop %16p\n", addr);
            result = false;
        }
        return result;
    }

    /**
     * @brief Allocator of AddressMapImpl object.
     * @param size Bytes to allocate.
     * @return The pointer to the allocated memory. On failure, throws a
     * bad_alloc exception.
     */
    void *operator new(size_t size) {
        void *instance = nullptr;
        instance = local_malloc(size);
        return instance;
    }

    /**
     * @brief Deallocator of AddressMapImpl object.
     * @param ptr The memory pointer to deallocate.
     */
    void operator delete(void *ptr) {
        local_free(ptr);
    }
};

/// @brief The pointer to unique AddressMapImpl object.
static AddressMapImpl *addr_map_impl = nullptr;
/// @brief Mutex for prevent multiple threads from initializing addr_map_impl.
static std::mutex addr_map_impl_mtx;

void AddressMap::Push(uintptr_t addr) {
    if (unlikely(addr_map_impl == nullptr)) {
        std::lock_guard<std::mutex> lock(addr_map_impl_mtx);
        if (addr_map_impl == nullptr) {
            addr_map_impl = new AddressMapImpl;
        }
    }
    addr_map_impl->Push(addr);
}

bool AddressMap::Pop(uintptr_t addr) {
    if (unlikely(addr_map_impl == nullptr)) {
        std::lock_guard<std::mutex> lock(addr_map_impl_mtx);
        if (addr_map_impl == nullptr)
            addr_map_impl = new AddressMapImpl;
    }
    return addr_map_impl->Pop(addr);
}

bool AddressMap::Find(uintptr_t addr) {
    if (unlikely(addr_map_impl == nullptr)) {
        std::lock_guard<std::mutex> lock(addr_map_impl_mtx);
        if (addr_map_impl == nullptr)
            addr_map_impl = new AddressMapImpl;
    }
    return addr_map_impl->Find(addr);
}
