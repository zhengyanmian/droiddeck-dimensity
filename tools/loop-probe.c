/* ahb-probe.c — AHardwareBuffer external-memory roundtrip for self-built PanVK ICD.
 * Proves the display bridge primitive: GPU renders into an Android
 * AHardwareBuffer (gralloc buffer) via VK_ANDROID_external_memory_android_hardware_buffer,
 * then the buffer is read back via CPU lock (as a compositor/app would).
 *
 * PASS: triangle green in center, clear red at corners (CPU-locked AHB pixels).
 * This is the core primitive for putting PanVK frames on screen without gamescope.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/hardware_buffer.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include "tri_spv.inc" /* tri_vert_spv[], tri_vert_spv_len, tri_frag_spv[], tri_frag_spv_len */

#define W 256
#define H 256

static PFN_vkGetInstanceProcAddr gipa;
static PFN_vkGetDeviceProcAddr dpa;
#define GI(name) ((PFN_vk##name)gipa(inst, "vk" #name))
#define GD(name) ((PFN_vk##name)dpa(dev, "vk" #name))
#define CHK(p, what) do { if (!(p)) { fprintf(stderr, "FATAL: %s == NULL\n", what); return 3; } } while (0)

static void die(const char *msg, VkResult r)
{
    fprintf(stderr, "FATAL: %s (r=%d)\n", msg, r);
    exit(1);
}

static const char *ext_names(const VkExtensionProperties *e, uint32_t n)
{
    static char buf[4096];
    buf[0] = 0;
    for (uint32_t i = 0; i < n; i++) {
        strncat(buf, e[i].extensionName, sizeof buf - strlen(buf) - 2);
        strcat(buf, " ");
    }
    return buf;
}

