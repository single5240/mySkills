# Common — 跨领域通用与工程基建技能 (`common/`)

本目录仅存放**不绑定特定技术栈（跨前后端与嵌入式通用）**的工程规则、技术决策记录、可视化表达与开发环境基础技能。

> **分类边界说明**：
> - Web 前端相关技能请归入 [`web-frontend/`](../web-frontend/README.md)
> - 后端服务相关技能请归入 [`backend-service/`](../backend-service/README.md)
> - MCU 固件门禁、Keil 构建、主机回归与裸机架构技能请归入 [`mcu-baremetal/`](../mcu-baremetal/README.md)

## 适用场景
- **Agent 规则与知识沉淀**：项目 `AGENTS.md` 质量约束审查、精简与跨工具规则提炼；关键技术决策笔记（ADR）归档。
- **可视化与文档表达**：架构图、协议时序、状态机与方案对比的交互式 HTML 看板或视频生成。
- **开发环境与网络基建**：AI 编程工具（Antigravity / Cursor / Codex / Copilot）网络分流与代理排障。

## 使用范围（Scope）命名约定
- **`global`**（默认无前缀）：全领域通用，如 `agents-md-quality-constraints`、`decision-notes`。
- **`project`**（`proj-<name>-*`）：特定团队或本机环境的工程配置规范。
- **`experimental`**（`exp-*`）：孵化中的通用自动化脚本或工作流。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 简述 |
| :--- | :--- | :--- |
| [`agents-md-quality-constraints`](./agents-md-quality-constraints/SKILL.md) | `global` | 审查/精简 AGENTS.md，保留不变量、风险升级与验证入口；支持提炼规则到 Skill 并保留项目事实 |
| [`decision-notes`](./decision-notes/SKILL.md) | `global` | 精简记录有长期价值的技术决策：先查旧记录，保留约束、真实备选、代价与验证依据，沿用项目文档结构 |
| [`answer-me-with-html`](./answer-me-with-html/SKILL.md) | `global` | 借助 `am.mjs` 将复杂架构、通信协议时序、状态机或方案对比渲染为单页交互式 HTML 图解看板或解说视频 |
| [`clash-tun-antigravity`](./clash-tun-antigravity/SKILL.md) | `project` | Clash Verge 的 AI-Code 前置分流（反重力 / Cursor / Codex / Copilot），以及用 TUN 加规则模式修复反重力直连 Google 超时 |
