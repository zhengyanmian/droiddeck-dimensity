# RESEARCH — 天玑 9500 跑 Steam/PC 游戏的前人路径调研

调研日期：2026-10-23

## 0. 目标硬件

- 芯片：MediaTek Dimensity 9500（2025-09-22 发布，3nm）
- GPU：**Arm Mali G1-Ultra MC12**（12 核心）
- 架构：Arm 第五代 GPU 架构（Lumex CSS 平台，G925 的后继，CSF 前端）
- 光追：RTUv2（第二代）
- API：Vulkan 1.3 / OpenGL ES 3.2 / OpenCL 3.0
- 内核侧：MediaTek 私有 mali_kbase 驱动，设备节点 `/dev/mali0`，**没有 DRM render node**

关键点：G1-Ultra 与 G720/G925 同属第五代架构（CSF），上游 panvk 已支持 CSF v10/v11。
G1 的 GPU 产品 ID 在 panvk 模型表里可能未登记（需要加 profile），但前端协议相同。

## 1. 为什么 DroidDeck 官方不支持

- DroidDeck 技术栈：PRoot Linux 容器 + FEX（x86→ARM64 转译）+ gamescope + Proton ARM64 + **Turnip**（Mesa 的 Adreno 开源 Vulkan 驱动）。
- Turnip 从寄存器级别就是 Qualcomm 专属，Mali 物理上不可能用。
- Mali 的开源驱动是 panvk，但它传统上走 DRM render node（panthor 内核驱动），
  而 Android 设备上联发科给的是私有 kbase 接口 —— 这就是所有适配工作的核心矛盾。

## 2. 前人项目（按重要性排序）

### 2.1 GunaCharanTeja/panvk-kbase-android ★ 主力参照
- 独立 Mesa PanVK 构建层，直连 Android 私有 mali_kbase（/dev/mali0），无需 DRM 节点。
- pin 在 Mesa 26.3.0-devel，含完整补丁栈：
  - `patches/csf-v11/`：Valhall v11 专属补丁 001-171
  - `123-vkd3d-dx12-essentials.patch`：D3D12 descriptor buffer 支持
  - `124-panvk-astc-bcn-transcode.patch`：BC1-BC7 → ASTC 转码（解决桌面游戏纹理格式不兼容）
- 构建脚本：`build-android.sh`（Bionic ICD）、`build-glibc.sh`（glibc ICD，给 proot/Termux 用）
- 参照机型：Poco X6 Pro（天玑 8300-Ultra，Mali-G615，v11 CSF）
- 我们的工作 = 给 G1-Ultra 加硬件 profile + 验证 v11 补丁栈是否覆盖 G1 的差异

### 2.2 MineGraph-bilibili/panvk-Android
- 覆盖 G710/G610/G615/G715/G720 的 Android 实验构建，有公开 beta 包。
- 基于 leegao/funnymdzz 的 bionic-vulkan-wrapper 路线。
- 价值：拿现成构建在真机上先验证 kbase 通路，再决定要不要自己编译。

### 2.3 huntergdavis/steamclienttermux
- 免 root 的 Termux + Debian PRoot + 打补丁的 PRoot（绕过 Pressure Vessel 问题）
- 完整跑通 ARM64 Steam 客户端登录/下载/启动（原方案用 Turnip，Adreno 机型）
- 价值：**绕过 gamescope 的 Steam 启动管线**可以直接参考 —— 正好对应 issue #736
  提出的"绕过 gamescope 走桌面路径"修法。把它的 Turnip 换成 PanVK-kbase ICD 就是我们要的。

### 2.4 DroidDeck 官方 issue 关键情报
- **#736**（天玑 8400 Ultra / Mali-G720 MC7 实测）：
  - PanVK Kbase G615（Mesa 26.3.0-devel, "PanVK-kbase RC3"）→ vulkaninfo 正常、vkcube 在 labwc 桌面渲染正常
  - gamescope 失败：`vulkan: physical device has no render node` → `Failed to create backend`
  - 根因：gamescope 后端要求 Vulkan 设备带 DRM primary/render node，kbase 只给 /dev/mali0
  - 修法 A：patch gamescope 容忍无 DRM 节点
  - 修法 B：Steam 不经 gamescope，直接在 labwc/桌面环境跑
- **#1130**：有人自己编译驱动 + 修改版 DroidDeck 在 Mali 机型跑通（未开源），
  评论区有人拿天玑 9500 实测"目前不行，等他开源"。**要持续盯这个 issue。**

### 2.5 其他参考
- lukevalen/panvk-mali-g52：JM（Job Manager）硬件上的实验，记录了 kbase UAPI 的坑
  （KBASE_IOCTL_MEM_EXEC_INIT EPERM 等），Bifrost 世代，参考价值中等。
