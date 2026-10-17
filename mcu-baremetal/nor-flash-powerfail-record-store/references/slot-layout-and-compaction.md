# 64 字节定长槽布局、动态扇区状态与稀疏迁移恢复规范

本参考文档提炼自 QW-603 (`N32G455_TensileTesting`) 测试结果存储 V2/V3 方案，定义 SPI NOR Flash（4 KiB 扇区、256B 页）掉电安全定长记录存储的字节级规范。

---

## 1. 分区物理布局与容量计算

以顶部 156 KiB（39 个 4 KiB 扇区，`0xFD9000 ~ 0xFFFFFF`）为例：

| 区域 | 扇区数 | 内部结构 | 职责说明 |
| :--- | :---: | :--- | :--- |
| **控制环 0~2** | 3 | 每扇区 64 个 64B 控制槽 | 轮转追加世代号（Generation）与迁移事务日志（`MIGRATE_BEGIN` / `MIGRATE_COMMIT`），始终保持 1 个全 `0xFF` 备用控制扇区 |
| **动态数据扇区池** | 36 | 1 个 64B 扇区头 + 63 个 64B 记录槽 | 存放数据记录；运行与迁移全过程始终保留至少 **2 个经整扇区读回校验为 `0xFF` 的 `EMPTY` 扇区** |

---

## 2. 64 字节数据记录槽显式小端布局

所有多字节字段均采用小端序（Little-Endian），且**正文、CRC、提交标志、生命周期码严禁在同一次页编程中混合改写**：

| 字节偏移 | 长度 | 字段名称 | 约束与两阶段编程规则 |
| ---: | ---: | :--- | :--- |
| `0` | 4 | `magic` | ASCII `'Q','W','R','D'` |
| `4` | 1 | `version` | 格式版本号（如 `2` 或 `3`），不兼容旧版时不静默混读 |
| `5` | 1 | `recordType` | 区分数据记录与控制记录 |
| `6` | 2 | `recordLength` | 固定为 `64` (`0x0040`) |
| `8` | 4 | `generation` | 世代号，每次“清空全部”成功后单调递增 |
| `12` | 4 | `recordId` | 当前世代内从 `1` 单调递增的主键 ID，单条删除后不重排 |
| `16` | 34 | `payload` | 绝对年月日时分秒（7B）、动作、有效掩码、单位、量程、判定、力值结果与扩展字段 |
| `50` | 9 | `reserved` | 必须保持全 `0xFF`；解码时若发现非 `0xFF` 污染判为字段错误 |
| `59` | 2 | `lifecycle` | **生命周期码**（不纳入正文 CRC，支持从 `0xFFFF` 单向改写） |
| `61` | 2 | `crc16` | **CRC-16/MODBUS**，仅覆盖偏移 `0 ~ 58` |
| `63` | 1 | `commitFlag` | **提交标志**：阶段一保持 `0xFF`；正文与 CRC 读回校验通过后，阶段二单独编程为 `0x00` |

### 生命周期状态码 (`lifecycle`, 偏移 59~60)

| 状态值 | 状态名 | 含义与可见性 | 允许转换来源 |
| :---: | :--- | :--- | :--- |
| `0xFFFF` | `ACTIVE` | 正常已提交有效记录（需同时满足 `commitFlag == 0x00` 且 CRC 正确） | 初始擦除态保持 |
| `0xA55A` | `DELETED` | 用户已单条删除；列表不可见，扇区整理时不搬迁 | 仅允许从 `0xFFFF` 写入 |
| `0x5AA5` | `MIGRATED` | 稀疏整理已在目标扇区提交副本；不可见，等待源扇区整扇区擦除 | 仅允许从 `0xFFFF` 写入 |
| 其他值 | `CORRUPT/DIRTY` | 掉电半写或异常中间态；保守隐藏，绝不作为空槽原地复用 | — |

---

## 3. 数据扇区运行期状态机

