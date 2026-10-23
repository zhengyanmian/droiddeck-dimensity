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

## 3. 已知风险

1. G1-Ultra 太新：panvk 模型表可能不认识，硬件 profile 需要试出来（参考 G925 参数起步）。
2. 联发科 kbase UAPI 版本差异：G1 世代内核的 kbase 接口和 G615 未必一致。
3. 无 root 时 /dev/mali0 权限：shell 用户能否访问待真机确认（诊断脚本会查）。
4. 性能预期：PanVK 目前即使跑通也远不如 Turnip 成熟，FEX + DXVK 全软件栈开销大，
   首个里程碑是"vkcube + vulkaninfo"，第二个是"Steam 大屏界面 30fps"，游戏是第三步。
