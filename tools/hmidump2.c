#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    void **hmi = (void **)dlsym(lib, "HMI");
    Dl_info d; memset(&d, 0, sizeof d);
    dladdr((void *)hmi, &d);
    unsigned long fbase = (unsigned long)d.dli_fbase;
    printf("fbase=%lx\n", fbase);
    for (int s = 1; s <= 4; s++) {
        unsigned long p = (unsigned long)hmi[s];
        printf("HMI[%d] off=0x%lx\n", s, p - fbase);
    }
    return 0;
}