- leegao/bionic-vulkan-wrapper、funnymdzz/mali_fxxker：bionic 层 Vulkan 注入的前人工作。
- steam-android-runtime-research（xXJSONDeruloXx）：Valve ARM64 客户端分发端点调研，
  steam_client_steamdeck_publicbeta_linuxarm64 通道 + SteamRT3C ARM64 + Holo aarch64 包源。

## 4. 真机诊断结果（Phase 1 完成，2026-10-23）

真机：OnePlus PMB110，Android 17 (SDK 37)，SoC MT6993（天玑 9500）
完整报告：`docs/diag-report.txt`

| 项目 | 结果 | 含义 |
|---|---|---|
| GPU | Mali-G1-Ultra MC12，GLES 驱动 r54p1，Vulkan 1.3 | 与预期一致，第五代 CSF |
| 内核 | 6.12.69-android16（GKI） | kbase 应在 vendor_dlkm 模块里 |
| `/dev/mali0` | **crw-rw-rw-（666，人人可读写）** | **免 root 即可访问 GPU！** PanVK ICD 可直接上机 |
| `/dev/dri/card0` | 存在但 crw-rw---- system:system，shell 无权访问 | gamescope 路线对 shell 用户彻底死路 |
| render node (renderD128) | **不存在** | 证实 issue #736 的卡点在 9500 上同样存在 |
| root | 无 su | 免 root 路线是唯一现实路线 |

**分支决策：走修法 B** —— steamclienttermux 式管线（proot + Steam ARM64）+ PanVK-kbase ICD，完全绕开 gamescope。

三个超预期的好消息：
1. `/dev/mali0` 是 666 权限，比参照机型（Poco X6 Pro 等）宽松，驱动验证阶段不需要 root。
2. Android 17 + 内核 6.12 很新，bionic/proot 兼容性风险低于老机型。
3. 厂商 Vulkan 1.3 完整可用（dumpsys 显示多处 Vulkan 应用正常创建 device）。

## 5. Phase 2 里程碑 A：Termux 厂商驱动基线（2026-10-23 ✅）

Termux（免 root）+ vulkan-loader-android 直通系统驱动，`vulkaninfo` 成功：

```
GPU0: Mali-G1-Ultra MC12
apiVersion    = 1.3.305
driverVersion = 54.1.0
vendorID      = 0x13b5 (ARM)
deviceID      = 0xe8800010   ← PanVK 模型表要登记的 G1-Ultra GPU ID
driverID      = DRIVER_ID_ARM_PROPRIETARY
driverInfo    = v1.r54p1-11eac0...
conformance   = 1.4.1.0
```

- `/dev/mali0` 在 Termux 里 666 可写 ✅（两次验证：adb shell + Termux）
- 内核模块确认为 `mali_kbase`（/sys/module/mali_kbase 存在）
- 实例层为空，实例扩展 15 个（标准 Android 配置）

**部署管线经验**：手机直连 Termux 镜像 403（代理出口 IP 被反爬拦截），
改为「PC 下载 deb → adb push /sdcard/droiddeck/deb2/ → Termux dpkg -i」，
该管线已验证可复用，后续所有 Termux 包都这么走。

**下一步（里程碑 B）**：用 NDK 交叉编译 panvk-probe 测试器，
dlopen MineGraph 的 libvulkan_panfrost.so（bionic），
验证 PanVK-kbase 能否在 G1-Ultra 上枚举出设备（预期会报 GPU ID 未识别，
那就到了打 profile 补丁的环节）。

## 6. MineGraph 构建分析（2026-10-23）

用 NDK 编译的 panvk-probe / hmidump 在真机上做了格式分析：

- `libvulkan_panfrost.so`（Mesa 26.3.0-panvk.4，CSF+JM 双路径）**不导出任何 vk* 符号**，
  唯一导出是 OBJECT 符号 `HMI`（248 字节）。
- HMI 布局 = Android HAL 风格模块描述符：
  - `[0]` magic `HWMT`（反写存储，hw_module_t 标签）+ 版本
  - `[1]` → "vulkan"（模块 id）
  - `[2]` → "Mesa 3D Vulkan HAL"（name）
  - `[3]` → "Mesa 3D"（author）
  - `[4]` → methods 区（紧邻 HMI 自身）
- 结论：该 .so 面向 `hw_get_module("vulkan")` 式 HAL 加载通道（他们自己的
  bionic-vulkan-wrapper 系 wrapper），**不能**用标准 dlsym 方式独立调用。

