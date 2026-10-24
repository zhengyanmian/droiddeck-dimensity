#!/usr/bin/env python3
src = open("/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/loop-probe.c").read()

old_start = src.index("    /* --- allocate AHardwareBuffer (gralloc) --- */")
old_end = src.index("    /* --- rest mirrors tri-probe")
new_block = """    /* --- plain-allocated image (regular MEM_ALLOC path, NO import) --- */
    PFN_vkCreateImage pCreateImage = GD(CreateImage);
    VkImageCreateInfo ici2 = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { W, H, 1 }, .mipLevels = 1, .arrayLayers = 1,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .samples = VK_SAMPLE_COUNT_1_BIT };
    VkImage img;
    if ((r = pCreateImage(dev, &ici2, NULL, &img)) != VK_SUCCESS) die("vkCreateImage", r);

    PFN_vkGetImageMemoryRequirements pGetImgReq = GD(GetImageMemoryRequirements);
    VkMemoryRequirements req; pGetImgReq(dev, img, &req);
    printf("[ok] image %ux%u OPTIMAL req size=%llu memTypeBits=0x%x\\n",
           W, H, (unsigned long long)req.size, req.memoryTypeBits);

    PFN_vkGetPhysicalDeviceMemoryProperties pGetMemProps =
        (PFN_vkGetPhysicalDeviceMemoryProperties)gipa(inst, "vkGetPhysicalDeviceMemoryProperties");
    VkPhysicalDeviceMemoryProperties mp; pGetMemProps(pd, &mp);
    int mem_idx = -1;
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((req.memoryTypeBits & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) { mem_idx = i; break; }
    if (mem_idx < 0) for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if (req.memoryTypeBits & (1u << i)) { mem_idx = i; break; }
    if (mem_idx < 0) { printf("NO_MEMTYPE\\n"); return 4; }

    PFN_vkAllocateMemory pAllocMem = GD(AllocateMemory);
    PFN_vkBindImageMemory2 pBindImg2 = GD(BindImageMemory2);
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                 .allocationSize = req.size,
                                 .memoryTypeIndex = (uint32_t)mem_idx };
    VkDeviceMemory img_mem;
    if ((r = pAllocMem(dev, &mai, NULL, &img_mem)) != VK_SUCCESS) die("AllocMem(plain)", r);
    VkBindImageMemoryInfo bimi = { .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                                   .image = img, .memory = img_mem, .memoryOffset = 0 };
    if ((r = pBindImg2(dev, 1, &bimi)) != VK_SUCCESS) die("BindImageMemory2", r);
    printf("[ok] plain image bound (memType %d)\\n", mem_idx);

"""
src = src[:old_start] + new_block + src[old_end:]
src = src.replace("AHardwareBuffer_", "NOOP_AHardwareBuffer_")
src = src.replace("#include <android/hardware_buffer.h>", "")
src = src.replace("#include <android/log.h>", "")
out = "/mnt/c/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/alloc-probe.c"
open(out, "w").write(src)
print("written", len(src))
