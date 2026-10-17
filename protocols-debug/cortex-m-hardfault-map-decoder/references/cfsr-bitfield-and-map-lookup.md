# Cortex-M `SCB->CFSR` 位域解码表与 `.map` / `fromelf` 反查指南

## 1. `SCB->CFSR` (`0xE000ED28`) 32 位故障位域速查

`CFSR` 由三个子状态寄存器拼接而成：`[31:16] UFSR` | `[15:8] BFSR` | `[7:0] MMFSR`。

| 位号 | 位名称 | 所属子域 | 典型触发根因与排查方向 |
| ---: | :--- | :---: | :--- |
| `Bit 0` | `IACCVIOL` | MMFSR | 指令访问违例：PC 跳到了不可执行区（如 `0xFFFFFFF0` 或 MPU 禁止执行区），常见于**栈溢出覆盖了压栈的 LR** |
| `Bit 1` | `DACCVIOL` | MMFSR | 数据访问违例：读写了 MPU 保护区或非法系统地址，查看 `MMFAR` |
| `Bit 7` | `MMARVALID` | MMFSR | `MMFAR` (`0xE000ED34`) 中的故障地址有效 |
| `Bit 8` | `IBUSERR` | BFSR | 指令预取总线错误：函数指针指向了不存在的物理地址空间 |
| `Bit 9` | `PRECISERR` | BFSR | **精确数据总线错误**：读或写了非法外设/内存地址（如未开 RCC 时钟的外设或越界野指针），此时 **`PC` 即故障指令，且 `BFAR` 保存了确切非法地址** |
| `Bit 10` | `IMPRECISERR`| BFSR | **非精确数据总线错误**：异步写缓冲报错，`PC` 比真实写指令滞后数条。在初始化中设置 `*(volatile uint32_t *)0xE000E008 |= 2UL;`（关闭写缓冲 `DISDEFWBUF`）复现即可转为 `PRECISERR` |
| `Bit 11` | `UNSTKERR` | BFSR | 异常退出出栈时总线错误：`SP` 栈指针已损坏或跌出 SRAM |
| `Bit 12` | `STKERR` | BFSR | 异常进入压栈时总线错误：**典型的 `MSP` 栈下溢（Stack Overflow）跌破 `0x20000000`** |
| `Bit 15` | `BFARVALID` | BFSR | `BFAR` (`0xE000ED38`) 中的故障地址有效 |
| `Bit 16` | `UNDEFINSTR` | UFSR | 执行了未定义指令：代码区被踩坏、Flash 擦写错误或函数指针跳进了数据表 |
| `Bit 17` | `INVSTATE` | UFSR | 非法 EPSR 状态（`xPSR.T == 0`）：**调用了函数指针，但指针值为 `0`（NULL）或最低位未置 1（非 Thumb 地址）**，立即查看 `sp[5] (LR)` 找调用方！ |
| `Bit 18` | `INVPC` | UFSR | 异常返回时 `EXC_RETURN` 值非法：中断栈里的 `LR` 被越界写坏 |
| `Bit 19` | `NOCP` | UFSR | 访问了未使能的协处理器：**工程开启了硬件浮点（FPU），但 `SystemInit()` 未置位 `SCB->CPACR |= (0xFUL << 20)`** |
| `Bit 24` | `UNALIGNED` | UFSR | 非对齐访问故障：使用 `LDRD / STRD / LDM / STM` 访问了非 4 字节对齐的指针（如将 `uint8_t buf[]` 强转为 `uint32_t *` 或 `uint64_t *`） |
| `Bit 25` | `DIVBYZERO` | UFSR | 除以零故障（当 `SCB->CCR.DIV_0_TRP == 1` 时触发） |

---

## 2. 使用 Keil `.map` 与 `fromelf.exe` 反查源码位置

1. **在 `.map` 中快速定位函数**：
   打开 `Listings/<Target>.map`，搜索 `PC` 的高 20 位（例如 `PC = 0x08004A32`，搜索 `0x08004`），找到基地址小于等于 `0x08004A32` 的最近一行 `Thumb Code` 符号。
2. **使用 Keil `fromelf.exe` 生成带 C 源码交织的反汇编文件**：
   ```powershell
   & "C:\Keil_v5\ARM\ARMCC\bin\fromelf.exe" --text -c -o "Objects\disasm.txt" "Objects\N32G455.axf"
   ```
   打开 `Objects\disasm.txt`，直接搜索 `0x08004a32`（注意区分大小写），即可看到对应的汇编指令及其上方标注的 C 源码函数与语句。
