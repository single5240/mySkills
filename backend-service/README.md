# Backend Service — 后端服务与中间件技能 (`backend-service/`)

本目录存放面向服务端应用（Spring Boot 多模块工程、REST API、权限控制、数据库事务、Redis 缓存、消息队列与物联网服务端 MQTT 通信）的研发、排障与验证技能。

## 适用场景
- **后端业务变更与缺陷定位**：接口鉴权与数据权限边界、Spring 事务与 Redis 缓存一致性、并发与幂等控制、构建 profile 与模块依赖检查。
- **物联网与异步通信**：服务端 MQTT 消息发布/订阅、设备应答关联、迟到/重复消息幂等处理、异步任务异常恢复与回归测试。

## 使用范围（Scope）命名约定
- **`global`**（默认无前缀）：跨后端项目复用的架构、排障与验证工作流，如 `spring-backend-change-workflow`。
- **`project`**（`proj-<name>-*`）：特定业务系统的领域模型、私有协议网关或部署规范。
- **`experimental`**（`exp-*`）：孵化中的后端分析或测试辅助脚本。

## 技能索引清单

| Skill 目录名 | 使用范围 (Scope) | 适用技术栈 | 简述 |
| :--- | :--- | :--- | :--- |
| [`spring-backend-change-workflow`](./spring-backend-change-workflow/SKILL.md) | `global` | Spring Boot / Java / Redis / MQTT | Spring 后端根因定位、最小变更与证据验证；涵盖事务/Redis、异步、MQTT、提交分组与推送二次确认，附公开来源及行为评估用例 |
