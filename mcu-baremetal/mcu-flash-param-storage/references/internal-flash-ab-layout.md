# 片内 Flash 双页乒乓 + 页内追加存储规范与代码模板

## 1. 2 KiB 双页乒乓 + 64B 槽追加物理布局

以单页 `2048 B`（如 N32G455 / STM32F103 大容量型）、定长槽 `64 B` 为例：

```text
+-------------------------------------------------------------------+
| Page A (2048 B, e.g. 0x0807F000 ~ 0x0807F7FF)                     |
|  [Slot 0: PageHeader 64B] (Magic, Version, PageEpoch, CommitWord) |
|  [Slot 1: ParamRecord #1] (Seq=1, Payload, CRC32, CommitWord=0)   |
|  [Slot 2: ParamRecord #2] (Seq=2, Payload, CRC32, CommitWord=0)   |
|  ...                                                              |
|  [Slot 31: Erased 0xFFFFFFFF...]                                  |
+-------------------------------------------------------------------+
| Page B (2048 B, e.g. 0x0807F800 ~ 0x0807FFFF)                     |
|  [Entire 2048 B Verified as 0xFFFFFFFF (Standby Blank Page)]      |
+-------------------------------------------------------------------+
```

### 单个 64 字节参数槽布局（32-bit Word 对齐）

| 字节偏移 | 长度 | 字段名 | 编程阶段 |
| ---: | ---: | :--- | :--- |
| `0` | 4 B | `magic` (`0x5041524D` `'PARM'`) | 阶段一写入 |
| `4` | 2 B | `version` | 阶段一写入 |
| `6` | 2 B | `payloadLen` | 阶段一写入 |
| `8` | 4 B | `sequence` (单调递增序号) | 阶段一写入 |
| `12` | 44 B | `payload` (设置与校准参数) | 阶段一写入 |
| `56` | 4 B | `crc32` (覆盖偏移 `0 ~ 55`) | 阶段一写入 |
| `60` | 4 B | `commitWord` | 阶段一保持 `0xFFFFFFFF`；阶段二单独编程为 `0x00000000` |

---

## 2. 换页（Page Transfer）与上电恢复真值表

| Page A 状态 | Page B 状态 | 上电恢复行为 |
| :--- | :--- | :--- |
| `ACTIVE`（含有效槽） | `ERASED`（全 `0xFF`） | 正常态：从 Page A 最后一个已提交有效槽加载参数 |
| `ERASED`（全 `0xFF`） | `ACTIVE`（含有效槽） | 正常态：从 Page B 最后一个已提交有效槽加载参数 |
| `ACTIVE` | `ACTIVE`（换页擦旧页前断电） | 比较两页最后一条有效槽的 `sequence`（无符号半量程差值），以最新页为准，并**立即擦除较旧页**恢复 `ERASED` 备用态 |
| `ACTIVE` | `DIRTY`（换页新页未提交完成即断电） | 以 Page A 旧有效槽为准，擦除 Page B 脏数据恢复 `ERASED` |
| 全空或均无效 | 全空或均无效 | 擦除并初始化 Page A 页头，写入编译期默认参数（`Seq = 1`） |

---

## 3. 片内 Flash 安全编程包装函数范式

```c
bool InternalFlash_ProgramWords(uint32_t flashAddr, const uint32_t *words, uint32_t wordCount)
{
    bool ok = true;

    /* 1. 对齐与地址范围门禁 */
    if (((flashAddr & 0x03UL) != 0UL) || (words == NULL) ||
        (flashAddr < PARAM_FLASH_START_ADDR) ||
        ((flashAddr + (wordCount * 4UL)) > PARAM_FLASH_END_ADDR))
    {
        return false;
    }

    /* 2. 解锁并清除历史错误标志 */
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    /* 3. 逐字编程并立即读回校验 */
    for (uint32_t i = 0UL; i < wordCount; i++)
    {
        uint32_t targetAddr = flashAddr + (i * 4UL);
        if (FLASH_ProgramWord(targetAddr, words[i]) != FLASH_COMPLETE)
        {
            ok = false;
            break;
        }
        if (*(const volatile uint32_t *)targetAddr != words[i])
        {
            ok = false;
            break;
        }
    }

    /* 4. 任何路径均强制重新上锁 */
    FLASH_Lock();
    return ok;
}
```
