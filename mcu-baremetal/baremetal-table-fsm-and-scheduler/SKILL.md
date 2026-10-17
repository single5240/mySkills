---
name: baremetal-table-fsm-and-scheduler
description: >-
  [Scope: global] 零动态内存 C99 表驱动有限状态机（FSM）、主循环四级时间片协作调度器、静态环形事件总线与条带 UI 页面路由解耦架构。
  当用户需要设计或重构裸机主循环（Super-Loop）、消除 while(1) 周期抖动、将数千行巨石 UI/控制代码拆分为表驱动状态机与页面契约时使用此技能。
---

# 裸机表驱动状态机与主循环四级分频调度架构 (Table-Driven FSM & Cooperative Scheduler)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`mcu-baremetal`
- **使用范围 (Scope)**：`global`（全平台 C99 裸机通用，提炼自 N32G455 量产架构与重构方案）
- **适用硬件与场景**：无 RTOS 的单片机前后台工程（ARMCC 5 / ARMClang / GCC / XC8），严格禁止 `malloc/free` 动态内存，要求主循环单圈耗时确定（如 `< 2 ms`）且安全维护独立看门狗（IWDG）。

---

## 2. 四大核心架构组件

### 2.1 零动态内存表驱动状态机 (`state_machine.c/.h`)
- **四元组转移表**：`{ event, currentState, action, nextState }` 驻留 `const` Flash，运行期零 RAM 膨胀。
- **静态 FIFO 事件队列 + 空闲循环事件 (`loopEvents`)**：
  - 调用方提供静态数组作为事件队列；每次 `StateMachine_Process()` 最多消费 **1 个事件**，确保非阻塞。
  - 当事件队列为空时，自动取当前状态绑定的 `loopEvents[currentState]` 触发周期性背景动作，无需高频向队列塞入 Tick 事件。
  - **动作先于状态更新**：`action(machine, userContext)` 在 `currentState = nextState` 之前执行，便于动作函数读取转移前状态或投递后续事件。
- **初始化期全表自检 (`StateMachine_IsConfigValid`)**：
  - `StateMachine_Init()` 自动校验：状态/事件越界、重复的 `(currentState, event)` 转移项、以及每个状态是否都定义了合法的 `loopEvents` 转移。

> 可直接复制到任何 C99 工程的生产级源码见 [`examples/state_machine.h`](./examples/state_machine.h) 与 [`examples/state_machine.c`](./examples/state_machine.c)。

### 2.2 主循环四级分频时间片调度器（防回绕）
将平铺在 `main.c` `while(1)` 中的十几个 `*_Process()` 划分为 4 个确定性等级，并使用**无符号 32 位差值 `(uint32_t)(now - last) >= period`** 天然免疫 49.7 天 `SysTick` 溢出回绕：

| 调度等级 | 周期 | 典型模块 | 单次耗时预算与约束 |
| :--- | :---: | :--- | :--- |
| **Level 0: 硬实时输入** | **1 ms** | 按键扫描消抖 (`MultiButton`)、急停/接点检测 | `< 50 μs`，消除消抖累积误差 |
| **Level 1: 测控与算法** | **10 ms** | 传感器 FIFO 排空、峰值/破断状态机、蜂鸣器节拍 | `< 200 μs`，确定性控制步长 |
| **Level 2: 慢速状态量** | **100 ms** | 电池 ADC、方向 ADC、软件日历推进、PD0 心跳灯 | `< 100 μs`，低频轮询降噪 |
| **Level 3: 空闲分片任务** | **每轮 1 步** | LCD 8 行条带 DMA 渲染、Flash 非阻塞擦写轮询、RTT CLI | 每次调用仅推进 1 个状态步或 1 个条带 |

### 2.3 静态环形事件总线 (`AppEventQueue`)
- 开辟固定 16 槽（约 128B SRAM）的无锁环形队列 `{ AppEventId id; uint16_t param1; uint32_t param2; }`。
- 按键、超载保护、存储完成等生产者仅调用 `AppEvent_Post()`，由顶层 UI 路由器或业务控制器统一 `AppEvent_Poll()` 消费，彻底斩断模块间的跨层直接调用与全局变量耦合。

### 2.4 巨石 UI 物理拆分与页面生命周期契约 (`UiPageInterface`)
当单一 UI 文件超过 1,000 行时，按 **`Engine`（条带画布与图元） + `Router`（顶层状态机与公共状态栏） + `Page_*`（独立页面）** 拆分，所有页面实现统一契约：
```c
typedef struct {
    void (*OnEnter)(void);
    bool (*OnEvent)(AppUiEventType event);
    void (*OnDrawStrip)(uint16_t stripY, uint16_t stripHeight);
    void (*OnExit)(void);
} UiPageInterface;
```

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **上电初始化顺序门禁**：
   - 严格遵循：`系统时钟/NVIC` $\rightarrow$ `无阻塞基础外设 Init` $\rightarrow$ `含长阻塞的 LCD Init / Flash 上电扫描` $\rightarrow$ `高频连续采样 DMA Start (如 AD7190)` $\rightarrow$ **最后启动 `Iwdg_Init()`**。
2. **接入表驱动状态机**：
   - 将复杂的 `switch-case` 嵌套业务改写为 `const StateMachineTransition` 表；在模块初始化时断言 `StateMachine_Init(...) == true`。
3. **收敛唯一安全喂狗点**：
   - 全工程仅在 `main.c` 主循环末尾调用一次 `Iwdg_Feed()`；严禁在定时器 ISR、外设驱动死等循环或 UI 绘制中途喂狗。

---

## 4. 关键安全与并发约束检查清单

- [ ] **单上下文约定**：`StateMachine_PostEvent()` 默认在主循环同一执行上下文调用；若需从 ISR 投递事件，必须在 `PostEvent` 与 `TakeEvent` 的 `eventCount` 读改写处加极短关中断临界区。
- [ ] **禁止长阻塞潜入分片任务**：Level 3 空闲分片任务（如 Flash 擦除或列表扫描）单次 `Process()` 严禁包含 `while(busy)`，必须拆成 `START -> POLL -> VERIFY` 状态步。

---

## 5. 验证与调试方法 (Verification)

1. **状态机配置自检**：验证 `StateMachine_Init()` 对故意构造的重复转移或缺失 `loopEvents` 返回 `false`。
2. **主循环周期抖动实测**：通过 RTT `stat` 命令（或翻转测试 GPIO）观测高负载（边刷屏边写 Flash）下的主循环最大周期，确认远低于看门狗超时阈值。
