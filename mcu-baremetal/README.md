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
| [`nor-flash-powerfail-record-store`](./nor-flash-powerfail-record-store/SKILL.md) | `global` | All MCU + SPI NOR Flash (W25Q128) | 外部 NOR Flash 掉电安全 64B 定长记录存储、两阶段提交、单条原位墓碑删除、3 扇区世代控制环、稀疏扇区非阻塞迁移与断电续作 |
| [`eeprom-dual-slot-param-store`](./eeprom-dual-slot-param-store/SKILL.md) | `global` | All MCU + I2C EEPROM (M24C02) | I2C EEPROM A/B 双槽乒乓掉电安全参数镜像、16B 页边界切分、tWR ACK 轮询及传感器大小量程双层 25 细段线性插值标定 |
| [`ad7190-exti-spi-dma-acquisition`](./ad7190-exti-spi-dma-acquisition/SKILL.md) | `vendor` | ADI AD7190/AD7192 + Cortex-M | AD7190 24-bit Σ-Δ ADC 的 DOUT/RDY 引脚 EXTI+SPI MISO 双模无竞态切换、SPI DMA 连续采样、256 槽环形队列与 40-bit 软复位自愈 |
| [`baremetal-table-fsm-and-scheduler`](./baremetal-table-fsm-and-scheduler/SKILL.md) | `global` | All MCU (C99 Bare-Metal) | 零动态内存表驱动有限状态机（含初始化自检与空闲循环事件）、主循环 1ms/10ms/100ms/空闲分片四级协作调度、静态事件总线与 UI 页面契约 |
| [`mcu-flash-param-storage`](./mcu-flash-param-storage/SKILL.md) | `global` | All MCU (Internal Flash) | 单片机片内 Flash 双页乒乓掉电安全参数存储、页内定长槽追加磨损均衡、同 Bank 擦写 CPU 取指停顿（Stall）中断保护与链接脚本隔离 |
| [`baremetal-menu-ui-architecture`](./baremetal-menu-ui-architecture/SKILL.md) | `global` | All MCU (LCD/OLED Key UI) | 裸机按键长按跨页防误触锁存、驻留 Flash 的表驱动菜单引擎 (`MenuItem`)、条带渲染帧快照 (`PageSnapshot` 防撕裂) 与多语言固定边界裁剪 |
