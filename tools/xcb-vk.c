/* xcb-vk.c — minimal Vulkan-on-XCB smoke test for PanVK X11 WSI.
 * Connects to $DISPLAY, opens a window, creates VK_KHR_xcb_surface,
 * renders rotating clear colors + one triangle-less clear, presents via
 * swapchain. Prints PROBE status. Exit 0 on success.
 * Build (host cross, headers only):
 *   aarch64 linux: use NDK clang + header-only xcb prefix in WSL.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>

static VkInstance inst;
static VkDevice dev;
static VkPhysicalDevice pd;
static VkQueue queue;
static uint32_t qfam;

/* driver-direct: dlopen the ICD, everything through vk_icdGetInstanceProcAddr */
static void *drv;
static PFN_vkGetInstanceProcAddr gipa;
static PFN_vkGetDeviceProcAddr dpa;
#define GI(name) ((PFN_vk##name)gipa(inst, "vk" #name))
#define GD(name) ((PFN_vk##name)dpa(dev, "vk" #name))

#define DIE(msg, r) do { printf("FAIL %s r=%d\n", msg, (int)(r)); exit(1); } while (0)

int main(int argc, char **argv)
{
    const char *icd = argc > 1 ? argv[1] : "./libvulkan_panfrost.so";
    int frames = argc > 2 ? atoi(argv[2]) : 120;

    drv = dlopen(icd, RTLD_NOW | RTLD_LOCAL);
    if (!drv) { printf("FAIL dlopen %s: %s\n", icd, dlerror()); return 1; }
    gipa = (PFN_vkGetInstanceProcAddr)dlsym(drv, "vk_icdGetInstanceProcAddr");
    if (!gipa) gipa = (PFN_vkGetInstanceProcAddr)dlsym(drv, "vkGetInstanceProcAddr");
    if (!gipa) { printf("FAIL no vk_icdGetInstanceProcAddr\n"); return 1; }

    /* ---- X connection + window ---- */
    xcb_connection_t *xc = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(xc)) { printf("FAIL xcb_connect\n"); return 1; }
    xcb_screen_t *scr = xcb_setup_roots_iterator(xcb_get_setup(xc)).data;
    xcb_window_t win = xcb_generate_id(xc);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t vals[2] = { scr->black_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY };
    xcb_create_window(xc, XCB_COPY_FROM_PARENT, win, scr->root,
                      64, 64, 800, 600, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      scr->root_visual, mask, vals);
    xcb_map_window(xc, win);
    xcb_flush(xc);
    usleep(300000); /* let the window map */

    /* ---- instance ---- */
    const char *iexts[] = { "VK_KHR_surface", "VK_KHR_xcb_surface" };
    VkApplicationInfo ai = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                 .pApplicationInfo = &ai,
                                 .enabledExtensionCount = 2, .ppEnabledExtensionNames = iexts };
    PFN_vkCreateInstance pCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    VkResult r = pCreateInstance(&ici, NULL, &inst);
    if (r) DIE("vkCreateInstance", r);

    /* ---- pick our physical device ---- */
    PFN_vkEnumeratePhysicalDevices pEnum = GI(EnumeratePhysicalDevices);
    uint32_t n = 0;
    pEnum(inst, &n, NULL);
    if (!n) { printf("FAIL no physical devices\n"); return 1; }
    VkPhysicalDevice pds[8]; pEnum(inst, &n, pds);
    pd = pds[0];
    PFN_vkGetPhysicalDeviceProperties pProps = GI(GetPhysicalDeviceProperties);
    VkPhysicalDeviceProperties props; pProps(pd, &props);
    printf("[ok] GPU0 %s driverAPI %u.%u.%u\n", props.deviceName,
           VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion), VK_API_VERSION_PATCH(props.apiVersion));

    /* ---- queue ---- */
    PFN_vkGetPhysicalDeviceQueueFamilyProperties pQF = GI(GetPhysicalDeviceQueueFamilyProperties);
    uint32_t qn = 0; pQF(pd, &qn, NULL);
    VkQueueFamilyProperties qp[16]; pQF(pd, &qn, qp);
    for (uint32_t i = 0; i < qn; i++) if (qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { qfam = i; break; }

    VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = qfam, .queueCount = 1,
                                    .pQueuePriorities = (float[]){1.0f} };
    const char *dexts[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                               .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci,
                               .enabledExtensionCount = 1, .ppEnabledExtensionNames = dexts };
    PFN_vkCreateDevice pCreateDev = GI(CreateDevice);
    if ((r = pCreateDev(pd, &dci, NULL, &dev))) DIE("vkCreateDevice", r);
    dpa = (PFN_vkGetDeviceProcAddr)gipa(inst, "vkGetDeviceProcAddr");
    GD(GetDeviceQueue)(dev, qfam, 0, &queue);

    /* ---- surface ---- */
    VkXcbSurfaceCreateInfoKHR sci = { .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                                      .connection = xc, .window = win };
    VkSurfaceKHR surf;
    PFN_vkCreateXcbSurfaceKHR pCreateSurf = (PFN_vkCreateXcbSurfaceKHR)gipa(inst, "vkCreateXcbSurfaceKHR");
    if (!pCreateSurf) { printf("FAIL no vkCreateXcbSurfaceKHR (WSI missing?)\n"); return 1; }
    if ((r = pCreateSurf(inst, &sci, NULL, &surf))) DIE("vkCreateXcbSurfaceKHR", r);
    printf("[ok] xcb surface created\n");

    VkBool32 present_ok = VK_FALSE;
    GI(GetPhysicalDeviceSurfaceSupportKHR)(pd, qfam, surf, &present_ok);
    if (!present_ok) { printf("FAIL surface not supported on queue %u\n", qfam); return 1; }

    /* ---- swapchain ---- */
    VkSurfaceCapabilitiesKHR caps;
    GI(GetPhysicalDeviceSurfaceCapabilitiesKHR)(pd, surf, &caps);
    printf("[caps] minImages=%u maxImages=%u extent=%ux%u layers=%u modes=%x\n",
           caps.minImageCount, caps.maxImageCount,
           caps.currentExtent.width, caps.currentExtent.height,
           caps.maxImageArrayLayers, caps.supportedUsageFlags);
    uint32_t fmt_n = 0; GI(GetPhysicalDeviceSurfaceFormatsKHR)(pd, surf, &fmt_n, NULL);
    if (!fmt_n) { printf("FAIL no surface formats\n"); return 1; }
    VkSurfaceFormatKHR fmts[16]; GI(GetPhysicalDeviceSurfaceFormatsKHR)(pd, surf, &fmt_n, fmts);
    for (uint32_t i = 0; i < fmt_n && i < 8; i++)
        printf("[fmt %u] format=%d colorspace=%d\n", i, fmts[i].format, fmts[i].colorSpace);
    VkSurfaceFormatKHR fmt = fmts[0];
    if (fmts[0].format == VK_FORMAT_UNDEFINED) fmt = (VkSurfaceFormatKHR){VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    uint32_t pm_n = 0; GI(GetPhysicalDeviceSurfacePresentModesKHR)(pd, surf, &pm_n, NULL);
    VkPresentModeKHR pms[8]; GI(GetPhysicalDeviceSurfacePresentModesKHR)(pd, surf, &pm_n, pms);
    printf("[present modes]"); for (uint32_t i = 0; i < pm_n && i < 8; i++) printf(" %d", pms[i]); printf("\n");

    uint32_t min_images = caps.minImageCount < 2 ? 2 : caps.minImageCount;
    VkSwapchainCreateInfoKHR scci = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surf, .minImageCount = min_images,
        .imageFormat = fmt.format, .imageColorSpace = fmt.colorSpace,
        .imageExtent = caps.currentExtent, .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = caps.currentTransform, .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR, .clipped = VK_TRUE };
    VkSwapchainKHR sc;
    if ((r = GD(CreateSwapchainKHR)(dev, &scci, NULL, &sc))) DIE("vkCreateSwapchainKHR", r);
    uint32_t img_n = 0; VkImage imgs[8];
    GD(GetSwapchainImagesKHR)(dev, sc, &img_n, NULL);
    if (img_n == 0) {
        /* some WSI implementations defer image creation; wait and retry */
        sleep(1);
        GD(GetSwapchainImagesKHR)(dev, sc, &img_n, NULL);
    }
    if (img_n == 0 || img_n > 8) { printf("FAIL swapchain images=%u\n", img_n); return 1; }
    GD(GetSwapchainImagesKHR)(dev, sc, &img_n, imgs);
    printf("[ok] swapchain %ux%u fmt=%d images=%u\n", scci.imageExtent.width, scci.imageExtent.height, fmt.format, img_n);

    /* ---- render: simple clears rotating hue, present each frame ---- */
    VkCommandPool cpool;
    VkCommandPoolCreateInfo cpci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qfam };
    GD(CreateCommandPool)(dev, &cpci, NULL, &cpool);
    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                        .commandPool = cpool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    GD(AllocateCommandBuffers)(dev, &cai, &cb);
    VkSemaphore done; VkSemaphoreCreateInfo sci2 = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    GD(CreateSemaphore)(dev, &sci2, NULL, &done);

    for (int f = 0; f < frames; f++) {
        uint32_t idx = 0;
        if (getenv("CBVK_NOACQUIRE")) {
            /* diagnostic: submit clears directly to every swapchain image,
             * bypassing acquire/present entirely */
            idx = f % img_n;
        } else {
            r = GD(AcquireNextImageKHR)(dev, sc, 1000000000ULL, VK_NULL_HANDLE, VK_NULL_HANDLE, &idx);
            if (r && r != VK_SUBOPTIMAL_KHR) DIE("acquire", r);
        }
        float t = f / (float)frames;
        VkClearColorValue cv = { .float32 = { 0.5f + 0.5f * t, 0.2f, 1.0f - 0.5f * t, 1.0f } };
        VkImageSubresourceRange srr = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        GD(ResetCommandBuffer)(cb, 0);
        GD(BeginCommandBuffer)(cb, &bi);
        VkImageMemoryBarrier toDst = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = imgs[idx], .subresourceRange = srr };
        GD(CmdPipelineBarrier)(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &toDst);
        GD(CmdClearColorImage)(cb, imgs[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &cv, 1, &srr);
        VkImageMemoryBarrier toPres = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = imgs[idx], .subresourceRange = srr };
        GD(CmdPipelineBarrier)(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &toPres);
        GD(EndCommandBuffer)(cb);
        VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb };
        if ((r = GD(QueueSubmit)(queue, 1, &si, VK_NULL_HANDLE))) { printf("FRAME %d submit r=%d\n", f, r); return 5; }
        if (getenv("CBVK_NOPRESENT")) {
            if (f % 10 == 0) printf("[frame %d] submitted (no present)\n", f);
            continue;
        }
        VkPresentInfoKHR pi = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .swapchainCount = 1, .pSwapchains = &sc, .pImageIndices = &idx };
        r = GD(QueuePresentKHR)(queue, &pi);
        if (r && r != VK_SUBOPTIMAL_KHR) { printf("FRAME %d present r=%d\n", f, r); return 6; }
        if (f % 30 == 0) printf("[frame %d] ok\n", f);
    }
    printf("XCB_VK_SUCCESS (%d frames)\n", frames);
    return 0;
}
