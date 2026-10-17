---
name: ad7190-exti-spi-dma-acquisition
description: >-
  [Scope: vendor] AD7190 等 24-bit Σ-Δ 高精度 ADC 的 DOUT/RDY 引脚 EXTI+SPI MISO 双模复用、SPI DMA 连续采样、无锁环形队列与 40-bit 自动软复位自愈。
  当用户需要在 N32G455/STM32 等 MCU 上驱动 AD7190/AD7192/ADS1256、处理 DOUT/RDY 共脚中断竞态、或排查高采样率下 SPI DMA 超时与总线死锁时使用此技能。
---

# AD7190 DOUT/RDY 复用中断 + SPI DMA 连续采集与自愈 (AD7190 EXTI + SPI DMA Acquisition)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`mcu-baremetal`
- **使用范围 (Scope)**：`vendor`（适用于 ADI AD7190/AD7192/AD779x 系列 + N32G455/STM32 Cortex-M MCU）
- **典型硬件配置**：SPI Mode 3（`CPOL=1, CPHA=1`），软件片选 `CS`；`DOUT/RDY` 共用同一引脚（同时接 SPI `MISO` 与 `EXTI` 下降沿中断）；SPI RX/TX 绑定双通道 DMA（高抢占优先级，如优先级 1）。

---

## 2. 核心硬件痛点与解决架构

1. **`DOUT/RDY` 引脚双模复用竞态控制**：
   - AD7190 在 Continuous Read（`0x5C`）模式下，转换完成时将 `DOUT/RDY` 拉低作为就绪中断信号；一旦主机发送 SCLK，该脚立即切换为 `SPI MISO` 输出 24-bit 数据 + 8-bit 状态字节（共 4 字节）。
   - **铁律**：在 `SPI MISO` 输出数据比特流期间，引脚会产生大量高低电平跳变，**若不关闭 EXTI，会引发灾难性的中断风暴与重入**！
   - **标准状态流**：
     - **等待就绪态**：`CS` 保持低电平，开启 `EXTI` 下降沿中断，`dma_busy = 0`。
     - **`EXTI` 触发 ISR**：确认 `DOUT/RDY` 为低电平且 `dma_busy == 0` $\rightarrow$ **立即在 ISR 中屏蔽本通道 `EXTI` 中断并清挂起位** $\rightarrow$ 启动 4 字节 SPI RX/TX DMA（TX 发送 `0x00` 产生 32 个时钟）。
     - **DMA RX 完成 ISR**：停用 SPI DMA 请求，拼装 24-bit 样本与 8-bit 状态字节压入 `256` 槽无锁环形队列 $\rightarrow$ 清除传输期间残留的 `EXTI` 挂起标志 $\rightarrow$ **重新使能 `EXTI` 下降沿中断**等待下一帧。
2. **5 ms DMA 超时监控与 40-bit 连续高电平自动恢复**：
   - 工业现场静电（ESD）或电源毛刺可能导致 AD7190 串行接口失步或 DMA 半途卡死。
   - 主循环 `ad7190_service()` 每轮检查：若 `dma_busy` 持续超过 `5 ms` 或发生 `DMA_ERR`，立即触发自愈序列：
     1. 屏蔽 `EXTI`，停止 SPI RX/TX DMA 并拉高 `CS`；
     2. 轮询发送 **至少 5~6 字节 `0xFF`（$\ge 40$ 个连续高电平 SCLK）** 软复位 AD7190 串行接口；
     3. 等待 `1 ms` 后读取并校验 `ID` 寄存器（`chip_id & 0x0FU == 0x04U`）；
     4. 重新写入缓存的 `GPOCON`、`CONF`、`MODE` 寄存器，重新发送 `0x5C` 进入连续读并恢复 `EXTI`。
3. **共享中断向量 (`EXTI15_10_IRQn`) 与低优先级 DMA 隔离**：
   - 若 `EXTI14`（AD7190）与 `EXTI11`（如 LTC4150 电量计）共用 `EXTI15_10_IRQn`，中断优先级只能在板级 `NVIC_Configuration()` 统一配置一次，驱动内只改写 `EXTI->IMASK` 位，绝不重配 NVIC 或误清兄弟通道标志。
   - AD7190 DMA（如 DMA1 CH4/CH5，抢占优先级 1）必须高于刷屏等大块慢速 DMA（如 LCD SPI1 DMA，抢占优先级 3）。

> 寄存器位域定义、2402 Hz 采样率计算与恢复时序详见 [`references/ad7190-register-and-timing.md`](./references/ad7190-register-and-timing.md)。

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **分阶段上电初始化（防启动期队列溢出）**：
   - 在 `main()` 前半段调用 `AppAd7190_Init()`：仅完成 GPIO/SPI2/DMA 初始化、40-bit 软复位、ID 校验与寄存器写入，**暂不开启连续读取**。
   - 等 LCD 初始化、Flash 扫描等所有长耗时阻塞初始化全部结束后，在进入 `while(1)` 与启动 `IWDG` 前一刻再调用 `AppAd7190_Start()`，避免初始化阻塞期间样本塞满环形队列导致 `overrun`。
2. **主循环排空与去皮/标定解算 (`AppAd7190_Process`)**：
   - 主循环每次调用 `ad7190_service()`（处理超时恢复），随后循环调用 `ad7190_pop_sample()` 排空环形队列。
   - 检查样本 `status` 字节：丢弃 `ERR` 或 `NOREF` 置位的异常帧，校验通道号 `status & 0x07U` 与预期一致。
3. **暴露六大可观测健康计数器**：
   - 驱动内置 `rdy_irq_count`、`dma_complete_count`、`dma_error_count`、`dma_timeout_count`、`recovery_count`、`overrun_count`，接入 RTT `stat` 命令实时监控。

---

## 4. 关键安全与并发约束检查清单

- [ ] **重使能 EXTI 前先清 Pending**：DMA 传输 4 字节期间 MISO 脚翻转必然再次置位 `EXTI->PEND`；DMA 完成 ISR 中必须**先 `EXTI_ClrITPendBit()`，再置位 `EXTI->IMASK`**，否则会立即误触发一次伪造的 RDY 中断！
- [ ] **退出连续读模式约束**：要把 AD7190 从 `CREAD (0x5C)` 切回寄存器读写模式，必须在 `DOUT/RDY` 变低后发送 `0x58`（`READ_DATA_NOCREAD`），或直接发送 $\ge 40$ 个 `1` 软复位。
- [ ] **恢复期采样空窗容忍**：一次自动软复位约产生 `11 ms` 采样空窗，上层滤波和峰值捕捉算法不得假定样本严格等间隔到达。

---

## 5. 验证与调试方法 (Verification)

1. **RTT 计数器稳态判据**：
   - 芯片在线 `ad7190_is_online() == 1`；稳态运行下 `rdy_irq_count` 与 `dma_complete_count` 按标称 ODR（如 `2402 Hz`）同步增长，差值恒为 `0` 或 `1`；`error / timeout / recovery / overrun` 保持 `0`。
2. **故障自愈注入测试**：
   - 用杜邦线短暂触碰扰动 `SCLK` 或在调试器中单次挂起 DMA，观察 `timeout_count` 与 `recovery_count` 各加 `1`，且 `15 ms` 内采样自动恢复正常。
