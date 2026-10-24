# C/S 重构文档入口

先读 [项目规范、约定与要求](project-conventions.md)（2026-10-12 现行版本）。永不设定 gate、不校验哈希；每次推进最小为 `tx-0x-0x`，只在整个目标完成后做一次整体测试验收，探讨过程不得测试；逆向与测试可以由同一智能体完成。

规范与要求以现行入口为准，下面的资料提供架构、功能目标、接口、观察与历史结果：

1. [总体架构](isaac-cs-architecture-plan.md)：Listen Server、模式隔离、输入与状态所有权。
2. [T0 任务书](cs-t0/isaac-cs-t0-task-plan.md)：各阶段研究目标与历史执行结果。
3. [T0-05 主题索引](cs-t0/t0-05/README.md)：local co-op、生命周期、输入隔离与 Virtual Device 资料。
4. [T0-07 阶段入口](cs-t0/t0-07/README.md)：AuthoritySession与tick/input scheduler；**状态 PASS（2026-10-20，用户确认）**。操作1–9均已有对应实现/验收记录，操作6–8为离线生产链矩阵，操作9为一次真实 Authority B源停发实验并通过 24/24 检查；history保留，脚本停发不冒充网络断线。

5. [REPENTOGON 适配重构设计](../design/repentogon-refactor.md)：2026-10-24 起的重构方向。目标为官方 REPENTOGON 与 v1.9.7.12.J273；Hook 常驻、逻辑启停；不再延续 ISCS／Isaac Sync 兼容约束。

旧资料中的 gate/解锁、哈希校验、作者分离、多组测试与预封条款已被现行规范覆盖，不作为后续执行要求。历史 PASS/FAIL/UNKNOWN 保留其事实含义。
