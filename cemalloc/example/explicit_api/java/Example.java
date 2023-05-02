/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

import cemalloc.Cemalloc;

class Example {
    public static void main(String[] args) {
        int arr_length = 1024 * 1024 * 1024;
        int[] arr_host = null;
        int[] arr_cxl = null;

        Cemalloc test = new Cemalloc();

        test.SetCxlMemory();
        assert test.GetMemoryMode().name() == "CXL";
        arr_cxl = new int[arr_length];

        test.SetHostMemory();
        assert test.GetMemoryMode().name() == "HOST";
        arr_host = new int[arr_length];
    }
}
