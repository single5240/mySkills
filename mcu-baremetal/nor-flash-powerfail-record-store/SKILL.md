---
name: nor-flash-powerfail-record-store
description: >-
  [Scope: global] 外部 SPI NOR Flash（如 W25Q128）掉电安全定长记录存储、两阶段提交、单条原位墓碑删除、稀疏扇区非阻塞迁移与世代控制环设计。
  当用户需要在裸机或 RTOS 下使用 NOR Flash 存储历史测试记录/日志、解决扇区擦除阻塞看门狗、实现掉电恢复或单条删除与空间回收时使用此技能。
---

# 外部 NOR Flash 掉电安全记录存储与稀疏迁移 (NOR Flash Power-Fail-Safe Record Store)

## 1. 适用范围与背景 (Metadata)

- **技术方向**：`mcu-baremetal`
- **使用范围 (Scope)**：`global`（全平台通用，提炼自 N32G455 + W25Q128 量产项目）
- **适用硬件与场景**：SPI NOR Flash（W25Q128 / GD25Qxx 等，页 256B、扇区擦除 4 KiB），裸机协作式主循环（含 1 s 独立看门狗 IWDG），严禁 `malloc/free` 动态内存。

---

## 2. 核心架构法则 (Core Architecture Principles)

1. **驱动与业务严格分层**：
   - `DRV` 层（如 `drv_w25q128`）只负责 SPI 时序、JEDEC ID 校验、256B 页边界拆分、4 KiB 对齐检查，以及**非阻塞启动 + 单次 `BUSY(WIP)` 查询接口**（`PageProgramStart` / `SectorEraseStart` / `PollOperation`）。
   - `APP` 层（如 `app_record_store` + `app_record_codec`）独占管理分区边界、世代号（Generation）、记录编解码、两阶段提交与扇区回收状态机。
2. **利用 NOR Flash `1 → 0` 单向编程特性的两阶段提交与原位删除**：
   - 擦除态为全 `0xFF`。64 字节定长槽内独立设置 **生命周期码（2B，偏移 59）**、**CRC-16/MODBUS（2B，偏移 61，覆盖 0~58）** 与 **提交标志（1B，偏移 63）**。
   - **保存两阶段提交**：① 先编程正文与 CRC（保持生命周期码 `0xFFFF`、提交标志 `0xFF`）并读回逐字节校验；② 单独将提交标志从 `0xFF` 编程为 `0x00` 并读回确认。
   - **单条原位删除（零擦除开销）**：仅将已提交有效记录的生命周期码从 `ACTIVE (0xFFFF)` 单向编程为 `DELETED (0xA55A)`，无需擦除扇区或重写后续记录；列表按可见顺序动态生成连续显示序号 `1~N`。
3. **3 扇区控制环 + 动态数据池 + 毫秒级逻辑清空**：
   - 前 3 个 4 KiB 扇区组成**世代控制环**，后 $N$ 个扇区组成**动态数据池**（始终保留至少 **2 个经 4 KiB 整扇区校验为 `0xFF` 的空扇区**）。
   - **清空全部**：确认路径绝不执行耗时的扇区擦除，只向控制环追加一条新世代控制记录（耗时仅数毫秒）；旧世代记录在扫描时自动不可见，新世代继续追加在当前数据扇区尾部的 `ERASED` 空槽中。
4. **稀疏扇区整理（Compaction）与掉电续作**：
   - 单条删除成功后检查目标所在 4 KiB 扇区：当 63 个数据槽中 `ACTIVE <= 21`（空置率 $\ge 2/3$）时触发整理。
   - 通过控制环持久化 `MIGRATE_BEGIN` 与 `MIGRATE_COMMIT` 事务日志，将仍为 `ACTIVE` 的记录复制提交到当前追加扇区（或新空扇区），再将源槽标为 `MIGRATED (0x5AA5)`，最后非阻塞擦除源扇区并逐页读回校验全 `0xFF`。

> 详细的 64B 槽字节布局、扇区状态转移表与 7 类断电位置恢复矩阵请参阅 [`references/slot-layout-and-compaction.md`](./references/slot-layout-and-compaction.md)。

