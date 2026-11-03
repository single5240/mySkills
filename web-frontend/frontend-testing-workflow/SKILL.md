---
name: frontend-testing-workflow
description: >-
  [Scope: global] 为 Vue 3 + TypeScript + Vite 前端项目提供分层测试流程、Vitest 单元测试设计、纯逻辑抽离与验证交付规范。
  用于编写或维护 API 契约测试、查询参数清洗测试、复杂确认/提示逻辑测试、Hooks 异步竞态回归、缺陷复现测试以及执行静态/类型/构建/浏览器联调验证。
---

# 前端测试与验证工作流

技术方向：`web-frontend`；使用范围：`global`。面向 Vue 3 + TypeScript + Vite 前端工程（包括 Element Plus / Pinia / Vue Router 及 RuoYi-Vue-Plus / plus-ui 派生后台）的分层测试与证据验证指南。核心原则是：**纯逻辑抽离单测化、网络与外部副作用替身化、验证证据分层化**，不把静态通过或 Mock 单测通过冒充为真实后端与硬件设备实测通过。

## 1. 测试分层与触发矩阵

接到前端功能开发、逻辑修改、Bug 修复或补充测试任务时，先从目标工程的 `package.json`、锁文件和现有 `*.test.ts` 确认实际安装的测试运行器、lint 和类型工具，再按改动性质选择测试层级：

| 改动类型 | 必选验证层 | 是否新增/更新 Vitest 单测 | 按需读取参考 |
| :--- | :--- | :--- | :--- |
| 纯文案、注释、简单样式微调 | 格式化 / lint 检查、浏览器视觉与布局检查 | 否（不为无逻辑改动机械补测试） | [浏览器与链路验收清单](./references/browser-and-e2e-checklist.md) |
| 查询参数清洗、范围校验、权限与状态分流、确认/结果文案拼接 | 靶向 `vitest`、lint、类型检查 | **是**（抽离纯函数并覆盖正常/边界/异常） | [Vitest 单元测试与 Mock 规范](./references/vitest-patterns.md) |
| API 请求组装、特殊 Header、批量/条件删除或设备控制参数 | 靶向 `vitest`（Mock 请求封装层）、类型检查 | **是**（断言实际请求 `url`/`method`/`params`/`data`） | [Vitest 单元测试与 Mock 规范](./references/vitest-patterns.md) |
| 公共 Hooks、异步 Loading、并发竞态、错误回滚 | 靶向 `vitest`、类型检查、lint | **是**（控制 Promise 时序验证状态归属与回滚） | [Vitest 单元测试与 Mock 规范](./references/vitest-patterns.md) |
| 路由守卫、登录鉴权、Vite 代理、打包配置或跨模块公共层 | lint、类型检查、生产构建、浏览器联调 | 视是否含纯逻辑分支而定 | [浏览器与链路验收清单](./references/browser-and-e2e-checklist.md) |

## 2. 标准 5 步测试工作流

### 步骤 1：勘察基线与可测边界

- 检查工作区 `git status --short`、`package.json`、`vite.config.ts` 及已有测试文件，确认测试运行器（如 `vitest`）与路径别名（如 `tsconfigPaths` 或 `@/`）。
- 识别目标功能中的**高风险分支**：空值与空白字符清洗、单端时间范围、跨页/全量操作防护、权限组合判断、大整数 ID / 字符串编号精度、异步失败提示与状态回滚。

### 步骤 2：抽离可测纯逻辑（Testable Extraction）

不要把复杂的参数转换、条件判断和提示文案硬编码在 `.vue` 组件模板或事件回调内部：

- 在同业务模块目录（如 `src/api/<module>/<business>/` 或独立 helper 文件）抽离无 DOM 依赖的纯函数。
- 保持纯函数输入输出明确：输入为 `Query`、`Form`、权限标志或后端 `VO`，输出为清洗后的请求参数（不直接篡改原响应式对象引用）、布尔判定结果或结构化提示对象 `{ type, message }`。
- `.vue` 页面仅负责收集响应式状态、调用纯函数、触发弹窗/API 并渲染结果。

### 步骤 3：编写三类核心用例（Happy / Edge / Failure）

在被测模块同目录创建就近测试文件 `<name>.test.ts`，每个行为分组（`describe`）覆盖：

1. **正常路径（Happy Path）**：标准筛选条件、完整时间范围、预期状态流转与成功提示。
2. **边界条件（Edge Cases）**：`undefined` / `null` / 空串 / 纯空格参数清洗、原查询对象不可变性、单端日期缺失、分页参数与业务筛选隔离。
3. **异常、安全与契约边界（Failure & Safety）**：
   - 无筛选条件时的全量风险警告；
   - 无权限或前置条件不满足时的降级提示；
   - 设备控制等外部副作用的诚实表述（区分“命令已发送 `SENT`”、“发送失败 `SEND_FAILED`”与“设备实际执行完成”，验证失败不自动重发）；
   - 使用 `vi.mock` 拦截请求封装（如 `@/utils/request`）断言真实发出的 HTTP 契约，不向真实后端或硬件发包。

详细代码模板与断言模式见 [Vitest 单元测试与 Mock 规范](./references/vitest-patterns.md)。

### 步骤 4：执行靶向验证门禁

按目标工程实际配置的包管理器与脚本按序执行（Windows PowerShell 策略受限时优先使用 `.cmd`，如 `pnpm.cmd`）：

```powershell
# 1. 运行目标单元测试（单次执行，快速反馈）
pnpm.cmd exec vitest run <目标测试文件路径>

# 2. 检查改动文件的格式与静态规则（避免全仓格式化污染无关文件）
pnpm.cmd exec oxfmt --check <修改或新增的文件路径>
pnpm.cmd run lint

# 3. 检查 TypeScript 与 Vue 模板类型
pnpm.cmd exec vue-tsc --noEmit

# 4. 涉及构建配置、公共层或大范围导入时执行生产构建
pnpm.cmd run build:prod
```

- 修复可复现缺陷时，优先验证测试在旧逻辑下失败、修复后通过。
- 若工程存在与本次改动无关的既有 lint / 类型报错基线，需核实报错位置，确保**本次修改涉及的文件零新增错误**，不在未授权时大面积重构无关代码。

### 步骤 5：浏览器交互与分层交付报告

静态检查与单测通过后，涉及 UI 交互、代理转发或后端接口的任务按 [浏览器与链路验收清单](./references/browser-and-e2e-checklist.md) 核验。交付时必须分开说明：

- **测试覆盖范围**：被测模块、覆盖的正常/边界/异常场景及断言重点；
- **实际执行结果**：单测通过数、静态检查、类型检查与构建结果；
- **未验证项与证据边界**：明确区分 Mock 单测通过与真实后端联调、真实设备实测，未执行的链路如实列出。

## 3. 避免事项

- 不写仅把实现代码原样复制一遍的“镜像断言”，围绕业务契约与输入输出边界断言。
- 不在单元测试中发起真实网络请求、触发真实文件下载或下发真实硬件控制命令。
- 不为制造测试通过而在生产代码中跳过鉴权、移除防重复提交保护或滥用 `any`。
- 不在通用技能或测试代码中硬编码本机绝对路径或暴露敏感凭据。
