> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 vectorShift copy/move exact25 v28 independent audit v1

## Verdict

The independent disk-only PE audit accepts the candidate only as a strict
negative partition: `0 VERIFIED / 25 UNKNOWN`.  The copy ledger remains
`319/405`, with `86` rows remaining.

All 25 destination/source/count relations and local spans are reproducible,
but their no-wrap and extent conclusions remain conditional on a valid incoming
allocator-owned tuple.  The first-gap distribution is:

- 22 `INCOMING_DEST_SOURCE_TUPLE_ALIAS_WRITER_FIXED_POINT_OPEN`;
- 2 `DIRECT_CALLER_BRANCH_FEASIBILITY_AND_INCOMING_TUPLE_WRITER_FIXED_POINT_OPEN`;
- 1 `OWNER_ADDRESS_TAKEN_INDIRECT_TRANSFER_AND_TUPLE_WRITER_FIXED_POINT_OPEN`.

Each row retains a manager-slot-covering countermodel.  No local transfer fact
was promoted into a complete incoming tuple or alias-writer proof.

## Independent replay

The audit independently re-read all 25 exact call bytes and rel32 targets,
recomputed all 25 local-span hashes, rescanned the complete whole-`.text`
memmove direct-call universe (`306` sites), and reconstructed raw E8/E9 and
non-relocation exact-VA universes for all 21 owner roots.  The resulting gap
distribution matched `22+2+1`.

It also replayed the candidate's `0/25`, `21` owners, and `130/130` mutation
result.  `rejected-v4` remains unopened and unconsumed.  The external-analysis
status is `TIMEOUT_NO_PASS` and was not counted as positive evidence.

## Reproduction

```powershell
python out\cs-t0\audit-manager-this-copy-move-vector-shift-exact25-v28-independent-v1.py --verify-existing
```

Two consecutive audit replays passed and all `15/15` independent audit
mutations were rejected.

## Artifact hashes

Candidate bindings:

- Analyzer: `8EDCA1CB4AEE7F656D4AD67C9CF947EB38872CABE7169D46463A627190C995B0`
- Machine JSON: `0B3A1ADFFB7ABD06C12C1F93131B3D52D8770866E93FE187325C353358BD0B8D`
- Handoff: `811984FFEE9F33CE1FD2C4D65DCB27BB00D1346F48AF86BE904C40C0B2C9E7F7`

Independent audit:

- Analyzer: `80AA21D585E03AEB2D02290605E2140BA09D6E08AF2184DA34A2E74BED21D943`
- Machine JSON: `4F8C5F804E613606D1C386483858217AD55FE8346A90AF3264FDCAF2728516DC`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
