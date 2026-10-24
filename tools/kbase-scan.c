/* kbase-scan.c — probe which kbase ioctls the MTK kernel actually implements.
 * For each KBASE_IOCTL_* from the upstream UAPI header, send a zeroed
 * _IOC_SIZE-sized payload and classify the errno:
 *   ENOTTY            -> kernel switch default hit -> ioctl ABSENT
 *   anything else     -> a handler ran and rejected our args -> PRESENT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "kbase_uapi.h"

struct e { const char *n; unsigned long cmd; };
#define E(x) { #x, x }

int main(void)
{
    struct e list[] = {
        E(KBASE_IOCTL_SET_FLAGS),
        E(KBASE_IOCTL_GET_GPUPROPS),
        E(KBASE_IOCTL_MEM_ALLOC),
        E(KBASE_IOCTL_MEM_QUERY),
        E(KBASE_IOCTL_MEM_FREE),
        E(KBASE_IOCTL_DISJOINT_QUERY),
        E(KBASE_IOCTL_GET_DDK_VERSION),
        E(KBASE_IOCTL_MEM_JIT_INIT),
        E(KBASE_IOCTL_MEM_SYNC),
        E(KBASE_IOCTL_MEM_FIND_CPU_OFFSET),
        E(KBASE_IOCTL_GET_CONTEXT_ID),
        E(KBASE_IOCTL_MEM_COMMIT),
        E(KBASE_IOCTL_MEM_ALIAS),
        E(KBASE_IOCTL_MEM_IMPORT),
        E(KBASE_IOCTL_MEM_FLAGS_CHANGE),
        E(KBASE_IOCTL_STREAM_CREATE),
        E(KBASE_IOCTL_FENCE_VALIDATE),
        E(KBASE_IOCTL_MEM_PROFILE_ADD),
        E(KBASE_IOCTL_STICKY_RESOURCE_MAP),
        E(KBASE_IOCTL_STICKY_RESOURCE_UNMAP),
        E(KBASE_IOCTL_MEM_FIND_GPU_START_AND_OFFSET),
        E(KBASE_IOCTL_MEM_EXEC_INIT),
        E(KBASE_IOCTL_GET_CPU_GPU_TIMEINFO),
        E(KBASE_IOCTL_CONTEXT_PRIORITY_CHECK),
        E(KBASE_IOCTL_SET_LIMITED_CORE_COUNT),
    };

    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open /dev/mali0"); return 1; }

    int absent = 0, present = 0;
    for (unsigned i = 0; i < sizeof list / sizeof list[0]; i++) {
        unsigned char buf[512];
        memset(buf, 0, sizeof buf);
        unsigned long size = _IOC_SIZE(list[i].cmd);
        if (size > sizeof buf) size = sizeof buf;
        errno = 0;
        long rc = ioctl(fd, list[i].cmd, buf);
        int e = errno;
        const char *cls;
        if (e == ENOTTY)      { cls = "ABSENT"; absent++; }
        else if (rc == 0)     { cls = "PRESENT(rc=0!)"; present++; }
        else                  { cls = "PRESENT"; present++; }
        printf("nr %-3lu size %-4lu %-42s rc=%ld errno=%d (%s) -> %s\n",
               (unsigned long)_IOC_NR(list[i].cmd), _IOC_SIZE(list[i].cmd),
               list[i].n, rc, e, strerror(e), cls);
    }
    close(fd);
    printf("== %d present, %d absent ==\n", present, absent);
    return 0;
}
