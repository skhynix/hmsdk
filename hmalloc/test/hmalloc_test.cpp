/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "catch.hpp"

#include <cstdlib>
#include <cstring>
#include <hmalloc.h>
#include <vector>

extern "C" void hmalloc_init(void);

__attribute__((constructor)) void init() {
    char *env = getenv("HMALLOC_JEMALLOC");
    if (!env || !strcmp(env, "1")) {
        setenv("HMALLOC_JEMALLOC", "1", 1);
        hmalloc_init();
    }
}

static void hmalloc_test(const std::vector<size_t> &sizes) {
    std::vector<unsigned char *> v;

    for (auto size : sizes) {
        auto *ptr = static_cast<unsigned char *>(hmalloc(size));
        REQUIRE(ptr != nullptr);
        v.push_back(ptr);
        if (size > 1) {
            memset(ptr, 0xff, size);
            CHECK(ptr[0] == 0xff);
            CHECK(ptr[size - 1] == 0xff);
        }
    }

    for (auto &ptr : v)
        hfree(ptr);
}

TEST_CASE("hmalloc") {
    SECTION("single hmalloc") {
        void *ptr = hmalloc(10);
        REQUIRE(ptr);
        hfree(ptr);
    }
    SECTION("multiple hmalloc") {
        std::vector<size_t> sizes = {0,      1,      10,     5000,    10000,
                                     700000, 800000, 900000, 1000000, 0x1fffffffUL};

        SECTION("hmalloc/hfree") {
            hmalloc_test(sizes);
        }
        SECTION("repeat hmalloc/hfree") {
            for (int i = 0; i < 3; i++)
                hmalloc_test(sizes);
        }
    }
}

TEST_CASE("hfree") {
    SECTION("legit hfree") {
        void *ptr = hmalloc(1024);
        REQUIRE(ptr);
        hfree(ptr);
    }

    SECTION("nullptr hfree") {
        hfree(nullptr);
    }
}
