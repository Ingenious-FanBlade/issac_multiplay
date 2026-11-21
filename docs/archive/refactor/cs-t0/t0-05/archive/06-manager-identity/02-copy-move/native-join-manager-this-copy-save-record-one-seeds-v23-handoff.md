> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 v23 stage-1: corrected save-record `record+0x500` seed census

This artifact is a read-only static seed census. It does **not** close the v22 alias fixed point and does not promote RVA `0x001CBEB5`.

## Bound inputs

- v22 corrected layout and only remaining gap:
  `out/cs-t0/native-join-manager-this-copy-save-record-one-v22.json`
  (`13BB9F33F0B500F4E4BEE74C9BE6628CA849F26F1E6A4065BE78DB94B9A5C23E`).
- corrected executable-range component v2:
  `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json`
  (`D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`).
- accepted successor-first engines: A-v19, B-v17-switchfix and C-v18, with their analyzer/core hashes frozen in the machine JSON.

The real topology remains `slot/header+4=objectBase`, array A at
`object+0x19D7C` with count 8, array B at `object+0x1CC5C` with count 4,
stride `0x5DC`, and the target vector fields at record `+0x500/+0x504/+0x508`.

## Exact finite universe

The analyzer raw-scans all of frozen `.text` and then, in IDA replay, confirms
instruction-head and operand semantics for nine relation values. There are 186
decoded candidates:

| value | decoded candidates |
|---|---:|
| `0x500` | 51 |
| `0x504` | 35 |
| `0x508` | 17 |
| `0x19D7C` | 18 |
| `0x1CC5C` | 8 |
| `0x19D80` | 1 |
| `0x1A280` | 1 |
| `0x5DC` | 47 |
| `0x20660` | 8 |

Seventy-three candidates have the v22-bound slot/object/record base relation.
The other 113 are explicit `NUMERIC_COLLISION_NOT_RELATION_SEED` candidates;
none is silently discarded. The primary record-helper address scan also finds
exactly nine absolute/address-taken instructions, zero exact RVA literals, and
zero image-base-plus-RVA formations.

The emitted seed union is exact 167 and is split into mutually exclusive work
packages:

- A = 78 helper entries, complete primary E8/E9 caller sets, and bound grow /
  deserialize / tuple-install edges.
- B = 73 already-offset field and parent/pre-offset/split-tail operand seeds.
- C = 16 absolute/address-taken indirect, return-alias, and stack-liveout seeds.

Every seed row records its function (or an explicit orphan EH fragment owner),
basic block, block end, and initial abstract state. The partition intersection
and unclassified seed counts are both zero. These counts only close the census,
not the writer proof.

## Required next stage and exact UNKNOWN

The first remaining predicate is
`V23_SEED_CONTEXTS_NOT_PROPAGATED_TO_SUCCESSOR_FIRST_FIXED_POINT`.

The 167 contexts still need to be propagated using the bound strict
successor-first rule through CFG successors, indirect calls, return aliases,
stack spills/reloads, and forwarded callees. Until that fixed point classifies
every tuple writer and every non-null begin install, a no-literal forwarded
tuple writer remains a valid countermodel. Therefore copy/move stays
`315/405`, the target row remains UNKNOWN, and all permissions/promotions stay
false.

## Reproducer and outputs

- analyzer:
  `out/cs-t0/analyze-manager-this-copy-save-record-one-seeds-v23.py`
- machine JSON:
  `out/cs-t0/native-join-manager-this-copy-save-record-one-seeds-v23.json`

Ordinary PE create-new and two consecutive byte-identical verify runs print:

```text
PASS PE stage1 seeds=167 A=78 B=73 C=16 decoded=186 collisions=113 unknown=1 mutations=36
```

The mode-independent analyzer contains the current-IDB replay path for the same
manifest: it re-decodes all 186 candidates and checks every seed function/block
plus the three stack-liveout successor sets without IDB mutation. At this
handoff, however, the IDA MCP proxy remained occupied by an earlier read-only
request and every spaced minimal probe timed out. The handoff therefore records
`IDA_REPLAY_ENVIRONMENT_BLOCKED` / `TIMEOUT_NO_PASS`; **no IDA PASS is counted**.
This environment result does not change the semantic first gap, does not promote
the row, and does not increment copy/move.

SHA-256 at handoff generation:

- analyzer: `D08E842A4CF115867B8FCC8FBC3EFA56C7760E53647A6FD0B802EEE7D7A52D21`
- JSON: `15C88FB29B3D8EAF7165454CF0396C614EDAEA157D8F97ABA22E17957CDB1AD3`
