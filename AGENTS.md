# 项目工作约定

先读 [现行项目规范](docs/archive/refactor/project-conventions.md)。该文档自 2026-10-12 起覆盖本仓库旧计划、历史记录和项目技能中冲突的流程要求，用户会话指示优先。

- 永不设定 gate；不校验任何哈希，不以旧 gate/解锁字段作为推进前置条件。
- 整个目标完成后进入统一的整体测试验收，进入验收前的探讨、分析与修改过程中不得运行其他测试。
- 每个 tx 任务本身不再保留 artifact；E2E 测试默认也不保留 artifact，仅在中途停下、需要记录信息以便后续继续时保留必要材料。正常测试所需临时材料用后清理，不要求最终 artifact。
- 每次实机测试前先告知用户需要配合的操作、何时停止手动操作；多次启动分别提示；需要手动配合时先确认用户已经准备好，再启动。
- 历史事实与失败记录如实保留；旧 gate、哈希、预封、多组验收和独立角色条款不再作为当前规范执行。
- NEVER write unit tests after you write code. 
- Highly prefer E2E tests as the sole testing mechanism. Use them to verify complex features work. Artifact retention and continuous execution follow the rules above.
- If you must test a system in isolation, FIRST write all the ways it could fail, THEN write the code.
- 编写代码时一定要为某个功能、核心函数和数据结构写注释，且编写中文注释
- 以撒原版wiki本地地址：F:\projects_isaac\IsaacDocs，REPENTOGONwiki本地地址：F:\projects_isaac\REPENTOGON-1.1.3\docs\docs

## 独立工程入口

本工程从原研究仓库复制到 F:\projects_isaac\issac_multiplay，保留 cs-t0 层级。当前资料入口为 docs/README.md：总体架构见 docs/architecture.md，采样、同步、Host 端与 Client 端设计分别见 docs/capture.md、docs/synchronization.md、docs/host.md、docs/client.md。历史设计、逆向报告与验收记录在 docs/archive/（入口 docs/archive/README.md），只作查证用；按功能整理的点位见 docs/archive/hooks/README.md，REPENTOGON 适配重构设计见 docs/archive/design/repentogon-refactor.md（目标为官方 REPENTOGON 与 v1.9.7.12.J273，原型点位须重新核对）。2026-11-12 起，二进制状态同步改为“只采客机需要的状态、显示交给客机原生代码、帧号方案B”，见 docs/archive/design/binary-state/client-display-delegation.md 与规范第 17 节。2026-11-17 起改为最终一致：ACK 表示接收，客机在一帧内直接写活对象并按对象隔离失败，见 docs/archive/design/binary-state/eventual-consistency-revision.md。历史资料的旧流程不作为当前要求，所有未知与原始失败保留。
