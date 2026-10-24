#!/usr/bin/env python3
p = "/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/kbase-scan13.c"
src = open(p).read()
old = """        unsigned char req[40];
        long rc = do_import(fd, (int *)&nh->data[0], req, F);"""
new = """        /* panvk GetMemoryFdProperties does a transient import+free probe
         * on the fd before the real import; prime the kernel first. */
        unsigned char probe[40];
        long prc = do_import(fd, (int *)&nh->data[0], probe, F);
        if (prc == 0) {
            uint64_t pc = *(uint64_t *)probe;
            struct kbase_ioctl_mem_free fr = { .gpu_addr = pc };
            ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
        }
        printf("%d: probe rc=%ld -> ", i, prc);

        unsigned char req[40];
        long rc = do_import(fd, (int *)&nh->data[0], req, F);"""
assert old in src
src = src.replace(old, new)
open(p, "w").write(src)
print("patched probe-first")
