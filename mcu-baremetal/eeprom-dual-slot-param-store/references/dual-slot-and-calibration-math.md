# EEPROM A/B 双槽布局与双层 25 段传感器标定数学规范

本参考文档提炼自 QW-603 (`N32G455_TensileTesting`) 的 `app_param_store`、`app_calibration` 与 `drv_m24c02` 模块。

---

## 1. 256B EEPROM A/B 双槽 128B 镜像结构

| 区域 | 槽内偏移 | 长度 | 内容说明 |
| :--- | ---: | ---: | :--- |
| **Header** | `0 ~ 7` | 8 B | `magic` (2B) + `version` (1B, V2=2) + `flags` (1B) + `sequence` (4B, LE) |
| **Settings** | `8 ~ 43` | 36 B | 用户菜单配置（单位、公差、自动归零时间、背光档位、重力加速度、通信速率等） |
| **Calibration** | `44 ~ 119` | 76 B | `rangeIndex` (1B) + 4 个有效位 (`tensionLarge/Small`, `compressionLarge/Small`) + 4 套 × 6 点 × 3B (24-bit ADC 小端码) |
| **Reserved** | `120 ~ 125` | 6 B | 预留扩展，填 `0` 或 `0xFF` |
| **CRC** | `126 ~ 127` | 2 B | 覆盖偏移 `0 ~ 125` 的 CRC-16/MODBUS（小端） |

### 上电双槽仲裁真值表 (`AppParam_Load`)

| Slot 0 状态 | Slot 1 状态 | 仲裁行为与返回值 |
| :---: | :---: | :--- |
| 有效 | 无效 | 选用 Slot 0，下次 `Save()` 写 Slot 1；返回 `APP_PARAM_OK` |
| 无效 | 有效 | 选用 Slot 1，下次 `Save()` 写 Slot 0；返回 `APP_PARAM_OK` |
| 有效 | 有效 | 使用无符号半量程差值 `(uint32_t)(seq1 - seq0) < 0x80000000UL` 选出较新槽，下次写较旧槽 |
| 无效 | 无效 | 加载编译期默认设置与理论默认标定曲线；返回 `APP_PARAM_DEFAULTS` |
| I2C 读失败 | I2C 读失败 | 保持默认参数；返回 `APP_PARAM_ERR_IO` |

---

## 2. M24C02 16B 页边界切分算法

M24C02 内部写缓冲为 16 字节页（页掩码 `0x0F`）。任意起始地址 `addr` 与长度 `len` 的安全切分逻辑：

```c
while (remaining > 0U)
{
    uint16_t pageOffset = currentAddr & 0x0FU;
    uint16_t chunkLen   = 16U - pageOffset;
    if (chunkLen > remaining)
    {
        chunkLen = remaining;
    }

    status = M24c02_WritePage(currentAddr, dataPtr, (uint8_t)chunkLen);
    if (status != M24C02_OK) { return status; }

    status = M24c02_WaitWriteComplete(10U); /* 10ms ACK polling */
    if (status != M24C02_OK) { return status; }

    currentAddr += chunkLen;
    dataPtr     += chunkLen;
    remaining   -= chunkLen;
}
```

---

## 3. 双层六点五段（25 细段）标定派生与定点插值

### 3.1 物理含义
- **大量程六点 $L[0..5]$**：对应标称载荷 `0%, 20%, 40%, 60%, 80%, 100% FS`（划分为 5 个 20% FS 大段 $i \in [0, 4]$）。
- **小量程六点 $S[0..5]$**：对应标称载荷 `0%, 4%, 8%, 12%, 16%, 20% FS`（划分为 5 个 4% FS 细段 $j \in [0, 5]$）。

### 3.2 派生 25 个细段锚点 $D[i][j]$
对第 $i$ 个大段（$i = 0..4$）内的第 $j$ 个细分点（$j = 0..5$）：
1. **端点精确锚定**（防止定点舍入破坏 20% 大段连续性）：
   $$D[i][0] = L[i], \quad D[i][5] = L[i+1]$$
2. **内部点按小曲线相对 ADC 比例派生**（$j = 1..4$）：
   $$D[i][j] = L[i] + \text{RoundHalfAwayFromZero}\left( \frac{(S[j] - S[0]) \times (L[i+1] - L[i])}{S[5] - S[0]} \right)$$

### 3.3 `int64_t` 向远离零舍入除法实现
```c
static int32_t Calib_DivRoundHalfAway(int64_t num, int64_t den)
{
    if (den == 0) { return 0; }
    if (den < 0)  { num = -num; den = -den; }

    if (num >= 0)
    {
        return (int32_t)((num + (den / 2)) / den);
    }
    else
    {
        return (int32_t)(-(((-num) + (den / 2)) / den));
    }
}
```
- **合法性门禁**：计算完某方向全部 26 个锚点后，必须验证相邻点满足严格单调（拉伸 $D[i][j] < D[i][j+1]$，压缩 $D[i][j] > D[i][j+1]$），否则拒绝保存。
