---
name: jlink-rtt-cli-hil-validation
description: >-
  [Scope: global] 基于 J-Link SEGGER RTT 的零串口占用表驱动 CLI、DWT 主循环/栈水位性能探针与非阻塞板级故障注入（HIL）自动化验收。
  当用户需要在 Cortex-M 单片机上通过 RTT 建立交互式调试命令台、测量主循环单圈极值耗时与外设丢包率、或在实板上对 Flash/传感器执行非阻塞故障注入与断电恢复验证时使用此技能。
---

# J-Link RTT 表驱动 CLI 与板级故障注入自测 (J-Link RTT CLI & HIL Validation)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`protocols-debug`
- **使用范围 (Scope)**：`global`（全平台 Cortex-M 通用，提炼自 N32G455 量产验证工程）
- **适用硬件与工具**：ARM Cortex-M0/M3/M4/M7 + J-Link 调试器（SWD 接口）+ SEGGER RTT 组件，无需占用任何硬件 UART 引脚。

---

## 2. 核心设计模式 (Core Patterns)

1. **零动态内存、单层表驱动 RTT CLI (`app_rtt_cli`)**：
   - 在主循环中非阻塞轮询 `SEGGER_RTT_ReadNoLock(0, ...)`（每次最多读 32 字节），组装以 `\r` 或 `\n` 结尾的命令行（静态缓冲 128B，超长行自动丢弃防溢出）。
   - 切分首词（Token）后在 `const AppRttCliCommand s_commands[]` 静态表中匹配分发，新增/裁剪命令仅需修改一张表。
   - 每次响应前置打印系统运行毫秒时间戳 `[HH:MM:SS.mmm]`，便于上位机日志对齐。
2. **DWT 周期级主循环耗时与硬件健康度探针 (`stat` 命令)**：
   - 在 `main.c` 的 `while(1)` 首尾调用 `AppRttCli_NoteMainLoopStart()` 与 `AppRttCli_NoteMainLoopEnd()`，利用 Cortex-M `DWT->CYCCNT` 硬件周期计数器统计主循环**最小/最大/平均执行耗时与调度周期**。
   - 结合栈涂漆（Stack Painting）水位检测、IWDG 标志以及高频外设（如 AD7190 `rdy/dma/error/overrun/recovery`）快照差值，一键输出压力测试期间的实时性证据。
3. **编译宏隔离的协作式板级故障注入状态机 (`*_VALIDATION_ENABLE`)**：
   - 破坏性或大体积验证代码（批量写入、持久化边界软复位注入、擦除超时/读回错误注入）置于独立模块（如 `app_record_store_test.c`），由 `#define APP_XXX_VALIDATION_ENABLE 0` 默认关闭。
   - **严禁在 RTT 命令回调里用死循环跑完长耗时测试**：RTT 命令仅启动测试状态机并返回 `PENDING`；由主循环每轮推进一小步，完成后打印确定性的 `PASS: <case>` 或 `FAIL: <case> reason=...` 终态日志。

> 可直接复用的表驱动 CLI 与 DWT 探针模板见 [`examples/rtt_cli_template.c`](./examples/rtt_cli_template.c)；板级 HIL 验收流程与持久化边界故障注入规范见 [`references/hil-validation-protocol.md`](./references/hil-validation-protocol.md)。

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **接入 SEGGER RTT 与非阻塞 CLI**：
   - 在 `SysTick` 初始化后调用 `SEGGER_RTT_Init()` 与 `AppRttCli_Init()`。
   - 在主循环中调用 `AppRttCli_Process()`；确保所有 RTT 输出均在主循环上下文执行，**禁止在高频 ISR 中调用 `SEGGER_RTT_printf`**。
2. **植入主循环性能探针与安全复位 (`reboot`)**：
   - 在主循环业务开始前记录 `DWT->CYCCNT`，在 `Iwdg_Feed()` 前记录结束周期。
   - 实现 `reboot` 命令：收到命令后设置 `s_rebootPending = 1`，**必须等待 Flash/EEPROM 等持久化外设处于 IDLE 状态且延迟 100 ms 让 RTT 排空输出缓冲后**，再调用 `NVIC_SystemReset()`。
3. **编写分级板级验证状态机**：
   - **Level 1（纯 RAM 无损自检）**：如 `codec` 命令，验证正反序列化固定字节向量、CRC 篡改、越界参数，不触碰真实 Flash。
   - **Level 2（非阻塞功能与压力验证）**：如 `append 1000`、`list_check`、`full_probe`，分步执行并支持 `stop` 在安全边界中止。
   - **Level 3（持久化边界软复位与故障注入）**：在两阶段提交或扇区迁移的精确边界点（如 `MIGRATE_BEGIN` 刚提交、目标槽刚写完、源槽刚标 `MIGRATED`）触发自动软复位，重启后执行 `verify_after_reboot` 校验自动续作结果。

---

## 4. 关键安全与并发约束检查清单

- [ ] **生产固件零污染**：交付前必须将 `*_VALIDATION_ENABLE` 恢复为 `0` 并重新全量编译，确认 `record_test` 等命令仅返回 `no support now`，且验证代码的 `Code/ZI` 占用已剥离。
- [ ] **破坏性擦写双门禁**：任何会擦除 Flash 分区的测试命令（如 `cleanup`）执行前，必须在文档和交互中确认目标分区无用户生产数据并已获擦写授权。
- [ ] **命令接受 != 测试通过**：上位机或测试人员发送异步验证命令后，必须等待终态日志 `PASS:` / `FAIL:`，且检查 `stat` 中 `iwdg_reset == 0`、`adc_overrun == 0`、`adc_recovery == 0`。

---

## 5. 验证与调试方法 (Verification)

1. **基线健康检查命令序列**：
   ```text
   ver
   help
   stat reset
   stat
   ```
2. **通过判据**：
   - `ver` 输出正确的编译日期时间与 `SystemCoreClock`（如 `128000000`）。
   - 高负载测试后执行 `stat`：主循环最大耗时远小于看门狗窗口（如 `< 10 ms` vs `1000 ms` IWDG），栈最低剩余水位 `> 512 B`，外设错误/溢出计数为 `0`。