static int have_ext(const VkExtensionProperties *e, uint32_t n, const char *name)
{
    for (uint32_t i = 0; i < n; i++)
        if (!strcmp(e[i].extensionName, name)) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    const char *so = argc > 1 ? argv[1] : "./libvulkan_panfrost.so";
    void *lib = dlopen(so, RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    printf("[ok] dlopen\n");

    int v = 5;
    ((int (*)(int *))dlsym(lib, "vk_icdNegotiateLoaderICDInterfaceVersion"))(&v);
    gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vk_icdGetInstanceProcAddr");
    if (!gipa) { fprintf(stderr, "FATAL: no vk_icdGetInstanceProcAddr\n"); return 1; }

    const char *iexts[] = {
        "VK_KHR_external_memory_capabilities",
        "VK_ANDROID_external_memory_android_hardware_buffer",
    };
    PFN_vkEnumerateInstanceExtensionProperties pEnumIExt =
        (PFN_vkEnumerateInstanceExtensionProperties)gipa(NULL, "vkEnumerateInstanceExtensionProperties");
    /* enumerate what the ICD actually advertises at instance level */
    if (pEnumIExt) {
        uint32_t ie = 0;
        pEnumIExt(NULL, &ie, NULL);
        if (ie) {
            VkExtensionProperties *iext_list = calloc(ie, sizeof *iext_list);
            pEnumIExt(NULL, &ie, iext_list);
            printf("[ext] instance list: %s\n", ext_names(iext_list, ie));
            free(iext_list);
        }
    }
    /* enable only the requested ones that exist */
    const char *enabled[2];
    uint32_t n_enabled = 0;
    if (pEnumIExt) {
        uint32_t ie = 0;
        pEnumIExt(NULL, &ie, NULL);
        VkExtensionProperties *iext_list = calloc(ie ? ie : 1, sizeof *iext_list);
        if (ie) pEnumIExt(NULL, &ie, iext_list);
        for (uint32_t i = 0; i < 2; i++)
            if (have_ext(iext_list, ie, iexts[i])) enabled[n_enabled++] = iexts[i];
        free(iext_list);
    }
    printf("[ext] enabling %u instance extensions\n", n_enabled);
    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                              .pApplicationName = "ahb-probe",
                              .apiVersion = VK_API_VERSION_1_3 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                 .pApplicationInfo = &app,
                                 .enabledExtensionCount = n_enabled, .ppEnabledExtensionNames = enabled };
    VkInstance inst;
    PFN_vkCreateInstance pCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    CHK(pCreateInstance, "vkCreateInstance");
    VkResult r;
    if ((r = pCreateInstance(&ici, NULL, &inst)) != VK_SUCCESS) die("vkCreateInstance", r);
    printf("[ok] instance (ext memory + AHB enabled)\n");

    PFN_vkEnumeratePhysicalDevices pEnumPD = GI(EnumeratePhysicalDevices);
    PFN_vkGetPhysicalDeviceProperties pGetProps = GI(GetPhysicalDeviceProperties);
    PFN_vkGetPhysicalDeviceQueueFamilyProperties pGetQF = GI(GetPhysicalDeviceQueueFamilyProperties);
    PFN_vkGetPhysicalDeviceMemoryProperties pGetMem = GI(GetPhysicalDeviceMemoryProperties);
    PFN_vkGetPhysicalDeviceProperties2 pGetProps2 = GI(GetPhysicalDeviceProperties2);
    PFN_vkEnumerateDeviceExtensionProperties pEnumDExt = GI(EnumerateDeviceExtensionProperties);
    PFN_vkCreateDevice pCreateDevice = (PFN_vkCreateDevice)gipa(inst, "vkCreateDevice");
    CHK(pEnumPD, "vkEnumeratePhysicalDevices");
    CHK(pEnumDExt, "vkEnumerateDeviceExtensionProperties");

    uint32_t n = 0;
    if ((r = pEnumPD(inst, &n, NULL)) != VK_SUCCESS || n == 0) die("enumerate", r);
    VkPhysicalDevice pd;
    n = 1;
    pEnumPD(inst, &n, &pd);
    VkPhysicalDeviceProperties props;
    pGetProps(pd, &props);
    printf("[ok] PD: %s api %u.%u.%u\n", props.deviceName,
           props.apiVersion >> 22, (props.apiVersion >> 12) & 0x3ff, props.apiVersion & 0xfff);

    uint32_t de = 0;
    pEnumDExt(pd, NULL, &de, NULL);
    VkExtensionProperties *dext = calloc(de, sizeof *dext);
    pEnumDExt(pd, NULL, &de, dext);
    const int has_ahb = have_ext(dext, de, "VK_ANDROID_external_memory_android_hardware_buffer");
    const int has_extmem = have_ext(dext, de, "VK_KHR_external_memory");
    printf("[ext] device has AHB ext: %s | KHR_external_memory: %s\n",
           has_ahb ? "YES" : "NO", has_extmem ? "YES" : "NO");
    printf("[ext] full list: %s\n", ext_names(dext, de));
    if (!has_ahb || !has_extmem) { printf("AHB_UNSUPPORTED\n"); return 4; }

    uint32_t qf_count = 0;
    pGetQF(pd, &qf_count, NULL);
    VkQueueFamilyProperties *qf = calloc(qf_count, sizeof *qf);
    pGetQF(pd, &qf_count, qf);
    int gfx = -1;
    for (uint32_t i = 0; i < qf_count; i++)
        if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { gfx = i; break; }
    if (gfx < 0) die("no graphics queue", VK_ERROR_FEATURE_NOT_PRESENT);

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = gfx, .queueCount = 1, .pQueuePriorities = &prio };
    const char *dexts[] = {
        "VK_KHR_external_memory",
        "VK_ANDROID_external_memory_android_hardware_buffer",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_bind_memory2",
    };
    VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                               .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci,
                               .enabledExtensionCount = 5, .ppEnabledExtensionNames = dexts };
    VkDevice dev;
    if ((r = pCreateDevice(pd, &dci, NULL, &dev)) != VK_SUCCESS) die("vkCreateDevice", r);
    printf("[ok] device\n");

    dpa = (PFN_vkGetDeviceProcAddr)gipa(inst, "vkGetDeviceProcAddr");

    /* --- allocate AHardwareBuffer (gralloc) --- */
    AHardwareBuffer_Desc adesc = {
        .width = W, .height = H, .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
        .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE
               | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
    };
    AHardwareBuffer *ahb = NULL;
    if (AHardwareBuffer_allocate(&adesc, &ahb) != 0 || !ahb) { fprintf(stderr, "FATAL: AHB allocate\n"); return 1; }
    printf("[ok] AHardwareBuffer allocated %ux%u\n", W, H);

    /* --- query compatible memory for this AHB --- */
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID pGetAHBProps =
        (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)gipa(inst, "vkGetAndroidHardwareBufferPropertiesANDROID");
    if (!pGetAHBProps) { fprintf(stderr, "FATAL: no vkGetAndroidHardwareBufferPropertiesANDROID\n"); return 3; }
    VkAndroidHardwareBufferPropertiesANDROID ahbprops = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID };
    if ((r = pGetAHBProps(dev, ahb, &ahbprops)) != VK_SUCCESS) die("GetAHBProps", r);
    printf("[ok] AHB vk props: size=%llu memTypeBits=0x%x\n",
           (unsigned long long)ahbprops.allocationSize, ahbprops.memoryTypeBits);
    if (!ahbprops.memoryTypeBits) { printf("AHB_NO_MEMTYPE\n"); return 4; }
    uint32_t mem_idx = __builtin_ctz(ahbprops.memoryTypeBits);

    /* --- create image importing the AHB --- */
    PFN_vkCreateImage pCreateImage = GD(CreateImage);
    VkExternalMemoryImageCreateInfo extimg = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID };
    VkImageCreateInfo ici2 = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &extimg,
        .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { W, H, 1 }, .mipLevels = 1, .arrayLayers = 1,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .samples = VK_SAMPLE_COUNT_1_BIT };
    VkImage img;
    if ((r = pCreateImage(dev, &ici2, NULL, &img)) != VK_SUCCESS) die("vkCreateImage(AHB)", r);

    PFN_vkGetImageMemoryRequirements pGetImgReq = GD(GetImageMemoryRequirements);
    VkMemoryRequirements req; pGetImgReq(dev, img, &req);
    printf("[ok] image created; dedicated=%u\n", req.size == ahbprops.allocationSize);

    PFN_vkAllocateMemory pAllocMem = GD(AllocateMemory);
    PFN_vkBindImageMemory2 pBindImg2 = GD(BindImageMemory2);
    VkImportAndroidHardwareBufferInfoANDROID imp = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID, .buffer = ahb };
    VkMemoryDedicatedAllocateInfo ded = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .pNext = &imp, .image = img };
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                 .pNext = &ded,
                                 .allocationSize = ahbprops.allocationSize,
                                 .memoryTypeIndex = mem_idx };
    VkDeviceMemory img_mem;
    if ((r = pAllocMem(dev, &mai, NULL, &img_mem)) != VK_SUCCESS) die("AllocMem(import AHB)", r);
    VkBindImageMemoryInfo bimi = { .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                                   .image = img, .memory = img_mem, .memoryOffset = 0 };
    if ((r = pBindImg2(dev, 1, &bimi)) != VK_SUCCESS) die("BindImageMemory2", r);
    printf("[ok] AHB imported as VkImage memory (memType %u)\n", mem_idx);

    /* --- rest mirrors tri-probe: renderpass clear red + green triangle --- */
    PFN_vkCreateImageView pCreateView = GD(CreateImageView);
    PFN_vkCreateRenderPass pCreateRP = GD(CreateRenderPass);
    PFN_vkCreateFramebuffer pCreateFB = GD(CreateFramebuffer);
    PFN_vkCreateShaderModule pCreateSM = GD(CreateShaderModule);
    PFN_vkCreatePipelineLayout pCreatePL = GD(CreatePipelineLayout);
    PFN_vkCreateGraphicsPipelines pCreateGP = GD(CreateGraphicsPipelines);
    PFN_vkCreateCommandPool pCreatePool = GD(CreateCommandPool);
    PFN_vkAllocateCommandBuffers pAllocCmd = GD(AllocateCommandBuffers);
    PFN_vkBeginCommandBuffer pBeginCmd = GD(BeginCommandBuffer);
    PFN_vkCmdBeginRenderPass pCmdBeginRP = GD(CmdBeginRenderPass);
    PFN_vkCmdBindPipeline pCmdBindPipe = GD(CmdBindPipeline);
    PFN_vkCmdSetViewport pCmdViewport = GD(CmdSetViewport);
    PFN_vkCmdSetScissor pCmdScissor = GD(CmdSetScissor);
    PFN_vkCmdDraw pCmdDraw = GD(CmdDraw);
    PFN_vkCmdEndRenderPass pCmdEndRP = GD(CmdEndRenderPass);
    PFN_vkEndCommandBuffer pEndCmd = GD(EndCommandBuffer);
    PFN_vkQueueSubmit pSubmit = GD(QueueSubmit);
    PFN_vkQueueWaitIdle pWait = GD(QueueWaitIdle);
    PFN_vkGetDeviceQueue pGetQueue = GD(GetDeviceQueue);

    VkImageViewCreateInfo vci = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = img, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    VkImageView view; pCreateView(dev, &vci, NULL, &view);

    VkAttachmentDescription att = { .format = VK_FORMAT_R8G8B8A8_UNORM, .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference aref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 .colorAttachmentCount = 1, .pColorAttachments = &aref };
    VkRenderPassCreateInfo rpci = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                    .attachmentCount = 1, .pAttachments = &att, .subpassCount = 1, .pSubpasses = &sub };
    VkRenderPass rp; if ((r = pCreateRP(dev, &rpci, NULL, &rp)) != VK_SUCCESS) die("vkCreateRenderPass", r);
    VkFramebufferCreateInfo fbci = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = rp, .attachmentCount = 1, .pAttachments = &view, .width = W, .height = H, .layers = 1 };
    VkFramebuffer fb; pCreateFB(dev, &fbci, NULL, &fb);

    VkShaderModuleCreateInfo smci_v = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = tri_vert_spv_len, .pCode = (const uint32_t *)tri_vert_spv };
    VkShaderModuleCreateInfo smci_f = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = tri_frag_spv_len, .pCode = (const uint32_t *)tri_frag_spv };
    VkShaderModule vs, fs;
    if ((r = pCreateSM(dev, &smci_v, NULL, &vs)) != VK_SUCCESS) die("vert shader", r);
    if ((r = pCreateSM(dev, &smci_f, NULL, &fs)) != VK_SUCCESS) die("frag shader", r);
    VkPipelineLayoutCreateInfo plci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    VkPipelineLayout pl; pCreatePL(dev, &plci, NULL, &pl);
    VkPipelineShaderStageCreateInfo stages[2] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main" },
    };
    VkPipelineVertexInputStateCreateInfo vi = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo ia = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                                  .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    VkViewport vp = { 0, 0, (float)W, (float)H, 0, 1 };
    VkRect2D sc = { { 0, 0 }, { W, H } };
    VkPipelineViewportStateCreateInfo vps = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                              .viewportCount = 1, .pViewports = &vp, .scissorCount = 1, .pScissors = &sc };
    VkPipelineRasterizationStateCreateInfo rs = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f };
    VkPipelineMultisampleStateCreateInfo ms = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
    VkPipelineColorBlendAttachmentState cba = { .colorWriteMask = 0xf };
    VkPipelineColorBlendStateCreateInfo cb = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                               .attachmentCount = 1, .pAttachments = &cba };
    VkGraphicsPipelineCreateInfo gpci = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2, .pStages = stages, .pVertexInputState = &vi, .pInputAssemblyState = &ia,
        .pViewportState = &vps, .pRasterizationState = &rs, .pMultisampleState = &ms,
        .pColorBlendState = &cb, .layout = pl, .renderPass = rp, .subpass = 0 };
    VkPipeline pipe; if ((r = pCreateGP(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipe)) != VK_SUCCESS) die("pipeline", r);
    printf("[ok] pipeline\n");

    VkCommandPoolCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = gfx };
    VkCommandPool pool; pCreatePool(dev, &pci, NULL, &pool);
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                        .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    VkCommandBuffer cmd; pAllocCmd(dev, &cai, &cmd);

    VkCommandBufferBeginInfo bbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags = 0 };
    pBeginCmd(cmd, &bbi);
    VkClearValue clear = { .color = { { 1.0f, 0.0f, 0.0f, 1.0f } } };
    VkRenderPassBeginInfo rpbi = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = rp, .framebuffer = fb, .renderArea = { { 0, 0 }, { W, H } },
        .clearValueCount = 1, .pClearValues = &clear };
    pCmdBeginRP(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    pCmdBindPipe(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    pCmdViewport(cmd, 0, 1, &vp);
    pCmdScissor(cmd, 0, 1, &sc);
    pCmdDraw(cmd, 3, 1, 0, 0);
    pCmdEndRP(cmd);
    pEndCmd(cmd);
    PFN_vkResetCommandBuffer pReset = GD(ResetCommandBuffer);

    VkQueue queue; pGetQueue(dev, gfx, 0, &queue);
    /* repeated-submit stability test: N frames, fence per frame */
    PFN_vkCreateFence pCreateFence = GD(CreateFence);
    PFN_vkWaitForFences pWaitF = GD(WaitForFences);
    PFN_vkResetFences pResetF = GD(ResetFences);
    VkFence fence; VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    pCreateFence(dev, &fci, NULL, &fence);
    int N = argc > 2 ? atoi(argv[2]) : 300;
    for (int f = 0; f < N; f++) {
        VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
        VkResult sr = pSubmit(queue, 1, &si, fence);
        if (sr != VK_SUCCESS) { printf("FRAME %d submit r=%d\n", f, sr); return 5; }
        if (pWaitF(dev, 1, &fence, VK_TRUE, 5000000000ULL) != VK_SUCCESS) { printf("FRAME %d fence timeout\n", f); return 6; }
        pResetF(dev, 1, &fence);
        pReset(cmd, 0);
        if (f % 50 == 0) printf("[frame %d] ok\n", f);
    }
    printf("ALL %d FRAMES OK\n", N);

    /* --- compositor-style readback: CPU lock the AHB --- */
    void *adata = NULL;
    if (AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &adata) != 0 || !adata) {
        fprintf(stderr, "FATAL: AHB lock\n"); return 1;
    }
    AHardwareBuffer_Desc rdesc; AHardwareBuffer_describe(ahb, &rdesc);
    uint32_t stride = rdesc.stride ? rdesc.stride : W;
    const unsigned char *ip = adata;
    struct { int x, y; const char *what; int green; } checks[] = {
        { W / 2, H / 2,       "center (triangle)", 1 },
        {   8,   8,           "corner (clear)",    0 },
        { W - 6, H - 6,       "far corner",        0 },
        { W / 2, (int)(H * 0.3), "apex area",      1 },
    };
    int pass = 1;
    for (unsigned i = 0; i < sizeof checks / sizeof checks[0]; i++) {
        const unsigned char *p = ip + checks[i].y * stride * 4 + checks[i].x * 4;
        int is_green = p[1] > 200 && p[0] < 50;
        printf("  %s @(%d,%d): R=%u G=%u B=%u A=%u -> %s\n", checks[i].what, checks[i].x, checks[i].y,
               p[0], p[1], p[2], p[3], is_green == checks[i].green ? "OK" : "MISMATCH");
        if (is_green != checks[i].green) pass = 0;
    }
    AHardwareBuffer_unlock(ahb, NULL);

    printf("%s\n", pass ? "AHB_ROUNDTRIP_SUCCESS" : "AHB_ROUNDTRIP_FAIL");
    AHardwareBuffer_release(ahb);
    return pass ? 0 : 2;
}
