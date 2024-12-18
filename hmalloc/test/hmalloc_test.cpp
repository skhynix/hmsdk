/* Copyright (c) 2024 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include "catch.hpp"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <hmalloc.h>
#include <jemalloc/jemalloc.h>
#include <numa.h>
#include <numaif.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <vector>

#ifndef MPOL_PREFERRED_MANY
#define MPOL_PREFERRED_MANY 5
#endif

#ifndef MPOL_WEIGHTED_INTERLEAVE
#define MPOL_WEIGHTED_INTERLEAVE 6
#endif

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

static void mempolicy_test(int policy, unsigned long nodemask, int maxnode, void *addr) {
    int hpolicy;
    unsigned long hnodemask;

    CHECK(0 == get_mempolicy(&hpolicy, &hnodemask, maxnode, addr, MPOL_F_ADDR));
    CHECK(policy == hpolicy);
    CHECK(nodemask == hnodemask);
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

TEST_CASE("haligned_alloc") {
    SECTION("alignment power of two") {
        size_t alignment = 1024;
        size_t size = 1 * mb;
        void *ptr = haligned_alloc(alignment, size);
        REQUIRE(ptr);
        CHECK(0 == reinterpret_cast<uintptr_t>(ptr) % alignment);
        CHECK(0 < hmalloc_usable_size(ptr));
        hfree(ptr);
    }

#if !defined(__SANITIZE_ADDRESS__)
    SECTION("alignment not a power of two (fails in ptmalloc)") {
        size_t size = 1 * mb;
        REQUIRE(nullptr == haligned_alloc(1024 + 1, size));
        CHECK(EINVAL == errno);
    }

    SECTION("alignment zero (fails in ptmalloc)") {
        size_t size = 1 * mb;
        REQUIRE(nullptr == haligned_alloc(0, size));
        CHECK(EINVAL == errno);
    }

    SECTION("size not a multiple of alignment") {
        size_t alignment = 1024;
        size_t size = 1 * kb * alignment; /* 1 mb */
        void *ptr = haligned_alloc(alignment, size + 1);
        REQUIRE(ptr);
        CHECK(0 == reinterpret_cast<uintptr_t>(ptr) % alignment);
        CHECK(0 < hmalloc_usable_size(ptr));
        hfree(ptr);
    }
#endif

    SECTION("size zero") {
        size_t alignment = 1024;
        size_t size = 0;
        void *ptr = haligned_alloc(alignment, size);
        REQUIRE(ptr);
        CHECK(0 == reinterpret_cast<uintptr_t>(ptr) % alignment);
        CHECK(0 < hmalloc_usable_size(ptr));
        hfree(ptr);
    }
}

