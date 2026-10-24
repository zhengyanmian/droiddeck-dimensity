/* panvk_host.c — DroidDeck Phase 4 milestone H: host APK that puts a PanVK frame
 * on the actual screen.
 *
 * Architecture (no Vulkan loader involved):
 *   [PanVK (Mesa, driver-direct)] --renders--> AHardwareBuffer (gralloc)
 *   [Vendor GL (EGL)] --samples AHB via EGLImage--> SurfaceView -> SurfaceFlinger
 *
 * Reuses the proven ahb-probe driver-direct pattern:
 *   dlopen(libvulkan_panfrost.so) -> vk_icdGetInstanceProcAddr -> everything.
 */
#define EGL_EGLEXT_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/system_properties.h>

#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/hardware_buffer.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include "host_spv.h" /* host_vert_spv[], host_vert_spv_len, host_frag_spv[], host_frag_spv_len */

#define LOG_TAG "panvk_host"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define DRIVER_SONAME "libvulkan_panfrost.so"

/* ------------------------------------------------------------------ */
/* Vulkan state (globals, mirrors ahb-probe style)                     */
/* ------------------------------------------------------------------ */
static struct android_app *g_app;

static void *drv_lib;
static PFN_vkGetInstanceProcAddr gipa;
static PFN_vkGetDeviceProcAddr dpa;
static VkInstance inst;
static VkDevice dev;
static VkQueue queue;

#define GI(name) ((PFN_vk##name)gipa(inst, "vk" #name))
#define GD(name) ((PFN_vk##name)dpa(dev, "vk" #name))

static VkRenderPass rp;
static VkPipelineLayout pl;
static VkPipeline gpipe;
static VkCommandPool cmdpool;
static VkCommandBuffer cmd;
static VkFence fence;

static int vk_ready;
static int vk_fatal;
static int vk_dead;   /* DEVICE_LOST: stop VK submits, keep GL presenting */

/* CSF-fatal experiments (configurable via adb shell setprop debug.panvk.*):
 *   debug.panvk.swap : GL swap every N VK frames; 0 = GL swap only on first frame
 *   debug.panvk.ms   : target frame interval in ms (default 16, pacing keeps VK rate fixed)
 * Counters are printed when DEVICE_LOST happens so we can correlate. */
static int cfg_swap_every = 1;
static int cfg_frame_ms = 16;
static int cfg_glfinish;   /* glFinish() after swap: serialize vendor GL vs panvk */
static long vk_frame, gl_swap;

/* ------------------------------------------------------------------ */
/* Per-window render target                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    ANativeWindow *win;
    int w, h;
    EGLSurface surf;
} Target;

/* One renderable/presentable buffer: AHB imported into VK (write) and GL (read).
 * Two planes are alternated so PanVK writes plane[cur] while the compositor
 * still consumes plane[cur^1] — no in-place read/write overlap (tearing). */
typedef struct {
    AHardwareBuffer *ahb;
    VkImage img;
    VkDeviceMemory mem;
    VkImageView view;
    VkFramebuffer fb;
    EGLImageKHR eimg;
    GLuint tex;
} Plane;

static Plane planes[2];
static int cur_plane;

static Target tgt;
static int tgt_valid;

static EGLDisplay dpy;
static EGLContext ctx;
static EGLConfig econfig;
static int egl_ready;
static GLuint glprog;
static GLint uTexLoc;
static int selftest_done;

/* pending window changes handed over from the event callback */
static ANativeWindow *g_newwin;
static int g_release_pending;

/* ------------------------------------------------------------------ */
/* GL compositor shaders (full-screen triangle sampling the AHB)       */
/* ------------------------------------------------------------------ */
static const char *comp_vs_src =
    "#version 300 es\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "  vec2 p[3];\n"
    "  p[0] = vec2(-1.0, -1.0);\n"
    "  p[1] = vec2( 3.0, -1.0);\n"
    "  p[2] = vec2(-1.0,  3.0);\n"
    "  vec2 pos = p[gl_VertexID];\n"
    "  vUV = pos * 0.5 + 0.5;\n"
    "  vUV.y = 1.0 - vUV.y;\n"
    "  gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n";

static const char *comp_fs_src =
    "#version 300 es\n"
    "#extension GL_OES_EGL_image_external_essl3 : require\n"
    "precision mediump float;\n"
    "in vec2 vUV;\n"
    "out vec4 o;\n"
    "uniform samplerExternalOES uTex;\n"
    "void main() { o = texture(uTex, vUV); }\n";

