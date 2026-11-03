# ARMCC 5 高频警告排查与 `.map` 文件深度审计指南

## 1. ARMCC V5.06 高频编译警告与标准修复法

| 警告编号与信息 | 触发原因 | 规范修复方式（零副作用） |
| :--- | :--- | :--- |
| `#870-D: invalid multibyte character sequence` | 源码为 UTF-8（无 BOM）且注释或字符串中含有多字节中文字符（如 `≈`、繁体字或奇数半角/全角边界），被 ARMCC 5 按系统默认 ANSI/GBK 代码页解析 | 1. C 字符串字面量改用十六进制字节转义（如 `"\xE5\xAD\x98\xE5\x82\xA8"`）；<br>2. 注释中避免生僻符号（如将 `≈` 改为 `~` 或 `约`）；<br>3. 未经授权不要批量把文件转码为 GBK。 |
| `#177-D: variable "xxx" was declared but never referenced` / `function was declared but never referenced` | 条件编译宏（如 `APP_RECORD_STORE_VALIDATION_ENABLE == 0`）关闭后，局部静态辅助函数或变量未参与调用 | 将仅在验证宏开启时使用的 `static` 函数放入同一 `#if ... #endif` 块内，或对未使用的接口形参显式写 `(void)param;`。 |
| `#188-D: enumerated type mixed with another type` | 将 `uint8_t` 整数直接赋值给枚举类型变量，或不同枚举间隐式混用 | 先校验 `uint8_t` 范围合法，再使用显式类型转换 `(MyEnumType)rawVal`。 |
| `#68-D: integer conversion resulted in a change of sign` | 无符号宏常量与有符号整数混算或将 `-1` 赋给无符号类型 | 明确无符号常量后缀 `U` / `UL`，有符号与无符号比较前先做范围确认与显式转换。 |
| `#223-D: function "xxx" declared implicitly` | 调用了未包含头文件的函数（C99 下极易导致返回值截断或入参压栈错误） | 必须在对应模块 `.h` 中声明公共原型并 `#include`，严禁在 `.c` 内部手写外部 `extern` 声明。 |

---

## 2. Keil `.map` 文件的四大关键审查段落

打开 `Listings/<Target>.map`（或 `Objects/<Target>.map`），按顺序核查以下 4 个核心区域：

1. **`Section Cross References`（模块交叉引用表）**：
   - 检查是否违反 `APP -> DRV` 单向依赖原则（若发现 `drv_xxx.o` 引用了 `app_xxx.o` 的符号，说明驱动层反向耦合了业务层）。
2. **`Removing Unused input sections from the image`（死代码消除表）**：
   - 确认开启 `--split_sections` 后未使用的库函数已被 Linker 裁剪；若新增的业务函数意外出现在此列表，说明漏接了主循环或表驱动入口。
3. **`Memory Map of the image`（内存地址排布表）**：
   - **`ER_IROM1 (0x08000000)`**：确认固件末尾地址未侵入片内 Flash 尾部参数扇区（如双槽 EEPROM 替代方案的尾页）或 Bootloader 区。
   - **`RW_IRAM1 (0x20000000)`**：
     - 定位最大的几个 `ZERO` (ZI) 段（如 `s_stripBuffer`、传感器环形队列、页缓存）；
     - 定位 `STACK` 段起始地址与顶地址（Cortex-M 满递减栈从 `STACK` 顶部向下生长），确认栈大小（如 `0x00001000 = 4 KiB`）充足。
4. **`Image Component Sizes`（组件体积汇总表）**：
   - 逐个 `.o` 目标文件列出 `Code | RO Data | RW Data | ZI Data`，快速抓出哪一个模块吃掉了最多的 SRAM 或 Flash。
