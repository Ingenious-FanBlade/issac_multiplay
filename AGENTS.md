# 项目工作约定

先读 [现行项目规范](docs/refactor/project-conventions.md)。该文档自 2026-10-12 起覆盖本仓库旧计划、历史记录和项目技能中冲突的流程要求，用户会话指示优先。

- 永不设定 gate；不校验任何哈希，不以旧 gate/解锁字段作为推进前置条件。
- 每次推进的最小粒度为 `tx-0x-0x`；整个目标完成后进入统一的整体测试验收，进入验收前的探讨、分析与修改过程中不得运行其他测试。
- 例外：原生二进制状态同步重构（`docs/design/binary-state/`）的步骤直接用数字编号（0—15），第 15 步是唯一的统一整体验收，见规范第 17 节。
- 每个 tx 任务本身不再保留 artifact；E2E 测试默认也不保留 artifact，仅在中途停下、需要记录信息以便后续继续时保留必要材料。正常测试所需临时材料用后清理，不要求最终 artifact。
- E2E 如无特殊说明，应连续完成测试；遇到问题先修复，再在同一 tx 的整体验收中继续必要测试，不因失败默认停下等待下一次指令，也不为修复续测另换 tx 编号。
- 取消逆向与测试必须独立、作者分离、独立审计及强制子智能体要求；同一智能体可以完成逆向、实现和验收。
- 每次实机测试前先告知用户需要配合的操作、何时停止手动操作；多次启动分别提示；需要手动配合时先确认用户已经准备好，再启动。
- 保持 C/S 与原版/Isaac Sync 的隔离、统一输入语义、线程与生命周期约束、Hook 常驻与逻辑启停，以及实际操作授权范围。
- 历史事实与失败记录如实保留；旧 gate、哈希、预封、多组验收和独立角色条款不再作为当前规范执行。
- NEVER write unit tests after you write code. 
- Highly prefer E2E tests as the sole testing mechanism. Use them to verify complex features work. Artifact retention and continuous execution follow the rules above.
- If you must test a system in isolation, FIRST write all the ways it could fail, THEN write the code.
- 编写代码时一定要为某个功能、核心函数和数据结构写注释，且编写中文注释
- 以撒原版wiki本地地址：F:\projects_isaac\IsaacDocs，REPENTOGONwiki本地地址：F:\projects_isaac\REPENTOGON-1.1.3\docs\docs

## 独立工程入口

本工程从原研究仓库复制到 F:\projects_isaac\issac_multiplay，保留 cs-t0 层级。当前资料入口为 docs/README.md；按功能整理的点位见 docs/hooks/README.md；新的状态同步方案见 docs/design/state-synchronization.md；REPENTOGON 适配重构设计见 docs/design/repentogon-refactor.md（目标为官方 REPENTOGON 与 v1.9.7.12.J273，原型点位须重新核对）。历史资料的旧流程不作为当前要求，所有未知与原始失败保留。
