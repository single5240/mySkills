# 板级 RTT 硬件在环（HIL）验证与故障注入规程

本参考文档提炼自 QW-603 (`N32G455_TensileTesting`) 存储切片 8/9/10/11 的板级 RTT 验证流程，规范如何在目标板上安全、可重复地执行自动化验证与故障注入。

---

## 1. 验证固件制作与双构建对比

1. **开启验证宏**：
   在测试模块头文件（如 `app_record_store_test.h`）中临时开启：
   ```c
   #define APP_RECORD_STORE_VALIDATION_ENABLE 1
   ```
2. **记录双配置资源基线**：
   分别构建默认生产配置（`ENABLE=0`）与验证配置（`ENABLE=1`），记录两者的 `Error / Warning` 数量以及 `Code / RO-data / RW-data / ZI-data` 字节数，确保生产固件不含测试状态机开销。
3. **测试收尾强制还原**：
   实板测试完成并执行 `cleanup` 恢复空白分区后，**必须在同一任务中将验证宏改回 `0`**，避免测试入口或故障注入钩子流入量产包。

---

## 2. 标准结果码与终态日志契约

所有异步验证命令（由主循环状态机分步推进）必须区分“已接受”与“终态完成”：

| 结果码 | 枚举名 | 语义说明 |
| ---: | :--- | :--- |
| `0` | `NONE` | 空闲无结果 |
| `1` | `PENDING` | 命令已接受，主循环状态机正在分步推进 |
| `2` | `OK` | 终态通过（伴随打印 `PASS: <subcommand> ...`） |
| `3` | `BUSY` | 底层服务正忙 |
| `4` | `NOT_READY` | 服务未初始化或处于恢复中 |
| `5` | `FULL` | 达到产品记录上限（如第 1,001 条拒绝写入验证） |
| `6` | `NO_SPACE` | 物理空扇区不足安全保留阈值 |
| `7~12` | `INVALID / NOT_FOUND / ALREADY_DELETED / PROTECTED / FLASH_ERROR / VERIFY_ERROR` | 精确映射错误分支供负向用例断言 |

---

## 3. 五阶段板级验证标准步骤

### 阶段一：健康基线与纯 RAM 自检（零擦写）
```text
ver
record_status
stat reset
record_test codec
```
- **判据**：`codec` 输出 `PASS: codec`，覆盖正常编解码向量、CRC 错误、未知版本、未提交字节和非法扩展字段。

### 阶段二：边界删除与连续序号一致性
```text
record_test append 10
record_test delete_pos first
record_test delete_pos middle
record_test delete_pos last
record_test list_check
```
- **判据**：正序与倒序分页遍历均连续编号为 `1..7`，重复删除返回 `ALREADY_DELETED` 且不产生额外 Flash 编程。

### 阶段三：满容量边界与删除后补存
```text
record_test append 993
record_test full_probe
record_test delete_pos middle
record_test append 1
```
- **判据**：`1000` 条时 `full_probe` 精确返回 `FULL` 且不破坏现有记录；删除 1 条后可正常补存第 1000 条。

### 阶段四：持久化边界软复位注入（模拟关键点掉电）
在稀疏扇区迁移或两阶段提交的 4 个关键持久化边界点埋设单次触发钩子（写入 Flash 刚返回立即调用 `NVIC_SystemReset()`）：
1. `reset_at migrate_begin`：`MIGRATE_BEGIN` 控制记录刚提交后复位；
2. `reset_at copy_commit`：第 $k$ 条目标副本刚完成两阶段提交、源槽尚未标 `MIGRATED` 时复位；
3. `reset_at mark_source`：源槽刚标为 `MIGRATED (0x5AA5)`、尚未全部迁完时复位；
4. `reset_at migrate_commit`：`MIGRATE_COMMIT` 刚提交、源扇区尚未擦除完成时复位。
- **判据**：每次复位重启后执行 `record_status` 与 `record_test list_check`，记录总数、主键集合与正反序内容 100% 一致，无重复主键、无丢失。

### 阶段五：软件故障注入与并发实时性核查
- 注入擦除启动失败、擦除 `BUSY` 超时、读回非 `0xFF` 校验失败，确认失败扇区保持隔离而不被误标为 `EMPTY`。
- 执行 `stat` 核查全程实时性：`iwdg_reset=0`，高频 ADC `error=0, timeout=0, overrun=0, recovery=0`，主循环最大周期 `< 10 ms`，栈涂漆最低余量 `> 512 B`。
