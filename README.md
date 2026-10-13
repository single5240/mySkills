# 嵌入式与通用研发 Skills 中心库

本目录用于集中存放和管理所有 AI Agent Skills，采用 **“一级按技术方向划分，内部通过标签/前缀区分使用范围”** 的架构。

详细的分类准则、目录规范与 Agent 维护守则请参阅 [AGENTS.md](./AGENTS.md)。

---

## 目录导航

| 目录 | 技术方向 | 典型涵盖内容 |
| :--- | :--- | :--- |
| [`common/`](./common/README.md) | **通用与工程基建** | C/C++ 规范 (MISRA-C)、CMake/Makefile/Kconfig、Git 工作流、技术文档、静态分析脚本 |
| [`mcu-baremetal/`](./mcu-baremetal/README.md) | **单片机裸机开发** | ARM Cortex-M / RISC-V、STM32/GD32 外设驱动、HAL/LL/寄存器开发、Linker Script、Bootloader/IAP |
| [`rtos/`](./rtos/README.md) | **实时操作系统 (RTOS)** | FreeRTOS、ThreadX (Azure RTOS)、RT-Thread、Zephyr、任务调度与同步、FileX/NetX/LwIP、栈与死锁分析 |
| [`linux-driver/`](./linux-driver/README.md) | **Linux 驱动与系统开发** | 设备树 (DTS)、字符/Platform/I2C/SPI/USB/V4L2 驱动、U-Boot、Buildroot/Yocto、Kernel Oops 调试 |
| [`protocols-debug/`](./protocols-debug/README.md) | **通信协议与硬件联调** | CAN/CANopen、Modbus、BLE、OpenOCD/J-Link/GDB 调试、HardFault/CmBacktrace 分析 |
| [`_templates/`](./_templates/skill-template/SKILL.md) | **Skill 标准模板** | 新建 Skill 的标准目录骨架与 `SKILL.md` 编写范例 |

---

## 在其他项目中按需引入本技能库

### 方式一：全量继承（加载所有技术方向技能）
在目标项目的 `.agents/skills.json`（或全局 `~/.gemini/config/skills.json`）中添加：

```json
{
  "inherits": [
    {
      "path": "e:/data/doc/skills/skills.json"
    }
  ]
}
```

### 方式二：按项目类型精准挂载（推荐）
例如在 **STM32 + ThreadX/FreeRTOS 项目** 中，仅挂载通用、裸机、RTOS 与调试技能，不加载 Linux 驱动技能：

```json
{
  "entries": [
    { "path": "e:/data/doc/skills/common" },
    { "path": "e:/data/doc/skills/mcu-baremetal" },
    { "path": "e:/data/doc/skills/rtos" },
    { "path": "e:/data/doc/skills/protocols-debug" }
  ]
}
```

例如在 **嵌入式 Linux 驱动项目** 中，仅挂载通用、Linux 驱动与调试技能：

```json
{
  "entries": [
    { "path": "e:/data/doc/skills/common" },
    { "path": "e:/data/doc/skills/linux-driver" },
    { "path": "e:/data/doc/skills/protocols-debug" }
  ]
}
```
