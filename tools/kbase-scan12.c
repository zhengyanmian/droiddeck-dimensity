/* kbase-scan12.c — probe MTK kernel legacy MEM_ALLOC ABI:
 * which struct sizes does nr=5 accept, and what do they return?
 * Context setup follows the proven scan8 pattern. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define LOCAL_PAGE_SHIFT 12
#include "kbase_uapi.h"
#include "kbase_csf_uapi.h"
#include "mali_base_common_kernel.h"
#include "mali_base_kernel.h"

#define PAGE 4096ULL

/* build _IOWR(0x80, nr, size) */
static unsigned int mkcmd(int nr, int size)
{
    return (3u << 30) | ((unsigned)size << 16) | (0x80u << 8) | (unsigned)nr;
}

int main(void)
{
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open"); return 1; }
    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc)) { perror("VER"); return 1; }
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &sf)) { perror("SET_FLAGS"); return 1; }
    if (mmap(NULL, PAGE, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE) == MAP_FAILED) {
        perror("tracking"); return 1;
    }
    printf("[ctx] ok\n");

    /* which sizes does nr=5 accept? */
    int sizes[] = { 16, 24, 32, 40, 48 };
    for (unsigned si = 0; si < sizeof(sizes)/sizeof(sizes[0]); si++) {
        int sz = sizes[si];
        unsigned char buf[64]; memset(buf, 0, sizeof buf);
        /* fill in-struct like our driver: va_pages, commit, extension, flags
         * (layout per size is whatever the kernel expects; we probe acceptance) */
        errno = 0;
        long rc = ioctl(fd, mkcmd(5, sz), buf);
        printf("MEM_ALLOC size=%2d rc=%ld errno=%d(%s)", sz, rc, errno, strerror(errno));
        if (rc == 0) {
            unsigned long long *u = (unsigned long long *)buf;
            printf("  out[0..3]=0x%llx 0x%llx 0x%llx 0x%llx",
                   u[0], u[1], u[2], u[3]);
            /* free whatever came back: try out.gpu_va candidates */
            for (int k = 0; k < 3; k++) {
                if (u[k] && (u[k] & 7) == 0 && u[k] > 0x1000) {
                    struct kbase_ioctl_mem_free fr = { .gpu_addr = u[k] };
                    ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
                }
            }
        }
        printf("\n");
    }

    /* also probe the canonical 32B union from the header for real */
    {
        union kbase_ioctl_mem_alloc req;
        memset(&req, 0, sizeof req);
        req.in.va_pages = 1;
        req.in.commit_pages = 1;
        req.in.extension = 0;
        req.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                     | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                     | BASE_MEM_CACHED_CPU;
        errno = 0;
        long rc = ioctl(fd, KBASE_IOCTL_MEM_ALLOC, &req);
        printf("MEM_ALLOC hdr(union %zu B) rc=%ld errno=%d(%s)",
               sizeof req, rc, errno, strerror(errno));
        if (rc == 0) {
            printf("  out.flags=0x%llx out.gpu_va=0x%llx",
                   (unsigned long long)req.out.flags, (unsigned long long)req.out.gpu_va);
            void *m = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, req.out.gpu_va);
            printf("  mmap=%s", m == MAP_FAILED ? strerror(errno) : "ok");
            if (m != MAP_FAILED) {
                memset(m, 0xAA, PAGE);
                printf(" cpuwrite=ok");
                munmap(m, PAGE);
            }
            struct kbase_ioctl_mem_free fr = { .gpu_addr = req.out.gpu_va };
            ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
        }
        printf("\n");
    }
    close(fd);
    return 0;
}
