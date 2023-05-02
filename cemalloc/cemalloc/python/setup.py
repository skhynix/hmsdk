# Copyright (c) 2022-2023 SK hynix, Inc.
# SPDX-License-Identifier: BSD 2-Clause

import os
from distutils.core import setup

import setuptools

setup(
    name="cemalloc",
    version="1.1",
    package_dir={"cemalloc": ""},
    packages=["cemalloc"],
)
