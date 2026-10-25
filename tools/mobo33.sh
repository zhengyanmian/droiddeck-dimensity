#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo33.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
proot-distro login ubuntu --shared-tmp -- bash -c 'cat > /root/ve2.c << "EOF"
#include <stdio.h>
#include <dlfcn.h>
typedef int VkResult;
typedef unsigned int U32;
struct ExtProps { char extName[256]; unsigned int specVersion; };
int main(int argc, char **argv) {
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { printf("dlopen FAIL: %s\n", dlerror()); return 1; }
    printf("dlopen OK: %s\n", argv[1]);
    VkResult (*f)(const char *, U32 *, struct ExtProps *) = dlsym(h, "vkEnumerateInstanceExtensionProperties");
    if (!f) { printf("dlsym FAIL\n"); return 1; }
    U32 n = 0;
    f(NULL, &n, NULL);
    struct ExtProps p[40];
    f(NULL, &n, p);
    printf("count=%u\n", n);
    for (U32 i = 0; i < n && i < 40; i++) printf("  EXT: %s\n", p[i].extName);
    return 0;
}
EOF
gcc /root/ve2.c -o /root/ve2 -ldl && echo compiled'
echo "== absolute path to OUR full loader =="
proot-distro login ubuntu --shared-tmp -- bash -c "export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve2 /root/panvk/libvulkan.so.1 2>&1 | head -20"
echo "== soname (control) =="
proot-distro login ubuntu --shared-tmp -- bash -c "/root/ve2 libvulkan.so.1 2>&1 | head -10"
echo "== android system loader =="
proot-distro login ubuntu --shared-tmp -- bash -c "/root/ve2 /system/lib64/libvulkan.so 2>&1 | head -10"
