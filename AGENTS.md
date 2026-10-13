# Embedded & General Skills Repository (`AGENTS.md`)

本仓库是集中管理所有 AI Agent Skills 的知识与工作流中心，主要面向**通用软件工程**与**嵌入式系统开发（单片机裸机、RTOS、Linux 驱动与系统移植）**。

当 Agent 在本仓库中工作（新增、修改、检索或维护 Skill）或将本仓库作为技能源加载时，必须遵循本文件定义的目录架构、分类准则与编写规范。

---

## 1. 目录架构设计（一级按技术方向，内部结合使用范围）

本仓库采用 **“一级目录按技术方向划分，内部通过标签与前缀区分使用范围”** 的组织方式。此结构既符合嵌入式领域的技术栈边界，又兼容 Antigravity `skills.json` 的单层扫描机制，方便不同项目按需挂载。

```text
e:/data/doc/skills/
├── AGENTS.md                   # 仓库全局 Agent 规则与维护指南（本文件）
├── README.md                   # 技能库总览与外部项目接入说明
├── skills.json                 # 根级技能注册清单（供外部项目 inherits 继承或全局挂载）
├── .agents/
│   └── skills.json             # 工作区自动发现配置
│
├── common/                     # 【通用/跨项目】工程规范、构建系统、Git、文档、静态分析
│   └── README.md
├── mcu-baremetal/              # 【单片机裸机】MCU 外设、寄存器/HAL、启动文件、链接脚本、IAP
│   └── README.md
├── rtos/                       # 【实时操作系统】FreeRTOS、ThreadX、RT-Thread、任务同步与中间件
│   └── README.md
├── linux-driver/               # 【Linux 驱动与系统】设备树 DTS、字符/总线驱动、U-Boot、根文件系统
│   └── README.md
├── protocols-debug/            # 【协议栈与联调测试】CAN/Modbus/USB/LwIP、JTAG/OpenOCD/GDB、故障分析
│   └── README.md
└── _templates/                 # 【模板库】新建 Skill 的标准骨架与示例
    └── skill-template/
        ├── SKILL.md
        ├── scripts/
        ├── references/
        ├── examples/
        └── resources/
```

---

## 2. 一级目录：技术方向边界说明

在创建或归档 Skill 时，严格按照以下技术职责放入对应的一级目录：

| 一级目录 | 技术方向 | 涵盖内容与典型场景 |
| :--- | :--- | :--- |
| [`common/`](./common/README.md) | **通用与工程基建** | 嵌入式 C/C++ 编码规范（如 MISRA-C）、CMake/Makefile/Kconfig 构建脚本编写、Git 提交与分支工作流、Doxygen/架构文档生成、Python 自动化辅助脚本、Clang-Tidy/Cppcheck 静态检查。 |
| [`mcu-baremetal/`](./mcu-baremetal/README.md) | **单片机裸机开发** | ARM Cortex-M / RISC-V 内核机制（NVIC、MPU、Cache、低功耗模式）、STM32/GD32/NXP 等芯片外设驱动（UART/I2C/SPI/ADC/TIM/DMA）、HAL/LL 库配置、Linker Script (`.ld`/`.sct`) 内存布局、Bootloader 与 OTA/IAP 升级、前后台状态机设计。 |
| [`rtos/`](./rtos/README.md) | **RTOS 实时系统** | **FreeRTOS**、**Azure RTOS (ThreadX)**、**RT-Thread**、**Zephyr** 等内核移植与配置；任务优先级规划、队列/信号量/互斥量/事件组设计；内存池管理；配套组件（FileX、NetX Duo、USBX、FreeRTOS+TCP、LwIP）集成；栈水位评估与死锁/优先级翻转排查。 |
| [`linux-driver/`](./linux-driver/README.md) | **Linux 驱动与系统** | Linux 内核模块开发、设备树（Device Tree `.dts`/`.dtsi`）编写与绑定校验、字符设备/Misc/Platform/I2C/SPI/Regmap/Pinctrl/GPIO/中断底半部（Workqueue/Tasklet/Threaded IRQ）驱动开发、DMA 与内存映射、U-Boot 移植、Yocto/Buildroot 构建、Kernel Oops/Panic/ftrace 调试。 |
| [`protocols-debug/`](./protocols-debug/README.md) | **现场总线、协议与硬件联调** | 工业与车载协议（CAN/CANopen/J1939、Modbus RTU/TCP、EtherCAT）、无线与接口协议（BLE、USB 协议栈）、GDB/OpenOCD/J-Link 脚本调试、HardFault/CmBacktrace 栈回溯分析、逻辑分析仪协议解码与功耗优化。 |