/* ------------------------------------------------------------------ */

static void die_fatal(const char *what)
{
    LOGE("FATAL: %s", what);
    vk_fatal = 1;
    if (g_app && g_app->activity)
        ANativeActivity_finish(g_app->activity);
}

static int have_ext(const VkExtensionProperties *e, uint32_t n, const char *name)
{
    for (uint32_t i = 0; i < n; i++)
        if (!strcmp(e[i].extensionName, name)) return 1;
    return 0;
}

/* ---- Vulkan one-time init (device + pipeline, no window needed) ---- */
static int vk_init(void)
{
    LOGI("vk_init: dlopen %s", DRIVER_SONAME);
    drv_lib = dlopen(DRIVER_SONAME, RTLD_NOW | RTLD_LOCAL);
    if (!drv_lib) {
        /* fallback: next to our own lib (nativeLibraryDir) */
        Dl_info info;
        char path[512];
        if (dladdr((void *)android_main, &info) && info.dli_fname) {
            char *slash = strrchr(info.dli_fname, '/');
            if (slash) {
                snprintf(path, sizeof path, "%.*s/%s", (int)(slash - info.dli_fname),
                         info.dli_fname, DRIVER_SONAME);
                LOGI("dlopen fallback: %s", path);
                drv_lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
            }
        }
    }
    if (!drv_lib) { LOGE("dlopen driver: %s", dlerror()); die_fatal("dlopen panvk driver"); return -1; }
    LOGI("[ok] driver dlopen");

    int v = 5;
    ((int (*)(int *))dlsym(drv_lib, "vk_icdNegotiateLoaderICDInterfaceVersion"))(&v);
    gipa = (PFN_vkGetInstanceProcAddr)dlsym(drv_lib, "vk_icdGetInstanceProcAddr");
    if (!gipa) { die_fatal("no vk_icdGetInstanceProcAddr"); return -1; }

    const char *iexts[] = {
        "VK_KHR_external_memory_capabilities",
        "VK_ANDROID_external_memory_android_hardware_buffer",
    };
    /* enable only the requested instance extensions the ICD actually
     * advertises (panvk has no AHB instance extension; AHB is device-level) */
    const char *enabled[2];
    uint32_t n_enabled = 0;
    PFN_vkEnumerateInstanceExtensionProperties pEnumIExt =
        (PFN_vkEnumerateInstanceExtensionProperties)gipa(NULL, "vkEnumerateInstanceExtensionProperties");
    if (pEnumIExt) {
        uint32_t ie = 0;
        pEnumIExt(NULL, &ie, NULL);
        VkExtensionProperties *iext_list = calloc(ie ? ie : 1, sizeof *iext_list);
        if (ie) pEnumIExt(NULL, &ie, iext_list);
        for (uint32_t i = 0; i < 2; i++)
            if (have_ext(iext_list, ie, iexts[i])) enabled[n_enabled++] = iexts[i];
        free(iext_list);
    }
    for (uint32_t i = 0; i < n_enabled; i++)
        LOGI("[ext] enabling instance ext: %s", enabled[i]);

    VkApplicationInfo appi = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               .pApplicationName = "panvk-host",
                               .apiVersion = VK_API_VERSION_1_3 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                 .pApplicationInfo = &appi,
                                 .enabledExtensionCount = n_enabled, .ppEnabledExtensionNames = enabled };
    VkResult r;
    PFN_vkCreateInstance pCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    if (!pCreateInstance) { die_fatal("no vkCreateInstance"); return -1; }
    if ((r = pCreateInstance(&ici, NULL, &inst)) != VK_SUCCESS) { LOGE("CreateInstance r=%d", r); die_fatal("vkCreateInstance"); return -1; }
    LOGI("[ok] vkCreateInstance (driver-direct, no loader)");

    PFN_vkEnumeratePhysicalDevices pEnumPD = GI(EnumeratePhysicalDevices);
    PFN_vkGetPhysicalDeviceProperties pGetProps = GI(GetPhysicalDeviceProperties);
    PFN_vkGetPhysicalDeviceQueueFamilyProperties pGetQF = GI(GetPhysicalDeviceQueueFamilyProperties);
    PFN_vkEnumerateDeviceExtensionProperties pEnumDExt = GI(EnumerateDeviceExtensionProperties);
    PFN_vkCreateDevice pCreateDevice = (PFN_vkCreateDevice)gipa(inst, "vkCreateDevice");
    if (!pEnumPD || !pEnumDExt || !pCreateDevice) { die_fatal("instance PFNs"); return -1; }

    uint32_t n = 0;
    if ((r = pEnumPD(inst, &n, NULL)) != VK_SUCCESS || n == 0) { die_fatal("enumerate PD"); return -1; }
    VkPhysicalDevice pd; n = 1; pEnumPD(inst, &n, &pd);
    VkPhysicalDeviceProperties props; pGetProps(pd, &props);
    LOGI("[ok] PD: %s api %u.%u.%u", props.deviceName,
         props.apiVersion >> 22, (props.apiVersion >> 12) & 0x3ff, props.apiVersion & 0xfff);

    uint32_t de = 0; pEnumDExt(pd, NULL, &de, NULL);
    VkExtensionProperties *dext = calloc(de ? de : 1, sizeof *dext);
    pEnumDExt(pd, NULL, &de, dext);
    int has_ahb = have_ext(dext, de, "VK_ANDROID_external_memory_android_hardware_buffer");
    int has_extmem = have_ext(dext, de, "VK_KHR_external_memory");
    LOGI("[ext] AHB=%d extmem=%d (%u device extensions)", has_ahb, has_extmem, de);
    free(dext);
    if (!has_ahb || !has_extmem) { die_fatal("AHB extension missing"); return -1; }

    uint32_t qfn = 0; pGetQF(pd, &qfn, NULL);
    VkQueueFamilyProperties *qf = calloc(qfn, sizeof *qf);
    pGetQF(pd, &qfn, qf);
    int gfx = -1;
    for (uint32_t i = 0; i < qfn; i++)
        if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { gfx = (int)i; break; }
    free(qf);
    if (gfx < 0) { die_fatal("no graphics queue"); return -1; }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = (uint32_t)gfx, .queueCount = 1, .pQueuePriorities = &prio };
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
    if ((r = pCreateDevice(pd, &dci, NULL, &dev)) != VK_SUCCESS) { LOGE("CreateDevice r=%d", r); die_fatal("vkCreateDevice"); return -1; }
    LOGI("[ok] vkCreateDevice");

    dpa = (PFN_vkGetDeviceProcAddr)gipa(inst, "vkGetDeviceProcAddr");

    VkAttachmentDescription att = { .format = VK_FORMAT_R8G8B8A8_UNORM, .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference aref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 .colorAttachmentCount = 1, .pColorAttachments = &aref };
    VkRenderPassCreateInfo rpci = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                    .attachmentCount = 1, .pAttachments = &att, .subpassCount = 1, .pSubpasses = &sub };
    if ((r = GD(CreateRenderPass)(dev, &rpci, NULL, &rp)) != VK_SUCCESS) { LOGE("RP r=%d", r); die_fatal("renderpass"); return -1; }

    VkPushConstantRange pcr = { VK_SHADER_STAGE_VERTEX_BIT, 0, 16 };
    VkPipelineLayoutCreateInfo plci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                        .pushConstantRangeCount = 1, .pPushConstantRanges = &pcr };
    if ((r = GD(CreatePipelineLayout)(dev, &plci, NULL, &pl)) != VK_SUCCESS) { die_fatal("pipeline layout"); return -1; }

    VkShaderModuleCreateInfo smci_v = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = host_vert_spv_len, .pCode = (const uint32_t *)host_vert_spv };
    VkShaderModuleCreateInfo smci_f = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = host_frag_spv_len, .pCode = (const uint32_t *)host_frag_spv };
    VkShaderModule vs, fs;
    if ((r = GD(CreateShaderModule)(dev, &smci_v, NULL, &vs)) != VK_SUCCESS) { die_fatal("vert shader"); return -1; }
    if ((r = GD(CreateShaderModule)(dev, &smci_f, NULL, &fs)) != VK_SUCCESS) { die_fatal("frag shader"); return -1; }

    VkPipelineShaderStageCreateInfo stages[2] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main" },
    };
    VkPipelineVertexInputStateCreateInfo vi = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo ia = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                                  .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    VkPipelineViewportStateCreateInfo vps = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                              .viewportCount = 1, .scissorCount = 1 };
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
    if ((r = GD(CreateGraphicsPipelines)(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &gpipe)) != VK_SUCCESS) { LOGE("pipe r=%d", r); die_fatal("pipeline"); return -1; }
    GD(DestroyShaderModule)(dev, vs, NULL);
    GD(DestroyShaderModule)(dev, fs, NULL);

    VkCommandPoolCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = (uint32_t)gfx };
    GD(CreateCommandPool)(dev, &pci, NULL, &cmdpool);
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                        .commandPool = cmdpool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    GD(AllocateCommandBuffers)(dev, &cai, &cmd);
    VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    GD(CreateFence)(dev, &fci, NULL, &fence);
    GD(GetDeviceQueue)(dev, (uint32_t)gfx, 0, &queue);

    vk_ready = 1;
    LOGI("[ok] vulkan init complete (queue family %d)", gfx);
    return 0;
}

