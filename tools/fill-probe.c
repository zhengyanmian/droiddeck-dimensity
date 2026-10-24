/* fill-probe.c — minimal GPU execution test: CmdFillBuffer 0xAA -> read back.
 * If fill data appears, the CSF submit path works; if not, GPU commands never run. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

static PFN_vkGetInstanceProcAddr gipa;
static PFN_vkGetDeviceProcAddr dpa;
#define GI(name) ((PFN_vk##name)gipa(inst, "vk" #name))
#define GD(name) ((PFN_vk##name)dpa(dev, "vk" #name))

static void die(const char *msg, VkResult r) { fprintf(stderr, "FATAL: %s (r=%d)\n", msg, r); exit(1); }

static uint32_t find_mem(const VkPhysicalDeviceMemoryProperties *mp, uint32_t bits, VkMemoryPropertyFlags want)
{
    for (int i = 0; i < 32; i++)
        if ((bits & (1u << i)) && (mp->memoryTypes[i].propertyFlags & want) == want) return i;
    fprintf(stderr, "FATAL: no mem type\n"); exit(1);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    void *lib = dlopen(argc > 1 ? argv[1] : "./libvulkan_panfrost.so", RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }

    int v = 5;
    ((int (*)(int *))dlsym(lib, "vk_icdNegotiateLoaderICDInterfaceVersion"))(&v);
    gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vk_icdGetInstanceProcAddr");

    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app };
    VkInstance inst;
    PFN_vkCreateInstance pCI = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    if (pCI(&ici, NULL, &inst) != VK_SUCCESS) die("CreateInstance", VK_ERROR_INITIALIZATION_FAILED);
    printf("[ok] instance\n");

    PFN_vkEnumeratePhysicalDevices pEPD = GI(EnumeratePhysicalDevices);
    PFN_vkGetPhysicalDeviceMemoryProperties pGM = GI(GetPhysicalDeviceMemoryProperties);
    PFN_vkGetPhysicalDeviceQueueFamilyProperties pGQ = GI(GetPhysicalDeviceQueueFamilyProperties);
    uint32_t n = 1; VkPhysicalDevice pd;
    pEPD(inst, &n, &pd);

    uint32_t qn = 0; pGQ(pd, &qn, NULL);
    VkQueueFamilyProperties *qf = calloc(qn, sizeof *qf); pGQ(pd, &qn, qf);
    int qfam = -1;
    for (uint32_t i = 0; i < qn; i++)
        if (qf[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) { qfam = i; break; }
    printf("[ok] queue family %d\n", qfam);

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = qfam, .queueCount = 1, .pQueuePriorities = &prio };
    VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci };
    VkDevice dev;
    PFN_vkCreateDevice pCD = (PFN_vkCreateDevice)gipa(inst, "vkCreateDevice");
    if (pCD(pd, &dci, NULL, &dev) != VK_SUCCESS) die("CreateDevice", VK_ERROR_INITIALIZATION_FAILED);
    printf("[ok] device\n");
    dpa = (PFN_vkGetDeviceProcAddr)gipa(inst, "vkGetDeviceProcAddr");

    PFN_vkGetDeviceQueue pGQ2 = GD(GetDeviceQueue);
    PFN_vkCreateCommandPool pPool = GD(CreateCommandPool);
    PFN_vkAllocateCommandBuffers pACB = GD(AllocateCommandBuffers);
    PFN_vkCreateBuffer pCB = GD(CreateBuffer);
    PFN_vkGetBufferMemoryRequirements pGBR = GD(GetBufferMemoryRequirements);
    PFN_vkAllocateMemory pAM = GD(AllocateMemory);
    PFN_vkBindBufferMemory pBBM = GD(BindBufferMemory);
    PFN_vkBeginCommandBuffer pBCB = GD(BeginCommandBuffer);
    PFN_vkCmdFillBuffer pFill = GD(CmdFillBuffer);
    PFN_vkEndCommandBuffer pECB = GD(EndCommandBuffer);
    PFN_vkQueueSubmit pSub = GD(QueueSubmit);
    PFN_vkQueueWaitIdle pWait = GD(QueueWaitIdle);
    PFN_vkMapMemory pMap = GD(MapMemory);

    VkQueue queue; pGQ2(dev, qfam, 0, &queue);
    VkCommandPoolCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qfam };
    VkCommandPool pool; pPool(dev, &pci, NULL, &pool);
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                        .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    VkCommandBuffer cmd; pACB(dev, &cai, &cmd);

    VkPhysicalDeviceMemoryProperties mp; pGM(pd, &mp);

    VkBufferCreateInfo bci = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                               .size = 4096, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT };
    VkBuffer buf; pCB(dev, &bci, NULL, &buf);
    VkMemoryRequirements req; pGBR(dev, buf, &req);
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = req.size,
        .memoryTypeIndex = find_mem(&mp, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
    VkDeviceMemory mem;
    VkResult r = pAM(dev, &mai, NULL, &mem);
    if (r != VK_SUCCESS) { mai.memoryTypeIndex = find_mem(&mp, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); r = pAM(dev, &mai, NULL, &mem); }
    if (r != VK_SUCCESS) die("alloc", r);
    pBBM(dev, buf, mem, 0);
    printf("[ok] buffer, memType %u flags 0x%x\n", mai.memoryTypeIndex, mp.memoryTypes[mai.memoryTypeIndex].propertyFlags);

    /* pre-fill CPU side with 0 to prove GPU wrote it */
    void *data; pMap(dev, mem, 0, VK_WHOLE_SIZE, 0, &data);
    memset(data, 0, 4096);

    VkCommandBufferBeginInfo bbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    pBCB(cmd, &bbi);
    pFill(cmd, buf, 0, VK_WHOLE_SIZE, 0xAAAAAAAA);
    pECB(cmd);
    VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    if ((r = pSub(queue, 1, &si, VK_NULL_HANDLE)) != VK_SUCCESS) die("submit", r);
    pWait(queue);
    printf("[ok] submitted\n");

    unsigned int *u = data;
    int ok = 1;
    for (int i = 0; i < 8; i++)
        printf("  word[%d] = 0x%08x %s\n", i, u[i], u[i] == 0xAAAAAAAAu ? "OK" : "BAD");
    for (int i = 0; i < 1024; i++) if (u[i] != 0xAAAAAAAAu) { ok = 0; break; }
    printf("%s\n", ok ? "FILL_SUCCESS: GPU executes commands" : "FILL_FAIL: GPU submission is a no-op");
    return ok ? 0 : 2;
}
