> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 save-record exact1 v24 stage-2 handoff

## Decision

Row RVA `0x001CBEB5` remains `UNKNOWN` and is not promoted.  The v23 exact
`167 = A78 + B73 + C16` seed universe was reconstructed against the fixed PE,
its `113` numeric collisions remained disjoint, and the partial successor-first
worklist was stable on two consecutive in-process solves.  It resolved 165
seeds, while B-016 and B-057 converge on one newly exposed semantic frontier.

The unique first gap is
`B016_B057_RECORD_BASE_STACK_LIVEOUT_AT_007B3BE0_NOT_CLOSED`:

- `0x007B3BB5`: selected record index is multiplied by `0x5DC`;
- `0x007B3BBF`: `0x19D80` plus the selected slot header forms the record base;
- `0x007B3BC9`: record base is spilled to `[EBP-0x298]`;
- strict first successor is `0x007B3BE0`;
- `0x007B3C0D` reloads the record base;
- the frontier continues through call `0x007B3C61 -> 0x005B39D0` and is absent
  from v23's three C-partition stack-liveout seeds.

Consequently the known allocator/grow/resize/install mechanics are byte-bound
and their allocation extents cover their installed capacities, but the target
row's complete begin/end/capacity source universe is not closed.  A no-literal
non-allocator installer remains a valid countermodel.

## Created artifacts

- `out/cs-t0/analyze-manager-this-copy-save-record-one-successor-first-v24.py`
  - SHA-256 `F95032B28291351F3B703619F5D420384CC3DE483FE716A17E826FC53B472EE9`
- `out/cs-t0/native-join-manager-this-copy-save-record-one-successor-first-v24.json`
  - SHA-256 `17B8B3F661FA01327B1DAD0762A513FA58926E7C62C4CE0184D97802E69D2A9F`

The analyzer and JSON are create-new-only.  No v23, v22, global-v20,
selector-v13, status/index, production, or test artifact was modified.

## Frozen bindings and exact replay

Direct bindings checked:

- v23 analyzer `D08E842A4CF115867B8FCC8FBC3EFA56C7760E53647A6FD0B802EEE7D7A52D21`
- v23 JSON `15C88FB29B3D8EAF7165454CF0396C614EDAEA157D8F97ABA22E17957CDB1AD3`
- v22 analyzer/JSON `70543708...` / `13BB9F33...`
- global-v20 builder/JSON `0A91926A...` / `E1D42FAB...`
- selector-v13 analyzer/JSON `4F14DB16...` / `9DA0FD48...`

The recursive JSON binding graph contains 25 visited JSON documents and 86
verified hash edges.  Its identity SHA-256 is
`8DC1FC26A304A1B9E3C268C02C2C9BCBC7761CDA710EFEBBB50B0D1B6EAE7CC1`.

Against PE SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`,
the analyzer rechecked:

- all v23 raw relation needles, decoded seed identities, direct rel32 sets,
  address-taken immediates, chain edges, and collision partition;
- 39 v22 function byte bindings;
- 5 global-v20 allocator function byte bindings;
- 110 unique seed blocks, identity SHA-256
  `6C69F78973C52C528250030B1A001E695FA7877F6D2DCDBD060C84915088BF13`;
- target call bytes `E8 03 4A 52 00`;
- the exact producer and first-successor bytes for the new unresolved state.

The v23 prose was not used as proof.  Its machine manifest was reconstructed by
the bound analyzer and revalidated against exact PE bytes before propagation.

## Commands and results

Create run:

```text
python out/cs-t0/analyze-manager-this-copy-save-record-one-successor-first-v24.py
PASS PE v24 seeds=167 resolved=165 unresolvedSeeds=2 unresolvedStates=1 row=UNKNOWN mutations=32
```

Two consecutive verify-existing runs:

```text
python out/cs-t0/analyze-manager-this-copy-save-record-one-successor-first-v24.py --verify-existing
PASS PE v24 seeds=167 resolved=165 unresolvedSeeds=2 unresolvedStates=1 row=UNKNOWN mutations=32

python out/cs-t0/analyze-manager-this-copy-save-record-one-successor-first-v24.py --verify-existing
PASS PE v24 seeds=167 resolved=165 unresolvedSeeds=2 unresolvedStates=1 row=UNKNOWN mutations=32
```

All 32 critical negative mutations were rejected.  They cover fixed-point
completeness, collision isolation, unresolved seed/state counts, stability,
writer completeness, allocator arithmetic/extent, row promotion, copy counts,
IDA status, bindings, first-gap bytes, and permissions.

## IDA and independence status

The shared IDA MCP request timed out.  This is recorded exactly as
`TIMEOUT_NO_PASS`; no scripted or manual IDA PASS is claimed and no IDB change
occurred.

All team slots were occupied, so a separate testing subagent could not be
started.  The PE create/double-replay/mutation results are self-verification,
not independent acceptance.  Parent or another independent task should replay
the analyzer before any later acceptance decision.

All runtime, process, debugger, IDB mutation, production/test mutation,
promotion, and T0-05 unlock permissions remain `false`.  Copy/move counts remain
`315/405` with accepted-new closure count `0`.
