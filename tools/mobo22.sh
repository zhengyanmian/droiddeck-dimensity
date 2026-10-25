#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo22.txt 2>&1
echo "START $(date)"
unset LD_PRELOAD
G=/data/data/com.termux/files/usr/glibc
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
I=/data/data/com.termux/files/home/panvk-icd

echo "== write+compile host vulkenum probe in proot =="
proot-distro login ubuntu --shared-tmp -- bash -c 'cat > /root/ve.c << "EOF"
#include <stdio.h>
#include <dlfcn.h>
typedef int VkResult;
typedef unsigned int U32;
struct ExtProps { char extName[256]; unsigned int specVersion; };
int main(int argc, char **argv) {
    void *h = dlopen("libvulkan.so.1", RTLD_NOW);
    if (!h) { printf("dlopen FAIL: %s\n", dlerror()); return 1; }
    printf("dlopen OK: %s\n", argv[1] ? argv[1] : "libvulkan.so.1");
    VkResult (*f)(const char *, U32 *, struct ExtProps *) = dlsym(h, "vkEnumerateInstanceExtensionProperties");
    if (!f) { printf("dlsym FAIL\n"); return 1; }
    U32 n = 0;
    VkResult r = f(NULL, &n, NULL);
    printf("count=%u res=%d\n", n, r);
    struct ExtProps p[32];
    r = f(NULL, &n, p);
    for (U32 i = 0; i < n && i < 32; i++) printf("  EXT: %s\n", p[i].extName);
    return 0;
}
EOF
gcc /root/ve.c -o /root/ve -ldl && echo compiled'
cp $R/root/ve /data/data/com.termux/files/home/ve 2>/dev/null
chmod +x /data/data/com.termux/files/home/ve 2>/dev/null

echo "== probe A: default loader search (no VK env) =="
(
  export LD_LIBRARY_PATH=$I:$G/lib/x86_64-linux-gnu
  /data/data/com.termux/files/home/ve A
) 2>&1 | head -12

echo "== probe B: with VK_DRIVER_FILES =="
(
  export LD_LIBRARY_PATH=$I:$G/lib/x86_64-linux-gnu
  export VK_DRIVER_FILES=$I/panvk_icd.json
  /data/data/com.termux/files/home/ve B
) 2>&1 | head -14

echo "ALL_DONE $(date)"
