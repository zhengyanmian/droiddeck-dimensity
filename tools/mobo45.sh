#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo45.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
proot-distro login ubuntu --shared-tmp -- bash -c 'cat > /root/ve3.c << "EOF"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
typedef int VkResult; typedef unsigned int U32; typedef void* Handle;
struct ExtProps { char extName[256]; unsigned int specVersion; };
struct F2s { void *pNext; struct { int robustBufferAccess, fullDrawIndexUint32, imageCubeArray, independentBlend, geometryShader, tessellationShader, logicOp, multiDrawIndirect, drawIndirectFirstInstance, depthClamp, depthBiasClamp, fillModeNonSolid, depthBounds, wideLines, largePoints, alphaToOne, multiViewport, samplerAnisotropy, textureCompressionETC2, textureCompressionASTC_LDR, textureCompressionBC, occlusionQueryPrecise, pipelineStatisticsQuery, vertexPipelineStoresAndAtomics, fragmentStoresAndAtomics, shaderTessellationAndGeometryPointSize, shaderImageGatherExtended, shaderStorageImageExtendedFormats, shaderStorageImageMultisample, shaderStorageImageReadWithoutFormat, shaderStorageImageWriteWithoutFormat, shaderUniformBufferArrayDynamicIndexing, shaderSampledImageArrayDynamicIndexing, shaderStorageBufferArrayDynamicIndexing, shaderStorageImageArrayDynamicIndexing, shaderClipDistance, shaderCullDistance, shaderFloat64, shaderInt64, shaderInt16, shaderResourceResidency, shaderResourceMinLod, sparseBinding, sparseResidencyBuffer, sparseResidencyImage2D, sparseResidencyAliased, variableMultisampleRate, inheritedQueries; } f; };
int main(int argc, char **argv) {
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { printf("dlopen FAIL: %s\n", dlerror()); return 1; }
    VkResult (*eni)(const char *, U32 *, struct ExtProps *) = dlsym(h, "vkEnumerateInstanceExtensionProperties");
    void *(*gipa)(Handle, const char *) = dlsym(h, "vkGetInstanceProcAddr");
    VkResult (*ci)(void *, void *) = dlsym(h, "vkCreateInstance");
    printf("syms: eni=%p gipa=%p ci=%p\n", eni, gipa, ci);
    /* create instance */
    struct { void *a; const void *b; } appi = {0,0}, ciinfo = {0,0};
    Handle inst = 0;
    long buf[32]; memset(buf, 0, sizeof(buf));
    unsigned int *p = (unsigned int *)buf;
    p[0] = 10; p[1] = 1; /* sType=1(INSTANCE_CREATE_INFO), 0 */
    VkResult r = ci(&ciinfo, 0, &inst);
    printf("ci rc=%d inst=%p\n", r, inst);
    if (!inst) return 1;
    U32 (*epd)(Handle, U32 *, Handle *) = gipa(inst, "vkEnumeratePhysicalDevices");
    U32 n = 0;
    r = epd(inst, &n, NULL);
    printf("physdevs=%u\n", n);
    if (!n) return 2;
    Handle pd[4]; epd(inst, &n, pd);
    void *(*gpdf2)(Handle, void *) = gipa(inst, "vkGetPhysicalDeviceFeatures2");
    if (!gpdf2) { printf("no Features2\n"); return 3; }
    struct F2s f2; memset(&f2, 0, sizeof(f2));
    f2.pNext = 0; f2.f.geometryShader = -1; /* sentinel */
    unsigned int *hdr = (unsigned int *)&f2;
    hdr[0] = 1000109000; /* VkPhysicalDeviceFeatures2 sType */
    gpdf2(pd[0], &f2);
    printf("FEATURES:\n");
    const char *names[] = {"robustBufferAccess","fullDrawIndexUint32","imageCubeArray","independentBlend","geometryShader","tessellationShader","logicOp","multiDrawIndirect","drawIndirectFirstInstance","depthClamp","depthBiasClamp","fillModeNonSolid","depthBounds","wideLines","largePoints","alphaToOne","multiViewport","samplerAnisotropy","textureCompressionETC2","textureCompressionASTC_LDR","textureCompressionBC","occlusionQueryPrecise","pipelineStatisticsQuery","vertexPipelineStoresAndAtomics","fragmentStoresAndAtomics","shaderTessellationAndGeometryPointSize","shaderImageGatherExtended","shaderStorageImageExtendedFormats","shaderStorageImageMultisample","shaderStorageImageReadWithoutFormat","shaderStorageImageWriteWithoutFormat","shaderUniformBufferArrayDynamicIndexing","shaderSampledImageArrayDynamicIndexing","shaderStorageBufferArrayDynamicIndexing","shaderStorageImageArrayDynamicIndexing","shaderClipDistance","shaderCullDistance","shaderFloat64","shaderInt64","shaderInt16","shaderResourceResidency","shaderResourceMinLod","sparseBinding","sparseResidencyBuffer","sparseResidencyImage2D","sparseResidencyAliased","variableMultisampleRate","inheritedQueries"};
    int *fv = &f2.f.robustBufferAccess;
    for (int i = 0; i < 48; i++) printf("  %s = %d\n", names[i], fv[i]);
    return 0;
}
EOF
gcc /root/ve3.c -o /root/ve3 -ldl && echo compiled'
echo "== run features probe =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve3 /root/panvk/libvulkan.so.1 2>&1 | head -60"
