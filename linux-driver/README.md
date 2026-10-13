# Linux Driver & BSP — 嵌入式 Linux 与驱动开发技能 (`linux-driver/`)

本目录存放面向嵌入式 Linux 内核驱动开发、设备树（Device Tree）、Bootloader 移植及系统构建的技能。

## 适用场景
- **设备树（Device Tree）**：`.dts` / `.dtsi` 节点编写、Pinctrl/GPIO/Interrupt/Clock/Regulator 属性配置、Device Tree Bindings (`yaml`) 校验、Overlay 动态加载。
- **内核驱动开发**：
  - 基础模型：字符设备（`cdev`）、`miscdevice`、`platform_driver`、Sysfs/Debugfs/Procfs 接口、`ioctl` 设计。
  - 总线与子系统：I2C client、SPI driver、Regmap 抽象、IIO（ADC/传感器）、Input 子系统、PWM/LED/RTC/Watchdog、DRM/显示、V4L2（摄像头/视频）、ALSA（音频）、USB、网络设备驱动（NAPI/PHY）。
  - 并发与内存：Spinlock、Mutex、RCU、原子操作；中断顶半部/底半部（Threaded IRQ、Workqueue、Tasklet）；DMA Engine 与一致性映射（`dma_alloc_coherent` / Streaming DMA）；`devm_*` 资源托管。
- **Bootloader 与系统构建**：U-Boot 驱动与环境变量配置、FIT Image、Kernel `defconfig` 裁剪、Buildroot / Yocto (BitBake recipe) 定制、Systemd 服务配置。
- **内核调试與性能剖析**：Kernel Panic / Oops 调用栈解析、`ftrace` / `perf` / `ebpf`、`dynamic_debug`、`kmemleak` 内存泄漏检测。

## 使用范围（Scope）命名约定
- **`global`**（Linux 通用子系统）：如 `linux-char-driver-template`、`linux-dts-troubleshooting`、`linux-kernel-oops-decoder`。
- **`vendor`**（SoC 原厂平台前缀）：如 `rk3588-mipi-csi-driver`、`imx6ull-pinctrl-dts`、`zynqmp-axi-dma-driver`。
- **`project`**（`bsp-<board>-*` 或 `proj-<name>-*`）：特定定制载板的 BSP 移植与外设适配技能。
- **`experimental`**（`exp-*`）：新内核特性或实验性驱动模块。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 内核子系统 / SoC 平台 | 简述 |
| :--- | :--- | :--- | :--- |
| *(待添加)* | `global` / `vendor` | Kernel / DTS / Rockchip / i.MX | 将新技能置于 `linux-driver/<skill-name>/SKILL.md` 并在此登记 |
