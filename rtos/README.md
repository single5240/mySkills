# RTOS — 实时操作系统开发技能 (`rtos/`)

本目录存放面向 **FreeRTOS**、**Azure RTOS (ThreadX)**、**RT-Thread**、**Zephyr** 等实时操作系统的内核移植、多任务架构设计、组件集成与调优技能。

## 适用场景
- **FreeRTOS 开发**：任务创建与静态/动态内存分配（`heap_1` ~ `heap_5`）、队列（Queue）、二值/计数信号量、互斥量（Mutex 优先级继承）、事件组（Event Group）、任务通知（Task Notification）、软件定时器、`FreeRTOSConfig.h` 裁剪、中断安全 API（`FromISR`）规范。
- **ThreadX (Azure RTOS) 开发**：线程（Thread）创建与抢占阈值（Preemption-Threshold）、字节池（Byte Pool）与块池（Block Pool）管理、消息队列、事件标志组、互斥锁、应用定时器，以及生态组件 **FileX**、**NetX Duo**、**USBX**、**GUIX** 的集成与移植。
- **RT-Thread / Zephyr 等其他 RTOS**：设备驱动框架（I/O Device）、FinSH 控制台、软件包管理、Devicetree 与 Kconfig 配置。
- **RTOS 稳定性与性能分析**：任务栈水位（High Water Mark）测算与栈溢出检测、死锁（Deadlock）与优先级翻转排查、CPU 利用率统计（Run Time Stats / Tracealyzer / SystemView 埋点）。

## 使用范围（Scope）命名约定
- **`global`**（以 RTOS 名称或通用概念开头）：如 `freertos-isr-safety-check`、`threadx-memory-pool-design`、`rtos-priority-planning`。
- **`vendor`**（结合芯片平台）：如 `stm32-threadx-netxduo-eth`、`cortex-m-freertos-porting`。
- **`project`**（`proj-<name>-*`）：特定产品的多任务划分架构与业务通信矩阵。
- **`experimental`**（`exp-*`）：试验性调度策略或新组件移植。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 目标 RTOS | 简述 |
| :--- | :--- | :--- | :--- |
| *(待添加)* | `global` / `vendor` | FreeRTOS / ThreadX / RT-Thread | 将新技能置于 `rtos/<skill-name>/SKILL.md` 并在此登记 |