/* ---- EGL one-time init ---- */
static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        LOGE("shader compile: %s", log);
        return 0;
    }
    return s;
}

static int egl_init(ANativeWindow *win)
{
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(dpy, NULL, NULL)) { die_fatal("eglInitialize"); return -1; }

    const EGLint cfgattr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_NONE };
    EGLint nc = 0;
    if (!eglChooseConfig(dpy, cfgattr, &econfig, 1, &nc) || nc < 1) { die_fatal("eglChooseConfig"); return -1; }

    const EGLint ctxattr[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    ctx = eglCreateContext(dpy, econfig, EGL_NO_CONTEXT, ctxattr);
    if (!ctx) { die_fatal("eglCreateContext"); return -1; }

    /* warm the GL program up on a throwaway current state (surface comes later) */
    EGLSurface tmp = eglCreateWindowSurface(dpy, econfig, (EGLNativeWindowType)win, NULL);
    if (!tmp) { die_fatal("eglCreateWindowSurface"); return -1; }
    eglMakeCurrent(dpy, tmp, tmp, ctx);

    GLuint vs = compile_shader(GL_VERTEX_SHADER, comp_vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, comp_fs_src);
    glprog = glCreateProgram();
    glAttachShader(glprog, vs); glAttachShader(glprog, fs);
    glLinkProgram(glprog);
    GLint ok = 0;
    glGetProgramiv(glprog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetProgramInfoLog(glprog, sizeof log, NULL, log);
        LOGE("program link: %s", log);
        die_fatal("program link");
        return -1;
    }
    glDeleteShader(vs); glDeleteShader(fs);
    uTexLoc = glGetUniformLocation(glprog, "uTex");

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(dpy, tmp);

    egl_ready = 1;
    LOGI("[ok] EGL init (client ext %s)", eglQueryString(dpy, EGL_EXTENSIONS) ? "" : "");
    return 0;
}