**决策**：MineGraph 构建降级为参考实现（它的价值是 Mesa 补丁内容本身：
G615 v11 CSF 适配 + vkd3d robustBufferAccess2 + MEM_ALLOC uAPI 回退）。
主攻路线改为用 GunaCharanTeja/panvk-kbase-android 补丁栈自编译标准导出的 ICD：
- Android/Bionic 格式 → 给真机直接用
- ARM64 glibc 格式 → 给 proot Debian（Steam 管线）用
构建环境需要 Linux/Termux（脚本自带 build-android.sh / build-glibc.sh）。

工具链现状：NDK r28b 已装（~/.workbuddy/bin/android-ndk/），
panvk-probe.c / hmidump.c / hmidump2.c 编译管线已打通（PC 交叉编译 → adb push → 真机运行）。

## 3. 已知风险

1. G1-Ultra 太新：panvk 模型表可能不认识，硬件 profile 需要试出来（参考 G925 参数起步）。
2. 联发科 kbase UAPI 版本差异：G1 世代内核的 kbase 接口和 G615 未必一致。
3. 无 root 时 /dev/mali0 权限：shell 用户能否访问待真机确认（诊断脚本会查）。
4. 性能预期：PanVK 目前即使跑通也远不如 Turnip 成熟，FEX + DXVK 全软件栈开销大，
   首个里程碑是"vkcube + vulkaninfo"，第二个是"Steam 大屏界面 30fps"，游戏是第三步。

## Phase 2 里程碑 B：真机 kbase 直连 + 构建环境（2026-10-24）

### 真机 GPU_ID 探测（kbase-gpuid 探针，免 root）
- `RAW_GPU_ID = 0xe8800010`，按 Mesa PAN_* 宏解码：
  - PAN_ARCH_MAJOR(bits31:28) = 0xE = **14**（pan_arch() 默认路径自动返回 14）
  - arch_minor = 8, arch_rev = 8, product_major = 0
- `CORE_FEATURES = 0x50000004` → gpu_variant = 低 8 位 = **0x04**
- L2_FEATURES 0x081206, MEM_FEATURES 0x701

### 重大利好：G1-Ultra 已在模型表内
- `pan_model.c` 已有 `FIFTHGEN_MODEL(PAN_PROD_ID(14, 8, 0), 4, "G1-Ultra", "G1", ...)`，
  prod_id 0xE0800 与真机完全匹配，variant 4 匹配 → **模型表零改动**
- 全代码库 0 处硬编码 `arch == N`，全部为范围比较（>= 10 / >= 12），v14 自然走 CSF 路径
- pan_arch() 默认路径 PAN_ARCH_MAJOR(gpu_id) 直接出新架构号，无需注册表补丁
- NULL 模型会 VK_ERROR_INCOMPATIBLE_DRIVER（panvk_physical_device.c:1174），但我们不会踩到

### WSL2 构建环境
- WSL 3.0.3（winget 商店源超时 → ghproxy.net 下载 msixbundle → UAC 手动安装）
- Ubuntu 24.04 rootfs 从 cloud-images.ubuntu.com 直连下载，wsl --import 为发行版 droiddeck-build
- 补丁栈 GunaCharanTeja/panvk-kbase-android 经 ghproxy.net 克隆，Mesa 26.3.0-devel (5a07217f) 浅拉成功
- `apply-patches.sh --profile g720-v12-csf`：20 个补丁全部干净套上（PATCH-DRIFT=0）
- 教训：git bash 调 wsl.exe 需 MSYS_NO_PATHCONV=1，否则 /root/... 会被改写成 PortableGit 路径

### 教训（网络路由）
- winget/微软商店 CDN：国内直连部署超时 0x80072ee2
- GitHub：直连/代理均不稳 → ghproxy.net 前缀镜像可用（git clone 与 release 下载均验证）
- gitlab.freedesktop.org、cloud-images.ubuntu.com、archive/USTC apt 源：直连均可达

## Phase 2 里程碑 C：自编译 PanVK bionic ICD 构建成功（2026-10-24）

- `build-android.sh --profile g720-v12-csf --api 35` 输出 `dist/android-g720-v12-csf/libvulkan_panfrost.so`（21.3MB，AArch64）
- 导出标准 ICD 接口：vk_icdGetInstanceProcAddr / vk_icdGetPhysicalDeviceProcAddr / vk_icdNegotiateLoaderICDInterfaceVersion
  （与 MineGraph 的 HMI HAL 封装完全不同，可直接挂 loader / proot 管线）
- 新增 icd-probe.c：免 loader 直连 ICD 接口枚举物理设备（dlopen → vk_icdNegotiate → CreateInstance → EnumeratePhysicalDevices）

