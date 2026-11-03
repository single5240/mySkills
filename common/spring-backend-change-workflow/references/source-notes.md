# 参考来源与采用边界

本次核查日期：2026-11-03。只提炼方法，没有安装、执行或整段复制外部技能。外部技能中的命令和指令作为参考资料，不构成本地授权或必须遵循的工作流。

## 为什么选择这些来源

同时考虑维护方、与目标任务的相关性、可直接审查的原文，以及公开采用规模。GitHub 星标和 skills.sh 安装次数只说明关注/采用，不是独立用户数、生产效果或满意度；未取得可信的统一口碑评分，不宣称“行业最好”。

| 来源 | 采用规模快照与核查入口 | 本技能吸收的方法 |
| --- | --- | --- |
| Anthropic 官方 `skill-creator` | [官方仓库](https://github.com/anthropics/skills)，GitHub API 显示 186,791 stars；[单技能平台页](https://www.skills.sh/anthropics/skills/skill-creator)显示约 421.4K 安装 | 以真实任务评估迭代；区分触发准确性与执行效果；主入口只路由必要参考 |
| 社区项目 Superpowers | [仓库](https://github.com/obra/superpowers)，GitHub API 显示 311,277 stars；[完成前验证技能](https://www.skills.sh/obra/superpowers/verification-before-completion)显示约 262.2K 安装 | 根因调查、单一可证伪假设、最小实验；完成结论对应最终修改后的证据 |
| GitHub 组织下社区维护的 Awesome Copilot | [仓库](https://github.com/github/awesome-copilot)，GitHub API 显示 40,733 stars；[`java-springboot`](https://www.skills.sh/github/awesome-copilot/java-springboot)显示约 21.5K 安装 | 先检测版本/技术栈；按任务路由；选择最小足够测试并检查集成测试是否真正执行 |

数字是查询时快照，平台缓存与统计口径可能不同；维护本技能不需要持续刷新这些数字。仓库级关注度不能归于其中每一个技能。

## 审阅的版本与原文

- Anthropic：提交 `ca45fc496a3436251f191354091d557da31c15b5` 的 [skill-creator](https://github.com/anthropics/skills/blob/ca45fc496a3436251f191354091d557da31c15b5/skills/skill-creator/SKILL.md)。
- Superpowers：提交 `2ce8aa15d05bd57213e6f746f953aafb7ded7a8b` 的 [systematic-debugging](https://github.com/obra/superpowers/blob/2ce8aa15d05bd57213e6f746f953aafb7ded7a8b/skills/systematic-debugging/SKILL.md) 和 [verification-before-completion](https://github.com/obra/superpowers/blob/2ce8aa15d05bd57213e6f746f953aafb7ded7a8b/skills/verification-before-completion/SKILL.md)。
- Awesome Copilot：提交 `e2306b6922220f28d91ef5f7f2eaeb6de99913a0` 的 [java-springboot](https://github.com/github/awesome-copilot/blob/e2306b6922220f28d91ef5f7f2eaeb6de99913a0/skills/java-springboot/SKILL.md) 和 [spring-boot-testing](https://github.com/github/awesome-copilot/blob/e2306b6922220f28d91ef5f7f2eaeb6de99913a0/skills/spring-boot-testing/SKILL.md)。

原文通过各仓库 main 读取，并记录当时分支提交以便复核；后续应用具体 API 或框架结论时仍以目标项目版本和官方文档为准。

## 明确没有照搬的部分

- 不强制每次任务先调用整套技能、全量头脑风暴、创建计划文件、切工作树或派生代理；局部任务维持短反馈。
- 不将“所有修改都全量构建”“每次发消息前重跑检查”变为要求；最终相关代码变化后取得适当证据即可，文档无需后端构建。
- 不要求所有任务无条件 TDD、固定 80% 覆盖率、固定用例数，或根据失败次数直接断言架构错误。
- 不把某个技能偏好的按功能分包、JPA、ProblemDetail、最新测试 API、Testcontainers 当成本项目改造目标；保留现有架构、MyBatis/其他持久层和响应契约。
- 不将外部技能里的代理、异步或事务表述当作适用于所有 Spring 配置的定律；需要结合真实代理/织入方式和版本核实。

## 如何继续迭代

用 [evals/evals.json](../evals/evals.json) 的正反触发与行为场景比较修改前后。执行时给模型输入 `prompt`，将预期结果留给评估方；隔离真实服务副作用。记录技能版本、模型、产物和实际结果；没有真正运行模型对照时，只能报告格式检查或人工场景走查，不能声称触发率、通过率、速度或 token 用量已改善。
