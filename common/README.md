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
| [`embedded-six-stage-workflow`](./embedded-six-stage-workflow/SKILL.md) | `global` | 嵌入式固件研发六阶段门禁工作流（Brainstorm → Design → Plan → Execute → Verify → Ship）与中断/DMA/看门狗/Flash 高风险改动安全检查清单 |
| [`answer-me-with-html`](./answer-me-with-html/SKILL.md) | `global` | 借助 `am.mjs` 将复杂架构、通信协议时序、状态机或方案对比渲染为单页交互式 HTML 图解看板或解说视频 |
