> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Fill、Memset 与 Allocation 证据族

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录整合 memset/fill destination 的数值边界、固定成员、容器 backing、allocator metadata、codec 和 persistent 家族。v33 当前仍正向接受 `225/242`；remaining exact17 已全部负向审计，但仍未正向证明 no-overlap。

## 家族索引

| 家族 | 记录 |
| --- | --- |
| 数值与固定成员 | [three-numeric v7](native-join-manager-this-fill-three-numeric-v7-handoff.md)、[miniz exact9 v8](native-join-manager-this-fill-miniz-nine-v8-handoff.md)、[checksum exact3 v9](native-join-manager-this-fill-checksum-three-v9-handoff.md)、[fixed640 v10](native-join-manager-this-fill-fixed640-one-v10-handoff.md)、[fixed-member v10](native-join-manager-this-fill-fixed-member-three-v10-handoff.md)、[remaining v11](native-join-manager-this-fill-fixed-member-remaining-one-v11-handoff.md)、[roots v12](native-join-manager-this-fill-fixed-member-roots-two-v12-handoff.md)、[allocator block v12](native-join-manager-this-fill-allocator-block-one-v12-handoff.md) |
| allocator metadata | [v14 superseder](native-join-manager-this-fill-allocator-metadata-one-v14-handoff.md)、[独立审计](native-join-manager-this-fill-allocator-metadata-one-v14-independent-audit-handoff.md)；v13 `UNKNOWN` 正文已清理 |
| n298 | [v13](native-join-manager-this-fill-n298-six-v13-handoff.md)、[v14](native-join-manager-this-fill-n298-six-v14-handoff.md) |
| standard/short vector | [exact15 v11](native-join-manager-this-fill-standard-vector-fifteen-v11-handoff.md)、[group A v12](native-join-manager-this-fill-standard-vector-group-a-exact-five-v12-handoff.md)、[A residual v28](native-join-manager-this-fill-standard-vector-group-a-residual-exact-three-v28-handoff.md)、[A audit](native-join-manager-this-fill-standard-vector-group-a-residual-exact-three-v28-independent-audit-v1-handoff.md)、[group B v13](native-join-manager-this-fill-standard-vector-group-b-exact-five-v13-handoff.md)、[B audit](native-join-manager-this-fill-standard-vector-group-b-v13-independent-audit-v1-handoff.md)、[group C v14](native-join-manager-this-fill-standard-vector-group-c-exact-five-v14-handoff.md)、[C audit](native-join-manager-this-fill-standard-vector-group-c-exact-five-v14-independent-audit-v1-handoff.md)、[short-vector v12](native-join-manager-this-fill-vector-short-seven-v12-handoff.md) |
| persistent | [exact6 v15](native-join-manager-this-fill-persistent-six-v15-handoff.md)、[static exact5 v16](native-join-manager-this-fill-persistent-static-five-v16-handoff.md) |
| fresh vector | [exact1 v17](native-join-manager-this-fill-fresh-vector-one-v17-handoff.md)、[queue-count v18](native-join-manager-this-fill-fresh-vector-queue-count-one-v18-handoff.md)、[codec-bounds v19](native-join-manager-this-fill-fresh-vector-codec-bounds-one-v19-handoff.md) |
| image/codec/SSO | [libpng exact2 v18](native-join-manager-this-fill-libpng-two-handoff-v18.md)、[PNG/zlib exact2 v20](native-join-manager-this-fill-png-zlib-two-handoff-v20.md)、[SSO/record exact2 v21](native-join-manager-this-fill-sso-record-two-handoff-v21.md)、[SSO exact1 v22](native-join-manager-this-fill-sso-one-handoff-v22.md)、[non-vector exact8 v27](native-join-manager-this-fill-nonvector-exact-eight-v27-handoff.md)、[独立审计](native-join-manager-this-fill-nonvector-exact-eight-v27-independent-audit-v1-handoff.md) |

v33 将 remaining exact17 重新表达为若干正向微任务；本目录的早期“verified”行只贡献既有 `225`，不能自动覆盖剩余 `17`。
