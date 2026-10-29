# Common — 通用与工程基建技能 (`common/`)

本目录存放跨项目、跨平台的通用软件工程与嵌入式基建技能。

## 适用场景
- **编码规范与静态检查**：嵌入式 C/C++ 编码规范（MISRA-C、BARR-C）、Clang-Tidy / Cppcheck 规则审查、头文件卫哨与模块解耦。
- **构建系统与工具链**：CMake、Makefile、Kconfig、交叉编译工具链（`arm-none-eabi-gcc`、`aarch64-linux-gnu-gcc`）配置与构建排错。
- **研发工作流**：Git 提交信息规范、分支管理、Changelog 生成、代码审查（Code Review）清单。
- **文档与设计**：模块架构设计文档、Doxygen 注释生成、时序图/状态机 Mermaid 图表绘制。

## 使用范围（Scope）命名约定
- **`global`**（默认无前缀）：全栈/全嵌入式通用，如 `misra-c-review`、`cmake-cross-compile`、`git-commit-workflow`。
- **`project`**（`proj-<name>-*`）：特定团队或项目的工程构建/发布规范。
- **`experimental`**（`exp-*`）：孵化中的通用自动化脚本或工作流。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 简述 |
| :--- | :--- | :--- |
| [`decision-notes`](./decision-notes/SKILL.md) | `global` | 精简记录有长期价值的技术决策：先查旧记录，保留约束、真实备选、代价与验证依据，沿用项目文档结构 |
| [`embedded-six-stage-workflow`](./embedded-six-stage-workflow/SKILL.md) | `global` | 嵌入式固件六阶段门禁与高风险检查；含最小实现、校验归属、资源和交付约束 |
| [`agents-md-quality-constraints`](./agents-md-quality-constraints/SKILL.md) | `global` | 审查/精简 AGENTS.md，保留不变量、风险升级与验证入口；支持提炼规则到 Skill 并保留项目事实 |
| [`keil-armcc-build-analyzer`](./keil-armcc-build-analyzer/SKILL.md) | `global` | Keil MDK (`UV4.exe` / ARMCC 5) 命令行无头构建、`.build_log.htm` 警告分类审查与 `.map` 资源占用 (`Code/RO/RW/ZI`) 自动对比分析 |
| [`embedded-lcd-web-simulator`](./embedded-lcd-web-simulator/SKILL.md) | `global` | 将嵌入式 LCD/OLED 固件的 C 点阵字库 (`font_*.c`)、RGB565 界面、实体键短/长按与测量/校准状态机 1:1 复刻为免服务器纯前端互动演示网页 |
| [`embedded-host-regression`](./embedded-host-regression/SKILL.md) | `global` | 使用当前生产源码建立主机逻辑回归，覆盖虚拟时间、双端协议与故障恢复；提供报告模板和自动摘要，区分目标构建与实板证据 |
| [`answer-me-with-html`](./answer-me-with-html/SKILL.md) | `global` | 借助 `am.mjs` 将复杂架构、通信协议时序、状态机或方案对比渲染为单页交互式 HTML 图解看板或解说视频 |
| [`clash-tun-antigravity`](./clash-tun-antigravity/SKILL.md) | `project` | Clash Verge 的 AI-Code 前置分流（反重力 / Cursor / Codex / Copilot），以及用 TUN 加规则模式修复反重力直连 Google 超时 |
