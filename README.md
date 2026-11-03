# 全栈与嵌入式研发 Skills 中心库

本目录用于集中存放和管理所有 AI Agent Skills，采用 **“一级按技术方向划分（跨领域通用 / Web 前端 / 后端服务 / 嵌入式各层），内部通过标签/前缀区分使用范围”** 的架构。

详细的分类准则、目录规范与 Agent 维护守则请参阅 [AGENTS.md](./AGENTS.md)。

---

## 目录导航

| 目录 | 技术方向 | 典型涵盖内容 |
| :--- | :--- | :--- |
| [`common/`](./common/README.md) | **跨领域通用基建** | `AGENTS.md` 质量约束审查、技术决策笔记 (`decision-notes`)、交互式 HTML 看板/视频渲染、AI 编程工具代理分流 |
| [`web-frontend/`](./web-frontend/README.md) | **Web 前端开发与测试** | Vue 3 + TypeScript + Vite 管理后台变更工作流、异步状态与权限检查、Vitest 单元测试与分层验证 |
| [`backend-service/`](./backend-service/README.md) | **后端服务与中间件** | Spring Boot 多模块后端根因定位、数据库事务与 Redis 一致性、服务端 MQTT 幂等与异步并发验证 |
| [`mcu-baremetal/`](./mcu-baremetal/README.md) | **单片机裸机与固件工程** | 嵌入式六阶段研发门禁、Keil MDK 构建与 `.map` 审计、主机 C/C++ 回归、点阵屏网页模拟器、外设驱动、掉电安全存储、表驱动状态机 |
| [`rtos/`](./rtos/README.md) | **实时操作系统 (RTOS)** | FreeRTOS、ThreadX (Azure RTOS)、RT-Thread、Zephyr、任务调度与同步、FileX/NetX/LwIP、栈与死锁分析 |
| [`linux-driver/`](./linux-driver/README.md) | **Linux 驱动与系统开发** | 设备树 (DTS)、字符/Platform/I2C/SPI/USB/V4L2 驱动、U-Boot、Buildroot/Yocto、Kernel Oops 调试 |
| [`protocols-debug/`](./protocols-debug/README.md) | **通信协议与硬件联调** | UART/RS-485/DMX512/Modbus 串口 DMA 拆包、J-Link RTT 交互式 CLI 与 HIL 验收、Cortex-M HardFault 栈回溯解码 |
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

### 方式二：按项目类型精准挂载（推荐，避免前后端与 MCU 技能互相干扰）

#### 1. Web 前后端项目（Vue 3 管理端 + Spring Boot 后端）
仅挂载通用、Web 前端与后端服务技能，不加载 MCU 裸机或硬件调试技能：

```json
{
  "entries": [
    { "path": "e:/data/doc/skills/common" },
    { "path": "e:/data/doc/skills/web-frontend" },
    { "path": "e:/data/doc/skills/backend-service" }
  ]
}
```

#### 2. 单片机 (MCU) 裸机 / RTOS 固件项目
仅挂载通用、裸机、RTOS 与协议调试技能，不加载 Web 前后端技能：

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

#### 3. 嵌入式 Linux 驱动项目
仅挂载通用、Linux 驱动与协议调试技能：

```json
{
  "entries": [
    { "path": "e:/data/doc/skills/common" },
    { "path": "e:/data/doc/skills/linux-driver" },
    { "path": "e:/data/doc/skills/protocols-debug" }
  ]
}
```