### x86_64 主机适配补丁（上游脚本假定 ARM64 Linux 构建机）
1. 依赖三件套用 NDK 交叉编译为 AArch64 静态库 → work/android-deps/（zlib 1.3.1、expat 2.3.0）
2. libdrm/zlib/expat 的 pkg-config 垫片写入 work/android-deps/lib/pkgconfig/
3. meson/android-aarch64.ini：c_args += -I shims/include；c_link_args += -L shims, --allow-shlib-undefined
4. build-android.sh 第 54 行：cpp_link_args 增补 -L shims + --allow-shlib-unsigned（注意 meson 列表语法需整体加引号，
   且必须用 cpp_link_args —— C++ 链接的目标不吃 c_link_args）
5. 坑位清单：wsl.exe 参数里的 shell 变量会被吞（一律字面路径）；Git Bash 路径转换需 MSYS_NO_PATHCONV=1；
   WSL 发行版重启会清 /tmp；API 必须 >= 29（AHardwareBuffer_isSupported）

### 待真机验证（icd-probe）
- 预期：EnumeratePhysicalDevices 返回 1 个设备，deviceName 含 G1-Ultra，deviceID 匹配 0xe8800010
- 手机断连（用户外出，无线调试休眠），恢复后执行：
  adb push libvulkan_panfrost.so icd-probe libdrm.so → LD_LIBRARY_PATH=/data/local/tmp ./icd-probe ./libvulkan_panfrost.so

## Phase 3 里程碑 D：真机验证通过 —— 自编译 PanVK ICD 在天玑 9500 上完成 Vulkan 初始化（2026-10-24）

### 首轮 icd-probe 结果（PROBE_SUCCESS）
```
WARNING: panvk is not a conformant Vulkan implementation, testing use only.
[ok] dlopen /data/local/tmp/libvulkan_panfrost.so
[ok] ICD interface version: 5
[ok] instance extensions: 13 (r=0)
[ok] instance created
[ok] physical devices: 1 (r=0)
GPU0: Mali-G1-Ultra MC12 | api 1.4.363 | driver 0x6802063 | vendorID 0x13b5 | deviceID 0xe8800010
PROBE_SUCCESS
```
- dlopen → ICD v5 协商 → CreateInstance → EnumeratePhysicalDevices 全链路通过（bionic 环境，免 loader）
- deviceID 0xe8800010 与 kbase-gpuid 硬件探针完全一致；vendorID 0x13b5 = ARM
- api 1.4.363：Mesa 26.3.0-devel 汇报的 Vulkan 版本
- 13 个 instance extensions（vkEnumerateInstanceExtensionProperties）

### 首轮失败与修复：X11 依赖污染
- 症状：dlopen failed: library "libxcb.so.1" not found
- 根因：build-android.sh `-Dplatforms=android,x11` 将 9 个 glibc X11 库硬链入 ICD
  （libxcb.so.1 / libX11-xcb.so.1 / libxcb-dri3,present,xfixes,sync,randr,shm / libxshmfence）
  —— 上游注释声称 "xcb 运行时 dlopen"，实际 Mesa 26.3 链接器行为是 DT_NEEDED 直链
- 修复：`-Dplatforms=android`（纯 Android WSI）重编，2m35s 增量构建
- 修复后 NEEDED 全为 bionic 库：liblog/libnativewindow/libsync/libdrm/libm/libdl/libc

### 运维记录
- GitHub 推送改走 SSH 22 端口（直连可达，梯子失效时可用）；remote 已改为 git@github.com:zhengyanmian/droiddeck-dimensity.git
- WSL 内 NDK 位于 /opt/android-ndk-r28b；build-android.sh 参数必须命令行传（--profile/--ndk），不吃环境变量
- adb push 本地路径用 C:/ 形式 + MSYS_NO_PATHCONV=1（/data/local/tmp 目标路径同理）
- 设备：192.168.1.11:38467（柚谈工具箱重连）

### 意义
- 天玑 9500（Mali-G1-Ultra, CSF v12）从 "Turnip/Adreno only" 到 PanVK Vulkan 初始化打通，
  里程碑 A→D 全部完成；下一步为 DroidDeck 场景适配（绕开 gamescope 的 launch 管线 / proot glibc 消费端）

## Phase 3 里程碑 E：真机 GPU 渲染验证通过（2026-10-24）

### 渲染探针 tri-probe（offscreen，免 WSI/窗口系统）
- 流程：CreateInstance(1.3) → CreateDevice → renderpass(clear红) → draw(3, 硬编码顶点, 片元输出绿) → 读回验证
- 结果（LINEAR image 直读）：
  center (128,128): R=0 G=255 B=0 → OK（三角形内部）
  corner  (8,8):    R=255 G=0 B=0 → OK（clear 背景）
  far corner / apex area：全部像素级正确
