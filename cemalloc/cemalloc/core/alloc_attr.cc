/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "alloc_attr.h"

#include "local_allocator.h"
#include "logging.h"
#include "utils.h"

void AllocAttrExtension::AttrHost(void) {
    valid_ = true;
    attr_.alloc = CE_ALLOC_HOST;
}

void AllocAttrExtension::AttrCxl(CeNodeMask nodemask) {
    valid_ = true;
    attr_.alloc = CE_ALLOC_CXL;
    attr_.interleave_node = nodemask;
}

void AllocAttrExtension::AttrUserDefined(int size, CeNodeMask nodemask,
                                         const unsigned int *weights) {
    valid_ = true;
    attr_.alloc = CE_ALLOC_USERDEFINED;
    attr_.interleave_node = nodemask;
    for (int i = 0; i < size; i++)
        attr_.interleave_node_weight[i] = weights[i];
}

void AllocAttrExtension::AttrBWAware(CeNodeMask nodemask) {
    valid_ = true;
    attr_.alloc = CE_ALLOC_BWAWARE;
    attr_.interleave_node = nodemask;
}

const AllocAttr &AllocAttrExtension::GetAllocAttr(void) {
    return attr_;
}

void *AllocAttrExtension::operator new(size_t size) {
    void *instance = nullptr;
    instance = local_malloc(size);
    return instance;
}

void AllocAttrExtension::operator delete(void *ptr) {
    local_free(ptr);
}

AllocAttrExtension *AllocAttrHandler::env_alloc_attr_ = nullptr;
std::mutex AllocAttrHandler::mtx_;

void AllocAttrHandler::Init(void) {
    std::lock_guard<std::mutex> lock(mtx_);
    InitEnvAttr();
}

void AllocAttrHandler::Deinit(void) {
    std::lock_guard<std::mutex> lock(mtx_);
    DeinitEnvAttr();
}

void AllocAttrHandler::InitEnvAttr(void) {
    if (env_alloc_attr_ == nullptr) {
        env_alloc_attr_ = new AllocAttrExtension;
        env_alloc_attr_->AttrHost();
    }
}

void AllocAttrHandler::DeinitEnvAttr(void) {
    delete env_alloc_attr_;
}

AllocAttrExtension *AllocAttrHandler::GetEnvAttr(void) {
    if (unlikely(env_alloc_attr_ == nullptr)) {
        CE_LOG_WARN("env_alloc_attr_ is not initialized\n");
        std::lock_guard<std::mutex> lock(mtx_);
        InitEnvAttr();
    }
    return env_alloc_attr_;
}

const AllocAttr &AllocAttrHandler::GetAllocAttr(void) {
    return env_alloc_attr_->GetAllocAttr();
}
