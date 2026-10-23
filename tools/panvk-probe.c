/* panvk-probe: minimal bionic Vulkan driver probe
 * Loads an arbitrary Vulkan driver .so directly (bypassing the system loader),
 * creates an instance and enumerates physical devices.
 * Build: aarch64-linux-android<api>-clang panvk-probe.c -o panvk-probe -ldl
 * Run (on device / Termux): ./panvk-probe [path-to-driver.so]
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "libvulkan_panfrost.so";
    void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen(%s) failed: %s\n", path, dlerror()); return 1; }
    printf("[probe] loaded %s\n", path);

    PFN_vkEnumerateInstanceExtensionProperties enumInstExt =
        (PFN_vkEnumerateInstanceExtensionProperties)dlsym(lib, "vkEnumerateInstanceExtensionProperties");
    PFN_vkGetInstanceProcAddr gipa =
        (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
    if (!enumInstExt || !gipa) { fprintf(stderr, "[probe] missing driver entrypoints\n"); return 1; }

    uint32_t n = 0;
    if (enumInstExt(NULL, &n, NULL) != VK_SUCCESS) { fprintf(stderr, "[probe] enum ext count failed\n"); return 1; }
    printf("[probe] instance extensions: %u\n", n);

    VkApplicationInfo app = {0};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "panvk-probe";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ici = {0};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &app;

    VkInstance inst = VK_NULL_HANDLE;
    PFN_vkCreateInstance createInstance = (PFN_vkCreateInstance)gipa(VK_NULL_HANDLE, "vkCreateInstance");
    if (!createInstance || createInstance(&ici, NULL, &inst) != VK_SUCCESS) {
        fprintf(stderr, "[probe] vkCreateInstance FAILED (driver rejected 1.3 instance)\n");
        return 1;
    }
    printf("[probe] vkCreateInstance OK\n");

    PFN_vkEnumeratePhysicalDevices enumPhys = (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
    uint32_t dn = 0;
    if (!enumPhys || enumPhys(inst, &dn, NULL) != VK_SUCCESS || dn == 0) {
        fprintf(stderr, "[probe] no physical devices (count=%u)\n", dn);
        return 1;
    }
    VkPhysicalDevice *devs = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * dn);
    enumPhys(inst, &dn, devs);
    printf("[probe] physical devices: %u\n", dn);
    for (uint32_t i = 0; i < dn; i++) {
        VkPhysicalDeviceProperties p = {0};
        PFN_vkGetPhysicalDeviceProperties getProps = (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
        getProps(devs[i], &p);
        printf("[probe] GPU%u: %s | api=%u.%u.%u | vendorID=0x%x deviceID=0x%x\n",
               i, p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
               VK_API_VERSION_MINOR(p.apiVersion), VK_API_VERSION_PATCH(p.apiVersion),
               p.vendorID, p.deviceID);
    }
    printf("[probe] PASS\n");
    return 0;
}
