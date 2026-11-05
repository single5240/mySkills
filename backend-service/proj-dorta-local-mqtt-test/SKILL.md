---
name: proj-dorta-local-mqtt-test
description: >-
  [Scope: project] 在 Dorta Vue/Spring 工程中启动 Windows 本地前后端，使用本地 MQTT 模拟指定设备，
  准备少量使用记录并验证删除、清零应答、超时及 SSE 提示。适用于修改后的本地联调和回归验证；不用于线上部署或真实设备控制。
---

# Dorta 本地设备链路测试

## 工作边界

- 项目专用：`dorta-ui` + `dorta-center`，Java 21、Vue/Vite、MySQL、Redis、mica-mqtt。先读目标仓库规则并核对当前实现，不将本文快照当作永久契约。
- 明确前后端根目录、测试设备编码和测试范围。所有造数、模拟上报、删除及清零只允许指定设备；示例中的 `cs66666666666` 不构成下一次测试的默认授权。
- 本地地址不能单独证明环境隔离。先核实端口监听进程及 VirtualBox/SSH 转发目标，确认是本地测试依赖，再启动业务服务或写测试数据。不要修改线上连接、Broker、设备配置或全局防火墙。
- 造数和模拟上报会修改本地库：造数只增加该设备记录；模拟器更新在线状态和次数。用户已授权相应本地测试时直接执行，不循环确认；技能调用本身不授权删数据或操作真实设备。
- 登录使用正常账号和验证码。没有浏览器工具或测试凭据时，由用户在页面操作；不提取其他会话 Token、不读 Redis 验证码绕过认证、不关闭权限或验证码。

## 按任务推进

1. **确认环境**：检查 Git 工作区、本地配置及转发。需要启动时读 [本地启动与连接核验](references/local-startup.md)。保留已有进程和用户配置，不重复启动、不复用旧 JAR 或旧 PID。
2. **分层验证**：先运行目标单测及所需编译/构建；公共 Maven 模块的 Surefire 分组故障可用“reactor `test-compile` + IoT 模块 `surefire:test`”，不能将零测试当通过。命令见启动参考。
3. **只读基线**：用下方工具的 `Inspect` 查询该设备 ID、有效记录数、次数及最后上报时间。页面显示空但库有记录时，核对环境和日期筛选，不直接认定库为空。
4. **本地模拟**：核对当前协议入口后，用 `Simulator` 模拟该设备，独立客户端 ID、精确主题、仅本地 Broker。支持成功和静默超时，默认 30 分钟自动退出；不模拟充值、锁机或参数设置。
5. **页面联调**：按 [验收场景与证据](references/usage-reset-cases.md) 先做部分删除，再做无时间筛选的单设备全删。需要记录时用 `Seed`，每次最多 10 条，记下新增 ID；不自动复活或回填旧记录。
6. **只读复核**：`Review` 核对逻辑删除、`0x84` 指令/ACK、`0x85` 状态与次数；检查后端定向推送和模拟器日志。前端是否出现弹窗要用浏览器或用户观察单独确认。

## 可执行工具

工具入口为 [scripts/local-test.cmd](scripts/local-test.cmd)，调用同目录 PowerShell/Java 实现；不需要 Python，不向项目新增依赖。包装器仅对当前 PowerShell 进程使用执行策略 Bypass，不修改全局策略。PowerShell 脚本使用 UTF-8 BOM，兼容 Windows PowerShell 5.1 的中文解析。复用当前 `dorta-admin.jar` 的库和已编译协议类，运行产物放入后端 `dorta-admin/target/skill-local-mqtt-test/`。**先重新构建后端，再 Prepare**，不要在修改代码后测试旧产物。

参数由当前工作区确定，示例路径只描述相邻目录关系：

```powershell
$tool = '<本技能目录>/scripts/local-test.cmd'
$backend = (Resolve-Path '../dorta-center').Path  # 从前端根目录执行
$device = 'cs66666666666'                       # 替换为本轮获准的设备

& $tool -Action Prepare -BackendRoot $backend
& $tool -Action SelfTest -BackendRoot $backend
& $tool -Action Inspect -BackendRoot $backend -DeviceNo $device

# 仅当本轮已授权在本地准备该设备的测试记录/模拟状态时执行
& $tool -Action Seed -BackendRoot $backend -DeviceNo $device -Count 3 -AllowWrite
& $tool -Action Simulator -BackendRoot $backend -DeviceNo $device -Mode Success -DurationSeconds 1800 -AllowWrite
```

`Simulator` 前台运行，使用工具执行会话或隐藏的独立进程保持运行。同一设备只运行一个模拟器，已有实例用 `SetMode` 切换，不重复启动导致客户端冲突或重复应答。停止只针对本次进程/会话，不执行全局 Java/Node 终止。它会读取该设备当前次数作为初始值；可显式传 `-UsedCount` / `-SurplusCount` 模拟非零次数，二者范围 0～65535。

其他终端可切换下一条清零命令的应答方式，并只读检查：

```powershell
& $tool -Action SetMode -BackendRoot $backend -DeviceNo $device -Mode Timeout
& $tool -Action Review -BackendRoot $backend -DeviceNo $device -Since '2026-11-05T11:08:00'
```

`SetMode` 只影响下一条清零命令，不会主动下发命令。完整删除后已无记录，再测另一场景需重新明确准备少量模拟记录；不能用独立清零按钮代替删除联动的超时测试。

## 交付口径

报告实际环境、设备、有效记录数、命令次数/流水号、应答与超时结果、次数和剩余次数变化、浏览器观察及未验证项。区分单测、构建、HTTP、MQTT 模拟、推送和真实设备；模拟成功不能证明线上/实机成功。创建技能或工具验证时，默认仅做 SelfTest 和已授权的只读 Inspect，不自动重演造数或清零。