/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "catch.hpp"

#include <cstdlib>
#include <cstring>
#include <hmalloc.h>
#include <jemalloc/jemalloc.h>
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>
#include <vector>

extern "C" {
void update_env(void);
void hmalloc_init(void);
void *extent_alloc(extent_hooks_t *extent_hooks, void *new_addr, size_t size, size_t alignment,
                   bool *zero, bool *commit, unsigned arena_ind);
}

static constexpr auto kb = 1024UL;
static constexpr auto mb = 1024UL * kb;
static constexpr auto gb = 1024UL * mb;

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

TEST_CASE("hcalloc") {
    size_t nmemb = 1 * mb;
    size_t size = sizeof(char);

    SECTION("zeroing check") {
        auto *hptr = static_cast<char *>(hcalloc(nmemb, size));
        REQUIRE(hptr);

        auto *ptr = static_cast<char *>(calloc(nmemb, size));
        REQUIRE(ptr);

        CHECK(0 == memcmp(ptr, hptr, nmemb * size));

        free(ptr);
        hfree(hptr);
    }
}

TEST_CASE("hmalloc_usable_size") {
    SECTION("legit pointer") {
        void *ptr = hmalloc(1024);
        REQUIRE(ptr);
        CHECK(0 < hmalloc_usable_size(ptr));
        hfree(ptr);
    }

    SECTION("NULL pointer") {
        CHECK(0 == hmalloc_usable_size(nullptr));
    }
}

TEST_CASE("hrealloc") {
    size_t old_size = 1 * mb;
    auto *old_ptr = static_cast<char *>(hcalloc(old_size, sizeof(char)));
    REQUIRE(old_ptr);

    SECTION("size zero") {
        size_t new_size = 0;
        CHECK(nullptr == static_cast<char *>(hrealloc(old_ptr, new_size)));
    }

    SECTION("old_size < new_size") {
        size_t new_size = old_size * 2;
        auto *new_ptr = static_cast<char *>(hrealloc(old_ptr, new_size));
        REQUIRE(new_ptr);
        CHECK(new_size <= hmalloc_usable_size(new_ptr));
        hfree(new_ptr);
    }

    SECTION("old_size > new_size") {
        size_t new_size = old_size / 2;
        auto *new_ptr = static_cast<char *>(hrealloc(old_ptr, new_size));
        REQUIRE(new_ptr);
        CHECK(new_size <= hmalloc_usable_size(new_ptr));
        CHECK(old_size > hmalloc_usable_size(new_ptr));
        hfree(new_ptr);
    }

    SECTION("old_ptr is nullptr") {
        size_t new_size = old_size;
        hfree(old_ptr);
        old_ptr = nullptr;
        auto *new_ptr = static_cast<char *>(hrealloc(old_ptr, new_size));
        REQUIRE(new_ptr);
        CHECK(new_size <= hmalloc_usable_size(new_ptr));
        hfree(new_ptr);
    }

    SECTION("old_ptr is nullptr and size zero") {
        hfree(old_ptr);
        old_ptr = nullptr;
        size_t new_size = 0;
        auto *new_ptr = static_cast<char *>(hrealloc(old_ptr, new_size));
        REQUIRE(new_ptr);
        CHECK(0 < hmalloc_usable_size(new_ptr));
        hfree(new_ptr);
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

TEST_CASE("mbind") {
    /* skip this test when the system has a single numa node */
    int maxnode = numa_max_node() + 2;
    if (maxnode < 3)
        return;

    void *new_addr = nullptr;
    size_t size = 0x1fffffffUL; /* 500 MiB */

    setenv("HMALLOC_NODEMASK", "2", 1);  /* nodemask 2 means node 1 */
    setenv("HMALLOC_MPOL_MODE", "2", 1); /* MPOL_BIND is 2 */
    update_env();

    new_addr = extent_alloc(nullptr, new_addr, size, 0, nullptr, nullptr, 0);
    REQUIRE(new_addr);
    memset(new_addr, 0, size);

    SECTION("success") {
        int nid = 1;
        unsigned long nodemask = 1 << nid;
        CHECK(0 == mbind(new_addr, size, MPOL_BIND, &nodemask, maxnode, MPOL_MF_STRICT));
    }

    SECTION("failure") {
        SECTION("incorrect nid") {
            int nid = 0;
            unsigned long nodemask = 1 << nid;
            CHECK(-1 == mbind(new_addr, size, MPOL_BIND, &nodemask, maxnode, MPOL_MF_STRICT));
            CHECK(errno == EIO);
        }
        SECTION("incorrect mpol") {
            int nid = 1;
            unsigned long nodemask = 1 << nid;
            CHECK(-1 != mbind(new_addr, size, MPOL_PREFERRED, &nodemask, maxnode, MPOL_MF_STRICT));
        }
    }
    CHECK(0 == munmap(new_addr, size));
}