---

## 3. 核心执行流程 (Step-by-Step Workflow)

1. **首次擦写前安全门禁**：
   - 检查目标分区地址范围是否与固件代码、升级区重叠；首次上板擦写前必须先**只读检查目标分区现有内容与状态寄存器写保护位**，未经用户确认不得自动把非空历史分区当作空白擦除。
2. **实现纯 RAM 显式小端编解码器 (`app_record_codec`)**：
   - 按固定偏移手工序列化/反序列化，严禁直接将未对齐的 C `struct` 强转为字节流写入 Flash。
   - 世代号与扇区分配序号比较必须使用**无符号半量程回绕比较**（覆盖 `0xFFFFFFFE -> 0xFFFFFFFF -> 0x00000000`）。
   - 编写启动期或 CLI 触发的纯 RAM 固定字节向量自检（覆盖 `ACTIVE`、`DELETED`、`MIGRATED`、CRC 错误、未提交 `0xFF`、保留区非 `0xFF` 污染）。
3. **改造非阻塞 Flash 驱动接口**：
   - 4 KiB 扇区擦除物理耗时 `40~200 ms`，页编程耗时 `1~3 ms`。运行期严禁调用死等 `WIP==0` 的同步接口。
   - 拆分为：`Start` 下发 SPI 指令（约 `5 μs`）立即返回 $\rightarrow$ 主循环每轮调用 `Poll` 读取 1 次状态寄存器（约 `5 μs`）并检查无符号 tick 超时。
4. **实现协作式记录状态机 (`AppRecordStore_Process`)**：
   - 上电初始化在 `Iwdg_Init()` 启动前完成分区扫描，构建紧凑的 RAM 扇区目录与序号位图（不分配全量记录 RAM 缓存）。
   - 运行期每次 `Process()` 最多执行一次有界读取、启动一次编程/擦除、轮询一次 `BUSY` 或校验一个 256B 页，确保主循环单圈耗时 `< 10 ms` 并按时返回主循环唯一喂狗点 `Iwdg_Feed()`。

---

## 4. 关键安全与并发约束检查清单

- [ ] **禁止跨页回绕**：页编程（256B）必须在页边界切分；扇区擦除必须校验 `address & 0xFFFU == 0U`。
- [ ] **半写脏槽隔离 (`DIRTY`)**：启动扫描将槽分为 `ERASED`（全 `0xFF`）、`VALID`（已提交且 CRC 正确）、`DIRTY`（半写/CRC 错/异常状态）。`DIRTY` 槽只跳过、不终止扫描，且**绝不在未擦除扇区的情况下原地重写**。
- [ ] **保存与归零联锁**：测试完成瞬间冻结不可变快照（`Snapshot`）与 `completionToken` 去重令牌；保存事务进行中锁存结果，失败时保留快照供重试，不因后台擦除暂时 `BUSY` 记为永久失败。
- [ ] **单事务所有者**：保存、列表分页扫描、单条删除、稀疏迁移与后台擦除共享同一个记录服务状态机，严禁并发访问 SPI Flash。

---

## 5. 验证与调试方法 (Verification)

1. **编译与资源审计**：
   - 检查 `Code / RO-data / RW-data / ZI-data` 增量，确认列表仅缓存当前页（如 5 条）及最多 21 条迁移槽位清单，且栈涂漆最低余量充足（建议 `> 512 B`）。
2. **板级 RTT 分片与故障注入验证**：
   - 依次验证：固定向量 `codec` 自检 $\rightarrow$ 空分区自动建代 $\rightarrow$ 满容量追加与上限拒绝（`FULL`） $\rightarrow$ 首/中/末条删除与正反序连续编号 $\rightarrow$ `22` 条不迁移 / `21` 条触发迁移 $\rightarrow$ 4 个持久化边界（`MIGRATE_BEGIN`、目标副本提交、源标 `MIGRATED`、`MIGRATE_COMMIT`）软复位续作 $\rightarrow$ 擦除失败注入恢复。
   - 全程监控 `IWDG` 复位次数为 0，高频传感器 DMA 采集（如 AD7190）无丢包或超时恢复。
