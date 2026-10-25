// vktri.exe: minimal Win32 Vulkan swapchain test, fully dynamic
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define VK_USE_PLATFORM_WIN32_KHR
typedef unsigned int U32;
#include <vulkan/vulkan.h>

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(h, m, w, l);
}

#define CK(x, msg) if (r != VK_SUCCESS) { printf("FAIL %s rc=%d\n", msg, r); return (int)r; } else printf("[ok] %s\n", msg);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    setvbuf(stdout, NULL, _IONBF, 0);
    freopen("C:\\test\\vklog.txt", "w", stdout);

    HMODULE vk = LoadLibraryA("vulkan-1.dll");
    if (!vk) { printf("FAIL LoadLibrary vulkan-1.dll\n"); return 9; }
    PFN_vkGetInstanceProcAddr gipa = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk, "vkGetInstanceProcAddr");
    PFN_vkCreateInstance pci = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");

    VkApplicationInfo ai = {VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL, "vktri", 0, "vktri", 0, VK_API_VERSION_1_1};
    const char *ie[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
    VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL, 0, &ai, 0, NULL, 2, ie};
    VkInstance inst;
    VkResult r = pci(&ici, NULL, &inst); CK(r, "vkCreateInstance");

    PFN_vkCreateWin32SurfaceKHR cws = (PFN_vkCreateWin32SurfaceKHR)gipa(inst, "vkCreateWin32SurfaceKHR");

    WNDCLASSA wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hInst; wc.lpszClassName = "vktri";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowExA(0, "vktri", "vktri", WS_OVERLAPPEDWINDOW, 50, 50, 400, 300, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, SW_SHOW);

    VkWin32SurfaceCreateInfoKHR sinfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, NULL, 0, hInst, hwnd};
    VkSurfaceKHR surf;
    r = cws(inst, &sinfo, NULL, &surf); CK(r, "vkCreateWin32SurfaceKHR");

    PFN_vkEnumeratePhysicalDevices epd = (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
    U32 n = 0; r = epd(inst, &n, NULL); CK(r, "enum count");
    VkPhysicalDevice pd[4]; r = epd(inst, &n, pd); CK(r, "enum");
    printf("[ok] physdevs=%u\n", n);
    VkPhysicalDevice gpu = pd[0];
    PFN_vkGetPhysicalDeviceProperties gpp = (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
    VkPhysicalDeviceProperties props; gpp(gpu, &props);
    printf("[ok] GPU: %s\n", props.deviceName);

    PFN_vkGetPhysicalDeviceQueueFamilyProperties gq = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)gipa(inst, "vkGetPhysicalDeviceQueueFamilyProperties");
    U32 qn = 0; gq(gpu, &qn, NULL);
    VkQueueFamilyProperties qp[8]; gq(gpu, &qn, qp);
    int gfx = -1;
    for (U32 i = 0; i < qn; i++) if (qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { gfx = (int)i; break; }
    printf("[ok] gfx queue family=%d of %u\n", gfx, qn);

    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR wps = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)gipa(inst, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
    printf("[ok] win32 present support=%d\n", (int)wps(gpu, (U32)gfx));

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL, 0, (U32)gfx, 1, &prio};
    const char *de[] = {"VK_KHR_swapchain"};
    VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, NULL, 0, 1, &qci, 1, de, 0, NULL};
    PFN_vkCreateDevice cdev = (PFN_vkCreateDevice)gipa(inst, "vkCreateDevice");
    VkDevice dev;
    r = cdev(gpu, &dci, NULL, &dev); CK(r, "vkCreateDevice");

    PFN_vkCreateSwapchainKHR csw = (PFN_vkCreateSwapchainKHR)gipa(inst, "vkCreateSwapchainKHR");
    VkSwapchainCreateInfoKHR swci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swci.surface = surf; swci.minImageCount = 3;
    swci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swci.imageExtent = {400, 300};
    swci.imageArrayLayers = 1; swci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swci.clipped = VK_TRUE;
    VkSwapchainKHR sw;
    r = csw(dev, &swci, NULL, &sw); CK(r, "vkCreateSwapchainKHR");
    printf("VKTRI_SWAPCHAIN_OK\n");

    PFN_vkGetSwapchainImagesKHR gsi = (PFN_vkGetSwapchainImagesKHR)gipa(inst, "vkGetSwapchainImagesKHR");
    VkImage imgs[8]; U32 in = 8;
    r = gsi(dev, sw, &in, imgs); CK(r, "vkGetSwapchainImagesKHR");
    printf("VKTRI_IMAGES=%u\n", in);

    PFN_vkAcquireNextImageKHR acq = (PFN_vkAcquireNextImageKHR)gipa(inst, "vkAcquireNextImageKHR");
    PFN_vkQueuePresentKHR presf = (PFN_vkQueuePresentKHR)gipa(inst, "vkQueuePresentKHR");
    PFN_vkGetDeviceQueue gdq = (PFN_vkGetDeviceQueue)gipa(inst, "vkGetDeviceQueue");
    VkQueue q; gdq(dev, (U32)gfx, 0, &q);
    for (int i = 0; i < 10; i++) {
        U32 idx; r = acq(dev, sw, 1000000000ULL, VK_NULL_HANDLE, VK_NULL_HANDLE, &idx);
        if (r) { printf("acq fail rc=%d\n", r); break; }
        VkPresentInfoKHR pi = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, NULL, 0, NULL, 1, &sw, &idx, NULL};
        r = presf(q, &pi);
        if (r) { printf("present fail rc=%d\n", r); break; }
        if (i == 9) printf("VKTRI_ALL_FRAMES_OK\n");
    }
    return 0;
}
