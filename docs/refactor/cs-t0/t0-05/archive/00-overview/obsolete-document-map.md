> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05 过时文档迁移表

本表记录 2026-09-05 文档整理中已压缩为迁移指针的阶段稿。清理只移除重复叙述，不改变历史机器产物、证据标签、正向计数、`UNKNOWN` 行或权限。

| 过时记录 | 当前替代项 | 清理依据 |
| --- | --- | --- |
| `native-exit-applicability-revision-v1.md` | `01-passive-lifecycle/native-exit-applicability-gate-v1.md` | gate 迁移已完成，修订稿只保留迁移前授权语境 |
| `native-join-manager-this-pause-checkpoint-v31.md` | `06-manager-identity/00-overview-and-gates/native-join-manager-this-pause-checkpoint-v32.md` | v32 已完整吸收 v31 的负向覆盖和暂停边界 |
| `native-join-manager-this-bulk-memset-handoff-v1.md` | `06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-memset-handoff-v4.md` | v4 为累计 closure 入口 |
| `native-join-manager-this-bulk-memset-handoff-v3.md` | `06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-memset-handoff-v4.md` | v4 为累计 closure 入口 |
| `native-join-manager-this-bulk-remaining-frontier-v29-handoff.md` | `06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v30-handoff.md` | v30 是当前接受的 create-new 机器前沿 |
| `native-join-manager-this-whole-image-alias-handoff-v3.md` | 根目录冻结 `native-join-manager-this-whole-image-alias-handoff-v5.md` | v5 修正 executable-range 并累计 v3/v4 component |
| `native-join-manager-this-whole-image-alias-handoff-v4.md` | 根目录冻结 `native-join-manager-this-whole-image-alias-handoff-v5.md` | v5 修正 executable-range 并累计 v3/v4 component |
| `native-join-manager-this-copy-move-caller-heap-v3-handoff.md` | 根目录冻结 `native-join-manager-this-copy-move-caller-heap-v5-handoff.md` | v5 是 caller/heap 累计分区入口 |
| `native-join-manager-this-copy-move-caller-heap-v4-handoff.md` | 根目录冻结 `native-join-manager-this-copy-move-caller-heap-v5-handoff.md` | v5 是 caller/heap 累计分区入口 |
| `native-join-manager-this-fill-allocator-metadata-one-v13-handoff.md` | `06-manager-identity/03-fill-and-allocation/native-join-manager-this-fill-allocator-metadata-one-v14-handoff.md` | v14 明确 supersede v13 的 `UNKNOWN` |
| `native-join-manager-this-manager-tuple-helpers-a-v12-handoff.md` | `06-manager-identity/04-manager-tuple/native-join-manager-this-manager-tuple-helpers-a-v13-handoff.md` | v13 收窄并取代 helpers-A v12 阅读入口 |
| `native-join-manager-this-manager-tuple-liveouts-a-v16-handoff.md` | `06-manager-identity/04-manager-tuple/native-join-manager-this-manager-tuple-liveouts-a-superseder-v19-handoff.md` | v19 明确采用 successor-first 修正 v16 |

上述 12 份文件已按用户明确授权直接清理正文，不创建备份；旧文件名只保留轻量迁移指针，避免已有 Markdown 引用失效。它们不再参与当前阅读链，也不得用于覆盖后继版本。
