/* micro-probe.c — compare vk_icdGetInstanceProcAddr behaviour for instance-level
 * names before/after CreateInstance, mirroring icd-probe (which works) vs tri-probe. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef void *(*icd_gipa_t)(void *, const char *);

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *so = argc > 1 ? argv[1] : "./libvulkan_panfrost.so";
    void *lib = dlopen(so, RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }

    int v = 5;
    ((int (*)(int *))dlsym(lib, "vk_icdNegotiateLoaderICDInterfaceVersion"))(&v);
    printf("negotiated v=%d\n", v);

    icd_gipa_t g = (icd_gipa_t)dlsym(lib, "vk_icdGetInstanceProcAddr");
    if (!g) { printf("no gipa\n"); return 1; }

    const char *names[] = { "vkCreateInstance", "vkEnumerateInstanceExtensionProperties",
                            "vkEnumeratePhysicalDevices", "vkGetPhysicalDeviceProperties",
                            "vkCreateDevice", "vkGetDeviceProcAddr" };
    printf("-- pre-instance (NULL) --\n");
    for (unsigned i = 0; i < 6; i++)
        printf("  %-44s %p\n", names[i], g(NULL, names[i]));

    /* also try the plain exported vkGetInstanceProcAddr symbol */
    icd_gipa_t plain = (icd_gipa_t)dlsym(lib, "vkGetInstanceProcAddr");
    printf("plain vkGetInstanceProcAddr sym: %p\n", (void *)plain);
    if (plain) {
        printf("-- plain, pre-instance --\n");
        for (unsigned i = 0; i < 6; i++)
            printf("  %-44s %p\n", names[i], plain(NULL, names[i]));
    }

    PFN_vkCreateInstance ci = (PFN_vkCreateInstance)g(NULL, "vkCreateInstance");
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance inst = NULL;
    VkResult r = ci(&ici, NULL, &inst);
    printf("CreateInstance r=%d inst=%p\n", r, (void *)inst);
    if (r != VK_SUCCESS) return 2;

    printf("-- post-instance (inst) --\n");
    for (unsigned i = 0; i < 6; i++)
        printf("  %-44s %p\n", names[i], g(inst, names[i]));

    if (plain) {
        printf("-- plain, post-instance --\n");
        for (unsigned i = 0; i < 6; i++)
            printf("  %-44s %p\n", names[i], plain(inst, names[i]));
    }
    return 0;
}