/* ---- per-window target ---- */
static void destroy_target(void)
{
    if (!tgt_valid) return;
    GD(DeviceWaitIdle)(dev);
    /* GL object deletion needs a current context: make the window surface
     * current (if we still have one) before tearing down EGL objects. */
    if (egl_ready && tgt.surf) eglMakeCurrent(dpy, tgt.surf, tgt.surf, ctx);
    for (int i = 0; i < 2; i++) {
        Plane *p = &planes[i];
        if (p->fb) GD(DestroyFramebuffer)(dev, p->fb, NULL);
        if (p->view) GD(DestroyImageView)(dev, p->view, NULL);
        if (p->img) GD(DestroyImage)(dev, p->img, NULL);
        if (p->mem) GD(FreeMemory)(dev, p->mem, NULL);
        if (p->tex) glDeleteTextures(1, &p->tex);
        if (p->eimg && egl_ready) eglDestroyImageKHR(dpy, p->eimg);
        if (p->ahb) AHardwareBuffer_release(p->ahb);
    }
    if (tgt.surf && egl_ready) eglDestroySurface(dpy, tgt.surf);
    if (tgt.win) ANativeWindow_release(tgt.win);
    memset(planes, 0, sizeof planes);
    memset(&tgt, 0, sizeof tgt);
    tgt_valid = 0;
    LOGI("target destroyed");
}

