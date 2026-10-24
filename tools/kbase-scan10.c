/* kbase-scan10.c — A/B test: cpu-map-before-import vs not; retry with backoff */
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

static int ctx_init(int *fd)
{
    *fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (*fd < 0) return -1;
    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(*fd, KBASE_IOCTL_VERSION_CHECK, &vc)) return -2;
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(*fd, KBASE_IOCTL_SET_FLAGS, &sf)) return -3;
    if (mmap(NULL, 4096, PROT_NONE, MAP_SHARED, *fd, BASE_MEM_MAP_TRACKING_HANDLE) == MAP_FAILED) return -4;
    return 0;
}

static int import_dmabuf(int fd, int dmabuf, uint64_t *out_gpu_va)
{
    const uint64_t F = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                     | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                     | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU
                     | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP;
    for (int a = 0; a < 12; a++) {
        unsigned char req[40];
        memset(req, 0, sizeof req);
        int fdp = dmabuf;
        memcpy(req, &F, 8);
        memcpy(req + 8, &fdp, 8);
        req[16] = 2;
        errno = 0;
        { unsigned char _d[40]; memcpy(_d, req, 40); printf("REQ:"); for (int _i=0;_i<40;_i++) printf("%02x", _d[_i]); printf("\n"); } if (ioctl(fd, MTK_MEM_IMPORT, req) == 0) {
            *out_gpu_va = *(uint64_t *)(req + 8);
            return 0;
        }
        if (errno != ENOMEM) return -1;
        usleep(50000 * (a + 1)); /* 50ms, 100ms, ... growing */
    }
    return -2;
}

static int heap_alloc(const char *node, uint64_t len)
{
    int h = open(node, O_RDONLY | O_CLOEXEC);
    if (h < 0) return -1;
    struct dma_heap_allocation_data ha = { .len = len, .fd_flags = O_RDWR | O_CLOEXEC };
    if (ioctl(h, DMA_HEAP_IOCTL_ALLOC, &ha)) { close(h); return -1; }
    close(h);
    return ha.fd;
}

int main(int argc, char **argv)
{
    int do_cpumap = argc > 1 && argv[1][0] == '1';
    int fd;
    if (ctx_init(&fd)) { perror("ctx"); return 1; }
    int dmabuf = heap_alloc("/dev/dma_heap/system", 4096);
    if (dmabuf < 0) { perror("heap"); return 1; }

    if (do_cpumap) {
        unsigned char *cpu = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dmabuf, 0);
        if (cpu != MAP_FAILED) { memset(cpu, 0x5A, 4096); printf("cpu-mapped dmabuf first\n"); }
    } else {
        printf("no cpu-map before import\n");
    }

    uint64_t gpu_va = 0;
    int r = import_dmabuf(fd, dmabuf, &gpu_va);
    printf("import result: r=%d gpu_va=0x%llx\n", r, (unsigned long long)gpu_va);
    close(dmabuf);
    close(fd);
    return r;
}
