/* kbase-scan5.c — flag/size matrix for MTK MEM_IMPORT (nr22).
 * Tries multiple flag combos and sizes to find a working import encoding.
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

static unsigned long cmd22(int size) { return (3UL << 30) | ((unsigned long)size << 16) | (0x80UL << 8) | 22; }

int main(void)
{
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open"); return 1; }
    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc)) { perror("VERSION_CHECK"); return 1; }
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &sf)) { perror("SET_FLAGS"); return 1; }
    void *tp = mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE);
    if (tp == MAP_FAILED) { perror("tracking"); return 1; }
    printf("[ok] init\n");

    int heap = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
    struct dma_heap_allocation_data ha = { .len = 4096, .fd_flags = O_RDWR | O_CLOEXEC };
    if (ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &ha)) { perror("heap alloc"); return 1; }
    close(heap);
    int dmabuf = ha.fd;
    printf("[ok] dma-buf fd=%d\n", dmabuf);

    const uint64_t RW = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                      | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR;
    struct { const char *name; uint64_t flags; } fsets[] = {
        { "RW+IMPORT_SHARED+CACHED_CPU",      RW | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU },
        { "RW+SAME_VA+CACHED_CPU",            RW | BASE_MEM_SAME_VA | BASE_MEM_CACHED_CPU },
        { "RW+SAME_VA+IMPORT_SHARED+CACHED",  RW | BASE_MEM_SAME_VA | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU },
        { "RW+UNCACHED_GPU+SAME_VA",          RW | BASE_MEM_UNCACHED_GPU | BASE_MEM_SAME_VA },
        { "RW only",                          RW },
        { "RW+COHERENT_SYSTEM(+SHARED)",      RW | BASE_MEM_COHERENT_SYSTEM | BASE_MEM_IMPORT_SHARED },
    };
    int sizes[] = { 32, 40, 48 };

    for (unsigned fi = 0; fi < sizeof fsets / sizeof fsets[0]; fi++) {
        for (unsigned si = 0; si < sizeof sizes / sizeof sizes[0]; si++) {
            unsigned char req[64];
            memset(req, 0, sizeof req);
            int fdp = dmabuf;
            uint8_t type = BASE_MEM_IMPORT_TYPE_UMM;
            memcpy(req + 0, &fsets[fi].flags, 8);
            memcpy(req + 8, &fdp, 8);
            req[16] = type; /* u8 */
            errno = 0;
            long rc = ioctl(fd, cmd22(sizes[si]), req);
            if (!(errno == ENOTTY || errno == ENOMEM))
                printf("flags[%s] size=%d rc=%ld errno=%d (%s)  out: gpu_va=0x%llx pages=%llu flags=0x%llx\n",
                       fsets[fi].name, sizes[si], rc, errno, strerror(errno),
                       *(unsigned long long *)req, *(unsigned long long *)(req + 8),
                       *(unsigned long long *)(req + 16));
            else if (sizes[si] == 40 && fi == 0)
                printf("flags[%s] size=%d rc=%ld errno=%d (baseline)\n", fsets[fi].name, sizes[si], rc, errno, strerror(errno));
        }
    }
    printf("[done]\n");
    close(dmabuf);
    close(fd);
    return 0;
}
