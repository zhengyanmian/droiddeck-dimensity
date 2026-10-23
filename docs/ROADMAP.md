# ROADMAP — 天玑 9500 适配路线图

## Phase 1：真机诊断（本周）✅ 2026-10-23 完成
**结果**（详见 RESEARCH.md 第 4 节 / diag-report.txt）：
- 设备：OnePlus PMB110，Android 17，MT6993，内核 6.12.69-android16
- **`/dev/mali0` 666 权限，免 root 可读写** → 驱动验证不需要 root
- `/dev/dri/` 只有 card0（shell 无权、无 renderD128）→ gamescope 路线死路
- **分支决策：修法 B —— 绕开 gamescope，走 steamclienttermux 管线 + PanVK-kbase ICD**
**目的**：拿到真机基线数据，决定后续走哪条具体分支。
- [ ] 双击 `tools/diagnose-phone.bat`（adb）采集：
  - 内核版本 / kbase 驱动版本 / GPU 产品 ID
  - `/dev/mali0` 是否存在、shell 用户是否有读写权限
  - 是否有 `/dev/dri/*` DRM 节点（决定 gamescope 修法 A 的难度）
  - 厂商 Vulkan ICD 信息、Vulkan 特性支持
- [ ] 确认手机是否已 root（决定权限方案）

**分支决策点**：
- 有 DRM render node → 优先试修法 A（gamescope 小补丁，DroidDeck 容器内直接换 ICD）
- 只有 /dev/mali0 → 走修法 B（steamclienttermux 管线 + PanVK ICD，绕过 gamescope）

## Phase 2：PanVK-kbase 上机验证（1-2 周）
- [ ] 拿 MineGraph/panvk-Android 的 G720 现成构建，在 9500 上试加载（大概率 GPU ID 不识别）
- [ ] 从 GunaCharanTeja/panvk-kbase-android 构建 glibc ICD，给 G1-Ultra 加硬件 profile
      （模型表加 ID + 参考 G925/G720 参数起步），先过 `vulkaninfo`，再过 `vkcube`
- [ ] 记录 kbase UAPI 版本差异，打针对性补丁

## Phase 3：Steam 客户端跑通（2-4 周）
- [ ] 复刻 steamclienttermux 管线（Debian proot + 补丁版 PRoot + SteamRT3C ARM64）
- [ ] 把图形栈从 Turnip 换成我们的 PanVK-kbase ICD
- [ ] 目标 1：Steam 登录成功；目标 2：大屏界面可用渲染
- [ ] （可选）gamescope 补丁：容忍无 render node / 或用 VK_EXT_image_drm_format_modifier 路线

## Phase 4：游戏 + 打包（1-2 月）
- [ ] FEX / Proton ARM64 加载，DXVK-over-PanVK 跑老游戏
- [ ] BC→ASTC 转码路径验证（GunaCharanTeja patch 124 已有现成实现）
- [ ] 打包成双击可用的 Termux 一键脚本，或参考 DroidDeck 架构做成 APK

## 并行跟踪项
- [ ] 盯 DroidDeck issue #1130（神秘 Mali 开发者是否开源）
- [ ] 盯 DroidDeck issue #736 后续（gamescope 补丁是否有人做出来）
- [ ] 盯 Mesa upstream panvk 对 G1 系列的支持进展

## 环境约定
- 构建环境：Termux（真机原生）或 Linux x86 交叉编译；Windows PC 上用 WSL/Git Bash 做脚本和 git
- 每个可运行的里程碑都打 tag 推 GitHub
