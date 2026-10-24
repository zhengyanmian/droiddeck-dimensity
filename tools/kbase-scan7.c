/* kbase-scan7.c — differential diagnosis of MTK MEM_IMPORT (nr22/size40).
 * If kernel parses our request correctly:
 *   bad fd        -> EBADF
 *   type=0        -> EINVAL
 *   bad phandle   -> EFAULT
 * If all come back ENOMEM, parsing is off or import gated elsewhere.
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
    uint64_t len; uint32_t fd; uint32_t fd_flags; uint64_t heap_flags;
};
#define DMA_HEAP_IOC_MAGIC 'H'
#define DMA_HEAP_IOCTL_ALLOC _IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)
#define MTK_MEM_IMPORT _IOWR(0x80, 22, char[40])

static void try_import(int fd, int *fdp, uint32_t type, uint64_t flags, const char *tag)
{
    unsigned char req[40];
    memset(req, 0, sizeof req);
    memcpy(req + 0, &flags, 8);
    memcpy(req + 8, &fdp, 8);
    memcpy(req + 16, &type, 4);
    errno = 0;
    long rc = ioctl(fd, MTK_MEM_IMPORT, req);
    printf("%-34s rc=%ld errno=%d (%s)\n", tag, rc, errno, strerror(errno));
    if (rc == 0) {
        struct kbase_ioctl_mem_free fr = { .gpu_addr = *(uint64_t *)req };
        ioctl(fd, KBASE_IOCTL_MEM_FREE, &fr);
    }
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

    const uint64_t RW = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                      | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                      | BASE_MEM_IMPORT_SHARED | BASE_MEM_CACHED_CPU;
    const uint64_t F = RW | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP;

    int badfd = open("/dev/null", O_RDONLY);
    int badptr_fd = 0;
    try_import(fd, &badfd, 2, F, "valid? no — bad fd (/dev/null)");
    try_import(fd, &dmabuf, 2, F, "valid dma-buf, type=2");
    try_import(fd, &dmabuf, 0, F, "valid dma-buf, type=0 (INVAL?)");
    try_import(fd, (int *)0x1234, 2, F, "bad phandle ptr (EFAULT?)");
    try_import(fd, &badptr_fd, 2, F, "phandle -> fd=0 (EBADF?)");
    try_import(fd, &dmabuf, 3, F, "type=3 USER_BUFFER");
    close(dmabuf);
    close(fd);
    return 0;
}
