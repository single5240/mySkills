---
name: cortex-m-hardfault-map-decoder
description: >-
  [Scope: global] ARM Cortex-M0/M3/M4/M7 HardFault 异常栈帧（MSP/PSP PC/LR）提取、SCB 故障状态寄存器（CFSR/HFSR/BFAR/MMFAR）解码与 Keil .map / 反汇编精确定位。
  当用户遇到单片机死机进入 HardFault_Handler、野指针/数组越界/除零/栈溢出崩溃、或需要根据 PC/LR 地址在 .map 文件中反查故障源码行时使用此技能。
---

# Cortex-M HardFault 栈回溯与 `.map` 故障定位 (Cortex-M HardFault & Map Decoder)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`protocols-debug`
- **使用范围 (Scope)**：`global`（全平台 ARM Cortex-M0/M0+/M3/M4/M7/M33 通用）
- **适用工具链**：Keil MDK (`ARMCC 5` / `ARMClang 6`, `.map` + `fromelf.exe`) 與 GCC (`arm-none-eabi-addr2line` / `objdump`)。

---

## 2. 核心诊断原理 (Core Diagnostic Flow)

当 Cortex-M 触发异常进入 `HardFault_Handler` 时，硬件已自动将故障现场的 8 个核心寄存器压入故障发生前正在使用的栈（`MSP` 或 `PSP`）：

```text
High Address
  +------------------+
  | xPSR             |  sp[7]  (检查 bit24 Thumb 位是否为 1)
  | PC (Return Addr) |  sp[6]  <-- 触发精确故障的指令地址！
  | LR (R14)         |  sp[5]  <-- 进入当前故障函数前的父函数返回地址！
  | R12              |  sp[4]
  | R3               |  sp[3]
  | R2               |  sp[2]
  | R1               |  sp[1]
  | R0               |  sp[0]  <-- 故障栈顶 (MSP 或 PSP)
Low Address
```

1. **第一步：由进入 Handler 时的 `LR (EXC_RETURN)` 确定栈指针**：
   - 检查 `EXC_RETURN` 的 `Bit 2`：`0` 表示压入主栈 `MSP`，`1` 表示压入线程栈 `PSP`（裸机工程默认全在 `MSP`）。
   - **注意**：若開啟了 FPU 且触发前使用了浮点寄存器，`EXC_RETURN` 的 `Bit 4 == 0` 表示栈帧还额外包含 `S0~S15, FPSCR`。
2. **第二步：读取 `SCB` 故障状态寄存器 (`0xE000ED28 ~ 0xE000ED38`)**：
   - **`SCB->HFSR` (`0xE000ED2C`)**：确认是否为 `FORCED (Bit 30)`（由 MemManage/BusFault/UsageFault 升级而来）或 `VECTTBL (Bit 1)`（取向量表故障）。
   - **`SCB->CFSR` (`0xE000ED28`)**：拆分为低 8 位 `MMFSR`（内存管理）、中 8 位 `BFSR`（总线故障）、高 16 位 `UFSR`（用法故障）。
   - **`SCB->MMFAR` (`0xE000ED34`) & `SCB->BFAR` (`0xE000ED38`)**：仅当 `MMARVALID (CFSR[7]) == 1` 或 `BFARVALID (CFSR[15]) == 1` 时，其中保存的才是**触发崩溃的确切非法内存地址**！
3. **第三步：结合 Keil `.map` 与 `fromelf` 还原函数与源码行**：
   - 在 `.map` 的 `Memory Map of the image` 中查找满足 `Symbol_Base <= sp[6] (PC) < Symbol_Base + Size` 的函数；
   - 若 `PC == 0x00000000`（调用了空函数指针），则改查 **`sp[5] (LR) & ~1UL`** 定位是哪个父函数发起了这次非法调用！

> 适配 ARMCC 5 / GCC 的零栈污染 `HardFault_Handler` 汇编+RTT 打印模板见 [`examples/hardfault_rtt_dump.c`](./examples/hardfault_rtt_dump.c)；完整 `CFSR` 位域速查表与 `fromelf` 反汇编命令见 [`references/cfsr-bitfield-and-map-lookup.md`](./references/cfsr-bitfield-and-map-lookup.md)。

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **植入汇编级故障栈捕获 (`HardFault_Handler`)**：
   - **严禁直接在 C 函数 `void HardFault_Handler(void)` 开头定义局部变量并读取 `MSP`**：编译器生成的函数序言（`PUSH {r4, lr}`）会先修改 `MSP`，导致读出的 `sp[0..7]` 发生偏移错位！
   - 必须使用裸汇编（ARMCC 5 `__asm` 或 GCC `__attribute__((naked))`）先提取 `MSP/PSP` 作为 `R0` 参数传给 C 函数 `HardFault_DumpC(uint32_t *stackFrame, uint32_t excReturn)`。
2. **解析非精确总线故障 (`IMPRECISERR`, `CFSR[10] == 1`)**：
   - 写缓冲（Write Buffer）导致写非法地址滞后几个周期才报错，此时 `PC` 已滑过真实故障指令且 `BFARVALID == 0`。
   - **排障绝招**：在调试初始化时临时置位 `SCB->ACTLR |= (1UL << 1)`（`DISDEFWBUF`，关闭 Cortex-M3/M4 默认写缓冲），将所有非精确写故障强制转化为**精确总线故障 (`PRECISERR`, `BFARVALID = 1`)**，直接抓出真凶指令！
3. **开启除零与非对齐陷阱（可选诊断增强）**：
   - 默认 Cortex-M3/M4 除以 0 不会触发异常（结果静默为 0）；排查数学异常时可设置 `SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk`。

---

## 4. 关键安全检查清单

- [ ] **异常内停用危险输出并安全停机**：进入 `HardFault` 后，先将高危功率/加热/电机 GPIO 置为安全关断电平，打印完 RTT/串口寄存器快照后执行断点 `if (CoreDebug->DHCSR & 1) __breakpoint(0);` 并死循环等待 IWDG 复位。
- [ ] **检查栈溢出破坏 (`MSP < Stack_Bottom`)**：若提取出的 `MSP` 已跌破 `.map` 中的 `STACK` 段起始地址，说明是栈溢出踩坏了上下文，此时应扩大 `startup_*.s` 中的 `Stack_Size` 或排查函数内大局部数组。

---

## 5. 验证与调试方法 (Verification)

1. **故障注入验证**：
   - 临时写入 `*(volatile uint32_t *)0xCCCCCCCCUL = 1UL;`（触发 `PRECISERR` / `IMPRECISERR`）或调用 `((void (*)(void))0x00000000UL)();`（触发 `INVSTATE`），验证 RTT 准确打印出故障函数地址，且与 `.map` 完全吻合。
