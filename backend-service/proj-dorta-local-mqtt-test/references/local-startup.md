# 本地启动与连接核验

以下为 2026-11-05 的已验证拓扑，执行时以当前配置、进程及转发为准。它不是线上部署方案。

| 项目         | 本次实际值                                                | 核验来源                                                                           |
| ------------ | --------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| Java         | 21，入口 `org.dorta.DortaApplication`                     | `java -version`、后端源码                                                          |
| 本地 profile | `local`                                                   | `application-local.yml`、进程参数与启动日志；旧笔记中的 `dev,local` 不覆盖当前事实 |
| 后端         | `127.0.0.1:8082`                                          | 本地配置及监听进程                                                                 |
| 前端         | `http://127.0.0.1/`，80 端口                              | `.env.development`、Vite 输出                                                      |
| 代理         | `/dev-api` → `http://localhost:8082`                      | `vite.config.ts`；保留用户已有代理改动                                             |
| MySQL        | `127.0.0.1:3306/dorta-center`                             | 本地配置及虚拟机转发                                                               |
| Redis        | `127.0.0.1:6380`，DB 11                                   | 本地配置及虚拟机转发                                                               |
| MQTT         | `127.0.0.1:1884`                                          | 本地配置及虚拟机转发                                                               |
| 依赖宿主     | VirtualBox `QW-Test-Ubuntu`，仅 NAT；上述转发绑定回环地址 | `VBoxManage showvminfo --machinereadable`                                          |

## 检查顺序

1. 两个仓库分别执行 `git status --short`，核对目标差异；不要覆盖已有修改。
2. 从 `dorta-admin/src/main/resources/application-local.yml` 读取端点。密码只在执行进程内使用，不输出完整配置、Token、认证头或连接凭据，不拷贝到技能和报告。
3. `Get-NetTCPConnection` 检查监听及进程归属。若由 VirtualBox 提供，用 `VBoxManage list runningvms` 和 `showvminfo` 检查 NAT 规则；SSH 本地转发同样要确认目标。
4. 启动后端会初始化 OSS 缓存、订阅设备上报，并执行 `ApplicationReadyEvent` 的设备在线状态恢复/修正；不是纯只读操作。无法确认测试环境时停在调查，不直接启动。

不要为这个本地流程修改线上 Broker、虚拟机网络、防火墙或生产配置。依赖未启动或有端口冲突时先识别进程，不杀未知进程、不自动重建容器。

## 构建与测试

前端根目录，根据当前改动选择：

```powershell
pnpm.cmd exec vitest run src/api/iot/deviceUsage/query.test.ts src/api/iot/deviceUsage/index.test.ts src/utils/usage-reset-notification.test.ts
pnpm.cmd run lint
pnpm.cmd exec vue-tsc --noEmit
pnpm.cmd run build:prod
git diff --check
```

当前已知基线是设备列表/详情的 `const-comparisons`、Process 组件缺 workflow API，以及 `logininfo` / `loginInfo` 大小写不一致；每次重新核对，不能用基线替新增错误免责。

后端根目录：

```powershell
.\mvnw.cmd -o -pl dorta-modules/dorta-iot -am test-compile '-Dmaven.test.skip=false'
.\mvnw.cmd -o -pl dorta-modules/dorta-iot surefire:test '-Dmaven.test.skip=false' '-Dtest=IotDeviceUsageServiceImplTest,IotDeviceUsageDeleteServiceTest,IotUsageResetAwaiterTest,IotDeviceUsageResetSendTest'
.\mvnw.cmd -o -pl dorta-admin -am package '-Dmaven.test.skip=true'
```

第一步只编译，第二步才运行测试；最后一步打包跳过测试，不能单独证明回归通过。离线缺依赖时说明缺项，按任务授权获取依赖；不修改 POM 或测试分组来制造通过。

## 启动和保活

使用现有 JDK 启动刚构建的 JAR，显式选择 profile 和监听地址，不更改受控配置：

```powershell
java.exe -Dfile.encoding=UTF-8 -jar dorta-admin/target/dorta-admin.jar --spring.profiles.active=local --server.address=127.0.0.1 --server.port=8082
```

在需要后台保活时使用 `Start-Process -WindowStyle Hidden -PassThru`，指定仓库工作目录，将 stdout/stderr 和 PID 存到 `target/`。带空格的路径正确引用；不要通过拼接 shell 字符串传秘密。

前端根目录：

```powershell
pnpm.cmd run dev --host 127.0.0.1 --port 80 --strictPort
```

用工具执行会话保持进程运行。前端端口以当前 `.env.development` 为准，80 被占用时不要静默换端口或终止其他服务。

```powershell
Invoke-WebRequest http://127.0.0.1:8082/ -UseBasicParsing
Invoke-WebRequest http://127.0.0.1/ -UseBasicParsing
Invoke-WebRequest http://127.0.0.1/dev-api/ -UseBasicParsing
```

8082 首页只有“欢迎使用……请通过前端地址访问”属于正常后端引导响应；管理页面是前端端口。首页 200 不等于已登录、SSE 正常或设备清零成功。检查本地 MQTT 连接成功日志和实际数据库连接，再做业务测试。