- **图形管线全链路（VS → 光栅化 → FS → 输出）在 Mali-G1-Ultra 上真实工作**

### 关键坑与修复（天玑 9500 / MTK CSF 内核）
1. `KBASE_IOCTL_MEM_ALLOC_EX` (ioctl 59) 返回 ENOTTY：内核版本报告 ≥1.9 但缺该 ioctl
   → fix-memalloc.py：ENOTTY 时自动回退 legacy `KBASE_IOCTL_MEM_ALLOC`（in/out ABI 一致）
   修复前症状：vkCreateDevice 返回 VK_ERROR_EXTENSION_NOT_FOUND (-2)（误导性错误码）
2. tri-probe 宏 bug：`#name` 丢 "vk" 前缀 → gipa 查询全部 NULL → SIGSEGV；修复 `"vk" #name`
3. 排查工具链：fill-probe（CmdFillBuffer 0xAA 读回）证明 CSF 提交链路正常 → 定位问题在渲染/copy 路径

### 已知问题
- CopyImageToBuffer 读回全 0（linear image direct map 内容正确）——copy path 或布局同步问题，待查；不影响渲染能力本身

### 对 DroidDeck 路线的意义
- "绕开 gamescope" 的核心前提成立：GPU 能离屏渲染，后续可用 dmabuf/合成器路径输出画面
- 里程碑 A→E 完成：厂商基线 → 硬件探针 → 自编译 ICD → 真机初始化 → **真机渲染**

## Phase 3 里程碑 E：真机 GPU 渲染验证通过（2026-10-24）

### 渲染探针 tri-probe（offscreen，免 WSI/窗口系统）
- 流程：CreateInstance(1.3) → CreateDevice → renderpass(clear红) → draw(3, 硬编码顶点, 片元输出绿) → 读回验证
- 结果（LINEAR image 直读）：
  center (128,128): R=0 G=255 B=0 → OK（三角形内部）
  corner  (8,8):    R=255 G=0 B=0 → OK（clear 背景）
  far corner / apex area：全部像素级正确
- **图形管线全链路（VS → 光栅化 → FS → 输出）在 Mali-G1-Ultra 上真实工作**

### 关键坑与修复（天玑 9500 / MTK CSF 内核）
1. `KBASE_IOCTL_MEM_ALLOC_EX` (ioctl 59) 返回 ENOTTY：内核版本报告 ≥1.9 但缺该 ioctl
   → fix-memalloc.py：ENOTTY 时自动回退 legacy `KBASE_IOCTL_MEM_ALLOC`（in/out ABI 一致）
   修复前症状：vkCreateDevice 返回 VK_ERROR_EXTENSION_NOT_FOUND (-2)（误导性错误码）
2. tri-probe 宏 bug：`#name` 丢 "vk" 前缀 → gipa 查询全部 NULL → SIGSEGV；修复 `"vk" #name`
3. 排查工具链：fill-probe（CmdFillBuffer 0xAA 读回）证明 CSF 提交链路正常 → 定位问题在渲染/copy 路径

### 已知问题
- CopyImageToBuffer 读回全 0（linear image direct map 内容正确）——copy path 或布局同步问题，待查；不影响渲染能力本身

### 对 DroidDeck 路线的意义
- "绕开 gamescope" 的核心前提成立：GPU 能离屏渲染，后续可用 dmabuf/合成器路径输出画面
- 里程碑 A→E 完成：厂商基线 → 硬件探针 → 自编译 ICD → 真机初始化 → **真机渲染**

## Phase 3 里程碑 F：系统 loader 集成成功（2026-10-24）

### vulkaninfo --summary 端到端输出（Termux + vulkan-loader-generic 1.4.366）
```
WARNING: panvk is not a conformant Vulkan implementation, testing use only.
GPU0:
        apiVersion  = 1.4.363
        driverVersion = 26.2.99
        vendorID    = 0x13b5 / deviceID = 0xe8800010
        deviceName  = Mali-G1-Ultra MC12
        driverID    = DRIVER_ID_MESA_PANVK
        driverName  = panvk
        driverInfo  = Mesa 26.3.0-devel (git-5a07217f03)
```

### 路径探索
- vulkan-loader-android（AOSP loader）：忽略 VK_DRIVER_FILES，只扫系统分区 → 不可用
- 方案：pkg install vulkan-loader-generic（mesa/Khronos loader，认 VK_DRIVER_FILES + $PREFIX/share/vulkan/icd.d）
- ICD JSON：~/panvk/panvk_icd.json，library_path 绝对路径指向自编译 so；LD_LIBRARY_PATH 带 ~/panvk（libdrm.so 依赖）
- 顺带收获：mesa-vulkan-icd-swrast（llvmpipe 软渲染）可作为 GPU 路径失败时的 fallback

