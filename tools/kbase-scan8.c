/* kbase-scan8.c — verify "first import fails ENOMEM, retry succeeds" hypothesis */
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

struct dma_heap_allocation_data {
    uint64_t len; uint32_t fd; uint32_t fd_flags; uint64_t heap_flags;
};
#define DMA_HEAP_IOC_MAGIC 'H'
#define DMA_HEAP_IOCTL_ALLOC _IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)
#define MTK_MEM_IMPORT _IOWR(0x80, 22, char[40])

static long do_import(int fd, int *fdp, unsigned char *req, uint64_t flags)
{
    memset(req, 0, 40);
    memcpy(req, &flags, 8);
    memcpy(req + 8, &fdp, 8);
    req[16] = 2;
    { printf("REQ:"); for (int _i=0;_i<40;_i++) printf("%02x", req[_i]); printf("\n"); }
    errno = 0;
    return ioctl(fd, MTK_MEM_IMPORT, req);
}

int main(void)
{
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open"); return 1; }
    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc)) { perror("VER"); return 1; }
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &sf)) { perror("SET_FLAGS"); return 1; }
    if (mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE) == MAP_FAILED) {
        perror("tracking"); return 1;
    }

    int heap = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
    struct dma_heap_allocation_data ha = { .len = 4096, .fd_flags = O_RDWR | O_CLOEXEC };
    if (ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &ha)) { perror("alloc"); return 1; }
    close(heap);
    int dmabuf = ha.fd;

    const uint64_t F = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                     | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                     | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU
                     | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP;

    unsigned char req[40];
    for (int attempt = 1; attempt <= 4; attempt++) {
        long rc = do_import(fd, &dmabuf, req, F);
        printf("attempt %d: rc=%ld errno=%d (%s)", attempt, rc, errno, strerror(errno));
        if (rc == 0) {
            printf("  gpu_va=0x%llx va_pages=%llu out.flags=0x%llx",
                   *(unsigned long long *)req, *(unsigned long long *)(req + 8),
                   *(unsigned long long *)(req + 16));
            /* keep the first success mapped for later attempts; free on last */
            if (attempt == 4) {
                struct kbase_ioctl_mem_free fr = { .gpu_addr = *(uint64_t *)req };
                ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
            }
        }
        printf("\n");
    }
    close(dmabuf);
    close(fd);
    return 0;
}
