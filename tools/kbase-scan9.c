/* kbase-scan9.c — hexdump full 40-byte MTK import response + mmap verification */
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

static void dump(const char *tag, const unsigned char *p)
{
    printf("%s:/n", tag);
    for (int r = 0; r < 5; r++) {
        printf("  +%02x:", r * 8);
        uint64_t v; memcpy(&v, p + r * 8, 8);
        printf(" %016llx (%llu)", (unsigned long long)v, (unsigned long long)v);
        printf("\n");
    }
}

int main(void)
{
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc)) { perror("VER"); return 1; }
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &sf)) { perror("SF"); return 1; }
    if (mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE) == MAP_FAILED) {
        perror("tracking"); return 1;
    }

    int heap = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
    struct dma_heap_allocation_data ha = { .len = 4096, .fd_flags = O_RDWR | O_CLOEXEC };
    if (ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &ha)) { perror("alloc"); return 1; }
    close(heap);
    int dmabuf = ha.fd;

    /* CPU-map dma-buf, write pattern */
    unsigned char *cpu = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dmabuf, 0);
    if (cpu == MAP_FAILED) { perror("cpu map"); return 1; }
    for (int i = 0; i < 4096; i++) cpu[i] = (unsigned char)(i & 0xff);

    const uint64_t F = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                     | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                     | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU
                     | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP;

    unsigned char req[40];
    memset(req, 0, sizeof req);
    int fdp = dmabuf;
    memcpy(req, &F, 8);
    memcpy(req + 8, &fdp, 8);
    req[16] = 2;

    int ok = 0;
    for (int a = 0; a < 40 && !ok; a++) {
        errno = 0;
        if (ioctl(fd, MTK_MEM_IMPORT, req) == 0) ok = 1;
        else { printf("attempt %d: %s\n", a, strerror(errno)); usleep(100000); memset(req, 0, 40);
               memcpy(req, &F, 8); memcpy(req + 8, &fdp, 8); req[16] = 2; }
    }
    if (!ok) { printf("IMPORT FAILED\n"); return 2; }
    dump("response", req);

    uint64_t flags, cookie, va_pages, off18, off20;
    memcpy(&flags, req, 8); memcpy(&cookie, req + 8, 8); memcpy(&va_pages, req + 16, 8);
    memcpy(&off18, req + 24, 8); memcpy(&off20, req + 32, 8);
    (void)off20;

    /* try CPU mmap at the returned cookie (NEED_MMAP semantics) */
    unsigned long size = 4096;
    void *gpumap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, cookie);
    if (gpumap == MAP_FAILED) {
        printf("mmap(cookie=%#llx) failed: %s\n", (unsigned long long)cookie, strerror(errno));
        gpumap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off18);
        if (gpumap == MAP_FAILED) printf("mmap(off18=%#llx) failed: %s\n", (unsigned long long)off18, strerror(errno));
        else printf("mmap(off18) OK @%p\n", gpumap);
    } else {
        printf("mmap(cookie) OK @%p\n", gpumap);
    }
    if (gpumap != MAP_FAILED) {
        int diff = memcmp(gpumap, cpu, 4096);
        printf("CPU-vs-GPUMAP compare: %s  first bytes: %02x %02x %02x %02x\n",
               diff == 0 ? "MATCH (coherent!)" : "DIFFER", ((unsigned char*)gpumap)[0],
               ((unsigned char*)gpumap)[1], ((unsigned char*)gpumap)[2], ((unsigned char*)gpumap)[3]);
    }

    struct kbase_ioctl_mem_free fr = { .gpu_addr = *(uint64_t *)req };
    ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
    close(dmabuf);
    return 0;
}
