/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "env_parser.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

#include <sstream>
#include <vector>

#include "logging.h"
#include "utils.h"

CeMode GetEnvCeMode(void) {
    char *env = getenv(ENV_CE_MODE);
    CeMode ce_mode{};

    if (!env) {
        CE_LOG_WARN("%s is not set, so it is set to default(CE_IMPLICIT).\n",
                    ENV_CE_MODE);
        return CE_IMPLICIT;
    }
    if (!strcmp(env, "CE_IMPLICIT"))
        ce_mode = CE_IMPLICIT;
    else if (!strcmp(env, "CE_EXPLICIT"))
        ce_mode = CE_EXPLICIT;
    else if (!strcmp(env, "CE_EXPLICIT_INDICATOR"))
        ce_mode = CE_EXPLICIT_INDICATOR;
    else
        CE_LOG_ERROR("Invalid Parameter: %s=%s\n", ENV_CE_MODE, env);

    return ce_mode;
}

/**
 * @brief Read $CE_ALLOC which means allocation attribute.
 * @return CeAlloc based on CE_ALLOC environment variable.
 * @note CE_ALLOC=CE_ALLOC_HOST | CE_ALLOC_CXL | CE_ALLOC_USERDEFINED |
 *                CE_ALLOC_BWAWARE
 */
CeAlloc GetEnvCeAlloc(void) {
    char *env = getenv(ENV_CE_ALLOC);
    CeAlloc ce_alloc{};

    if (!env) {
        CE_LOG_WARN("%s is not set, so it is set to default(CE_ALLOC_HOST).\n",
                    ENV_CE_ALLOC);
        return CE_ALLOC_HOST;
    }
    if (!strcmp(env, "CE_ALLOC_HOST"))
        ce_alloc = CE_ALLOC_HOST;
    else if (!strcmp(env, "CE_ALLOC_CXL"))
        ce_alloc = CE_ALLOC_CXL;
    else if (!strcmp(env, "CE_ALLOC_USERDEFINED"))
        ce_alloc = CE_ALLOC_USERDEFINED;
    else if (!strcmp(env, "CE_ALLOC_BWAWARE"))
        ce_alloc = CE_ALLOC_BWAWARE;
    else
        CE_LOG_ERROR("Invalid Parameter: %s=%s\n", ENV_CE_ALLOC, env);

    return ce_alloc;
}

/**
 * @brief Parser of $CE_CXL_NODE which means cxl node number.
 * @param ce_alloc Reference to AllocAttr.
 * @note CE_CXL_NODE should be an integer.
 */
void ParseCeCxlNode(AllocAttr &ce_alloc) {
    char *env = getenv(ENV_CE_CXL_NODE);

    if (!env)
        CE_LOG_ERROR("%s is not set. Please set as below.\n"
                     "export CE_CXL_NODE=[node]\n",
                     ENV_CE_CXL_NODE);

    if (!IsNumber(env) || !MaxNodeCheck(atoi(env)))
        CE_LOG_ERROR("Invalid Parameter: %s=%s\n", ENV_CE_CXL_NODE, env);

    SetNode(ce_alloc.interleave_node, atoi(env));
}

/**
 * @brief Parser of $CE_INTERLEAVE_NODE which means node number and allocation
 *        ratio.
 * @param ce_attr Reference to AllocAttr.
 * @note CE_INTERLEAVE_NODE is a list of node_number*weight.\n
 *       If the weight is omitted, it means the weight=1.\n
 *    ex> CE_INTERLEAVE_NODE=0*2,1*1\n
 *    ex> CE_INTERLEAVE_NODE=1,2 // weight of node 1 and 2 is set to 1.\n
 *    ex> CE_INTERLEAVE_NODE=1*3,2 // weight of node 2 is set to 1.
 */
void ParseCeInterleaveNode(AllocAttr &ce_attr) {
    char *env = getenv(ENV_CE_INTERLEAVE_NODE);

    if (!env)
        CE_LOG_ERROR("%s is not set. Please set as below.\n"
                     "export CE_INTERLEAVE_NODE=[node]*[weight]\n",
                     ENV_CE_INTERLEAVE_NODE);

    int max_node = GetMaxNode();
    int result = ParseWeightString(
        ce_attr.interleave_node, ce_attr.interleave_node_weight, env, max_node);
    if (!result)
        CE_LOG_ERROR("Invalid Parameter: %s=%s\n", ENV_CE_INTERLEAVE_NODE, env);
}

int32_t EnvParser(void) {
    CeMode ce_mode{};
    AllocAttr ce_attr{};
    int size = GetMaxNode();
    AllocAttrExtension *env_alloc_attr = nullptr;
    int32_t enabled_status = 0;

    env_alloc_attr = AllocAttrHandler::GetEnvAttr();

    ce_mode = GetEnvCeMode();
    CeModeHandler::SetCeMode(ce_mode);

    ce_attr.alloc = GetEnvCeAlloc();
    switch (ce_attr.alloc) {
    case CE_ALLOC_HOST:
        env_alloc_attr->AttrHost();
        break;
    case CE_ALLOC_CXL:
        ParseCeCxlNode(ce_attr);
        env_alloc_attr->AttrCxl(ce_attr.interleave_node);
        break;
    case CE_ALLOC_BWAWARE:
        enabled_status = CheckBWAwareEnabled();
        if (enabled_status == -ENOENT) {
            CE_LOG_ERROR(
                "/sys/kernel/mm/interleave_weight/enabled is not found.\n"
                "CE_ALLOC_BWAWARE mode works only in HMSDK kernel.\n");
        } else if (enabled_status == -EPERM) {
            CE_LOG_ERROR("CE_ALLOC_BWAWARE is not enabled.\n"
                         "(try executing `bwactl.py`)\n");
        }
        env_alloc_attr->AttrBWAware(ce_attr.interleave_node);
        break;
    case CE_ALLOC_USERDEFINED:
        ParseCeInterleaveNode(ce_attr);
        env_alloc_attr->AttrUserDefined(size, ce_attr.interleave_node,
                                        ce_attr.interleave_node_weight);
        break;
    }

    return 0;
}
