# 项目工作约定

先读 [现行项目规范](docs/refactor/project-conventions.md)。该文档自 2026-10-12 起覆盖本仓库旧计划、历史记录和项目技能中冲突的流程要求，用户会话指示优先。

- 永不设定 gate；不校验任何哈希，不以旧 gate/解锁字段作为推进前置条件。
- 每次推进的最小粒度为 `tx-0x-0x`；整个目标完成后只执行一次整体测试验收，探讨、分析与修改过程中不得运行其他测试。
- 取消逆向与测试必须独立、作者分离、独立审计及强制子智能体要求；同一智能体可以完成逆向、实现和验收。
- 每次实机测试前先告知用户需要配合的操作、何时停止手动操作；多次启动分别提示；需要手动配合时先确认用户已经准备好，再启动。
- 保持 C/S 与原版/Isaac Sync 的隔离、统一输入语义、线程与生命周期约束、Hook 常驻与逻辑启停，以及实际操作授权范围。
- 历史事实与失败记录如实保留；旧 gate、哈希、预封、多组验收和独立角色条款不再作为当前规范执行。
- NEVER write unit tests after you write code. 
- Highly prefer E2E tests as the sole testing mechanism. Use them to verify complex features work. At the end of E2E tests, produce a verifiable and repeatable artifact. 
- If you must test a system in isolation, FIRST write all the ways it could fail, THEN write the code.
- 编写代码时一定要为某个功能、核心函数和数据结构写注释，且编写中文注释

## 独立工程入口

本工程从原研究仓库复制到 F:\projects_isaac\issac_multiplay，保留 cs-t0 层级。当前资料入口为 docs/README.md；按功能整理的点位见 docs/hooks/README.md；新的状态同步方案见 docs/design/state-synchronization.md；REPENTOGON 适配重构设计见 docs/design/repentogon-refactor.md（目标为官方 REPENTOGON 与 v1.9.7.12.J273，原型点位须重新核对）。历史资料的旧流程不作为当前要求，所有未知与原始失败保留。