/* create one AHB-backed plane (VK color attachment + GL external texture) */
static int create_plane(int i, uint32_t mem_idx,
                        PFN_vkGetAndroidHardwareBufferPropertiesANDROID pGetAHBProps)
{
    Plane *p = &planes[i];
    memset(p, 0, sizeof *p);

    AHardwareBuffer_Desc adesc = {
        .width = (uint64_t)tgt.w, .height = (uint64_t)tgt.h, .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
        .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE
               | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
    };
    if (AHardwareBuffer_allocate(&adesc, &p->ahb) != 0 || !p->ahb) { die_fatal("AHB allocate"); return -1; }

    VkAndroidHardwareBufferPropertiesANDROID ahbprops = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID };
    VkResult r;
    if ((r = pGetAHBProps(dev, p->ahb, &ahbprops)) != VK_SUCCESS) { LOGE("GetAHBProps r=%d", r); die_fatal("GetAHBProps"); return -1; }
    LOGI("[ok] plane %d AHB %dx%d size=%llu memType=%u", i, tgt.w, tgt.h,
         (unsigned long long)ahbprops.allocationSize, mem_idx);

    VkExternalMemoryImageCreateInfo extimg = {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID };
    VkImageCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &extimg,
        .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { (uint32_t)tgt.w, (uint32_t)tgt.h, 1 }, .mipLevels = 1, .arrayLayers = 1,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .samples = VK_SAMPLE_COUNT_1_BIT };
    if ((r = GD(CreateImage)(dev, &ici, NULL, &p->img)) != VK_SUCCESS) { LOGE("CreateImage r=%d", r); die_fatal("CreateImage(AHB)"); return -1; }

    VkImportAndroidHardwareBufferInfoANDROID imp = {
        .sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID, .buffer = p->ahb };
    VkMemoryDedicatedAllocateInfo ded = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .pNext = &imp, .image = p->img };
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                 .pNext = &ded,
                                 .allocationSize = ahbprops.allocationSize,
                                 .memoryTypeIndex = mem_idx };
    if ((r = GD(AllocateMemory)(dev, &mai, NULL, &p->mem)) != VK_SUCCESS) { LOGE("AllocMem r=%d", r); die_fatal("AllocMem(import AHB)"); return -1; }
    VkBindImageMemoryInfo bimi = { .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                                   .image = p->img, .memory = p->mem, .memoryOffset = 0 };
    if ((r = GD(BindImageMemory2)(dev, 1, &bimi)) != VK_SUCCESS) { die_fatal("BindImageMemory2"); return -1; }

    VkImageViewCreateInfo vci = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = p->img, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    GD(CreateImageView)(dev, &vci, NULL, &p->view);
    VkFramebufferCreateInfo fbci = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = rp, .attachmentCount = 1, .pAttachments = &p->view,
        .width = (uint32_t)tgt.w, .height = (uint32_t)tgt.h, .layers = 1 };
    GD(CreateFramebuffer)(dev, &fbci, NULL, &p->fb);

    EGLClientBuffer nwb = eglGetNativeClientBufferANDROID(p->ahb);
    if (!nwb) { LOGE("eglGetNativeClientBufferANDROID 0x%x", eglGetError()); die_fatal("client buffer"); return -1; }
    p->eimg = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, nwb, NULL);
    if (!p->eimg) { LOGE("eglCreateImageKHR 0x%x", eglGetError()); die_fatal("EGLImage"); return -1; }

    glGenTextures(1, &p->tex);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, p->tex);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, p->eimg);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    return 0;
}

static int create_target(ANativeWindow *win)
{
    if (!egl_ready && egl_init(win) != 0) return -1;

    memset(&tgt, 0, sizeof tgt);
    memset(planes, 0, sizeof planes);
    tgt.win = win;
    ANativeWindow_acquire(win);
    tgt.w = ANativeWindow_getWidth(win);
    tgt.h = ANativeWindow_getHeight(win);
    LOGI("create_target %dx%d", tgt.w, tgt.h);

    PFN_vkGetAndroidHardwareBufferPropertiesANDROID pGetAHBProps =
        (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)gipa(inst, "vkGetAndroidHardwareBufferPropertiesANDROID");

    /* EGL window surface (single, shared by both planes) */
    tgt.surf = eglCreateWindowSurface(dpy, econfig, (EGLNativeWindowType)win, NULL);
    if (!tgt.surf) { LOGE("eglCreateWindowSurface 0x%x", eglGetError()); die_fatal("surface"); return -1; }
    eglMakeCurrent(dpy, tgt.surf, tgt.surf, ctx);
    eglSwapInterval(dpy, 1);   /* vsync-paced queueing */

    /* probe memory type with a properties query on a temp AHB of plane 0 size */
    {
        AHardwareBuffer_Desc probe_desc = {
            .width = (uint64_t)tgt.w, .height = (uint64_t)tgt.h, .layers = 1,
            .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
        };
        AHardwareBuffer *probe = NULL;
        if (AHardwareBuffer_allocate(&probe_desc, &probe) != 0 || !probe) { die_fatal("AHB probe"); return -1; }
        VkAndroidHardwareBufferPropertiesANDROID ahbprops = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID };
        if (pGetAHBProps(dev, probe, &ahbprops) != VK_SUCCESS) { AHardwareBuffer_release(probe); die_fatal("GetAHBProps"); return -1; }
        uint32_t mem_idx = __builtin_ctz(ahbprops.memoryTypeBits);
        AHardwareBuffer_release(probe);
        for (int i = 0; i < 2; i++)
            if (create_plane(i, mem_idx, pGetAHBProps) != 0) return -1;
    }

    cur_plane = 0;
    tgt_valid = 1;
    LOGI("[ok] target created (2 planes, AHB imported into both VK and GL)");
    return 0;
}