---

## 3. 使用范围（Scope）标识规范

在每个技术方向目录下，通过 **目录命名规范** 和 **`SKILL.md` 元数据** 区分该技能的使用范围：

### 3.1 四个使用范围层级

1. **`global`（全平台/通用级）**
   - **定义**：不绑定具体芯片型号或特定项目的通用方法论与工作流。
   - **命名建议**：直接使用功能名，如 `freertos-deadlock-diagnosis`、`linux-dts-binding-check`、`misra-c-review`。
2. **`vendor`（芯片原厂/架构级）**
   - **定义**：适用于特定芯片架构（如 Cortex-M7、RISC-V）或特定厂商产品线（如 STM32、NXP i.MX、Rockchip RK3588）。
   - **命名建议**：以厂商或架构作为前缀，如 `stm32-dma-uart-config`、`rk3588-v4l2-camera-driver`、`cortex-m-hardfault-analyzer`。
3. **`project`（板级 BSP / 项目专用级）**
   - **定义**：绑定特定硬件单板原理图、私有通信协议或特定业务项目的定制化技能。
   - **命名建议**：以 `proj-<项目代号>-` 或 `bsp-<板卡名>-` 作为前缀，如 `bsp-motor-ctrl-can-protocol`。
4. **`experimental`（实验/孵化级）**
   - **定义**：正在验证、尚未稳定或仅用于临时测试的技能。
   - **命名建议**：以 `exp-` 作为前缀，默认可在生产项目的 `skills.json` 中通过 `"exclude": ["exp-.*"]` 过滤。

---

## 4. 单个 Skill 的标准结构与编写准则

每个 Skill 必须是对应技术方向目录下的一个独立子文件夹，内部结构如下：

```text
<tech-direction>/<skill-name>/
├── SKILL.md          # 【必需】核心指令文件（含 YAML Frontmatter）
├── scripts/          # 【可选】可执行的辅助脚本（Python/Shell/OpenOCD/GDB 脚本等）
├── references/       # 【可选】芯片手册摘录、寄存器表、协议规范等长文档（按需读取，节省上下文）
├── examples/         # 【可选】标准驱动代码模板、设备树片段、配置文件范例
└── resources/        # 【可选】静态资源、链接脚本模板、头文件模板
```

### 4.1 `SKILL.md` Frontmatter 规范

`SKILL.md` 顶部必须包含标准 YAML Frontmatter，其中 `description` 决定了 Agent 何时激活该技能，必须用清晰的第三人称或触发条件描述：

```yaml
---
name: <skill-name>
description: >-
  [Scope: global|vendor|project] 简明描述该技能的作用及触发时机。
  例如：当用户需要编写、审查或调试 ThreadX 任务调度、内存池(Byte/Block Pool)或排查 ThreadX 栈溢出问题时使用此技能。
---
```

### 4.2 编写核心原则（嵌入式特别要求）

1. **渐进式披露（Progressive Disclosure）**：
   - 保持 `SKILL.md` 精炼（聚焦步骤、检查清单与决策树，建议控制在 200 行以内）。
   - 将冗长的寄存器定义、协议帧格式表、大型代码模板放入 `references/` 或 `examples/` 目录，并在 `SKILL.md` 中使用相对路径链接（如 `[寄存器映射表](./references/register_map.md)`），让 Agent 仅在需要时按需读取。
