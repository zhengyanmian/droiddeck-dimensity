/* kbase-scan3.c — after proper context init (VERSION_CHECK -> SET_FLAGS ->
 * tracking page mmap), probe import-related ioctl encodings on the MTK kernel.
 *
 * If a (nr,size,direction) combo does NOT return ENOTTY, the kernel has a
 * handler there -> try a real dma-heap import at that encoding.
 */
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

/* dma-heap (avoid kernel header deps) */
struct dma_heap_allocation_data {
    uint64_t len;
    uint32_t fd;
    uint32_t fd_flags;
    uint64_t heap_flags;
};
#define DMA_HEAP_IOC_MAGIC 'H'
#define DMA_HEAP_IOCTL_ALLOC _IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)

int main(void)
{
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open /dev/mali0"); return 1; }

    /* 1. VERSION_CHECK (CSF nr 52) */
    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc)) {
        fprintf(stderr, "VERSION_CHECK failed: %s (maj=%u min=%u)\n",
                strerror(errno), vc.major, vc.minor);
        struct kbase_ioctl_version_check vc2 = { .major = 0, .minor = 0 };
        if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc2)) {
            fprintf(stderr, "VERSION_CHECK 0.0 also failed: %s\n", strerror(errno));
            return 1;
        }
        printf("version: %u.%u\n", vc2.major, vc2.minor);
    } else {
        printf("version: %u.%u\n", vc.major, vc.minor);
    }

    /* 2. SET_FLAGS */
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &sf)) { perror("SET_FLAGS"); return 1; }
    printf("[ok] SET_FLAGS\n");

    /* 3. tracking page */
    void *tp = mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE);
    if (tp == MAP_FAILED) { perror("mmap tracking"); return 1; }
    printf("[ok] tracking page mapped\n");

    /* 4. scan import-related nrs across sizes/directions */
    int nrs[] = { 20, 21, 22, 23, 24 };
    int sizes[] = { 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128 };
    static unsigned char buf[512];
    for (unsigned ni = 0; ni < sizeof nrs / sizeof nrs[0]; ni++) {
        for (unsigned si = 0; si < sizeof sizes / sizeof sizes[0]; si++) {
            unsigned long cmd = _IOWR(0x80, nrs[ni], char[1]); /* base */
            cmd = (3UL << 30) | ((unsigned long)sizes[si] << 16) | (0x80UL << 8) | (unsigned long)nrs[ni];
            memset(buf, 0, sizeof buf);
            errno = 0;
            long rc = ioctl(fd, cmd, buf);
            int e = errno;
            if (e != ENOTTY)
                printf("nr=%-2d size=%-3d IOWR rc=%ld errno=%d (%s)\n", nrs[ni], sizes[si], rc, e, strerror(e));
        }
    }
    printf("[done] scan (only non-ENOTTY shown; nothing shown = all ENOTTY)\n");
    close(fd);
    return 0;
}