数据扇区状态由上电扫描与当前事务推导，不依赖扇区头里反复改写的单字节：

| 扇区状态 | 判定标准与允许操作 |
| :--- | :--- |
| `EMPTY` | 整个 4 KiB 经 16 页逐字节验证全为 `0xFF`；可分配，但全局至少保留 2 个 `EMPTY` 扇区 |
| `APPENDING` | 扇区头有效，尾部仍有连续 `ERASED`（全 `0xFF`）槽；允许跨世代混放，全局唯一追加点 |
| `FULL` | 扇区头有效，尾部已无 `ERASED` 槽；不可再追加 |
| `RECLAIMABLE` | 非当前追加扇区，且不含任何当前世代 `ACTIVE` 记录；可由后台非阻塞擦除回收 |
| `ERASE_PENDING` | 已发出 `0x20` 扇区擦除命令或尚未完成 4 KiB 全 `0xFF` 读回校验；不可分配 |
| `CORRUPT` | 扇区头损坏且非全 `0xFF`；从可用池隔离 |

---

## 4. 稀疏扇区整理（Compaction）与断电恢复矩阵

### 4.1 触发条件与目标选择
- **触发阈值**：单条删除将某记录标为 `0xA55A` 后，统计该 4 KiB 扇区的 63 个槽；若当前世代 `ACTIVE <= 21` 条，启动稀疏整理。
- **目标扇区选择**：
  1. **`fit` 路径**：若源扇区不是当前 `APPENDING` 扇区，且当前 `APPENDING` 扇区尾部连续 `ERASED` 空槽数 $\ge$ 源扇区 `ACTIVE` 条数，直接迁移至当前 `APPENDING` 扇区。
  2. **`new` 路径**：否则启用一个已验证的 `EMPTY` 扇区（两阶段提交新扇区头），作为新 `APPENDING` 扇区接收迁移记录。

### 4.2 协作式迁移状态序列
```text
DELETE_LOCATE -> DELETE_PROGRAM_START -> DELETE_POLL -> DELETE_VERIFY
  -> (若 ACTIVE <= 21)
MIGRATE_BEGIN_WRITE -> MIGRATE_SCAN_SOURCE
  -> MIGRATE_COPY_BODY -> MIGRATE_COPY_COMMIT -> MIGRATE_COPY_VERIFY
  -> MIGRATE_MARK_SOURCE (0xFFFF -> 0x5AA5)
  -> (全部 ACTIVE 复制标记完毕)
MIGRATE_COMMIT_WRITE
  -> MIGRATE_ERASE_START -> MIGRATE_ERASE_POLL -> MIGRATE_ERASE_VERIFY (16 页全 0xFF 校验)
```

### 4.3 断电重启恢复规则

| 断电发生位置 | 上电扫描恢复行为 |
| :--- | :--- |
| `MIGRATE_BEGIN` 提交前 | 忽略控制环未提交槽，源扇区记录保持原样 |
| 目标副本正文或提交字节写入中途 | 目标半写槽归为 `DIRTY` 跳过；源记录仍为 `ACTIVE`，按 `MIGRATE_BEGIN` 续作 |
| 目标副本已提交 (`0x00`)，源槽尚未标为 `MIGRATED (0x5AA5)` | 扫描识别同一迁移事务源/目标扇区中一对正文逐字节一致的重复 `{generation, recordId}`，以目标副本为准，自动续作将源槽补标为 `0x5AA5` |
| 部分源记录已完成迁移 | 根据 `MIGRATE_BEGIN` 逐槽比对，仅继续复制剩余 `ACTIVE` 源记录，不重复生成副本 |
| `MIGRATE_COMMIT` 提交前 | 不擦除源扇区；继续完成剩余复制/标记并提交 `MIGRATE_COMMIT` |
| `MIGRATE_COMMIT` 提交后、源扇区擦除前或擦除中途断电 | 目标扇区副本已生效；重启后自动重新对源扇区发起非阻塞擦除并逐页校验全 `0xFF` |