/* ---- one frame: VK render into AHB, then GL blit to screen ---- */
static void render_frame(void)
{
    if (vk_fatal) return;
    if (!vk_ready && vk_init() != 0) return;

    if (g_release_pending) { destroy_target(); g_release_pending = 0; }
    if (g_newwin) {
        ANativeWindow *w = g_newwin; g_newwin = NULL;
        if (tgt_valid && tgt.win != w) destroy_target();
        if (!tgt_valid && create_target(w) != 0) { destroy_target(); return; }
    }
    if (!tgt_valid) return;

    /* handle window resize */
    int cw = ANativeWindow_getWidth(tgt.win), ch = ANativeWindow_getHeight(tgt.win);
    if (cw > 0 && ch > 0 && (cw != tgt.w || ch != tgt.h)) {
        LOGI("window resized %dx%d -> %dx%d", tgt.w, tgt.h, cw, ch);
        destroy_target();
        create_target(tgt.win);
        if (!tgt_valid) return;
    }

    if (vk_dead) {
        /* VK side is gone (DEVICE_LOST); keep compositing the last frame */
        goto present_only;
    }

    /* frame pacing: fixed VK rate regardless of GL swap frequency */
    {
        static struct timespec next;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (next.tv_sec == 0) next = now;
        if (now.tv_sec < next.tv_sec ||
            (now.tv_sec == next.tv_sec && now.tv_nsec < next.tv_nsec)) {
            long ns = (next.tv_sec - now.tv_sec) * 1000000000L + (next.tv_nsec - now.tv_nsec);
            if (ns > 0) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        }
        next.tv_nsec += cfg_frame_ms * 1000000L;
        while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
    }
    vk_frame++;
    if (vk_frame % 100 == 0) LOGI("heartbeat vk_frame=%ld gl_swap=%ld", vk_frame, gl_swap);

    static float angle;
    angle += 0.03f;
    if (angle > 6.2832f) angle -= 6.2832f;

    PFN_vkResetCommandBuffer pReset = GD(ResetCommandBuffer);
    PFN_vkBeginCommandBuffer pBegin = GD(BeginCommandBuffer);
    PFN_vkCmdBeginRenderPass pCmdBeginRP = GD(CmdBeginRenderPass);
    PFN_vkCmdBindPipeline pCmdBindPipe = GD(CmdBindPipeline);
    PFN_vkCmdSetViewport pCmdViewport = GD(CmdSetViewport);
    PFN_vkCmdSetScissor pCmdScissor = GD(CmdSetScissor);
    PFN_vkCmdPushConstants pCmdPush = GD(CmdPushConstants);
    PFN_vkCmdDraw pCmdDraw = GD(CmdDraw);
    PFN_vkCmdEndRenderPass pCmdEndRP = GD(CmdEndRenderPass);
    PFN_vkEndCommandBuffer pEndCmd = GD(EndCommandBuffer);
    PFN_vkQueueSubmit pSubmit = GD(QueueSubmit);
    PFN_vkWaitForFences pWaitF = GD(WaitForFences);
    PFN_vkResetFences pResetF = GD(ResetFences);

    pReset(cmd, 0);
    VkCommandBufferBeginInfo bbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    if (pBegin(cmd, &bbi) != VK_SUCCESS) return;
    VkClearValue clear = { .color = { { 1.0f, 0.0f, 0.0f, 1.0f } } };
    VkRenderPassBeginInfo rpbi = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = rp, .framebuffer = planes[cur_plane].fb, .renderArea = { { 0, 0 }, { (uint32_t)tgt.w, (uint32_t)tgt.h } },
        .clearValueCount = 1, .pClearValues = &clear };
    pCmdBeginRP(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    pCmdBindPipe(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gpipe);
    VkViewport vp = { 0, 0, (float)tgt.w, (float)tgt.h, 0, 1 };
    VkRect2D sc = { { 0, 0 }, { (uint32_t)tgt.w, (uint32_t)tgt.h } };
    pCmdViewport(cmd, 0, 1, &vp);
    pCmdScissor(cmd, 0, 1, &sc);
    pCmdPush(cmd, pl, VK_SHADER_STAGE_VERTEX_BIT, 0, 16, &angle);
    pCmdDraw(cmd, 3, 1, 0, 0);
    pCmdEndRP(cmd);
    pEndCmd(cmd);

    VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    VkResult r = pSubmit(queue, 1, &si, fence);
    if (r != VK_SUCCESS) {
        LOGE("submit r=%d (%s) at vk_frame=%ld gl_swap=%ld", r,
             r == VK_ERROR_DEVICE_LOST ? "DEVICE_LOST" : "error", vk_frame, gl_swap);
        if (r == VK_ERROR_DEVICE_LOST || r == VK_ERROR_INITIALIZATION_FAILED) {
            vk_dead = 1;
            LOGE("disabling further VK submits; keeping last frame on screen");
        }
        goto present_only;
    }
    pWaitF(dev, 1, &fence, VK_TRUE, 5000000000ULL);
    pResetF(dev, 1, &fence);

    /* periodic content scan: each row of a healthy frame has at most one
     * red->green transition (the triangle span). Multiple runs in a row mean
     * stale (previous-frame) content survived in that row — lost bins. */
    if (vk_frame % 60 == 0) {
        void *adata = NULL;
        if (AHardwareBuffer_lock(planes[cur_plane].ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &adata) == 0 && adata) {
            AHardwareBuffer_Desc rd; AHardwareBuffer_describe(planes[cur_plane].ahb, &rd);
            uint32_t stride = rd.stride ? rd.stride : (uint32_t)tgt.w;
            int bad_rows = 0, first_bad = -1, max_runs = 0;
            for (int y = 0; y < tgt.h; y++) {
                const uint32_t *row = (const uint32_t *)((const uint8_t *)adata + (size_t)y * stride * 4);
                int runs = 0, ing = 0;
                for (int x = 0; x < tgt.w; x++) {
                    uint32_t px = row[x];
                    int g = ((px >> 8) & 0xff) > 200 && (px & 0xff) < 50;
                    if (g && !ing) { runs++; ing = 1; }
                    else if (!g && ing) ing = 0;
                }
                if (runs > 1) { bad_rows++; if (first_bad < 0) first_bad = y; if (runs > max_runs) max_runs = runs; }
            }
            AHardwareBuffer_unlock(planes[cur_plane].ahb, NULL);
            LOGI("SCAN plane=%d frame=%ld bad_rows=%d first_bad=%d max_runs=%d",
                 cur_plane, vk_frame, bad_rows, first_bad, max_runs);
        }
    }

    /* first-frame CPU self-check of the AHB content (like ahb-probe) */
    if (!selftest_done) {
        selftest_done = 1;
        void *adata = NULL;
        if (AHardwareBuffer_lock(planes[cur_plane].ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &adata) == 0 && adata) {
            AHardwareBuffer_Desc rd; AHardwareBuffer_describe(planes[cur_plane].ahb, &rd);
            uint32_t stride = rd.stride ? rd.stride : (uint32_t)tgt.w;
            const unsigned char *p = (const unsigned char *)adata
                + (size_t)(tgt.h / 2) * stride * 4 + (size_t)(tgt.w / 2) * 4;
            LOGI("SELFTEST center px R=%u G=%u B=%u A=%u -> %s",
                 p[0], p[1], p[2], p[3],
                 (p[1] > 200 && p[0] < 50) ? "GREEN_OK" : "NOT_GREEN");
            AHardwareBuffer_unlock(planes[cur_plane].ahb, NULL);
        } else {
            LOGI("SELFTEST skipped (AHB lock failed)");
        }
    }

    /* GL compositor: draw the AHB texture over the whole window.
     * Experiment hook: cfg_swap_every == 0 -> present only on the very first
     * frame (zero GL submits afterwards); N > 1 -> present every N VK frames. */
present_only:
    if (!egl_ready || !tgt_valid) return;
    if (cfg_swap_every == 0) {
        if (gl_swap > 0) return;            /* already presented once */
    } else if (vk_frame % cfg_swap_every != 0) {
        return;                              /* skip this frame's swap */
    }
    eglMakeCurrent(dpy, tgt.surf, tgt.surf, ctx);
    glViewport(0, 0, tgt.w, tgt.h);
    glUseProgram(glprog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, planes[cur_plane].tex);
    /* Re-create the EGLImage every frame: the persistent EGLImage let the
     * vendor GPU sample stale cache lines after PanVK rewrote the AHB
     * (speckles exactly at moving edges). Per-frame create/destroy forces a
     * kbase map/unmap round trip (BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP). */
    EGLClientBuffer nwb = eglGetNativeClientBufferANDROID(planes[cur_plane].ahb);
    EGLImageKHR fresh = nwb ? eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, nwb, NULL) : NULL;
    if (fresh) glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, fresh);
    glUniform1i(uTexLoc, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    if (cfg_glfinish)
        glFinish();   /* finish BEFORE queueing: the compositor must never scan
                         out a buffer the GPU is still writing (edge trails) */
    if (fresh) eglDestroyImageKHR(dpy, fresh);
    if (!eglSwapBuffers(dpy, tgt.surf))
        LOGE("eglSwapBuffers 0x%x", eglGetError());
    else
        gl_swap++;
    cur_plane ^= 1;   /* next VK frame renders into the other plane */
}