TEST_CASE("hposix_memalign") {
    SECTION("alignment power of two") {
        void *ptr;
        size_t alignment = 1024;
        size_t size = 1 * mb;
        REQUIRE(0 == hposix_memalign(&ptr, alignment, size));
        REQUIRE(ptr);
        CHECK(0 == reinterpret_cast<uintptr_t>(ptr) % alignment);
        CHECK(0 < hmalloc_usable_size(ptr));
        hfree(ptr);
    }

#if !defined(__SANITIZE_ADDRESS__)
    SECTION("alignment not a power of two") {
        void *ptr;
        size_t alignment = 1024;
        size_t size = 1 * mb;
        CHECK(EINVAL == hposix_memalign(&ptr, alignment + 1, size));
        CHECK(nullptr == ptr);
    }

    SECTION("alignment zero") {
        void *ptr;
        size_t alignment = 0;
        size_t size = 1 * mb;
        CHECK(EINVAL == hposix_memalign(&ptr, alignment, size));
        CHECK(nullptr == ptr);
    }
#endif

    SECTION("size zero") {
        void *ptr;
        size_t alignment = 1024;
        size_t size = 0;
        REQUIRE(0 == hposix_memalign(&ptr, alignment, size));
        REQUIRE(ptr);
        CHECK(0 == reinterpret_cast<uintptr_t>(ptr) % alignment);
        CHECK(0 < hmalloc_usable_size(ptr));
        hfree(ptr);
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

TEST_CASE("hmmap/hmunmap") {
    SECTION("anonymous") {
        size_t size = 1 * mb;
        void *ptr = hmmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
        REQUIRE(MAP_FAILED != ptr);
        CHECK(0 == hmunmap(ptr, size));
    }

    SECTION("file map") {
        const char *filename = "/tmp/__hmalloc.txt";
        FILE *fp = fopen(filename, "w+");
        REQUIRE(fp);

        int fd = fileno(fp);
        size_t file_size = 1 * mb;

        REQUIRE(fd >= 0);
        REQUIRE(0 == ftruncate(fd, file_size));

        void *ptr = hmmap(NULL, file_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
        REQUIRE(MAP_FAILED != ptr);

        CHECK(0 == hmunmap(ptr, file_size));
        CHECK(0 == remove(filename));

        CHECK(0 == fclose(fp));
    }
}

TEST_CASE("mbind") {
    /* skip this test when the system has a single numa node */
    int maxnode = numa_max_node() + 2;
    if (maxnode < 3)
        return;

    void *new_addr = nullptr;
    size_t size = 0x1fffffffUL; /* 500 MiB */

    struct bitmask *mask = numa_get_mems_allowed();
    unsigned long nodemask = *mask->maskp;

    char bm[1024] = "0";
    snprintf(bm, sizeof(bm), "%lu", nodemask);

    SECTION("MPOL_BIND") {
        setenv("HMALLOC_MPOL_MODE", "2", 1); /* MPOL_BIND is 2 */
        setenv("HMALLOC_NODEMASK", bm, 1);
        update_env();

        new_addr = extent_alloc(nullptr, new_addr, size, 0, nullptr, nullptr, 0);
        REQUIRE(new_addr);
        memset(new_addr, 0, size);

        mempolicy_test(MPOL_BIND, nodemask, maxnode, new_addr);
        CHECK(0 == munmap(new_addr, size));
    }

    SECTION("MPOL_PREFERRED") {
        /* MPOL_PREFERRED accepts only a single node so pick lsb node only */
        unsigned long testnode = 1 << (ffs(nodemask) - 1);
        snprintf(bm, sizeof(bm), "%lu", testnode);

        setenv("HMALLOC_MPOL_MODE", "1", 1); /* MPOL_PREFERRED is 1 */
        setenv("HMALLOC_NODEMASK", bm, 1);
        update_env();

        new_addr = extent_alloc(nullptr, new_addr, size, 0, nullptr, nullptr, 0);
        REQUIRE(new_addr);
        memset(new_addr, 0, size);

        mempolicy_test(MPOL_PREFERRED, testnode, maxnode, new_addr);
        CHECK(0 == munmap(new_addr, size));
    }

    SECTION("MPOL_PREFERRED_MANY") {
        setenv("HMALLOC_MPOL_MODE", "5", 1); /* MPOL_PREFERRED_MANY is 5 */
        setenv("HMALLOC_NODEMASK", bm, 1);
        update_env();

        new_addr = extent_alloc(nullptr, new_addr, size, 0, nullptr, nullptr, 0);
        REQUIRE(new_addr);
        memset(new_addr, 0, size);

        mempolicy_test(MPOL_PREFERRED_MANY, nodemask, maxnode, new_addr);
        CHECK(0 == munmap(new_addr, size));
    }
    SECTION("MPOL_INTERLEAVE") {
        setenv("HMALLOC_MPOL_MODE", "3", 1); /* MPOL_INTERLEAVE is 3 */
        setenv("HMALLOC_NODEMASK", bm, 1);
        update_env();

        new_addr = extent_alloc(nullptr, new_addr, size, 0, nullptr, nullptr, 0);
        REQUIRE(new_addr);
        memset(new_addr, 0, size);

        mempolicy_test(MPOL_INTERLEAVE, nodemask, maxnode, new_addr);
        CHECK(0 == munmap(new_addr, size));
    }
    SECTION("MPOL_WEIGHTED_INTERLEAVE") {
        struct utsname buffer;
        int major, minor;

        if (uname(&buffer) != 0) {
            perror("uname");
            return;
        }

        /*
         * MPOL_WEIGHTED_INTERLEAVE is supported from kernel v6.9 so skip this test
         * if kernel version is lower than v6.9.
         */
        sscanf(buffer.release, "%d.%d", &major, &minor);
        if (major < 6 || (major == 6 && minor < 9)) {
            CHECK("SKIP: not supported kernel version");
            return;
        }

        setenv("HMALLOC_MPOL_MODE", "6", 1); /* MPOL_WEIGHTED_INTERLEAVE is 6 */
        setenv("HMALLOC_NODEMASK", bm, 1);
        update_env();

        new_addr = extent_alloc(nullptr, new_addr, size, 0, nullptr, nullptr, 0);
        REQUIRE(new_addr);
        memset(new_addr, 0, size);

        mempolicy_test(MPOL_WEIGHTED_INTERLEAVE, nodemask, maxnode, new_addr);
        CHECK(0 == munmap(new_addr, size));
    }
}
