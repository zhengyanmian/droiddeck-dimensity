# droiddeck-dimensity

给 DroidDeck（或等价的 Steam-on-Android 方案）做联发科天玑 9500 适配的研究与工程仓库。

**目标硬件**：天玑 9500（MT6991？），GPU = Arm Mali G1-Ultra MC12（第五代架构 / CSF 前端，kbase 内核驱动）

**最终目标**：在天玑 9500 上本地跑通 Valve ARM64 Steam 客户端 + Proton ARM64 / FEX，游戏能渲染。

## 当前状态

Phase 1（诊断 + 情报收集）进行中。见 `docs/ROADMAP.md`。

## 核心结论（调研摘要）

1. DroidDeck 官方不支持 Mali/Immortalis，卡点不是应用层，是 **Turnip 驱动只支持 Adreno**。
2. 开源社区已有三条关键前人路径（详见 `docs/RESEARCH.md`）：
   - `GunaCharanTeja/panvk-kbase-android`：Mesa PanVK 直连 mali_kbase（/dev/mali0）的补丁栈 + 构建脚本，参照机型 Mali-G615（Valhall v11 CSF）。
   - `MineGraph-bilibili/panvk-Android`：G710/G610/G615/G715/G720 的 PanVK Android 实验构建。
   - `huntergdavis/steamclienttermux`：免 root 的 Termux + Debian proot + ARM64 Steam 客户端完整方案（原用 Turnip，可替换为 PanVK）。
3. DroidDeck issue #736 实测：PanVK-kbase 上 Vulkan 已能跑通（vkcube 渲染正常），卡在 gamescope 要求 DRM render node 而 kbase 没有。两条修法：改 gamescope，或绕过 gamescope 走桌面路径。
4. 天玑 9500 的 G1-Ultra 与 G720/G925 同属第五代架构（CSF），适配主力是**加 GPU ID / 硬件 profile**，不是重写驱动。

## 使用

双击 `tools\diagnose-phone.bat`（手机需开 USB 调试并连电脑），
把生成的 `docs/diag-report.txt` 回传，作为 Phase 1 的真机基线数据。