2. **安全与硬件约束优先**：
   - 涉及裸机、RTOS 与 Linux 内核驱动的 Skill，必须明确列出**并发安全（中断上下文 vs 任务上下文）**、**内存安全（DMA 缓存一致性 Cache Coherency、对齐要求、禁用动态分配场景）**及**错误处理（Goto 清理路径、超时退出机制）**检查项。
3. **可验证闭环**：
   - 每个 Skill 应包含明确的验证步骤（例如：交叉编译无警告、静态检查命令、`checkpatch.pl`、GDB/日志验证方法）。

---

## 5. Agent 维护本仓库的自动化守则

当用户要求在本仓库中**新增**或**整理** Skill 时，Agent 必须执行以下步骤：

1. **分类研判**：确认新 Skill 属于哪个一级技术方向（`common` / `mcu-baremetal` / `rtos` / `linux-driver` / `protocols-debug`）以及哪个使用范围（`global` / `vendor` / `project` / `experimental`）。
2. **模板初始化**：参考 [`_templates/skill-template/SKILL.md`](./_templates/skill-template/SKILL.md) 创建对应的 `<tech-direction>/<skill-name>/SKILL.md` 及必要的子目录。
3. **更新索引**：在对应技术方向目录下的 `README.md` 技能清单表格中追加该 Skill 的名称、范围标签与简介链接，保持目录索引同步。
4. **多工具同步挂载**：运行 `powershell -NoProfile -ExecutionPolicy Bypass -File .\sync-skills.ps1`，自动将该技能以目录联接（Junction）同步至 `~/.codex/skills`、`~/.cursor/skills`、`~/.agents/skills`（Grok Bot 等）与 `~/.claude/skills`。

---

## 6. 嵌入式开发全局通用行为准则（跨工具共享基线）

当本文件被挂载为全局规则（如 `~/.codex/AGENTS.md` 或跨项目引用）时，Agent 在回答和修改嵌入式固件代码时必须遵循以下准则：

1. **语言与 Git 提交规范**：
   - 默认使用中文交流与编写注释；Git Commit 标题必须使用简洁、明确的中文（能用一句话说清唯一目的，如 `修复DMX短帧发布条件`），正文详细说明修改内容、原因、影响范围、验证结果及未验证项。
   - 除非用户明确要求，不自动创建 Git Commit、不推送、不烧录固件、不擦除片内/片外 Flash；不将 `Objects/`、`*.hex`、`*.axf`、`*.bin`、`*.map`、`.lst` 等构建产物纳入版本控制。
2. **可靠性与资源优先**：
   - 优先保证可靠性、实时性、资源占用（Flash/RAM）和可维护性；避免使用 `malloc`/`free` 动态内存、变长数组、递归、复杂抽象和大规模依赖。
   - 修改现有工程前，先理解芯片型号、编译器版本（如 Keil ARMCC 5 / MPLAB XC8 / GCC）、时钟树、寄存器定义和编码风格；保持改动最小化，未经要求不更换工具链、不转换文件字符集编码（UTF-8/GBK）、不格式化无关文件。
3. **高风险硬件与并发铁律**：
   - 涉及 **中断(ISR)、并发、DMA、缓冲区、位操作、定时器、看门狗(IWDG/WDT)、通信帧、Flash/EEPROM、低功耗休眠唤醒** 时，重点执行 [`embedded-six-stage-workflow`](./common/embedded-six-stage-workflow/SKILL.md) 门禁检查，严查边界、溢出、竞态、原子性、无符号 Tick 超时、错误恢复和掉电风险。
   - 对寄存器、时序、电气特性或硬件连接无依据时，绝不编造或静默猜测；主动向用户索取原理图、数据手册或协议文档。
4. **提示词缺失反馈规范（最多 3 项）**：
   - 当任务目标/验收标准不明确、范围过大，或涉及中断/DMA/通信/Flash/寄存器但缺少关键硬件依据时，先简短提示最多 **3 个最关键缺失信息**，并给出一条可直接复制的优化后提示词；对复杂任务按“分析 → 方案确认 → 最小修改 → 验证”拆分推进（若用户明确要求“直接做”，则说明合理假设后直接执行）。