### 状态
- 里程碑 A→F 全部完成：诊断 → 硬件探针 → 自编译 ICD → 真机初始化 → 真机渲染 → loader 集成
- 下一步（Phase 4）：DroidDeck 场景管线——绕开 gamescope 的 launch 链路 / proot glibc 消费端 / 显示输出（dmabuf 合成）

## Phase 3 里程碑 F：系统 loader 集成成功（2026-10-24）

### vulkaninfo --summary 端到端输出（Termux + vulkan-loader-generic 1.4.366）
```
WARNING: panvk is not a conformant Vulkan implementation, testing use only.
GPU0:
        apiVersion  = 1.4.363
        driverVersion = 26.2.99
        vendorID    = 0x13b5 / deviceID = 0xe8800010
        deviceName  = Mali-G1-Ultra MC12
        driverID    = DRIVER_ID_MESA_PANVK
        driverName  = panvk
        driverInfo  = Mesa 26.3.0-devel (git-5a07217f03)
```

### 路径探索
- vulkan-loader-android（AOSP loader）：忽略 VK_DRIVER_FILES，只扫系统分区 → 不可用
- 方案：pkg install vulkan-loader-generic（mesa/Khronos loader，认 VK_DRIVER_FILES + $PREFIX/share/vulkan/icd.d）
- ICD JSON：~/panvk/panvk_icd.json，library_path 绝对路径指向自编译 so；LD_LIBRARY_PATH 带 ~/panvk（libdrm.so 依赖）
- 顺带收获：mesa-vulkan-icd-swrast（llvmpipe 软渲染）可作为 GPU 路径失败时的 fallback

### 状态
- 里程碑 A→F 全部完成：诊断 → 硬件探针 → 自编译 ICD → 真机初始化 → 真机渲染 → loader 集成
- 下一步（Phase 4）：DroidDeck 场景管线——绕开 gamescope 的 launch 链路 / proot glibc 消费端 / 显示输出（dmabuf 合成）

### 更正（v0.11）：CopyImageToBuffer "已知问题"撤销
- 复查 tri-probe 源码发现：探针从未录制 vkCmdCopyImageToBuffer 命令（注释声称有），
  读回 buffer 为未初始化内存 → 全 0 是**探针 bug，非驱动 bug**
- 修复：renderpass（finalLayout=TRANSFER_SRC_OPTIMAL）后直接追加 copy 命令，
  无需额外 barrier（renderpass 隐式外部依赖覆盖）
- 真机结果：RENDER_PASS_SUCCESS (copy + direct)，四点像素检查全 OK
  → 图形管线 + transfer 路径全部无已知问题

### 更正（v0.11）：CopyImageToBuffer "已知问题"撤销
- 复查 tri-probe 源码发现：探针从未录制 vkCmdCopyImageToBuffer 命令（注释声称有），
  读回 buffer 为未初始化内存 → 全 0 是**探针 bug，非驱动 bug**
- 修复：renderpass（finalLayout=TRANSFER_SRC_OPTIMAL）后直接追加 copy 命令，
  无需额外 barrier（renderpass 隐式外部依赖覆盖）
- 真机结果：RENDER_PASS_SUCCESS (copy + direct)，四点像素检查全 OK
  → 图形管线 + transfer 路径全部无已知问题

## Phase 4 里程碑 G：AHardwareBuffer 显示桥打通（2026-10-24）

### ahb-probe 端到端结果（AHB_ROUNDTRIP_SUCCESS）
- AHardwareBuffer_allocate(gralloc, 256x256 RGBA8, GPU_COLOR_OUTPUT|CPU_READ_OFTEN)
- vkGetAndroidHardwareBufferPropertiesANDROID 查询（262144B, memTypeBits=0x3）
- vkCreateImage(VK_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, ANDROID_HARDWARE_BUFFER_BIT)
- vkAllocateMemory(VkImportAndroidHardwareBufferInfoANDROID + dedicated) 导入成功
- 渲染 pass → AHardwareBuffer_lock CPU 读取（合成器视角）：
  center=绿(三角形) corner=红(clear) 像素级全部正确
- 意义：PanVK 帧 → gralloc buffer → 任意 Android 消费端（SurfaceFlinger/应用），
  "绕开 gamescope 上屏"的核心原语成立

### 破案过程：MTK 内核 MEM_IMPORT 考古
1. 首败：vkAllocateMemory 导入 AHB 报 INVALID_EXTERNAL_HANDLE
   logcat: `kbase: KBASE_IOCTL_MEM_IMPORT failed: Inappropriate ioctl for device`
