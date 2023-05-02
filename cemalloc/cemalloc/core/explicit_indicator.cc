/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "explicit_indicator.h"

#include "logging.h"
#include "operation_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

void EnableExplicitIndicator(void) {
    CeMode mode = CeModeHandler::GetCeMode();
    if (mode != CE_EXPLICIT_INDICATOR)
        CeModeHandler::SetCeMode(CE_EXPLICIT_INDICATOR);
}

int SetCxlMemory(void) {
    CeMode mode = CeModeHandler::GetCeMode();
    if (mode == CE_EXPLICIT_INDICATOR) {
        AllocPathImpl::SetExplicitIndicatorStatus(true);
        return 0;
    }
    CE_LOG_WARN("CeMode is not CE_EXPLICIT_INDICATOR:[%d]\n", (int)mode);
    return -1;
}

int SetHostMemory(void) {
    CeMode mode = CeModeHandler::GetCeMode();
    if (mode == CE_EXPLICIT_INDICATOR) {
        AllocPathImpl::SetExplicitIndicatorStatus(false);
        return 0;
    }
    CE_LOG_WARN("CeMode is not CE_EXPLICIT_INDICATOR:[%d]\n", (int)mode);
    return -1;
}

int GetMemoryMode(void) {
    bool status = AllocPathImpl::GetExplicitIndicatorStatus();
    if (status)
        return 1;
    return 0;
}

#ifdef __cplusplus
}
#endif
