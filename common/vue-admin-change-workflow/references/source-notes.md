# 来源与取舍

维护技能时读取；执行开发任务不需要联网读取这些来源。以下为 2026-11-03 查阅到的公开资料，Stars 与安装计数仅代表平台指标，不等于独立用户、满意度或质量认证；不把这些易变数字固化为选择规则。

| 来源                                                                                                                                       | 借鉴                                       | 裁剪或不采用                                                                        |
| ------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------ | ----------------------------------------------------------------------------------- |
| [Anthropic skill-creator](https://github.com/anthropics/skills/blob/main/skills/skill-creator/SKILL.md)                                    | 明确触发、渐进披露、用真实提示评估决策     | 不为每次技能维护强制大规模基准或专属工具依赖                                        |
| [Superpowers systematic-debugging](https://github.com/obra/superpowers/blob/main/skills/systematic-debugging/SKILL.md)                     | 先复现与隔离边界，单一可证伪假设后再修复   | 不把所有小改变成固定仪式，不照搬外部指令的权限                                      |
| [Superpowers verification-before-completion](https://github.com/obra/superpowers/blob/main/skills/verification-before-completion/SKILL.md) | 结论对应实际命令与新证据                   | 通过且后续修改未影响的检查不机械重跑                                                |
| [Superpowers writing-plans](https://github.com/obra/superpowers/blob/main/skills/writing-plans/SKILL.md)                                   | 文件边界、可独立验证的小切片               | 不强制每项写计划文档、每步 Commit 或委派子代理                                      |
| [Vue 社区 vue-best-practices](https://github.com/vuejs-ai/skills/blob/main/skills/vue-best-practices/SKILL.md)                             | 状态归属、明确数据流、组合式逻辑的生命周期 | 不要求所有任务预读全部参考，不强制拆每个页面；该项目自称社区实验，并非 Vue 官方标准 |
| [Vue 社区 composables](https://github.com/vuejs-ai/skills/blob/main/skills/vue-best-practices/references/composables.md)                   | 聚合状态、动作与清理，处理失效异步副作用   | 按业务复杂度抽取，不按固定文件大小拆分                                              |
| [Vercel React Best Practices](https://github.com/vercel-labs/agent-skills/blob/main/skills/react-best-practices/SKILL.md)                  | 按影响优先、分主题按需读取                 | 只借鉴组织方式；不把 React / Next 专属规则移植到 Vue                                |

技术规则仍需核对 Vue 官方的[响应式基础](https://vuejs.org/guide/essentials/reactivity-fundamentals.html)、[侦听器清理](https://vuejs.org/guide/essentials/watchers.html#side-effect-cleanup)、[性能](https://vuejs.org/guide/best-practices/performance.html)，以及 [Vite 环境变量](https://vite.dev/guide/env-and-mode)、[OWASP 授权](https://cheatsheetseries.owasp.org/cheatsheets/Authorization_Cheat_Sheet.html)。例如，不采纳社区参考中“原始值总用 shallowRef”的绝对优化建议；应按语义、版本与测量决策。

本地嵌入式六阶段工作流贡献风险裁剪与验证层级的思路；保留六阶段通过条件，排除 ISR / DMA / Flash 等无关硬件步骤。具体项目的端口、故障快照、API 类型路径和权限标识留在工程 AGENTS.md 与编码技能，不固化到本技能。

## 评估方式

使用 `../evals/evals.json` 的正向、反向及约束场景检查技能是否作出正确决定。复杂修订可在可用且获授权的独立环境做行为评估；保存每次实际输出、执行命令、满足/未满足的可观察条件，必要时比较无技能或旧版输出。

元数据合法、链接有效、评估用例存在，只证明结构完整；不等于已经运行行为评估。本次未执行独立代理对照实验，不能宣称质量或成功率提升。用例中“不改变什么”是范围判据，不要求模型逐字输出特定标题或句式。