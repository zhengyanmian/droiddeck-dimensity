/* kbase-scan4.c — real dma-buf import attempt at MTK's 40-byte MEM_IMPORT encoding.
 * nr=22, size=40 (upstream is 24). Layout guess (union, in/out overlay):
 *   in : flags u64 | phandle u64 (ptr to fd) | type u32 | pad | ext[16]
 *   out: gpu_va u64 | va_pages u64 | flags u64 | ext[16]
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

static void hexdump(const char *tag, const unsigned char *p, int n)
{
    printf("%s:", tag);
    for (int i = 0; i < n; i++) {
        if (i % 8 == 0) printf("  ");
        printf("%02x", p[i]);
    }
    printf("\n");
}

int main(void)
{
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open /dev/mali0"); return 1; }

    struct kbase_ioctl_version_check vc = { .major = 1, .minor = 21 };
    if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &vc)) { perror("VERSION_CHECK"); return 1; }
    printf("[ok] version %u.%u\n", vc.major, vc.minor);
    struct kbase_ioctl_set_flags sf = { .create_flags = 0 };
    if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &sf)) { perror("SET_FLAGS"); return 1; }
    void *tp = mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE);
    if (tp == MAP_FAILED) { perror("tracking page"); return 1; }
    printf("[ok] context initialized\n");

    /* dma-heap alloc */
    int heap = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
    if (heap < 0) { perror("open dma_heap"); return 1; }
    struct dma_heap_allocation_data ha = { .len = 4096, .fd_flags = O_RDWR | O_CLOEXEC };
    if (ioctl(heap, DMA_HEAP_IOCTL_ALLOC, &ha)) { perror("DMA_HEAP_ALLOC"); return 1; }
    close(heap);
    int dmabuf = ha.fd;
    printf("[ok] dma-buf fd=%d (4096 bytes)\n", dmabuf);

    /* sanity: CPU-map the dma-buf and write a pattern */
    unsigned char *cpu = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dmabuf, 0);
    if (cpu == MAP_FAILED) { perror("cpu mmap dmabuf"); return 1; }
    memset(cpu, 0x5A, 4096);

    /* import attempt: MTK 40-byte encoding */
    unsigned char req[40];
    memset(req, 0, sizeof req);
    uint64_t flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                   | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                   | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU;
    int phandle_fd = dmabuf;
    memcpy(req + 0, &flags, 8);
    memcpy(req + 8, &phandle_fd, 8);   /* phandle: pointer to the fd */
    uint32_t type = BASE_MEM_IMPORT_TYPE_UMM; /* 2 */
    memcpy(req + 16, &type, 4);
    /* req[20..40] = 0 (vendor extension fields) */

    hexdump("request", req, 40);
    errno = 0;
    long rc = ioctl(fd, MTK_MEM_IMPORT, req);
    printf("MEM_IMPORT(40B) rc=%ld errno=%d (%s)\n", rc, errno, strerror(errno));
    hexdump("response", req, 40);

    if (rc == 0) {
        uint64_t gpu_va, va_pages, outflags;
        memcpy(&gpu_va, req + 0, 8);
        memcpy(&va_pages, req + 8, 8);
        memcpy(&outflags, req + 16, 8);
        printf("[IMPORT SUCCESS] gpu_va=0x%llx va_pages=%llu flags=0x%llx\n",
               (unsigned long long)gpu_va, (unsigned long long)va_pages,
               (unsigned long long)outflags);
        /* try GPU-side verification is driver's job; here just free */
        struct kbase_ioctl_mem_free fr = { .gpu_addr = gpu_va };
        ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
        printf("IMPORT_PATH_WORKS\n");
    }
    munmap(cpu, 4096);
    close(dmabuf);
    close(fd);
    return rc == 0 ? 0 : 2;
}
