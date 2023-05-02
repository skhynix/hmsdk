/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "operation_mode.h"

#include "logging.h"

__thread bool AllocPathImpl::use_explicit_indicator_ = false;

CeAllocPath AllocPathImpl::ImplicitAllocPath(void) {
    CE_LOG_VERBOSE("ImplicitAllocPath\n");
    return CeAllocPath::kJePath;
}

CeAllocPath AllocPathImpl::ExplicitAllocPath(void) {
    CE_LOG_VERBOSE("ExplicitAllocPath\n");
    return CeAllocPath::kLibcPath;
}

CeAllocPath AllocPathImpl::ExplicitIndicatorAllocPath(void) {
    CeAllocPath path = CeAllocPath::kLibcPath;
    CE_LOG_VERBOSE("ExplicitIndicatorAllocPath\n");

    if (use_explicit_indicator_) {
        AllocAttr attr = AllocAttrHandler::GetAllocAttr();
        path = AttrToMode(attr);
    } else {
        path = CeAllocPath::kLibcPath;
    }
    return path;
}

void AllocPathImpl::SetExplicitIndicatorStatus(bool status) {
    use_explicit_indicator_ = status;
}

bool AllocPathImpl::GetExplicitIndicatorStatus(void) {
    return use_explicit_indicator_;
}

CeAllocPath AllocPathImpl::AttrToMode(AllocAttr attr) {
    if (attr.alloc == CE_ALLOC_HOST)
        return CeAllocPath::kLibcPath;
    return CeAllocPath::kJePath;
}

CeMode CeModeHandler::mode_ = CE_EXPLICIT;
AllocPathHandler CeModeHandler::path_handler_ =
    &AllocPathImpl::ExplicitAllocPath;
std::mutex CeModeHandler::mtx_;

CeAllocPath CeModeHandler::GetAllocPath(void) {
    return path_handler_();
}

bool CeModeHandler::IsLibcPath(void) {
    return GetAllocPath() == CeAllocPath::kLibcPath;
}

CeMode CeModeHandler::GetCeMode(void) {
    return mode_;
}

int32_t CeModeHandler::SetCeMode(CeMode mode) {
    CE_LOG_VERBOSE("SetCeMode: %d->%d\n", (int)mode_, (int)mode);

    std::lock_guard<std::mutex> lock(mtx_);
    switch (mode) {
    case CE_IMPLICIT:
        path_handler_ = &AllocPathImpl::ImplicitAllocPath;
        break;
    case CE_EXPLICIT:
        path_handler_ = &AllocPathImpl::ExplicitAllocPath;
        break;
    case CE_EXPLICIT_INDICATOR:
        path_handler_ = &AllocPathImpl::ExplicitIndicatorAllocPath;
        break;
    default:
        CE_LOG_ERROR("unknown CE_MODE: %d\n", mode);
    }
    mode_ = mode;
    return 0;
}
