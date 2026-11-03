# Full-Stack & Embedded Skills Repository (`AGENTS.md`)

本仓库是集中管理所有 AI Agent Skills 的知识与工作流中心，面向**跨领域通用基建**、**Web 前端与后端服务**以及**嵌入式系统开发（单片机裸机、RTOS、Linux 驱动与通信联调）**。

当 Agent 在本仓库中工作（新增、修改、检索或维护 Skill）或将本仓库作为技能源加载时，必须遵循本文件定义的目录架构、分类准则与编写规范。

---

## 1. 目录架构设计（一级按技术方向隔离，内部结合使用范围）

本仓库采用 **“一级目录按技术方向严格隔离（通用 / 前端 / 后端 / MCU / RTOS / Linux / 协议调试），内部通过标签与前缀区分使用范围”** 的组织方式。此结构既保证前后端开发与 MCU 固件开发互不混杂，又兼容 Antigravity `skills.json` 的单层扫描机制，方便不同项目按需精准挂载。

```text
e:/data/doc/skills/
├── AGENTS.md                   # 仓库全局 Agent 规则与维护指南（本文件）
├── README.md                   # 技能库总览与外部项目接入说明
├── skills.json                 # 根级技能注册清单（供外部项目 inherits 继承或全局挂载）
├── sync-skills.ps1             # 一键同步所有分类下技能到 Codex / Cursor / .agents / Claude
├── .agents/
│   └── skills.json             # 工作区自动发现配置
│
├── common/                     # 【跨领域通用】不绑定特定技术栈：AGENTS.md 约束、决策笔记、HTML 图解、代理分流
│   └── README.md
├── web-frontend/               # 【Web 前端】Vue 3 + TS + Vite 管理端开发、异步状态、Vitest 单元测试与链路验证
│   └── README.md
├── backend-service/            # 【后端服务】Spring Boot 多模块后端、事务/Redis 一致性、服务端 MQTT 与并发验证
│   └── README.md
├── mcu-baremetal/              # 【单片机裸机与固件工程】六阶段门禁、Keil 构建、主机回归、外设驱动、掉电存储、状态机
│   └── README.md
├── rtos/                       # 【实时操作系统】FreeRTOS、ThreadX、RT-Thread、任务同步与中间件
│   └── README.md
├── linux-driver/               # 【Linux 驱动与系统】设备树 DTS、字符/总线驱动、U-Boot、根文件系统
│   └── README.md
├── protocols-debug/            # 【协议栈与联调测试】串口 DMA 拆包、CAN/Modbus、J-Link RTT/HIL、HardFault 解码
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

在创建或归档 Skill 时，严格按照以下技术职责放入对应的一级目录，**禁止将 Web 前后端技能或 MCU 专用工具混入 `common/`**：

| 一级目录 | 技术方向 | 涵盖内容与典型场景 |
| :--- | :--- | :--- |
| [`common/`](./common/README.md) | **跨领域通用基建** | 仅限所有工程共用的方法与工具：`AGENTS.md` 质量约束审查、技术决策记录 (`decision-notes`)、交互式 HTML 看板/视频生成 (`answer-me-with-html`)、AI 编程工具代理分流 (`clash-tun-antigravity`)。 |
| [`web-frontend/`](./web-frontend/README.md) | **Web 前端开发与测试** | Vue 3 / TypeScript / Vite / Element Plus 管理后台改动工作流 (`vue-admin-change-workflow`)、前端分层测试与 Vitest 单元测试规范 (`frontend-testing-workflow`)、组件/路由/代理排障。 |
| [`backend-service/`](./backend-service/README.md) | **后端服务与中间件** | Spring Boot / Java 多模块后端变更工作流 (`spring-backend-change-workflow`)、接口鉴权、数据库事务与 Redis 一致性、服务端 MQTT 消息发布/应答幂等与集成测试。 |
| [`mcu-baremetal/`](./mcu-baremetal/README.md) | **单片机裸机与固件工程** | 嵌入式固件六阶段门禁 (`embedded-six-stage-workflow`)、Keil MDK 构建与 `.map` 审计 (`keil-armcc-build-analyzer`)、主机 C/C++ 逻辑回归 (`embedded-host-regression`)、LCD 固件网页模拟器 (`embedded-lcd-web-simulator`)、ARM Cortex-M / RISC-V 外设驱动、Flash/EEPROM 掉电安全存储、表驱动状态机与菜单架构。 |
| [`rtos/`](./rtos/README.md) | **RTOS 实时系统** | **FreeRTOS**、**Azure RTOS (ThreadX)**、**RT-Thread**、**Zephyr** 等内核移植与配置；任务优先级规划、队列/信号量/互斥量/事件组设计；内存池管理；栈水位评估与死锁/优先级翻转排查。 |
| [`linux-driver/`](./linux-driver/README.md) | **Linux 驱动与系统** | Linux 内核模块开发、设备树（Device Tree `.dts`/`.dtsi`）编写与绑定校验、字符设备/Platform/I2C/SPI/Regmap/Pinctrl/GPIO/中断驱动、DMA 与内存映射、U-Boot 移植、Yocto/Buildroot 构建、Kernel Oops/ftrace 调试。 |
| [`protocols-debug/`](./protocols-debug/README.md) | **现场总线、协议与硬件联调** | 串口 `IDLE + DMA` 帧解析与 RS-485/DMX512/Modbus、CAN/CANopen、J-Link RTT 交互式 CLI 与 HIL 板级故障注入验收、Cortex-M `HardFault` 栈帧与 `.map` 解码、逻辑分析仪协议解码。 |

---

## 3. 使用范围（Scope）标识规范

在每个技术方向目录下，通过 **目录命名规范** 和 **`SKILL.md` 元数据** 区分该技能的使用范围：

### 3.1 四个使用范围层级

1. **`global`（全平台/该方向通用级）**
   - **定义**：在该技术方向内不绑定单一芯片型号或单一业务项目的通用方法论与工作流。
   - **命名建议**：直接使用功能名，如 `vue-admin-change-workflow`、`frontend-testing-workflow`、`spring-backend-change-workflow`、`baremetal-table-fsm-and-scheduler`。
2. **`vendor`（芯片原厂/特定框架级）**
   - **定义**：适用于特定芯片架构（如 Cortex-M4、N32G455）或特定硬件外设型号（如 AD7190）。
   - **命名建议**：以厂商、芯片或器件型号作为前缀，如 `n32g455-lcd-async-render`、`ad7190-exti-spi-dma-acquisition`。
3. **`project`（板级 BSP / 项目专用级）**
   - **定义**：绑定特定硬件单板、私有通信协议或特定本机环境的定制化技能。
   - **命名建议**：以 `proj-<项目代号>-` 或 `bsp-<板卡名>-` 作为前缀（或标注 `[Scope: project]`）。
4. **`experimental`（实验/孵化级）**
   - **定义**：正在验证、尚未稳定或仅用于临时测试的技能。
   - **命名建议**：以 `exp-` 作为前缀，默认在 `skills.json` 中通过 `"exclude": ["exp-.*"]` 过滤。

---

## 4. 单个 Skill 的标准结构与编写准则

每个 Skill 必须是对应技术方向目录下的一个独立子文件夹，内部结构如下：

```text
<tech-direction>/<skill-name>/
├── SKILL.md          # 【必需】核心指令文件（含 YAML Frontmatter）
├── agents/           # 【可选】Agent UI 展示配置（如 openai.yaml）
├── scripts/          # 【可选】可执行的辅助脚本（Python/Shell/OpenOCD/GDB 脚本等）
├── references/       # 【可选】长篇规范、寄存器表、测试模式、协议说明（按需读取，节省上下文）
├── examples/         # 【可选】标准代码模板、测试用例范例、配置文件范例
└── resources/        # 【可选】静态资源、报告模板、头文件模板
```

### 4.1 `SKILL.md` Frontmatter 规范

`SKILL.md` 顶部必须包含标准 YAML Frontmatter，其中 `description` 决定了 Agent 何时激活该技能，必须用清晰的第三人称或触发条件描述：

```yaml
---
name: <skill-name>
description: >-
  [Scope: global|vendor|project] 简明描述该技能的作用及触发时机。
