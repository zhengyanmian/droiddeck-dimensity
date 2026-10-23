/* panvk-probe v2: Vulkan driver probe with HMI table auto-decode.
 * Mode 1: standard exports (dlsym vkGetInstanceProcAddr)
 * Mode 2: MineGraph-style HMI dispatch table (auto-decode slot layout)
 * Build: aarch64-linux-android26-clang panvk-probe.c -O2 -o panvk-probe -ldl
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef struct { unsigned long start, end; } Range;

static int get_exec_range(const char *so_path, Range *r) {
    const char *base = strrchr(so_path, '/');
    base = base ? base + 1 : so_path;
    FILE *f = fopen("/proc/self/maps", "r");
    char line[512];
    r->start = r->end = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, base) && strstr(line, "x")) {
            unsigned long a, b;
            if (sscanf(line, "%lx-%lx", &a, &b) == 2) {
                if (!r->start) r->start = a;
                r->end = b;
            }
        }
    }
    fclose(f);
    return r->end > r->start;
}

static int probe_instance(void *gipa_raw, const char *tag) {
    PFN_vkGetInstanceProcAddr gipa = (PFN_vkGetInstanceProcAddr)gipa_raw;
    VkApplicationInfo app = {0};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "panvk-probe";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ici = {0};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &app;
    VkInstance inst = VK_NULL_HANDLE;
    PFN_vkCreateInstance createInstance = (PFN_vkCreateInstance)gipa(VK_NULL_HANDLE, "vkCreateInstance");
    if (!createInstance) { printf("[%s] no vkCreateInstance\n", tag); return 0; }
    if (createInstance(&ici, NULL, &inst) != VK_SUCCESS) { printf("[%s] vkCreateInstance FAILED\n", tag); return 0; }
    printf("[%s] vkCreateInstance OK\n", tag);
    PFN_vkEnumeratePhysicalDevices enumPhys = (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
    uint32_t dn = 0;
    if (!enumPhys || enumPhys(inst, &dn, NULL) != VK_SUCCESS || dn == 0) {
        printf("[%s] physical devices: 0 (driver loaded but GPU not matched)\n", tag);
        return 1;
    }
    VkPhysicalDevice *devs = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * dn);
    enumPhys(inst, &dn, devs);
    PFN_vkGetPhysicalDeviceProperties getProps = (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
    for (uint32_t i = 0; i < dn; i++) {
        VkPhysicalDeviceProperties p = {0};
        getProps(devs[i], &p);
        printf("[%s] GPU%u: %s | api=%u.%u.%u | vendorID=0x%x deviceID=0x%x\n",
               tag, i, p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
               VK_API_VERSION_MINOR(p.apiVersion), VK_API_VERSION_PATCH(p.apiVersion),
               p.vendorID, p.deviceID);
    }
    printf("[%s] PASS\n", tag);
    return 1;
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "libvulkan_panfrost.so";
    void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen(%s) failed: %s\n", path, dlerror()); return 1; }
    printf("[probe] loaded %s\n", path);

    /* Mode 1: standard exports */
    void *gipa_std = dlsym(lib, "vkGetInstanceProcAddr");
    if (gipa_std) {
        printf("[probe] standard export found, probing...\n");
        return probe_instance(gipa_std, "std") ? 0 : 2;
    }
    printf("[probe] no standard export; trying HMI table decode\n");

    /* Mode 2: HMI pointer table */
    void **hmi = (void **)dlsym(lib, "HMI");
    if (!hmi) { fprintf(stderr, "[probe] no HMI either; unknown driver format\n"); return 1; }
    Range r;
    if (!get_exec_range(path, &r)) { fprintf(stderr, "[probe] cannot resolve exec range\n"); return 1; }
    printf("[probe] exec range %lx-%lx\n", r.start, r.end);

    /* HMI is 248 bytes = 31 slots; try every slot as vkGetInstanceProcAddr,
     * but only if the pointer lands in the driver's exec segment. */
    for (int i = 0; i < 31; i++) {
        unsigned long p = (unsigned long)hmi[i];
        if (!p || p < r.start || p >= r.end) continue;
        PFN_vkGetInstanceProcAddr f = (PFN_vkGetInstanceProcAddr)p;
        void *a = f(VK_NULL_HANDLE, "vkCreateInstance");
        void *b = f(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties");
        void *c = f(VK_NULL_HANDLE, "not-a-real-vulkan-function");
        if (a && b && !c) {
            printf("[probe] HMI[%d] = vkGetInstanceProcAddr (verified)\n", i);
            probe_instance(f, "hmi");
            return 0;
        }
    }
    fprintf(stderr, "[probe] HMI decode failed: no slot matches vkGetInstanceProcAddr\n");
    return 3;
}
