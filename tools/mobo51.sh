#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo51.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
cp -r /sdcard/Download/vkinc/vk_video $R/root/vkinc/ 2>/dev/null
echo "== compile =="
proot-distro login ubuntu --shared-tmp -- bash -c 'cat > /root/ve5.c << "EOF"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
typedef unsigned int U32;
int main(int argc, char **argv) {
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { printf("dlopen FAIL: %s\n", dlerror()); return 1; }
    PFN_vkGetInstanceProcAddr gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vkGetInstanceProcAddr");
    VkApplicationInfo ai = {VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL, "probe", 0, "probe", 0, VK_API_VERSION_1_3};
    VkInstanceCreateInfo ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL, 0, &ai, 0, NULL, 0, NULL};
    VkInstance inst = NULL;
    PFN_vkCreateInstance pci = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    VkResult r = pci(&ci, NULL, &inst);
    printf("ci rc=%d\n", r);
    if (r) return 1;
    PFN_vkEnumeratePhysicalDevices epd = (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
    PFN_vkGetPhysicalDeviceFeatures2 gp2 = (PFN_vkGetPhysicalDeviceFeatures2)gipa(inst, "vkGetPhysicalDeviceFeatures2");
    PFN_vkGetPhysicalDeviceProperties gpp = (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
    U32 n = 0; epd(inst, &n, NULL);
    printf("physdevs=%u\n", n);
    if (!n) return 2;
    VkPhysicalDevice pd[4]; epd(inst, &n, pd);
    VkPhysicalDeviceProperties props; memset(&props, 0, sizeof(props));
    gpp(pd[0], &props);
    printf("name=%s apiVersion=0x%x vendor=0x%x\n", props.deviceName, props.apiVersion, props.vendorID);
    VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    gp2(pd[0], &f2);
    const VkPhysicalDeviceFeatures *f = &f2.features;
    printf("F1 robustBufferAccess=%d fullDrawIndexUint32=%d geometryShader=%d tessellationShader=%d logicOp=%d multiDrawIndirect=%d imageCubeArray=%d independentBlend=%d\n", f->robustBufferAccess, f->fullDrawIndexUint32, f->geometryShader, f->tessellationShader, f->logicOp, f->multiDrawIndirect, f->imageCubeArray, f->independentBlend);
    printf("F2 drawIndirectFirstInstance=%d depthClamp=%d depthBiasClamp=%d fillModeNonSolid=%d sampleRateShading=%d shaderClipDistance=%d shaderCullDistance=%d textureCompressionBC=%d\n", f->drawIndirectFirstInstance, f->depthClamp, f->depthBiasClamp, f->fillModeNonSolid, f->sampleRateShading, f->shaderClipDistance, f->shaderCullDistance, f->textureCompressionBC);
    printf("F3 fragmentStoresAndAtomics=%d vertexPipelineStoresAndAtomics=%d shaderStorageImageWriteWithoutFormat=%d sparseBinding=%d\n", f->fragmentStoresAndAtomics, f->vertexPipelineStoresAndAtomics, f->shaderStorageImageWriteWithoutFormat, f->sparseBinding);
    return 0;
}
EOF
gcc /root/ve5.c -o /root/ve5 -ldl -I/root/vkinc && echo compiled'
echo "== run features probe =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve5 /root/panvk/libvulkan.so.1 2>&1 | head -20"