2. ioctl 门禁：kbase 对未初始化上下文一律 EPERM（VERSION_CHECK→SET_FLAGS→tracking page
   之前），裸 fd 扫描 ioctl 表不可行
3. 反汇编 vendor libGLES_mali.so (53MB)：AArch64 立即数经 MOVZ/MOVK 拼装，
   扫描指令对得 vendor 编码 = **IOWR(0x80,22,40)**（上游 union 24B → cmd 不匹配 ENOTTY）；
   并确认 vendor flags OR bit26 (BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP)
4. MTK 40B out 布局：flags@0x00(输入|NEED_MMAP 0x4000)、gpu_va cookie@0x08、
   va_pages@0x10（是 VA span 计数，非 BO 页数——bo_size 改用 lseek(dma-buf)）
5. 探针 ENOMEM 假象：`int fdp; memcpy(req+8,&fdp,8)` 从 4 字节对象拷 8 字节（UB），
   clang 优化成传 fd 值 0x5 当指针 → 内核报 ENOMEM；改用 int* 参数后 import 即成功

### 修复（fix-memimport.py，v0.12）
- kbase_kmod.c：MTK 40B 联合体 + IOWR(0x80,22,40) + flags|SYNC_ON_MAP_UNMAP
  + ENOMEM 重试(8 次×10ms) + bo_size 改 lseek(dma-buf)
- 回归：tri-probe RENDER_PASS_SUCCESS (copy+direct) 通过

### 后续（Phase 4 续）
- 显示桥下一步：panvk AHB → SurfaceView/SurfaceFlinger 上屏（需一个宿主 APK 或
  Termux:X11 场景）；VK_ANDROID_native_buffer/VK_KHR_swapchain 已在设备扩展列表
- proot glibc 消费端管线待启动

## Phase 4 里程碑 G：AHardwareBuffer 显示桥打通（2026-10-24）

### ahb-probe 端到端结果（AHB_ROUNDTRIP_SUCCESS）
- AHardwareBuffer_allocate(gralloc, 256x256 RGBA8, GPU_COLOR_OUTPUT|CPU_READ_OFTEN)
- vkGetAndroidHardwareBufferPropertiesANDROID 查询（262144B, memTypeBits=0x3）
- vkCreateImage(VK_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, ANDROID_HARDWARE_BUFFER_BIT)
- vkAllocateMemory(VkImportAndroidHardwareBufferInfoANDROID + dedicated) 导入成功
- 渲染 pass → AHardwareBuffer_lock CPU 读取（合成器视角）：
  center=绿(三角形) corner=红(clear) 像素级全部正确
- 意义：PanVK 帧 → gralloc buffer → 任意 Android 消费端（SurfaceFlinger/应用），
  "绕开 gamescope 上屏"的核心原语成立

### 破案过程：MTK 内核 MEM_IMPORT 考古
1. 首败：vkAllocateMemory 导入 AHB 报 INVALID_EXTERNAL_HANDLE
   logcat: `kbase: KBASE_IOCTL_MEM_IMPORT failed: Inappropriate ioctl for device`
2. ioctl 门禁：kbase 对未初始化上下文一律 EPERM（VERSION_CHECK→SET_FLAGS→tracking page
   之前），裸 fd 扫描 ioctl 表不可行
3. 反汇编 vendor libGLES_mali.so (53MB)：AArch64 立即数经 MOVZ/MOVK 拼装，
   扫描指令对得 vendor 编码 = **IOWR(0x80,22,40)**（上游 union 24B → cmd 不匹配 ENOTTY）；
   并确认 vendor flags OR bit26 (BASE_MEM_IMPORT_SYNC_ON_MAP_UNMAP)
4. MTK 40B out 布局：flags@0x00(输入|NEED_MMAP 0x4000)、gpu_va cookie@0x08、
   va_pages@0x10（是 VA span 计数，非 BO 页数——bo_size 改用 lseek(dma-buf)）
5. 探针 ENOMEM 假象：`int fdp; memcpy(req+8,&fdp,8)` 从 4 字节对象拷 8 字节（UB），
   clang 优化成传 fd 值 0x5 当指针 → 内核报 ENOMEM；改用 int* 参数后 import 即成功

### 修复（fix-memimport.py，v0.12）
- kbase_kmod.c：MTK 40B 联合体 + IOWR(0x80,22,40) + flags|SYNC_ON_MAP_UNMAP
  + ENOMEM 重试(8 次×10ms) + bo_size 改 lseek(dma-buf)
- 回归：tri-probe RENDER_PASS_SUCCESS (copy+direct) 通过

