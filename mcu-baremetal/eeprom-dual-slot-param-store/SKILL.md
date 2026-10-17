---
name: eeprom-dual-slot-param-store
description: >-
  [Scope: global] I2C EEPROM（如 M24C02/AT24Cxx）A/B 双槽乒乓掉电安全参数存储、16B 页边界拆分、tWR ACK 轮询与双层多段传感器标定曲线管理。
  当用户需要设计嵌入式设备设置与校准参数持久化、解决 EEPROM 跨页覆盖或写入掉电损坏量产标定数据、或实现大小量程多段线性插值校准时使用此技能。
---

# I2C EEPROM 双槽乒乓参数镜像与双层多段校准 (EEPROM Dual-Slot Parameter Store & Calibration)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`mcu-baremetal`
- **使用范围 (Scope)**：`global`（全平台通用，提炼自 N32G455 + M24C02 + 测力标定系统）
- **适用硬件与场景**：I2C 串行 EEPROM（如 ST M24C02 / AT24C02，256B 容量，16B 页写，内部写周期 $t_{\text{WR}} \le 5\text{ ms}$）及高精度工业传感器多点分段标定。

---

## 2. 核心设计模式 (Core Patterns)

1. **A/B 双槽乒乓镜像（Dual-Slot Ping-Pong Image）**：
   - 将 256B EEPROM 对半划分为 **Slot 0 (`0x00~0x7F`, 128B)** 与 **Slot 1 (`0x80~0xFF`, 128B)**。
   - 每个槽包含显式小端头部（`Magic` + `Version` + `Sequence`）、**用户设置区（Settings）**、**传感器标定区（Calibration）** 和尾部 **CRC-16**。
   - **原子保存铁律**：每次 `AppParam_Save()` 永远只写入**非活动槽（或较旧槽）** $\rightarrow$ **整槽 128B 读回并逐字节比对 + CRC 校验** $\rightarrow$ 验证完全一致后才更新 RAM 中的活动槽索引与 `Sequence`。写中途掉电绝不破坏另一槽中上一份有效的量产标定参数。
2. **EEPROM 16B 页边界自动拆分与 `tWR` ACK 轮询**：
   - EEPROM 页写若跨越 16B 页尾（`0x0F`, `0x1F`...），内部地址会在**当前页首回绕覆盖旧数据**。驱动层 `M24c02_Write()` 必须按 `16 - (addr & 0x0FU)` 自动切分每次 `Page Write` 长度。
   - 每页 `STOP` 后器件进入内部编程（$t_{\text{WR}} \le 5\text{ ms}$），期间对地址 NACK。驱动通过周期发送 `START + 0xA0` 执行 **ACK 轮询**（带 10 ms 无符号 tick 超时），一旦收到 ACK 立即发 `STOP` 并进入下一页写入。
3. **设置重置与标定数据解耦保护 (`FactoryReset`)**：
   - 恢复出厂设置（`AppParam_FactoryReset()`）只将 RAM 中的用户菜单设置恢复默认值，**必须完整保留 RAM 中已有的传感器标定数据**，再提交到非活动槽；若 EEPROM 提交失败，必须回滚 RAM 中的用户设置。
4. **双层六点五段（25 细段）传感器标定模型**：
   - 每个方向（拉伸/压缩）维护**大量程六点曲线（`0/20/40/60/80/100% FS`）**与**小量程六点曲线（`0/4/8/12/16/20% FS`）**。
   - 小曲线提供相对非线性细分比例 $q_j = (S_j - S_0)/(S_5 - S_0)$，迁移映射到全部 5 个大量程区间，形成每方向 **25 个高精度线性细段**（实现 $\pm 0.2\%\text{ FS}$ 精度）。

> 双槽启动恢复真值表、24-bit 紧凑编码与双层 25 段 `int64_t` 定点插值公式详见 [`references/dual-slot-and-calibration-math.md`](./references/dual-slot-and-calibration-math.md)。

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **驱动层 (`drv_m24c02`) 实现与 I2C 保护**：
   - 配置 SCL/SDA 为开漏输出（依赖外部上拉电阻），所有硬件标志等待均带 `10 ms` 超时；超时后发送 `STOP` 并复位 I2C 外设防总线死锁。
   - 在写函数内实现按 16B 页边界循环切分 + `WaitWriteComplete()` ACK 轮询。
2. **参数存储层 (`app_param_store`) 序列化与自检**：
   - 采用显式字节偏移读写，禁止依赖编译器 `struct` 填充对齐；为节省空间，24-bit 原始 ADC 标定点可按 **3 字节无符号小端（U24 LE）** 紧凑打包（4 套 × 6 点 × 3B = 72B，轻松塞入 128B 单槽）。
   - 编写纯 RAM 自检函数 `AppParam_RunSettingsSelfTest()`，校验字段偏移不重叠、CRC 检测有效、跨版本升级（如 V1 $\rightarrow$ V2）迁移正确。
3. **标定曲线合法性与降级策略**：
   - 提交标定前强制校验 `AppParam_IsCurveDataValid()`：ADC 码在有效物理范围、拉压共用同一大量程零点（更新任一方向大量程时同步更新另一方向第 0 点）、拉伸点严格递增 / 压缩点严格递减、派生内部细段点无重复或反转。
   - **平滑降级**：大+小曲线均有效 $\rightarrow$ 启用 25 细段插值；仅大量程有效 $\rightarrow$ 降级为 5 大段插值；未标定 $\rightarrow$ 使用理论两点斜率默认曲线。

---

## 4. 关键安全与并发约束检查清单

- [ ] **单上下文访问**：EEPROM 读写全部在主循环执行，禁止在 ISR 中发起 I2C 传输。
- [ ] **写看门狗预算**：128B 槽共 8 页（$8 \times 16\text{B}$），最坏写入耗时约 $8 \times 5\text{ ms} = 40\text{ ms}$，远小于 1 s IWDG 窗口；若页数更多，必须改为分步状态机。
- [ ] **定点算术防溢出与截断**：25 段派生与力值线性插值必须遵循**先乘后除**并使用 `int64_t` 中间量，商按向远离零的四舍五入（Half-away-from-zero）处理，大段端点 `D[i][0]` 与 `D[i][5]` 直接取锚点 `L[i]`、`L[i+1]` 消除累积舍入误差。

---

## 5. 验证与调试方法 (Verification)

1. **RAM 自测与单槽破坏恢复验证**：
   - 上电运行 `AppParam_RunSettingsSelfTest()`。
   - 人为破坏 Slot 0 的 CRC 或 Magic，验证 `AppParam_Load()` 自动回退加载 Slot 1；破坏双槽后验证返回 `APP_PARAM_DEFAULTS` 并加载安全默认值。
2. **标定事务回滚验证**：
   - 在校正向导中途取消或模拟写 EEPROM 失败（如拉高 `WC` 写保护脚），验证旧有效校正曲线分毫无损。
