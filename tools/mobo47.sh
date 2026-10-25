#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/Download/mobo47.txt 2>&1
R=/data/data/com.termux/files/usr/var/lib/proot-distro/containers/ubuntu/rootfs
proot-distro login ubuntu --shared-tmp -- bash -c 'cat > /root/ve4.c << "EOF"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
typedef int VkResult; typedef unsigned int U32; typedef void* Handle;
struct ExtProps { char extName[256]; unsigned int specVersion; };
struct F2s { void *pNext; struct { int robustBufferAccess, fullDrawIndexUint32, imageCubeArray, independentBlend, geometryShader, tessellationShader, logicOp, multiDrawIndirect, drawIndirectFirstInstance, depthClamp, depthBiasClamp, fillModeNonSolid, depthBounds, wideLines, largePoints, alphaToOne, multiViewport, samplerAnisotropy, textureCompressionETC2, textureCompressionASTC_LDR, textureCompressionBC, occlusionQueryPrecise, pipelineStatisticsQuery, vertexPipelineStoresAndAtomics, fragmentStoresAndAtomics, shaderTessellationAndGeometryPointSize, shaderImageGatherExtended, shaderStorageImageExtendedFormats, shaderStorageImageMultisample, shaderStorageImageReadWithoutFormat, shaderStorageImageWriteWithoutFormat, shaderUniformBufferArrayDynamicIndexing, shaderSampledImageArrayDynamicIndexing, shaderStorageBufferArrayDynamicIndexing, shaderStorageImageArrayDynamicIndexing, shaderClipDistance, shaderCullDistance, shaderFloat64, shaderInt64, shaderInt16, shaderResourceResidency, shaderResourceMinLod, sparseBinding, sparseResidencyBuffer, sparseResidencyImage2D, sparseResidencyAliased, variableMultisampleRate, inheritedQueries; } f; };
long appinfo[8]; /* sType,pNext,pAppName,appVer,pEngineName,engVer,apiVersion,pad */
int main(int argc, char **argv) {
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { printf("dlopen FAIL: %s\n", dlerror()); return 1; }
    void *(*gipa)(Handle, const char *) = dlsym(h, "vkGetInstanceProcAddr");
    VkResult (*ci)(const void *, const void *, void *) = dlsym(h, "vkCreateInstance");
    memset(appinfo, 0, sizeof(appinfo));
    appinfo[0] = 0; /* sType filled below via byte write */
    ((unsigned int *)appinfo)[0] = 0; ((unsigned int *)appinfo)[1] = 0;
    unsigned int *u = (unsigned int *)appinfo;
    u[0] = 0; /* will set sType via short */
    unsigned short *s16 = (unsigned short *)appinfo; s16[0] = 0;
    /* VkApplicationInfo: sType=APPLICATION_INFO(0), pNext=0, apiVersion=1.3 at offset 48 */
    unsigned int *ai = (unsigned int *)appinfo;
    ai[0] = 0; /* sType=0 (APPLICATION_INFO) */
    ai[12] = 0x00403000; /* apiVersion 1.3 at byte offset 48 */
    long cinfo[8]; memset(cinfo, 0, sizeof(cinfo));
    unsigned short *c16 = (unsigned short *)cinfo; c16[0] = 1; /* sType=INSTANCE_CREATE_INFO */
    cinfo[2] = (long)appinfo; /* pApplicationInfo at offset 16 */
    Handle inst = 0;
    VkResult r = ci(cinfo, 0, &inst);
    printf("ci rc=%d inst=%p\n", r, inst);
    if (!inst) return 1;
    U32 (*epd)(Handle, U32 *, Handle *) = gipa(inst, "vkEnumeratePhysicalDevices");
    U32 (*gpdf2)(Handle, void *) = (U32 (*)(Handle, void *))gipa(inst, "vkGetPhysicalDeviceFeatures2");
    U32 (*gpp)(Handle, U32 *, void *) = (U32 (*)(Handle, U32 *, void *))gipa(inst, "vkGetPhysicalDeviceProperties");
    U32 n = 0;
    epd(inst, &n, NULL);
    printf("physdevs=%u\n", n);
    if (!n) return 2;
    Handle pd[4]; epd(inst, &n, pd);
    long props[128]; memset(props, 0, sizeof(props));
    ((unsigned short *)props)[0] = 2; /* PHYSICAL_DEVICE_PROPERTIES_2 sType=2 */
    gpp(pd[0], NULL, props);
    /* properties: sType(4)+pad(4)+pNext(8)+apiVersion(4)+driverVersion(4)+vendorID(4)+deviceID(4)+deviceType(4)+name[256]... apiVersion at offset 16 */
    printf("apiVersion=0x%x vendorID=0x%x deviceID=0x%x name=%s\n", ((unsigned int *)props)[4], ((unsigned int *)props)[6], ((unsigned int *)props)[7], (char *)((long *)props)[3]);
    struct F2s f2; memset(&f2, 0, sizeof(f2));
    unsigned short *f16 = (unsigned short *)&f2; f16[0] = 1000109000 & 0xFFFF; /*Features2 sType lo*/
    f2.f.geometryShader = -1;
    gpdf2(pd[0], &f2);
    printf("FEATURES:\n");
    const char *names[] = {"robustBufferAccess","fullDrawIndexUint32","imageCubeArray","independentBlend","geometryShader","tessellationShader","logicOp","multiDrawIndirect","drawIndirectFirstInstance","depthClamp","depthBiasClamp","fillModeNonSolid","depthBounds","wideLines","largePoints","alphaToOne","multiViewport","samplerAnisotropy","textureCompressionETC2","textureCompressionASTC_LDR","textureCompressionBC","occlusionQueryPrecise","pipelineStatisticsQuery","vertexPipelineStoresAndAtomics","fragmentStoresAndAtomics","shaderTessellationAndGeometryPointSize","shaderImageGatherExtended","shaderStorageImageExtendedFormats","shaderStorageImageMultisample","shaderStorageImageReadWithoutFormat","shaderStorageImageWriteWithoutFormat","shaderUniformBufferArrayDynamicIndexing","shaderSampledImageArrayDynamicIndexing","shaderStorageBufferArrayDynamicIndexing","shaderStorageImageArrayDynamicIndexing","shaderClipDistance","shaderCullDistance","shaderFloat64","shaderInt64","shaderInt16","shaderResourceResidency","shaderResourceMinLod","sparseBinding","sparseResidencyBuffer","sparseResidencyImage2D","sparseResidencyAliased","variableMultisampleRate","inheritedQueries"};
    int *fv = &f2.f.robustBufferAccess;
    for (int i = 0; i < 48; i++) printf("  %s = %d\n", names[i], fv[i]);
    return 0;
}
EOF
gcc /root/ve4.c -o /root/ve4 -ldl && echo compiled'
echo "== run features probe =="
proot-distro login ubuntu --shared-tmp -- bash -c "export LD_LIBRARY_PATH=/root/panvk; export VK_DRIVER_FILES=/root/panvk/panvk_icd.json; /root/ve4 /root/panvk/libvulkan.so.1 2>&1 | head -60"
