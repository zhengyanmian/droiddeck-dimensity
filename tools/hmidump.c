#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    void **hmi = (void **)dlsym(lib, "HMI");
    if (!hmi) { fprintf(stderr, "no HMI\n"); return 1; }
    for (int s = 0; s < 31; s++) {
        unsigned long p = (unsigned long)hmi[s];
        const char *where = "";
        if (p) {
            Dl_info d; memset(&d, 0, sizeof d);
            if (dladdr((void *)p, &d)) where = d.dli_fname ? d.dli_fname : "mapped";
        }
        printf("HMI[%02d] = %016lx  %s\n", s, p, p ? where : "(null)");
    }
    return 0;
}