---
```

### 4.2 编写核心原则

1. **渐进式披露（Progressive Disclosure）**：
   - 保持 `SKILL.md` 精炼（聚焦步骤、检查清单与决策树，建议控制在 200 行以内）。
   - 将长篇代码模式、寄存器定义、协议帧格式表放入 `references/` 或 `examples/` 目录，并在 `SKILL.md` 中使用相对路径链接，让 Agent 仅在需要时按需读取。
2. **领域边界与安全约束优先**：
   - **Web 前端 / 后端 Skill**：明确状态归属、API 契约、权限校验边界、事务/缓存一致性与证据分层（不把静态/Mock 通过冒充为真实服务或硬件通过）。
   - **裸机 / RTOS / Linux 驱动 Skill**：明确列出**并发安全（中断上下文 vs 任务上下文）**、**内存安全（DMA 缓存一致性、对齐要求、禁用动态分配场景）**及**错误处理（超时退出、掉电恢复）**检查项。
3. **可验证闭环**：
   - 每个 Skill 应包含明确的验证步骤（例如：Vitest 单元测试、类型检查、交叉编译无警告、`.map` 审计、J-Link RTT 日志验证方法）。

---

## 5. Agent 维护本仓库的自动化守则

当用户要求在本仓库中**新增**或**整理** Skill 时，Agent 必须执行以下步骤：

1. **分类研判**：确认新 Skill 属于哪个一级技术方向（`common` / `web-frontend` / `backend-service` / `mcu-baremetal` / `rtos` / `linux-driver` / `protocols-debug`）以及哪个使用范围（`global` / `vendor` / `project` / `experimental`）。
2. **模板初始化**：参考 [`_templates/skill-template/SKILL.md`](./_templates/skill-template/SKILL.md) 创建对应的 `<tech-direction>/<skill-name>/SKILL.md` 及必要的子目录。
3. **更新索引**：在对应技术方向目录下的 `README.md` 技能清单表格中追加该 Skill 的名称、范围标签与简介链接，保持目录索引同步。
4. **多工具同步挂载**：运行 `powershell -NoProfile -ExecutionPolicy Bypass -File .\sync-skills.ps1`，自动将该技能以目录联接（Junction）同步至 `~/.codex/skills`、`~/.cursor/skills`、`~/.agents/skills` 与 `~/.claude/skills`。

---

## 6. 跨项目通用行为准则

1. **语言与 Git 提交规范**：
   - 默认使用中文交流与编写注释；Git Commit 标题必须使用简洁、明确的中文（能用一句话说清唯一目的），正文详细说明修改内容、原因、影响范围、验证结果及未验证项。
   - 除非用户明确要求，不自动创建 Git Commit、不推送、不执行生产发布、不烧录固件或擦除真实设备存储；不将构建产物、缓存或日志纳入版本控制。
2. **嵌入式项目特别铁律**：
   - 涉及 **中断(ISR)、并发、DMA、缓冲区、位操作、定时器、看门狗(IWDG/WDT)、通信帧、Flash/EEPROM、低功耗休眠唤醒** 时，重点执行 [`embedded-six-stage-workflow`](./mcu-baremetal/embedded-six-stage-workflow/SKILL.md) 门禁检查。
   - 对寄存器、时序、电气特性或硬件连接无依据时，绝不编造或静默猜测；主动向用户索取原理图、数据手册或协议文档。
3. **提示词缺失反馈规范（最多 3 项）**：
   - 当任务目标/验收标准不明确、范围过大，或缺少关键业务契约/硬件依据时，先简短提示最多 **3 个最关键缺失信息**，并给出一条可直接复制的优化后提示词；可安全推进的部分继续执行。
