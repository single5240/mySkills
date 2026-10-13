---
name: template-skill-name
description: >-
  [Scope: global | vendor | project] 用第三人称简明描述该技能的用途与触发条件。
  例如：当用户需要编写、配置或排查 <具体技术模块，如 STM32 DMA UART 收发 / ThreadX 内存池 / Linux I2C 设备树与驱动> 时激活此技能。
---

# <Skill 标题名称>

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`common` | `mcu-baremetal` | `rtos` | `linux-driver` | `protocols-debug`
- **使用范围 (Scope)**：`global`（全平台通用） | `vendor`（特定芯片/架构） | `project`（特定板卡/项目）
- **适用硬件/内核版本**：（如 ARM Cortex-M4/M7、FreeRTOS v10+、ThreadX 6.x、Linux Kernel 5.10+）

---

## 2. 核心执行流程 (Step-by-Step Workflow)

1. **环境与上下文确认**：
   - 检查目标芯片型号、内核/RTOS 版本、编译器工具链及现有头文件配置。
2. **代码实现 / 配置修改**：
   - 按照标准步骤生成或修改代码（详细代码范例可参考 `[examples/reference_impl.c](./examples/reference_impl.c)`）。
   - 若涉及复杂寄存器或协议规范，按需查阅 `[references/spec_notes.md](./references/spec_notes.md)`。
3. **关键安全与并发约束检查**：
   - [ ] **中断上下文安全**：确认在 ISR 中未调用阻塞型 API 或非可重入函数（如 FreeRTOS 必须使用 `FromISR` 后缀并处理 `pxHigherPriorityTaskWoken`；ThreadX 在 ISR 中等待参数必须为 `TX_NO_WAIT`；Linux 顶半部不可睡眠）。
   - [ ] **内存与 Cache 一致性**：若涉及 DMA 传输，确认缓冲区地址对齐（如 32-byte 对齐）及 D-Cache Clean/Invalidate 或非缓存区（MPU / `dma_alloc_coherent`）配置。
   - [ ] **错误处理与资源释放**：确认超时退出机制（避免死循环等待硬件标志位）及异常分支的资源清理（如 Linux 驱动的 `goto err_*` 或 `devm_*`）。

---

## 3. 验证与调试方法 (Verification)

1. **编译与静态检查**：
   - 执行构建命令并确保无新增 Warning。
2. **运行态验证**：
   - 列出预期日志输出、寄存器状态、设备节点（如 `/dev/xxx` 或 `/sys/kernel/debug/...`）或逻辑分析仪波形判据。
