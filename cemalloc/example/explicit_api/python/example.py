# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import numpy as np

import cemalloc

ARRAY_SIZE = 1024

if __name__ == "__main__":
    cemalloc.SetCxlMemory()
    assert cemalloc.GetMemoryMode().name == "CXL"

    arr = np.zeros(ARRAY_SIZE)
    for i in range(ARRAY_SIZE):
        arr[i] = i

    cemalloc.SetHostMemory()
    assert cemalloc.GetMemoryMode().name == "HOST"

    arr2 = np.zeros(ARRAY_SIZE)
    for i in range(ARRAY_SIZE):
        arr2[i] = i
