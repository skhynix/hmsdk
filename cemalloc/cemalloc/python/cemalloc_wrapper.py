# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import ctypes
import os

if os.environ.get("LIBCEMALLOC_DIR") is not None:
    _lib_path = os.path.join(os.environ["LIBCEMALLOC_DIR"], "libcemalloc.so")
else:
    _lib_path = "libcemalloc.so"

_lib = ctypes.CDLL(_lib_path)


def SetCxlMemory():
    return _lib.SetCxlMemory()


def SetHostMemory():
    return _lib.SetHostMemory()


def EnableExplicitIndicator():
    _lib.EnableExplicitIndicator()


def GetMemoryMode():
    return _lib.GetMemoryMode()
