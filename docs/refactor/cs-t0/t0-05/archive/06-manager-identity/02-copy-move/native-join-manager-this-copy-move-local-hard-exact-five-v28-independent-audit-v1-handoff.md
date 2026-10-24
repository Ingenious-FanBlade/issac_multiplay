> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 local-hard copy/move exact5 v28 independent audit v1

## Verdict

The independent disk-only PE audit accepts the candidate only as a strict
negative boundary: `0 VERIFIED / 5 UNKNOWN`.  No copy/move row is promoted,
so the accepted copy ledger remains `319/405` with `86` rows remaining.

The retained first gaps are:

- `0x002EE04F`: the transitive `n4` copy from source `+0x2094` has no closed
  writer fixed point;
- `0x004FCD78`: the `0x008F7080` caller's incoming EDI begin/end/backing tuple
  producer fixed point is open;
- `0x0061D0B4`: class copy/move, split-alias, and lifecycle writers for the
  `this+0x2C/+0x30/+0x34` tuple are open;
- `0x00663CFC`: the PNG IHDR width source and 32-bit allocation-product
  no-wrap premise are open;
- `0x006936B7`: the pixel subrow inherits the same wrapped row-backing model
  and still lacks offset/add no-wrap closure.

## Independent replay

The audit independently checked all five exact call byte sequences and rel32
targets, 24 local control/allocation/source anchors, eight whole-`.text`
owner caller universes, 22 direct `n4` writers, and 28 positive `n4` alias
LEAs.  It replayed the candidate's `0/5`, `34/34` mutation result and required
all candidate permissions and promotion flags to remain false.

The candidate's external-analysis disposition remains `TIMEOUT_NO_PASS`; it
was not counted as a PASS or as positive evidence.

## Reproduction

```powershell
python out\cs-t0\audit-manager-this-copy-move-local-hard-exact-five-v28-independent-v1.py --verify-existing
```

Two consecutive audit replays passed and all `12/12` independent audit
mutations were rejected.

## Artifact hashes

Candidate bindings:

- Analyzer: `234ADEF93C1E0875D9945602030AAC450BFB8E3DC8F50E3E10DE08F3E0D23AF5`
- Machine JSON: `90346CCDFDFCFC3B1825A7A3666C66FBFE191A999F300E354EE09034799F04E2`
- Handoff: `F2CB8F068E0A7158306DFCF18FB2E129C162B1D12B3A33D54A0B4BB1882B2583`

Independent audit:

- Analyzer: `77143FB6DDBBE7C5C377DBE31DA77E6F03BA5867734442D2F8A2DB1AB99993AC`
- Machine JSON: `95148047034CA738BB426BCD46F68D19B052EA3A67360551E22729E1815A5102`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
