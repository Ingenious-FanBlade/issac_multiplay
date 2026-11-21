> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 copyMove string exact14 v29 independent audit v1

## Verdict

The independent disk-only PE audit accepts the candidate only as a strict
negative partition: `0 VERIFIED / 14 UNKNOWN`.  The copy ledger remains
`319/405`, with `86` rows remaining.

All 14 local string transfers reproduce the `+0` storage union, `+0x10`
length, `+0x14` capacity, SSO/heap selection, offset/count expressions, and
conditional no-wrap arithmetic.  They do not close incoming object identity,
borrowed/external pointer provenance, SSO-union state, allocation capacity,
copy/move/adoption/free lifetime, or return/out-parameter liveout consumers.

The retained first-gap distribution is:

- 2 adoption/move and incoming identity/capacity gaps;
- 10 borrowed/external pointer, SSO-union, and capacity gaps;
- 2 incoming string identity, SSO-union, and capacity gaps.

## Independent replay

The audit independently re-read all 14 exact helper calls and targets,
recomputed every local span hash, rescanned the complete whole-`.text` memcpy
and memmove universes, and reconstructed all six owner functions.  The six
owner universes contain 230 direct E8 callers, no E9 tail callers, and no
non-relocation exact-VA escapes.  Function hashes, SSO/heap state anchors, and
allocator/free call targets also match the frozen PE.

It replayed the candidate's `0/14`, six-owner, 230-caller, and `100/100`
mutation result.  Legacy rejected evidence remains unopened.  The external
analysis disposition is `TIMEOUT_NO_PASS` and was not counted as positive
evidence.

## Reproduction

```powershell
python out\cs-t0\audit-manager-this-copy-move-string-exact14-v29-independent-v1.py --verify-existing
```

Two consecutive audit replays passed and all `15/15` independent audit
mutations were rejected.

## Artifact hashes

Candidate bindings:

- Analyzer: `B99894F74CD9D88CEC110D32BCAC4D3E79D1B2B5EFBCFA0ED0924A1CEF5E73F1`
- Machine JSON: `F9A85F3022E659250A7A386AD367B0CD400599C1ECBC14DB93FCC4DBF8CE0790`
- Handoff: `11E1C1B8259CC783C9FAEE56FDA2FE41FC1E41626F64DD94F4950B80208201ED`

Independent audit:

- Analyzer: `34289BB7C2C9991F75C1FB2556591497FAF45C566C2F599A264ADEA3529C62E3`
- Machine JSON: `812B0F9F21128829EA85B1C055BE13E750000C838B4BEA44DC71D0BC5634C92E`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
