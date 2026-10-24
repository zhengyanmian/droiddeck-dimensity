/* tri-probe.c — offscreen Vulkan triangle render test for self-built PanVK ICD.
 * No loader, no WSI, no window system: dlopen ICD -> device -> renderpass ->
 * draw -> copy to HOST_VISIBLE buffer -> verify pixels.
 *
 * PASS criteria: center pixel green (triangle), corner pixel red (clear).
 * This proves the GPU command stream truly executes (beyond vkCreateInstance).
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

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

static uint32_t find_mem(const VkPhysicalDeviceMemoryProperties *mp, uint32_t bits, VkMemoryPropertyFlags want)
{
    for (int i = 0; i < 32; i++) {
        if (!(bits & (1u << i))) continue;
        if ((mp->memoryTypes[i].propertyFlags & want) == want) return i;
    }
    fprintf(stderr, "FATAL: no memory type bits=0x%x want=0x%x\n", bits, want);
    exit(1);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    const char *so = argc > 1 ? argv[1] : "./libvulkan_panfrost.so";
    void *lib = dlopen(so, RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    printf("[ok] dlopen\n");

typedef int (*PFN_vkNegotiateLoaderICDInterfaceVersion)(int *version);
    int v = 5;
    ((PFN_vkNegotiateLoaderICDInterfaceVersion)dlsym(lib, "vk_icdNegotiateLoaderICDInterfaceVersion"))(&v);
    gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vk_icdGetInstanceProcAddr");
    if (!gipa) { fprintf(stderr, "FATAL: no vk_icdGetInstanceProcAddr\n"); return 1; }

    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                              .pApplicationName = "tri-probe",
                              .apiVersion = VK_API_VERSION_1_3 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                 .pApplicationInfo = &app };
    VkInstance inst;
    VkResult r = gipa(NULL, "vkCreateInstance") ? VK_SUCCESS : VK_ERROR_INITIALIZATION_FAILED;
    PFN_vkCreateInstance pCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    if (!pCreateInstance) die("no vkCreateInstance", r);
    if ((r = pCreateInstance(&ici, NULL, &inst)) != VK_SUCCESS) die("vkCreateInstance", r);
    printf("[ok] instance inst=%p\n", (void *)inst);

    /* debug: same comparison as micro-probe */
    printf("DBG gipa(inst, ...) ->\n");
    printf("  EnumeratePhysicalDevices = %p\n", gipa(inst, "vkEnumeratePhysicalDevices"));
    printf("  GetPhysicalDeviceProperties = %p\n", gipa(inst, "vkGetPhysicalDeviceProperties"));

    PFN_vkEnumeratePhysicalDevices pEnumPD = GI(EnumeratePhysicalDevices);
    PFN_vkGetPhysicalDeviceProperties pGetProps = GI(GetPhysicalDeviceProperties);
    PFN_vkGetPhysicalDeviceQueueFamilyProperties pGetQF = GI(GetPhysicalDeviceQueueFamilyProperties);
    PFN_vkGetPhysicalDeviceMemoryProperties pGetMem = GI(GetPhysicalDeviceMemoryProperties);
    PFN_vkCreateDevice pCreateDevice = (PFN_vkCreateDevice)gipa(inst, "vkCreateDevice");
    CHK(pEnumPD, "vkEnumeratePhysicalDevices");
    CHK(pGetProps, "vkGetPhysicalDeviceProperties");
    CHK(pGetQF, "vkGetPhysicalDeviceQueueFamilyProperties");
    CHK(pGetMem, "vkGetPhysicalDeviceMemoryProperties");
    CHK(pCreateDevice, "vkCreateDevice");

    uint32_t n = 0;
    if ((r = pEnumPD(inst, &n, NULL)) != VK_SUCCESS || n == 0) die("enumerate", r);
    VkPhysicalDevice pd;
    n = 1;
    pEnumPD(inst, &n, &pd);
    VkPhysicalDeviceProperties props;
    pGetProps(pd, &props);
    printf("[ok] PD: %s api %u.%u.%u devID 0x%08x\n", props.deviceName,
           props.apiVersion >> 22, (props.apiVersion >> 12) & 0x3ff, props.apiVersion & 0xfff,
           props.deviceID);

    uint32_t qf_count = 0;
    pGetQF(pd, &qf_count, NULL);
    VkQueueFamilyProperties *qf = calloc(qf_count, sizeof *qf);
    pGetQF(pd, &qf_count, qf);
    int gfx = -1;
    for (uint32_t i = 0; i < qf_count; i++)
        if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { gfx = i; break; }
    if (gfx < 0) die("no graphics queue", VK_ERROR_FEATURE_NOT_PRESENT);
    printf("[ok] queue family %d\n", gfx);

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = gfx, .queueCount = 1, .pQueuePriorities = &prio };
    VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                               .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci };
    VkDevice dev;
    if ((r = pCreateDevice(pd, &dci, NULL, &dev)) != VK_SUCCESS) die("vkCreateDevice", r);
    printf("[ok] device\n");

    dpa = (PFN_vkGetDeviceProcAddr)gipa(inst, "vkGetDeviceProcAddr");
    if (!dpa) die("no vkGetDeviceProcAddr", r);

    PFN_vkGetDeviceQueue pGetQueue = GD(GetDeviceQueue);
    PFN_vkCreateCommandPool pCreatePool = GD(CreateCommandPool);
    PFN_vkAllocateCommandBuffers pAllocCmd = GD(AllocateCommandBuffers);
    PFN_vkCreateImage pCreateImage = GD(CreateImage);
    PFN_vkGetImageMemoryRequirements pGetImgReq = GD(GetImageMemoryRequirements);
    PFN_vkAllocateMemory pAllocMem = GD(AllocateMemory);
    PFN_vkBindImageMemory pBindImg = GD(BindImageMemory);
    PFN_vkCreateImageView pCreateView = GD(CreateImageView);
    PFN_vkCreateRenderPass pCreateRP = GD(CreateRenderPass);
    PFN_vkCreateFramebuffer pCreateFB = GD(CreateFramebuffer);
    PFN_vkCreateShaderModule pCreateSM = GD(CreateShaderModule);
    PFN_vkCreatePipelineLayout pCreatePL = GD(CreatePipelineLayout);
    PFN_vkCreateGraphicsPipelines pCreateGP = GD(CreateGraphicsPipelines);
    PFN_vkCreateBuffer pCreateBuf = GD(CreateBuffer);
    PFN_vkGetBufferMemoryRequirements pGetBufReq = GD(GetBufferMemoryRequirements);
    PFN_vkBindBufferMemory pBindBuf = GD(BindBufferMemory);
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
    PFN_vkMapMemory pMap = GD(MapMemory);
    PFN_vkUnmapMemory pUnmap = GD(UnmapMemory);
    PFN_vkFreeMemory pFreeMem = GD(FreeMemory);
    PFN_vkDestroyImage pDestroyImg = GD(DestroyImage);
    PFN_vkDestroyBuffer pDestroyBuf = GD(DestroyBuffer);

    VkQueue queue; pGetQueue(dev, gfx, 0, &queue);

    VkCommandPoolCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = gfx };
    VkCommandPool pool; pCreatePool(dev, &pci, NULL, &pool);
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                        .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    VkCommandBuffer cmd; pAllocCmd(dev, &cai, &cmd);

    VkPhysicalDeviceMemoryProperties mp; pGetMem(pd, &mp);

    /* offscreen color image — LINEAR tiling + HOST_VISIBLE so we can read it
     * back both via CopyImageToBuffer and by direct CPU mapping (bisect). */
    VkImageCreateInfo ici2 = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { W, H, 1 }, .mipLevels = 1, .arrayLayers = 1,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .samples = VK_SAMPLE_COUNT_1_BIT };
    VkImage img; if ((r = pCreateImage(dev, &ici2, NULL, &img)) != VK_SUCCESS) die("vkCreateImage", r);
    VkMemoryRequirements req; pGetImgReq(dev, img, &req);
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = req.size,
        .memoryTypeIndex = find_mem(&mp, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
    VkDeviceMemory img_mem; if ((r = pAllocMem(dev, &mai, NULL, &img_mem)) != VK_SUCCESS) {
        mai.memoryTypeIndex = find_mem(&mp, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        if ((r = pAllocMem(dev, &mai, NULL, &img_mem)) != VK_SUCCESS) die("alloc img mem", r);
    }
    pBindImg(dev, img, img_mem, 0);
    printf("[ok] image %ux%u (LINEAR, memType %u)\n", W, H, mai.memoryTypeIndex);

    VkImageViewCreateInfo vci = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = img, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    VkImageView view; pCreateView(dev, &vci, NULL, &view);

    /* render pass: clear red -> store, final layout TRANSFER_SRC */
    VkAttachmentDescription att = { .format = VK_FORMAT_R8G8B8A8_UNORM, .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    VkAttachmentReference aref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 .colorAttachmentCount = 1, .pColorAttachments = &aref };
    VkRenderPassCreateInfo rpci = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                    .attachmentCount = 1, .pAttachments = &att, .subpassCount = 1, .pSubpasses = &sub };
    VkRenderPass rp; if ((r = pCreateRP(dev, &rpci, NULL, &rp)) != VK_SUCCESS) die("vkCreateRenderPass", r);

    VkFramebufferCreateInfo fbci = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = rp, .attachmentCount = 1, .pAttachments = &view, .width = W, .height = H, .layers = 1 };
    VkFramebuffer fb; pCreateFB(dev, &fbci, NULL, &fb);

    /* shaders */
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

    /* readback buffer */
    VkBufferCreateInfo bci = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = W * H * 4, .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT };
    VkBuffer buf; pCreateBuf(dev, &bci, NULL, &buf);
    VkMemoryRequirements breq; pGetBufReq(dev, buf, &breq);
    VkMemoryAllocateInfo bmai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = breq.size,
        .memoryTypeIndex = find_mem(&mp, breq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
    VkDeviceMemory buf_mem; if ((r = pAllocMem(dev, &bmai, NULL, &buf_mem)) != VK_SUCCESS) die("alloc buf mem", r);
    pBindBuf(dev, buf, buf_mem, 0);

    /* record */
    VkCommandBufferBeginInfo bbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    pBeginCmd(cmd, &bbi);

    VkClearValue clear = { .color = { { 1.0f, 0.0f, 0.0f, 1.0f } } }; /* red clear */
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
    printf("[ok] recorded\n");

    VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    if ((r = pSubmit(queue, 1, &si, VK_NULL_HANDLE)) != VK_SUCCESS) die("vkQueueSubmit", r);
    pWait(queue);
    printf("[ok] submitted + done\n");

    void *data; pMap(dev, buf_mem, 0, VK_WHOLE_SIZE, 0, &data);
    const unsigned char *px = data;
    struct { int x, y; const char *what; int green; } checks[] = {
        { 128, 128, "center (triangle)", 1 },
        {   8,   8, "corner (clear)",    0 },
        { 250, 250, "far corner",        0 },
        { 128,  70, "apex area",         1 },
    };

    /* readback A: via CopyImageToBuffer (what we just did) */
    int pass_copy = 1;
    printf("== readback A: via CopyImageToBuffer ==\n");
    for (unsigned i = 0; i < sizeof checks / sizeof checks[0]; i++) {
        const unsigned char *p = px + (checks[i].y * W + checks[i].x) * 4;
        int is_green = p[1] > 200 && p[0] < 50;
        printf("  %s @(%d,%d): R=%u G=%u B=%u -> %s\n", checks[i].what, checks[i].x, checks[i].y,
               p[0], p[1], p[2], is_green == checks[i].green ? "OK" : "MISMATCH");
        if (is_green != checks[i].green) pass_copy = 0;
    }

    /* readback B: direct CPU map of the LINEAR image */
    printf("== readback B: direct map of linear image ==\n");
    PFN_vkGetImageSubresourceLayout pLayout = GD(GetImageSubresourceLayout);
    PFN_vkMapMemory pMapImg = GD(MapMemory);
    VkImageSubresource isub = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout lay; pLayout(dev, img, &isub, &lay);
    void *idata; VkResult rm = pMapImg(dev, img_mem, lay.offset, lay.size, 0, &idata);
    int pass_direct = 0;
    if (rm == VK_SUCCESS && idata) {
        const unsigned char *ip = (const unsigned char *)idata + lay.offset;
        pass_direct = 1;
        for (unsigned i = 0; i < sizeof checks / sizeof checks[0]; i++) {
            const unsigned char *p = ip + checks[i].y * lay.rowPitch + checks[i].x * 4;
            int is_green = p[1] > 200 && p[0] < 50;
            printf("  %s @(%d,%d): R=%u G=%u B=%u -> %s\n", checks[i].what, checks[i].x, checks[i].y,
                   p[0], p[1], p[2], is_green == checks[i].green ? "OK" : "MISMATCH");
            if (is_green != checks[i].green) pass_direct = 0;
        }
    } else {
        printf("  map image failed r=%d\n", rm);
    }
    pUnmap(dev, buf_mem);

    if (pass_copy && pass_direct)      printf("RENDER_PASS_SUCCESS (copy + direct)\n");
    else if (pass_direct)              printf("RENDER_PARTIAL: direct read OK, CopyImageToBuffer broken\n");
    else if (pass_copy)                printf("RENDER_PARTIAL: copy OK, direct map mismatch\n");
    else                               printf("RENDER_FAIL: image has no GPU-written content\n");
    PFN_vkFreeMemory pFreeMem2 = GD(FreeMemory);
    pFreeMem2(dev, buf_mem, NULL);
    pDestroyBuf(dev, buf, NULL);
    pDestroyImg(dev, img, NULL);
    pFreeMem2(dev, img_mem, NULL);
    return (pass_copy && pass_direct) ? 0 : 2;
}
