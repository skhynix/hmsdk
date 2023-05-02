/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

package cemalloc;

/**
 * @brief Define native methods which are defined in cemalloc.
 */
public class Cemalloc {
    public Cemalloc() {
        EnableExplicitIndicator();
    }

    /// @brief Define memory mode.
    public enum MemoryMode { HOST, CXL }

    native public void EnableExplicitIndicator_();
    native public void SetHostMemory_();
    native public void SetCxlMemory_();
    native public int GetMemoryMode_();

    /// @brief Call jni function
    public void EnableExplicitIndicator() {
        EnableExplicitIndicator_();
    }
    /// @brief Call jni function
    public void SetHostMemory() {
        SetHostMemory_();
    }
    /// @brief Call jni function
    public void SetCxlMemory() {
        SetCxlMemory_();
    }
    /**
     * @brief Call jni function
     * @return HOST if the current memory mode is host, CXL if it is cxl.
     */
    public MemoryMode GetMemoryMode() {
        if (GetMemoryMode_() == 0) {
            return MemoryMode.HOST;
        } else {
            return MemoryMode.CXL;
        }
    }

    static {
        System.loadLibrary("cemallocjava");
    }
}