/* ------------------------------------------------------------------ */
/* native_app_glue glue                                                */
/* ------------------------------------------------------------------ */
static void handle_cmd(struct android_app *app, int32_t cmd)
{
    (void)app;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (g_app->window) {
            ANativeWindow_acquire(g_app->window);
            if (g_newwin) ANativeWindow_release(g_newwin);
            g_newwin = g_app->window;
            g_release_pending = 0;
            LOGI("APP_CMD_INIT_WINDOW %dx%d",
                 ANativeWindow_getWidth(g_app->window), ANativeWindow_getHeight(g_app->window));
        }
        break;
    case APP_CMD_TERM_WINDOW:
        g_release_pending = 1;
        LOGI("APP_CMD_TERM_WINDOW");
        break;
    default:
        break;
    }
}

void android_main(struct android_app *app)
{
    g_app = app;
    app->onAppCmd = handle_cmd;

    /* The process may host several activity lifetimes (force-stop missed,
     * activity relaunch). Reset all one-shot state so a relaunch retries
     * instead of being short-circuited by a stale vk_fatal. */
    if (tgt_valid) destroy_target();
    if (g_newwin) { ANativeWindow_release(g_newwin); g_newwin = NULL; }
    g_release_pending = 0;
    selftest_done = 0;
    if (dev && dpa) {
        GD(DeviceWaitIdle)(dev);
        GD(DestroyDevice)(dev, NULL);
        dev = NULL;
    }
    if (inst && gipa) {
        PFN_vkDestroyInstance pDestroyInstance = (PFN_vkDestroyInstance)gipa(inst, "vkDestroyInstance");
        if (pDestroyInstance) pDestroyInstance(inst, NULL);
        inst = NULL;
    }
    dpa = NULL;
    rp = VK_NULL_HANDLE; pl = VK_NULL_HANDLE; gpipe = VK_NULL_HANDLE;
    cmdpool = VK_NULL_HANDLE; cmd = VK_NULL_HANDLE; fence = VK_NULL_HANDLE;
    vk_ready = 0; vk_fatal = 0; vk_dead = 0;
    vk_frame = 0; gl_swap = 0;

    /* read experiment config: adb shell setprop debug.panvk.swap N / debug.panvk.ms N */
    {
        char v[PROP_VALUE_MAX] = {0};
        if (__system_property_get("debug.panvk.swap", v) > 0) cfg_swap_every = atoi(v);
        else cfg_swap_every = 1;
        if (__system_property_get("debug.panvk.finish", v) > 0) cfg_glfinish = atoi(v);
        else cfg_glfinish = 1;
        if (__system_property_get("debug.panvk.ms", v) > 0 && atoi(v) >= 2) cfg_frame_ms = atoi(v);
        else cfg_frame_ms = 16;
        LOGI("cfg: swap_every=%d frame_ms=%d glfinish=%d", cfg_swap_every, cfg_frame_ms, cfg_glfinish);
    }

    LOGI("panvk_host starting (pid %d)", getpid());

    while (1) {
        int ident, events;
        struct android_poll_source *source;
        int timeout = tgt_valid ? 0 : 100;
        while ((ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                LOGI("destroy requested, exiting");
                destroy_target();
                return;
            }
            timeout = 0;
        }
        render_frame();
    }
}
