import re
p = "/root/build/panvk-kbase-android/work/mesa/src/vulkan/wsi/wsi_common_x11.c"
src = open(p).read()

# 1. mark all fail-path gotos
for lbl in ["fail_image", "fail_pixmap", "fail_shmfence_alloc", "fail_fence_fd", "fail_alloc", "fail_oom"]:
    src = src.replace(f"goto {lbl};", f'{{ fprintf(stderr, "X11WSI: goto {lbl}\\n"); goto {lbl}; }}')

# 2. entry print of x11_surface_create_swapchain
src = src.replace(
    "assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);",
    'fprintf(stderr, "X11WSI: create_swapchain entry\\n");\n   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);', 1)

# 3. print present mode decision + dri3 checks
src = src.replace(
    "if (!x11_surface_is_thread_safe(icd_surface))\n      return VK_ERROR_UNKNOWN;",
    'if (!x11_surface_is_thread_safe(icd_surface)) { fprintf(stderr, "X11WSI: not threadsafe\\n"); return VK_ERROR_UNKNOWN; }')

open(p, "w").write(src)
print("x11 wsi instrumented")

# 4. wsi_common.c: print create_swapchain result
p2 = "/root/build/panvk-kbase-android/work/mesa/src/vulkan/wsi/wsi_common.c"
src2 = open(p2).read()
src2 = src2.replace(
    """   VkResult result = iface->create_swapchain(surface, _device, wsi_device,
                                             &info, alloc,
                                             &swapchain);
   if (result != VK_SUCCESS)
      return result;""",
    """   VkResult result = iface->create_swapchain(surface, _device, wsi_device,
                                             &info, alloc,
                                             &swapchain);
   fprintf(stderr, "WSICOMMON: iface->create_swapchain result=%d\\n", (int)result);
   if (result != VK_SUCCESS)
      return result;""")
open(p2, "w").write(src2)
print("wsi_common instrumented")
