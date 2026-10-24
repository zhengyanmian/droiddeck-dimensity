/* kbase-scan13.c — bisect MTK MEM_IMPORT ENOMEM pattern:
 * import 10 AHB-backed dma-bufs in sequence, keep all mappings alive,
 * print cookie/VA per import. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <android/hardware_buffer.h>

#define LOCAL_PAGE_SHIFT 12
#include "kbase_uapi.h"
#include "kbase_csf_uapi.h"
#include "mali_base_common_kernel.h"
#include "mali_base_kernel.h"

#define MTK_MEM_IMPORT _IOWR(0x80, 22, char[40])
#define PAGE 4096ULL

static long do_import(int fd, int *fdp, unsigned char *req, uint64_t flags)
{
    memset(req, 0, 40);
    memcpy(req, &flags, 8);
    memcpy(req + 8, &fdp, 8);
    req[16] = 2; /* BASE_MEM_IMPORT_TYPE_UMM */
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
    if (mmap(NULL, PAGE, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE) == MAP_FAILED) {
        perror("tracking"); return 1;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[ctx] ok\n");

    /* create a CSF queue group before importing (mimics driver init) */
    {
        union kbase_ioctl_cs_queue_group_create_1_18 g = {
            .in = {
                .tiler_mask = 1,
                .fragment_mask = ~0ull,
                .compute_mask = ~0ull,
                .cs_min = 1,
                .priority = 0,
                .tiler_max = 1,
                .fragment_max = 64,
                .compute_max = 64,
            },
        };
        if (ioctl(fd, KBASE_IOCTL_CS_QUEUE_GROUP_CREATE_1_18, &g)) {
            printf("group create failed: %s\n", strerror(errno));
            /* fall through: try import anyway */
        } else {
            printf("[group] handle=%u\n", g.out.group_handle);
        }
    }

    /* NDK < API-27 header stubs lack getNativeHandle; declare manually */
    typedef struct scan13_native_handle {
        int version;
        int numFds;
        int numInts;
        int data[1];
    } scan13_native_handle_t;
    extern const scan13_native_handle_t *AHardwareBuffer_getNativeHandle(
        const AHardwareBuffer *buffer);

    const uint64_t F = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR
                     | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR
                     | BASE_MEM_IMPORT_SHARED | BASE_MEM_COHERENT_SYSTEM
                     | BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP;

    int sizes_kb[] = {64, 128, 256, 512, 1024, 4096};
    for (int i = 0; i < 6; i++) {
        uint32_t bytes = sizes_kb[i] * 1024;
        uint32_t w = 1; while (w * w * 4 < bytes) w <<= 1;
        AHardwareBuffer_Desc d = {
            .width = w, .height = w, .layers = 1,
            .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                     AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                     AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
        };
        AHardwareBuffer *ahb = NULL;
        if (AHardwareBuffer_allocate(&d, &ahb) != 0 || !ahb) {
            printf("%d: AHB alloc FAIL\n", i);
            continue;
        }
        const scan13_native_handle_t *nh = AHardwareBuffer_getNativeHandle(ahb);
        if (!nh || nh->numFds < 1) { printf("%d: no handle\n", i); AHardwareBuffer_release(ahb); continue; }

        /* panvk GetMemoryFdProperties does a transient import+free probe
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
        long rc = do_import(fd, (int *)&nh->data[0], req, F);
        int used = 0;
        if (rc != 0) {
            for (int k = 1; k < nh->numFds; k++) {
                rc = do_import(fd, (int *)&nh->data[k], req, F);
                if (rc == 0) { used = k; break; }
            }
        }
        printf("%d [%uKB]: import rc=%ld errno=%d(%s) fd_slot=%d numFds=%d", i, sizes_kb[i], rc, errno, strerror(errno), used, nh->numFds);
        if (rc == 0) {
            uint64_t oflags = *(uint64_t *)req;
            uint64_t gpuva = *(uint64_t *)(req + 8);
            void *m = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpuva);
            printf(" outflags=0x%llx mmap=%s",
                   (unsigned long long)oflags,
                   m == MAP_FAILED ? strerror(errno) : "ok");
            if (m != MAP_FAILED) memset(m, 0x5A, 4096);
            /* keep everything alive */
        } else {
            AHardwareBuffer_release(ahb);
        }
        printf("\n");
    }
    printf("done\n");
    return 0;
}
