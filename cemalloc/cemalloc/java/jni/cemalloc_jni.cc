/* Copyright (c) 2022-2023 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "cemalloc.h"
#include "cemalloc_Cemalloc.h"
#include "explicit_indicator.h"

/**
 * @brief Java interface of SetHostMemory().
 * @param env The pointer of structure which includes JVM interface.
 * @param obj The class object which is pointing to itself.
 */
JNIEXPORT void JNICALL Java_cemalloc_Cemalloc_SetHostMemory_1(JNIEnv *env,
                                                              jobject obj) {
    SetHostMemory();
}

/**
 * @brief Java interface of SetCxlMemory().
 * @param env The pointer of structure which includes JVM interface.
 * @param obj The class object which is pointing to itself.
 */
JNIEXPORT void JNICALL Java_cemalloc_Cemalloc_SetCxlMemory_1(JNIEnv *env,
                                                             jobject obj) {
    SetCxlMemory();
}

/**
 * @brief Java interface of EnableExplicitIndicator().
 * @param env The pointer of structure which includes JVM interface.
 * @param obj The class object which is pointing to itself.
 */
JNIEXPORT void JNICALL
Java_cemalloc_Cemalloc_EnableExplicitIndicator_1(JNIEnv *env, jobject obj) {
    EnableExplicitIndicator();
}

/**
 * @brief Java interface of GetMemoryMode().
 * @param env The pointer of structure which includes JVM interface.
 * @param obj The class object which is pointing to itself.
 * @return 0 if the current memory mode is host, 1 if it is cxl.
 */
JNIEXPORT int JNICALL Java_cemalloc_Cemalloc_GetMemoryMode_1(JNIEnv *env,
                                                             jobject obj) {
    return GetMemoryMode();
}
