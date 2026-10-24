/* kbase-scan6.c — final import matrix: dma-heap type x bit26(SYNC_ON_MAP_UNMAP) x flags.
 * Vendor driver uses IOWR(0x80,22,40) with flags |= 0x4000000.
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
#include "mali_base_kernel.h"

struct dma_heap_allocation_data {
    uint64_t len;
    uint32_t fd;
    uint32_t fd_flags;
    uint64_t heap_flags;
};
#define DMA_HEAP_IOC_MAGIC 'H'
#define DMA_HEAP_IOCTL_ALLOC _IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)

#define MTK_MEM_IMPORT _IOWR(0x80, 22, char[40])

static int heap_alloc(const char *node, uint64_t len)
{
    int h = open(node, O_RDONLY | O_CLOEXEC);
    if (h < 0) return -1;
    struct dma_heap_allocation_data ha = { .len = len, .fd_flags = O_RDWR | O_CLOEXEC };
    if (ioctl(h, DMA_HEAP_IOCTL_ALLOC, &ha)) { close(h); return -1; }
    close(h);
    return ha.fd;
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
    printf("[ok] init\n");

    const uint64_t RW = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                      | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR;
    struct { const char *heap; } heaps[] = {
        { "/dev/dma_heap/system" },
        { "/dev/dma_heap/mtk_mm" },
        { "/dev/dma_heap/mtk_mm-uncached" },
    };
    struct { const char *name; uint64_t flags; } fsets[] = {
        { "RW+SHARED+CACHED",          RW | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU },
        { "RW+SHARED+CACHED+SYNC26",   RW | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP },
        { "RW+SHARED+CACHED+SAME_VA",  RW | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU | BASE_MEM_SAME_VA },
        { "RW+CACHED+SYNC26",          RW | BASE_MEM_CACHED_CPU | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP },
    };

    for (unsigned hi = 0; hi < sizeof heaps / sizeof heaps[0]; hi++) {
        int dmabuf = heap_alloc(heaps[hi].heap, 4096);
        if (dmabuf < 0) { printf("[skip] %s: %s\n", heaps[hi].heap, strerror(errno)); continue; }
        printf("heap %s fd=%d\n", heaps[hi].heap, dmabuf);
        for (unsigned fi = 0; fi < sizeof fsets / sizeof fsets[0]; fi++) {
            unsigned char req[40];
            memset(req, 0, sizeof req);
            int fdp = dmabuf;
            uint8_t type = 2;
            uint64_t flags = fsets[fi].flags;
            memcpy(req + 0, &flags, 8);
            memcpy(req + 8, &fdp, 8);
            req[16] = type;
            errno = 0;
            long rc = ioctl(fd, MTK_MEM_IMPORT, req);
            printf("  %-28s rc=%ld errno=%d (%s)", fsets[fi].name, rc, errno, strerror(errno));
            if (rc == 0) {
                uint64_t gpu_va, va_pages, of;
                memcpy(&gpu_va, req, 8); memcpy(&va_pages, req + 8, 8); memcpy(&of, req + 16, 8);
                printf("  GPU_VA=0x%llx pages=%llu flags=0x%llx", (unsigned long long)gpu_va,
                       (unsigned long long)va_pages, (unsigned long long)of);
                struct kbase_ioctl_mem_free fr = { .gpu_addr = gpu_va };
                ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
            }
            printf("\n");
        }
        close(dmabuf);
    }
    printf("[done]\n");
    close(fd);
    return 0;
}
