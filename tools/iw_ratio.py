#!/usr/bin/env python3
# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import math

SOFT_MAX_RATIO_VAL = 10


def list_gcd(data):
    lgcd = data[0]
    for i in range(1, len(data)):
        lgcd = math.gcd(lgcd, data[i])
    return lgcd


def get_iw_ratio(data, soft_max=SOFT_MAX_RATIO_VAL):
    new_data = [0] * len(data)
    quit = False

    while True:
        if max(data) <= soft_max:
            break
        for i in range(len(data)):
            new_data[i] = data[i] / 2
            # new_data[i] != 0 check is to handle when cpu node doesn't have memory.
            if new_data[i] != 0 and new_data[i] <= 1:
                quit = True
                break
        if quit:
            break
        data = new_data[:]

    data = list(map(lambda x: round(x), data))
    lgcd = list_gcd(data)
    if lgcd != 0:
        data = list(map(lambda x: round(x / lgcd), data))

    return data


def get_iw_ratio_matrix(bw_matrix, possible):
    ratio_matrix = [0] * len(bw_matrix)
    for nid in possible:
        ratio_matrix[nid] = get_iw_ratio(bw_matrix[nid])
    return ratio_matrix
