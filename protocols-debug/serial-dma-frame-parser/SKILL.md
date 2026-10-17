---
name: serial-dma-frame-parser
description: >-
  [Scope: global] UART/RS-485/DMX512/Modbus RTU 串口 IDLE+DMA 环形缓冲区接收、表驱动/状态机拆包、短帧/断帧/粘包防御与半双工 DE/RE 方向切换时序。
  当用户需要编写或调试串口自定义协议、Modbus RTU、DMX512 灯光协议、解决 RS-485 尾字节截断、DMA 缓冲回绕丢包或粘包错位问题时使用此技能。
---

# 串口 DMA 环形接收与协议帧拆包状态机 (Serial DMA & Frame Parser)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`protocols-debug`
- **使用范围 (Scope)**：`global`（全平台 MCU 通用，涵盖 UART / RS-232 / RS-485 / Modbus RTU / DMX512）
- **核心设计目标**：ISR 零协议解析负担、零动态内存、天然免疫粘包（Back-to-Back Frames）/ 断帧（Fragmented Bytes）/ 伪帧头（False Header Inside Payload）与半双工尾字节截断。

---

## 2. 四大核心场景架构准则

### 2.1 `UART IDLE + DMA Circular` 双游标搬运模型
- DMA RX 配置为**循环模式（Circular Mode）**写入静态数组 `s_dmaRxBuf[N]`。
- 在 **UART 空闲中断（IDLE ISR）**、**DMA 半传输中断（HT ISR）** 和 **DMA 传输完成中断（TC ISR）** 中调用同一个搬运函数：
  - 计算 `writePos = N - DMA_GetCurrDataCounter()`；
  - 将 `[lastPos, writePos)` 新增字节（含回绕段）压入软件无锁环形队列 `RingBuffer`，仅更新 `lastPos = writePos`。
- **禁止在每次 IDLE 中断里暴停 DMA 重装 `CNDTR`**：停 DMA 重装期间到达的高速串口字节会直接丢失或触发 `ORE`（Overrun Error）。

### 2.2 状态机拆包与“伪帧头回退扫描”机制
- 主循环从 `RingBuffer` 逐字节喂入拆包状态机（`SYNC_HEAD -> LEN -> PAYLOAD -> CRC -> PUBLISH`）。
- **关键防死锁设计**：
  1. **帧内字节超时（Inter-Byte Timeout）**：若状态机停留在非 `SYNC_HEAD` 状态且超过 $T_{\text{timeout}}$（如 `20 ms`）未收齐剩余字节，自动复位状态机并丢弃残帧，防止断帧导致下一帧合法报文被吞。
  2. **CRC 失败滑动重同步**：若帧头命中但最终 CRC 校验失败（说明帧头字节恰好是上一帧残片里的载荷数据），不要整包丢弃全部已收字节，应从帧头下一字节重新扫描 `SYNC_HEAD`。

### 2.3 DMX512 协议接收与短帧安全发布法则
- **Break + MAB 帧同步**：利用 UART 帧错误标志（`FE` Framing Error + 接收数据为 `0x00`）识别 $\ge 88\ \mu\text{s}$ 的 `BREAK` 低电平脉冲，随后第 1 个正常字节为 `Start Code`（标准调光帧为 `0x00`），后续为 `Slot 1..512`（250 kbps, 8N2）。
- **短帧发布铁律**：DMX 控台允许发送少于 512 通道的短帧（Short Frame）。**不得要求必须收满 513 字节才发布数据**；当检测到下一个 `BREAK` 或帧空闲超时，只要已收到合法 `Start Code == 0x00` 且已接收通道数 $\ge$ 本灯具所需的目标起始通道范围，即刻将双缓冲中的完整快照原子发布给调光层；若在中途发生 `ORE/NE` 错误或不足本设备通道数，则丢弃本帧保持上一帧输出，绝不把半帧残缺数据直接写向 PWM！

### 2.4 半双工 RS-485 `DE/RE` 方向切换与 Modbus 3.5T 边界
- **发送切接收铁律（等待 `TC` 而非 `TXE`）**：
  - `TXE=1`（或 DMA TX 完成）仅代表最后一个字节刚送入移位寄存器，此时停止位尚未从引脚发出！
  - **必须在 `USART_FLAG_TC`（Transmission Complete）置位后**，才可将 RS-485 `DE/RE` 引脚拉低切回接收态，否则最后一个字节（通常是 CRC 高字节）会被物理截断。
- **硬件异常自愈（`ORE / FE / NE`）**：
  - 在 UART ISR 中必须同时检查并清除 `ORE`（过载）、`FE`（帧错误）、`NE`（噪声错误）标志（在 STM32/N32G45x 上需先读 `STS/SR` 再读 `DAT/DR` 清除），否则 `ORE` 置位后会永久锁死后续接收中断。

> 可直接复用的无锁 `RingBuffer` + 带超时状态机拆包 C99 源码模板见 [`examples/serial_ring_frame_parser.c`](./examples/serial_ring_frame_parser.c)。

---

## 3. 关键安全与并发约束检查清单

- [ ] **单生产者/单消费者无锁队列**：`RingBuffer` 的 `head`（ISR 写）与 `tail`（主循环读）各自只由单一上下文修改，无需关中断即可保证线程安全。
- [ ] **长度字段防缓冲区溢出**：状态机解析出 `LEN` 字段后，必须立即校验 `LEN >= MIN_LEN && LEN <= MAX_PAYLOAD_SIZE`；越界立即拒绝并退回 `SYNC_HEAD`，严禁按未校验的 `LEN` 继续向 `payload[]` 写入。
- [ ] **上电 `DE/RE` 默认接收态**：RS-485 控制脚初始化时先写低电平（接收态）再配输出模式，防止多机总线上电瞬间同时抢占驱动器拉死总线。

---

## 4. 验证与调试方法 (Verification)

1. **粘包与断帧压力测试**：
   - 上位机一次发送 3 帧首尾相连的合法报文（粘包），验证解析计数 `+3`；
   - 先发送半截伪造帧头，间隔 `50 ms`（超过帧内超时）后再发 1 帧合法报文，验证合法报文不被吞没。
2. **示波器/逻辑分析仪 RS-485 波形核查**：
   - 测量 `TX` 最后一个停止位结束边沿到 `DE` 变低的延迟：确认 `DE` 在停止位完整发出后 `5~50 μs` 内拉低，既无尾字节截断，也不影响从机最快响应。
