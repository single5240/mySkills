---
name: keil-armcc-build-analyzer
description: >-
  [Scope: global] Keil MDK-ARM (ARMCC 5 / ARMClang 6) 命令行无头构建、.build_log.htm 警告分类审查与 .map 内存占用（Code/RO/RW/ZI 及栈水位）自动对比分析。
  当用户需要编译 .uvprojx 工程、排查 ARMCC 5 编译/链接警告（如 UTF-8/GBK 编码告警、C89/C99 混用）、或审计每次代码修改前后的 Flash/RAM 字节增量时使用此技能。
---

# Keil MDK 命令行构建与 `.map` 资源审计 (Keil ARMCC Build & Map Analyzer)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`common`
- **使用范围 (Scope)**：`global`（适用于所有使用 Keil MDK-ARM `.uvprojx` / `.uvproj` 的 MCU 工程）
- **适用工具链**：Keil $\mu$Vision (`UV4.exe`)，ARM Compiler 5 (`armcc` V5.06) 与 ARM Compiler 6 (`armclang`)。

---

## 2. 核心设计准则 (Core Principles)

1. **绝不盲目宣称“编译通过”或“零警告”**：
   - `UV4.exe -b` 或 `-r` 的进程退出码（Exit Code）：`0` 表示 0 Error / 0 Warning，`1` 表示仅存在 Warning（无 Error），`>= 2` 表示存在 Error。
   - **铁律**：退出码为 `1` 时虽然生成了 `.axf/.hex`，但必须逐行解析 `.build_log.htm` 区分**本次新增警告**与**历史遗留警告**（如历史 GBK/UTF-8 特殊字符警告），严禁新增任何警告，也严禁在存在历史警告时笼统汇报“零警告”。
2. **强制量化每次改动的 Flash / RAM 增量 (`Code / RO / RW / ZI`)**：
   - 在嵌入式裸机工程中新增缓冲、字库、状态表后，必须从 `.build_log.htm` 或 `.map` 提取四项核心指标并计算 $\Delta$：
     - **Flash 占用** $= \text{Code} + \text{RO-data} + \text{RW-data}$
     - **SRAM 静态占用** $= \text{RW-data} + \text{ZI-data}$
3. **保护 `.uvprojx` 与编译器基线**：
   - 未经用户明确授权，绝不将 ARMCC 5 (V5.06) 工程切换为 ARMClang 6，不修改优化等级（如 `-O1`）、芯片 Pack 版本或分散加载文件（`.sct`）。

> 可直接运行的构建日志与 `.map` 对比脚本见 [`scripts/analyze_keil_build.ps1`](./scripts/analyze_keil_build.ps1)（Windows 免依赖首选）与 [`scripts/analyze_keil_build.py`](./scripts/analyze_keil_build.py)；ARMCC 5 高频警告修复与 `.map` 符号审计指南见 [`references/armcc5-common-warnings-and-map.md`](./references/armcc5-common-warnings-and-map.md)。

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **修改前采集基线（Baseline Snapshot）**：
   - 修改代码前先运行分析脚本读取现有 `Objects/*.build_log.htm` 与 `Listings/*.map`（或 `Objects/*.map`），保存基线 JSON：
     ```powershell
     powershell -NoProfile -ExecutionPolicy Bypass -File "<skill-dir>/scripts/analyze_keil_build.ps1" -LogPath "N32G455/MDK-ARM/Objects/N32G455.build_log.htm" -SaveBaseline "baseline.json"
     ```
2. **执行 Keil 命令行无头构建（Headless Build）**：
   - 优先增量构建 `-b`（修改头文件或编译宏时使用全量重构 `-r`），指定 `-t <TargetName>` 与 `-j0 -o build_cmd.log`：
     ```powershell
     & "C:\Keil_v5\UV4\UV4.exe" -b "N32G455\MDK-ARM\N32G455.uvprojx" -t "N32G455" -j0 -o "build_output.log"
     ```
   - 若当前环境未安装 `UV4.exe`，必须明确向用户报告“当前环境无法运行 Keil 构建，仅完成静态检查”，绝不伪造编译结果。
3. **解析构建日志与对比资源增量**：
   - 运行 [`scripts/analyze_keil_build.ps1`](./scripts/analyze_keil_build.ps1) 对比修改前后的 `Code / RO-data / RW-data / ZI-data` 及警告列表：
     ```powershell
     powershell -NoProfile -ExecutionPolicy Bypass -File "<skill-dir>/scripts/analyze_keil_build.ps1" -LogPath "N32G455/MDK-ARM/Objects/N32G455.build_log.htm" -MapPath "N32G455/MDK-ARM/Listings/N32G455.map" -CompareBaseline "baseline.json"
     ```
4. **清理构建副产物（Git 保护）**：
   - 确认 `Objects/`、`Listings/`、`*.build_log.htm`、`*.dep`、`*.uvguix.*` 均未被 `git add` 暂存。

---

## 4. 关键安全与工具链检查清单

- [ ] **只读常量进 Flash (`RO-data`)**：新增字库、状态转移表、菜单配置表、CRC 查表必须加 `const`，并在 `.map` 中核对已落入 `RO-data` 而非 `RW-data`。
- [ ] **UTF-8 无 BOM 与 ARMCC 5 中文字符串**：ARMCC 5 在未加 `--locale=english` 或多字节字符跨界时易触发 `#870-D: invalid multibyte character sequence`；对 UI 字面量优先使用 `\xHH` 十六进制 UTF-8 转义或统一通过 `app_i18n` 字典表管理。
- [ ] **栈空间 (`STACK`) 与堆 (`HEAP`) 检查**：在 `.map` 中检查 `STACK` 段的基址与大小，确保 `ZI-data` 增长后不会挤压栈空间，且裸机工程 `HEAP` 设为 `0`（禁用 `malloc`）。

---

## 5. 验证与交付汇报模板 (Verification Report)

每次交付代码时，按以下标准格式输出构建与资源证据：
- **编译器版本**：`ARMCC V5.06 update 6 (build 750)`
- **编译结果**：`0 Error(s), 0 New Warning(s)`（如有历史警告，注明具体 `文件:行号` 与原因）
- **资源占用与增量**：
  - `Code`: `127,928 B (+1,796 B)`
  - `RO-data`: `34,272 B (+968 B)`
  - `RW-data`: `304 B (+0 B)`
  - `ZI-data`: `28,776 B (+16 B)`
  - **总 Flash**: `162,504 B` | **总静态 RAM**: `29,080 B`