### 后续（Phase 4 续）
- 显示桥下一步：panvk AHB → SurfaceView/SurfaceFlinger 上屏（需一个宿主 APK 或
  Termux:X11 场景）；VK_ANDROID_native_buffer/VK_KHR_swapchain 已在设备扩展列表
- proot glibc 消费端管线待启动

## 里程碑 H（2026-10-24）：宿主 APK 真上屏 —— PanVK 帧第一次出现在手机屏幕上

### 架构（无 Vulkan loader 参与）
```
[PanVK (Mesa, driver-direct dlopen)] --VK_ANDROID_external_memory_android_hardware_buffer--> AHardwareBuffer (gralloc, 1272x2631)
[Vendor GL (EGL)] --eglGetNativeClientBufferANDROID + EGLImage + samplerExternalOES--> SurfaceView -> SurfaceFlinger
```
- NativeActivity + hasCode=false：无 dex、无 Java，纯 native APK（aapt 打包、zipalign、debug key v1 签名，WSL 全自建工具链）
- WSL 构建链：dl.google.com 下载 build-tools r34 + platform-34 android.jar；apt 装 default-jre-headless/glslang-tools
- APK 打包 3 个 lib：libpanvk_host.so（45KB）、libvulkan_panfrost.so（22MB，v0.12+诊断）、libdrm.so（shims 版，NEEDED 补齐）
- adb 经柚谈工具箱无线连接（192.168.1.11:38467），adb install 直装

### 结果
- `vkCreateInstance`（driver-direct）→ PD Mali-G1-Ultra MC12 api 1.4.363 → vkCreateDevice → AHB 1272x2631 双端导入（VK 渲染 + GL 采样）→ **用户肉眼确认：红底绿三角上屏，且转动了数帧**
- 首帧 CPU 自检：SELFTEST center px G=217 → GREEN_OK
- app 生命周期健壮：同进程 activity 重启（重置静态状态）、TERM_WINDOW 后台存活、force-stop 干净退出

### 排错记录（3 个根因）
1. dlopen 失败 libdrm.so：系统无 libdrm，Termux 里是单独推的 —— 把 shims/libdrm.so 打进 APK lib/arm64-v8a
2. 进程复用 + 静态 vk_fatal：activity 重启后 render_frame 被旧 vk_fatal 短路 → 黑屏无日志；android_main 入口重置全部状态
3. **vkCreateInstance r=-7 = VK_ERROR_EXTENSION_NOT_PRESENT**（-7 不是 INITIALIZATION_FAILED，记错码排查绕远）：panvk 实例级扩展表没有 VK_ANDROID_external_memory_android_hardware_buffer（它是 device 级），ahb-probe 有过滤逻辑、host 代码搬运时丢了。且 Mesa vk_errorf 的 WARNING 级日志在 Android 默认被过滤 → 静默失败。给 panvk_CreateInstance/vk_instance_init 加 mesa_loge 诊断后定位

### 已知问题（下一步）：连续提交 ~0.5s 后 CSF 组 fatal
- `kbase: CSF group 0/1/2 fatal error: status 0x004c0488 (exception 0x88), sideband 0xffffffffffffffff` → 随后 submit r=-4 (DEVICE_LOST)
- 状态解码：exception type=LSB=0x88（CS 资源类异常，疑 tiler heap 资源相关），data=0x004c04
- 单次提交永远正常（所有 Termux 探针 + 首帧），连续提交约 45 帧后死 → 疑似与 vendor GL（同进程双 kbase 客户端）或 tiler heap 增长相关
- 缓解已实装：vk_dead 状态下停 VK 提交、GL 继续合成最后一帧（画面稳定不闪退）
- 待查：app 与 Termux 的 kbase 上下文差异、heap 初始化日志、限制提交频率实验

### CSF fatal 对照实验（决定性）
- loop-probe（ahb-probe 改 300 帧循环提交，fence/帧，无 vendor GL）：adb shell 环境 **ALL 300 FRAMES OK + 像素全对**
- 结论：panvk 连续提交本身稳定（shell 环境 300 帧）；app 内 ~200 帧死亡 → 根因是 **同进程内 vendor GL 与 panvk 双 kbase 客户端交互**（vendor EGL 每次 swap 提交后，固件最终杀掉 panvk 的 CSG；exception 0x88, sideband 0xffffffffffffffff 疑为组资源被回收）
- 下一步实验方向：① app 内降低 GL swap 频率看 fatal 延迟是否变化；② GL 首帧后停止 swap（静态画面）验证零冲突；③ 查 kbase CSG 槽位/优先级策略（vendor driver 常驻 CSG 是否挤占）；④ 极端方案：panvk 侧换 fence+外部 sync 或降低 CSG 需求
