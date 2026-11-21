# T0-05 当前阅读入口

更新时间：2026-10-16；文档整理：tx-01-44。

先读[现行项目规范](../../project-conventions.md)。本页提供功能状态和阅读顺序；历史 gate、哈希、预封、作者分离、多组测试及解锁字段不作为当前执行要求。

## 当前状态

T0-05 仍在研究与设计阶段。已采纳自有对象 vtable、自有 control block、S2 action 层覆写、原生 Join 路线 A 和 NEVER 回收策略；限定正常路径的原生静态主干已由 tx-01-43 / v18 说明。当前源码仍为 A-only V1，自有 ABI、JoinControl、双 participant S2、V2 held 与退休协调器尚未生产实现。

v18 的 Authority-pre/current、normal-post/previous 写入时点候选仍与契约 v2 的 +0x78 唯一写锁存约定冲突，尚未采纳。Join 可靠创建并发布 B 仍是用户接受的规划假设；现场状态、实机行为和完整 T0-05 功能不得据此写成已完成。详见[当前执行状态](00-overview/isaac-cs-t0-05-status.md)。

## 推荐阅读顺序

1. [目标与范围](00-overview/isaac-cs-t0-05-research-plan.md)、[当前状态](00-overview/isaac-cs-t0-05-status.md)。
2. [Virtual Device 当前设计与证据](07-virtual-device/README.md)：已采纳契约、v18 结论、继承证据和未决选择。
3. 按具体实现问题读取下面的主题摘要；需要原始地址、反例或历史场景时进入归档。

| 主题 | 当前用途 |
| --- | --- |
| [00 总览](00-overview/README.md) | 目标、完成边界、当前状态与历史定位 |
| [01 生命周期](01-passive-lifecycle/README.md) | 设备重绑、generation、失效与 teardown |
| [02 原生 Join](02-active-join/README.md) | 原生路径、B 创建/发布规划假设与剩余事实边界 |
| [03 Reader 与准入资料](03-admission/README.md) | 局部公式、读取失败策略与尚未证明的原生身份 |
| [04 Init / Fresh-B / P0](04-init-closure/README.md) | 初始化传播、局部结论与未决来源 |
| [05 持久化与停止窗口](05-persistence/README.md) | readonly、保存副作用、自然更新与恢复窗口 |
| [06 Manager identity](06-manager-identity/README.md) | 身份风险、历史严格计数与已接受假设 |
| [07 Virtual Device](07-virtual-device/README.md) | 当前自有 ABI 路线和限定正常路径结论 |

## 历史资料与兼容路径

[统一归档](archive/README.md)保留旧计划、状态长日志、版本化交接、失败、反例、旧验收记录及已完成工作设计；[迁移表](archive/migration-map.md)列出正文位置。旧路径上的短导航只用于找到归档正文，不承担当前结论。

归档保留原层级，Markdown 链接按新位置调整，正文中的事实、证据标签、历史结果和旧哈希字面值保留。冻结 JSON、脚本、源码及机器产物未在本次整理中修改。v18 仍继承前版技术依据，不能按版本号把旧证据视为无用。
