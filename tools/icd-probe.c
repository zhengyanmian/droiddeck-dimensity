/* icd-probe.c — dlopen a PanVK ICD directly (no Vulkan loader) and enumerate
 * physical devices via the vk_icd* interface. Purpose: verify on-device that
 * our self-built driver recognizes Mali-G1-Ultra (GPU_ID 0xe8800010).
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void *(*PFN_vkIcdGetInstanceProcAddr)(void *instance, const char *name);
typedef int (*PFN_vkIcdNegotiate)(int *version);

/* minimal Vulkan bits we need */
typedef int VkResult;
typedef unsigned int VkExtensionProperties_count;

struct VkExtensionProperties {
    char extensionName[256];
    unsigned int specVersion;
};

struct VkApplicationInfo {
    char pad[256];
};

/* Use the real vulkan_core.h types minimally: we declare only what we call.
 * To stay safe we work with void* and cast function pointers explicitly. */
int main(int argc, char **argv)
{
    const char *so = argv[1];
    void *lib = dlopen(so, RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    printf("[ok] dlopen %s\n", so);

    PFN_vkIcdNegotiate negotiate =
        (PFN_vkIcdNegotiate)dlsym(lib, "vk_icdNegotiateLoaderICDInterfaceVersion");
    if (negotiate) {
        int v = 5;
        negotiate(&v);
        printf("[ok] ICD interface version: %d\n", v);
    } else {
        printf("[warn] no vk_icdNegotiateLoaderICDInterfaceVersion\n");
    }

    PFN_vkIcdGetInstanceProcAddr gipa =
        (PFN_vkIcdGetInstanceProcAddr)dlsym(lib, "vk_icdGetInstanceProcAddr");
    if (!gipa) {
        fprintf(stderr, "FATAL: no vk_icdGetInstanceProcAddr\n");
        return 1;
    }

    /* vkEnumerateInstanceExtensionProperties can be called with NULL instance */
    void *enum_ext = gipa(NULL, "vkEnumerateInstanceExtensionProperties");
    if (enum_ext) {
        unsigned int count = 0;
        VkResult r = ((VkResult (*)(const char *, unsigned int *, void *))
                      enum_ext)(NULL, &count, NULL);
        printf("[ok] instance extensions: %u (r=%d)\n", count, r);
    }

    void *create_inst = gipa(NULL, "vkCreateInstance");
    if (!create_inst) {
        fprintf(stderr, "FATAL: no vkCreateInstance\n");
        return 1;
    }

    /* VkInstanceCreateInfo on Android (no allocator): zeroed 48-byte struct
     * {sType=1, pNext, flags, pApplicationInfo, enabledLayerCount,
     *  ppEnabledLayerNames, enabledExtensionCount, ppEnabledExtensionNames} */
    unsigned long long ci[8] = {0};
    ci[0] = 1; /* VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO */
    void *instance = NULL;
    VkResult r = ((VkResult (*)(const void *, const void *, void *, void *))
                  create_inst)(ci, NULL, &instance, NULL);
    if (r != 0) {
        fprintf(stderr, "FATAL: vkCreateInstance failed r=%d\n", r);
        return 1;
    }
    printf("[ok] instance created\n");

    void *enum_dev = gipa(instance, "vkEnumeratePhysicalDevices");
    if (!enum_dev) {
        fprintf(stderr, "FATAL: no vkEnumeratePhysicalDevices\n");
        return 1;
    }
    unsigned int ndev = 0;
    r = ((VkResult (*)(void *, unsigned int *, void *))enum_dev)(instance, &ndev, NULL);
    printf("[ok] physical devices: %u (r=%d)\n", ndev, r);
    if (ndev == 0)
        return 2;

    void **devs = calloc(ndev, sizeof(void *));
    r = ((VkResult (*)(void *, unsigned int *, void **))enum_dev)(instance, &ndev, devs);

    void *get_props = gipa(instance, "vkGetPhysicalDeviceProperties");
    if (!get_props) {
        fprintf(stderr, "FATAL: no vkGetPhysicalDeviceProperties\n");
        return 1;
    }

    /* VkPhysicalDeviceProperties is a large struct; fields we care about:
     * apiVersion (u32, offset 0), driverVersion (u32, 4), vendorID (u32, 8),
     * deviceID (u32, 12), deviceType (u32, 16), deviceName (char[256], 20) */
    unsigned char props[1024];
    for (unsigned int i = 0; i < ndev; i++) {
        memset(props, 0, sizeof props);
        ((void (*)(void *, void *))get_props)(devs[i], props);
        unsigned int api = *(unsigned int *)(props + 0);
        unsigned int drv = *(unsigned int *)(props + 4);
        unsigned int ven = *(unsigned int *)(props + 8);
        unsigned int did = *(unsigned int *)(props + 12);
        printf("GPU%u: %s | api %u.%u.%u | driver 0x%x | vendorID 0x%04x | deviceID 0x%08x\n",
               i, props + 20,
               api >> 22, (api >> 12) & 0x3ff, api & 0xfff, drv, ven, did);
    }

    printf("PROBE_SUCCESS\n");
    return 0;
}
