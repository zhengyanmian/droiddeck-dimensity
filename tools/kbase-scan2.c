/* kbase-scan2.c — test if kbase gates ioctls before dispatch (bogus nr test) */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#define TYPE 0x80
static unsigned long mk(int dir, int nr, int size) {
    return (dir << 30) | ((size & 0x3fff) << 16) | (TYPE << 8) | nr;
}
int main(void) {
    int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror("open"); return 1; }
    /* first: proper context init like the driver does (SET_FLAGS with sane flags) */
    unsigned int flags = 0; /* try 0 and a few known values */
    int nrs[] = { 1, 5, 22, 60, 99, 120 };
    for (unsigned i = 0; i < sizeof nrs / sizeof nrs[0]; i++) {
        unsigned char buf[64] = { 0 };
        long rc = ioctl(fd, mk(3 /*IOWR*/, nrs[i], 64), buf);
        printf("bogus-mode nr=%-3d IOWR(64B) rc=%ld errno=%d (%s)\n",
               nrs[i], rc, errno, strerror(errno));
    }
    /* also try _IO (no size) variants */
    for (unsigned i = 0; i < sizeof nrs / sizeof nrs[0]; i++) {
        long rc = ioctl(fd, mk(1 /*_IO no rw bits? dir none=0*/, nrs[i], 0), NULL);
        printf("bogus-mode nr=%-3d _IO(0B)    rc=%ld errno=%d (%s)\n",
               nrs[i], rc, errno, strerror(errno));
    }
    close(fd);
    return 0;
}
