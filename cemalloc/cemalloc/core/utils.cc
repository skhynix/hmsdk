/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "utils.h"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"

#define SYSTEM_NODE_DIR "/sys/devices/system/node"

static int max_node; ///< The number of system numa memory node.

void SetMaxNode() {
    struct dirent *dir = nullptr;
    uint32_t node_count = 0;
    DIR *directory = opendir(SYSTEM_NODE_DIR);

    CE_ASSERT(directory, "opendir error (errno: %d)\n", errno);

    while ((dir = readdir(directory)) != nullptr) {
        if (strncmp(dir->d_name, "node", 4) == 0) {
            node_count += 1;
            continue;
        }
    }
    max_node = node_count;
    closedir(directory);
}

bool MaxNodeCheck(int node) {
    bool result = false;

    if (node < max_node)
        result = true;
    return result;
}

int GetMaxNode() {
    return max_node;
}

void SetNode(CeNodeMask &nodemask, uint32_t node) {
    nodemask |= (1 << node);
}

void UnSetNode(CeNodeMask &nodemask, uint32_t node) {
    nodemask &= ~(1 << node);
}

void InitNodeMask(CeNodeMask &nodemask) {
    nodemask = 0;
}

bool ParseWeightString(CeNodeMask &interleave_node,
                       unsigned int *interleave_node_weight, const char *s,
                       int max_node) {
    char *ptr_comma = nullptr;
    char *ptr_asterisk = nullptr;
    char *s_dup = nullptr;
    char *to_free = nullptr;
    int node = 0;
    int weight = 0;
    bool ret = true;

    if (!(to_free = s_dup = strdup(s)))
        CE_LOG_ERROR("not enough memory, strdup failed\n");

    InitNodeMask(interleave_node);
    while ((ptr_comma = strsep(&s_dup, ","))) {
        ptr_asterisk = strsep(&ptr_comma, "*");
        if (strlen(ptr_asterisk) <= 0) {
            ret = false;
            break;
        }
        if (!IsNumber(ptr_asterisk)) {
            ret = false;
            break;
        }
        node = atoi(ptr_asterisk);

        if (ptr_comma == nullptr) {
            weight = 1;
        } else {
            ptr_asterisk = ptr_comma;
            if (strlen(ptr_asterisk) <= 0) {
                ret = false;
                break;
            }
            if (!IsNumber(ptr_asterisk)) {
                ret = false;
                break;
            }
            weight = atoi(ptr_asterisk);
        }

        if (node >= (int)max_node || weight < 0) {
            ret = false;
            break;
        }

        SetNode(interleave_node, node);
        interleave_node_weight[node] = (unsigned int)weight;
    }
    free(to_free);

    return ret;
}

bool IsNumber(const char *str) {
    bool result = true;

    for (size_t i = 0; i < strlen(str); ++i) {
        if (!isdigit(str[i])) {
            result = false;
            break;
        }
    }
    return result;
}

int32_t CheckBWAwareEnabled() {
    int fd = 0;
    char buf[1];
    ssize_t nread = 0;
    const std::string enabled = "/sys/kernel/mm/interleave_weight/enabled";

    if (access(enabled.c_str(), F_OK) != 0)
        return -ENOENT;

    fd = open(enabled.c_str(), O_RDONLY);
    if (fd == -1)
        return -EPERM;

    nread = read(fd, &buf, sizeof(buf));
    if ((nread == 1) && (buf[0] == '1'))
        return 0;

    return -EPERM;
}
