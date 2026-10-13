# MCU Bare-Metal — 单片机裸机开发技能 (`mcu-baremetal/`)

本目录存放面向无操作系统（Bare-Metal / 前后台系统）的微控制器（MCU）底层开发、芯片外设配置与固件架构技能。

## 适用场景
- **内核与架构级**：ARM Cortex-M (M0/M3/M4/M7/M33) 及 RISC-V 内核机制（NVIC 中断优先级分组、SysTick、MPU、SCB、I-Cache/D-Cache 一致性、低功耗睡眠/停机/待机唤醒）。
- **芯片与外设驱动**：STM32、GD32、NXP、TI、ESP32 等芯片的 GPIO、UART、I2C、SPI、CAN、ADC、DAC、TIM/PWM、DMA、Watchdog、Flash 读写（寄存器直接操作、HAL 库、LL 库、标准库）。
- **启动与内存布局**：启动文件（`startup_*.s`）、链接脚本（`.ld` / `.sct` / `.icf`）、TCM/SRAM/Flash 分区规划、分散加载。
- **固件基础组件**：Bootloader 引导与 IAP/OTA 固件升级（Ymodem/CAN/UART 升级、双区 AB 备份、CRC/签名校验）、环形缓冲区（RingBuffer）、时间片轮询调度器与有限状态机（FSM）。

## 使用范围（Scope）命名约定
- **`global`**（直接功能命名）：跨 MCU 通用机制，如 `baremetal-ringbuffer-dma`、`iap-bootloader-design`、`linker-script-memory-layout`。
- **`vendor`**（以芯片/架构为前缀）：如 `stm32-hal-uart-dma`、`cortex-m7-cache-coherency`、`gd32-spi-flash`。
- **`project`**（`bsp-<board>-*` 或 `proj-<name>-*`）：特定硬件单板引脚定义与板级外设初始化技能。
- **`experimental`**（`exp-*`）：实验性外设驱动或测试流程。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 适用架构/芯片 | 简述 |
| :--- | :--- | :--- | :--- |
| [`embedded-bitmap-fonts`](./embedded-bitmap-fonts/SKILL.md) | `global` | All MCU (ARMCC/Keil/GCC) | 嵌入式 LCD/OLED/TFT 点阵字库诊断、子集裁剪与生成（支持中文 BDF 点阵、基线防裁剪、RGB565 条带局部渲染、ARMCC5 UTF-8 字符串转义） |
| [`n32g455-lcd-async-render`](./n32g455-lcd-async-render/SKILL.md) | `vendor` | Nationstech N32G455 (Cortex-M4) | 将 N32G455 上的 ST7789 等 SPI LCD 页面绘制改为 DMA 异步条带渐进刷新，并在不干扰 AD7190 高优先级采集 DMA 的前提下完成 Keil 验证 |
