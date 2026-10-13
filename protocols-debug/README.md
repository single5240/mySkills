# Protocols & Debug — 通信协议栈与软硬件联调技能 (`protocols-debug/`)

本目录存放面向现场总线、工业/车载/物联网通信协议栈，以及软硬件协同调试、故障定位的技能。

## 适用场景
- **工业与车载总线**：CAN / CAN FD、CANopen、SAE J1939、Modbus (RTU / ASCII / TCP)、EtherCAT、RS-485/RS-232 自定义帧协议解析（状态机拆包、CRC16/CRC32 校验、超时重传）。
- **网络与外设协议**：LwIP / NetX Duo TCP/IP 调优、MQTT、CoAP、USB (CDC/HID/MSC)、BLE GATT 服务配置。
- **调试工具与崩溃诊断**：
  - MCU 故障诊断：ARM Cortex-M `HardFault` / `MemManage` / `BusFault` / `UsageFault` 寄存器（CFSR/HFSR/MMFAR/BFAR）分析、CmBacktrace、`.map` 与 `addr2line` / `objdump` 反汇编定位。
  - 在线调试自动化：OpenOCD 配置脚本、J-Link Commander / RTT 日志、GDB Python 扩展脚本、CoreDump 离线分析。
  - 硬件辅助分析：Saleae 逻辑分析仪时序解码数据分析、I2C 死锁（SDA 拉低）恢复流程、信号毛刺与电磁兼容软件滤波策略。

## 使用范围（Scope）命名约定
- **`global`**（标准协议或通用调试工具）：如 `cortex-m-hardfault-debug`、`modbus-rtu-frame-parser`、`canopen-pdo-sdo-config`、`openocd-gdb-workflow`。
- **`vendor`**（特定芯片调试或原厂协议栈）：如 `stm32-bxcan-filter-config`、`jlink-rtt-logger`。
- **`project`**（`proj-<name>-*`）：私有上下位机通信协议、特定网关协议转换规则。
- **`experimental`**（`exp-*`）：试验性协议抓包或自动化测试脚本。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 协议 / 调试工具 | 简述 |
| :--- | :--- | :--- | :--- |
| *(待添加)* | `global` / `vendor` | CAN / Modbus / GDB / HardFault | 将新技能置于 `protocols-debug/<skill-name>/SKILL.md` 并在此登记 |
