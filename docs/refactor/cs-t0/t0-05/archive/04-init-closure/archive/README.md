> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 研究过程归档

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录集中保存 `CS-T0-05-04` 的版本化研究记录。归档只调整主题目录中的阅读布局，不改变任何证据标签、结论、哈希、权限或机器产物。

## Init closure 演进

| 记录 | 作用 |
| --- | --- |
| [v1](native-join-init-closure-v1.md) | ordinary 初始化条件组合与初始边界 |
| [v2](native-join-init-closure-v2.md) | 证据更正，保留历史不覆盖 |
| [v3](native-join-init-closure-v3.md) | 有效来源、consumer 与到达定义补证 |
| [v4](native-join-init-closure-v4.md) | Init/P0 根函数库存与局部静态证明 |
| [v5](native-join-init-closure-v5.md) | fresh-B reaching-definition 证据 |
| [v6](native-join-init-closure-v6.md) | publication identity 到 Init dispatch |
| [v7](native-join-init-closure-v7.md) | 累计 manager-context guard 章节 |
| [v8](native-join-init-closure-v8.md) | 累计 manager receiver 章节并保留全部未决前提 |

## Cache / Familiar / P0 支线

| 记录 | 作用 |
| --- | --- |
| [v1](native-join-init-v4-cache-p0-v1.md) | 初始静态支线 |
| [v4](native-join-init-v4-cache-p0-v4.md) | 中间修订 |
| [v5](native-join-init-v4-cache-p0-v5.md) | 当前累计支线入口 |

## 兼容边界

- 冻结 JSON、脚本和历史审计绑定的 `docs/refactor/cs-t0/t0-05/<filename>.md` 原文继续保留在原路径，不移动、不改写。
- 上级主线计划、执行状态和 `t0-05/README.md` 不因本次归档改写。
- 当前主题结论仍从[上级入口](../README.md)进入。
