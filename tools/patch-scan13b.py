#!/usr/bin/env python3
p = "/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/kbase-scan13.c"
src = open(p).read()

old = """    for (int i = 0; i < 10; i++) {"""
new = """    int sizes_kb[] = {64, 128, 256, 512, 1024, 4096};
    for (int i = 0; i < 6; i++) {
        uint32_t bytes = sizes_kb[i] * 1024;
        uint32_t w = 1; while (w * w * 4 < bytes) w <<= 1;"""
assert old in src, "loop"
src = src.replace(old, new)

old2 = """        AHardwareBuffer_Desc d = {
            .width = 256, .height = 256, .layers = 1,"""
new2 = """        AHardwareBuffer_Desc d = {
            .width = w, .height = w, .layers = 1,"""
assert old2 in src, "desc"
src = src.replace(old2, new2)

old3 = """        if (rc == 0) {
            uint64_t cookie = *(uint64_t *)req;
            uint64_t oflags = *(uint64_t *)(req + 16);
            void *m = mmap(NULL, 128 * 128 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, cookie);
            printf(" cookie=0x%llx outflags=0x%llx mmap=%s",
                   (unsigned long long)cookie, (unsigned long long)oflags,
                   m == MAP_FAILED ? strerror(errno) : "ok");
            if (m != MAP_FAILED) memset(m, 0x5A, 4096);
            /* keep everything alive */
        } else {"""
new3 = """        if (rc == 0) {
            uint64_t oflags = *(uint64_t *)req;
            uint64_t gpuva = *(uint64_t *)(req + 8);
            void *m = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpuva);
            printf(" outflags=0x%llx mmap=%s",
                   (unsigned long long)oflags,
                   m == MAP_FAILED ? strerror(errno) : "ok");
            if (m != MAP_FAILED) memset(m, 0x5A, 4096);
            /* keep everything alive */
        } else {"""
assert old3 in src, "mmap"
src = src.replace(old3, new3)

old4 = "printf(\"%d: import rc=%ld errno=%d(%s) fd_slot=%d numFds=%d\", i, rc, errno, strerror(errno), used, nh->numFds);"
new4 = "printf(\"%d [%uKB]: import rc=%ld errno=%d(%s) fd_slot=%d numFds=%d\", i, sizes_kb[i], rc, errno, strerror(errno), used, nh->numFds);"
assert old4 in src, "printf"
src = src.replace(old4, new4)

open(p, "w").write(src)
print("patched size sweep")
