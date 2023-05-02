# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

from enum import Enum

from cemalloc import cemalloc_wrapper


## MemoryMode enum class
class MemoryMode(Enum):
    HOST = 0
    CXL = 1


cemalloc_wrapper.EnableExplicitIndicator()


def SetCxlMemory():
    return cemalloc_wrapper.SetCxlMemory()


def SetHostMemory():
    return cemalloc_wrapper.SetHostMemory()


def GetMemoryMode():
    if cemalloc_wrapper.GetMemoryMode() == 0:
        return MemoryMode.HOST
    else:
        return MemoryMode.CXL
