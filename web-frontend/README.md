# Web Frontend — Web 前端开发与测试技能 (web-frontend/)

本目录存放面向现代 Web 前端工程（Vue 3、TypeScript、Vite、Element Plus、Pinia、Vue Router 及管理后台）的功能演进、状态契约、分层测试与联调交付技能。

## 适用场景
- **管理端功能变更与缺陷排查**：列表/表单异步状态、竞态与取消、登录鉴权与动态路由、权限指令（-hasPermi）、Vite 代理排障与 HTTP 设备控制边界。
- **前端分层测试与验证**：纯逻辑抽离、Vitest 单元测试（Happy / Edge / Failure）、请求封装层 Mock 契约断言、异步 Hooks 测试及静态/类型/打包/浏览器多维验收。

## 使用范围（Scope）命名约定
- **global**（默认无前缀）：跨前端工程复用的工作流与测试规范，如 ue-admin-change-workflow、rontend-testing-workflow。
- **project**（proj-<name>-*）：绑定特定前端项目的页面模板或私有组件库规范。
- **experimental**（exp-*）：孵化中的前端自动化脚本或实验性技能。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 适用技术栈 | 简述 |
| :--- | :--- | :--- | :--- |
| [ue-admin-change-workflow](./vue-admin-change-workflow/SKILL.md) | global | Vue 3 + TS + Vite 管理后台 | Vue 3 管理端的根因定位、异步状态与接口权限检查、风险分级及验证交付；沿用项目 CRUD 技能，附公开来源取舍及行为评估场景 |
| [rontend-testing-workflow](./frontend-testing-workflow/SKILL.md) | global | Vue 3 + TS + Vite + Vitest | Vue 3 + TS 前端分层测试工作流：纯逻辑抽离、Vitest 单测与 Mock 契约模式、异步竞态回归及静态/类型/构建/浏览器链路验收 |