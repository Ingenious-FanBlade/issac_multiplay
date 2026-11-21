> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 v32 successor independent MT-T02 audit v1 handoff

## Result

The proposed v32 frontier passed the independent compact MT-T02 audit. TEST
reconstructed it only from the hash-bound v31 frontier and Wave E independent
audit, without reading or importing
`build-manager-this-bulk-remaining-frontier-v32.py`.

The exact accepted delta is the Wave E row `0x00689F63`. Independent set and
partition reconstruction confirmed that v32 removes this RVA, and only this
RVA, from:

- `copyMove.groups.callerHeap`;
- `copyMove.remainingRvas`;
- `independentAuditCoverage.acceptedNegativeCopyRvas`.

Every other copy/move group and the entire memset partition are unchanged. The
recomputed ledger is copy/move `321/405`, memset `225/242`, total `546/647`,
remaining `101`. All manager-this, whole-image, runtime-mutation, process,
debugger, IDB, production, test, prior-evidence, and index/status permissions
remain false. This successor does not advance manager-this or whole-image
status.

## Bound inputs

- v31 frontier: `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v31.json`,
  SHA-256 `37C23D80BF45F4162305EA174A053E0D294BD9D32B0494D4639B516C2D7551EB`.
- Wave E independent audit:
  `out/cs-t0/native-join-manager-this-mt-wave-e-independent-audit-v1.json`,
  SHA-256 `C351089C8CCAF56D3E26CF866D9FCCC692166759489F4E60B009EBC57376EE45`.
- audited v32 frontier:
  `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v32.json`,
  SHA-256 `A336FF06F05A11CFF97CE937BF0E8CEE030E498B3E7C401A811021EFB665EFF3`.

All three physical hashes were independently recomputed before validation.

## Replay and rejection checks

The audit JSON was created once and reproduced byte-identically by exactly one
`verify-existing` run. All 12 compact high-value mutations were rejected:
missing v31 target, changed Wave E acceptance, bad binding hash, wrong accepted
RVA, target retained in callerHeap, target retained in remaining, target
retained in negative coverage, unrelated group deletion, copy count drift,
memset ledger drift, enabled manager promotion, and enabled runtime permission.

A separate invocation supplied the wrong v32 physical hash. It failed with
exit code 1 before output creation; the probe path was absent before and after.
No self-test artifact, Cargo test, full T0 build, game process, debugger, or IDB
operation was used.

