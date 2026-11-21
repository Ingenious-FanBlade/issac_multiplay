> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this copy/move caller-heap v5 handoff

## Outcome

This append-only, candidate-only pass re-opened exactly the 24 `UNKNOWN` rows from caller-heap v4. It preserves all 19 prior closed row objects exactly and excludes the other 139 copy/move unknown rows.

- newly fully-backed closed: **2**
- cumulative closed in the 43-row delegated inventory: **21**
- remaining unknown: **22**
- explicit fixed-slot overlap observed: **0**
- promotion allowed by this candidate: **no**

The requested 24/24 target is not asserted. Each of the remaining 22 rows retains its concrete v4 countermodel and exact next interface, plus the v5 attempted-interface result.

## Frozen bindings

| Role | Path | SHA-256 |
|---|---|---|
| caller-heap v4 candidate | `out/cs-t0/native-join-manager-this-copy-move-caller-heap-v4-candidate.json` | `A933571B958401BF3A806198947624DCA5A973E55232B8E748206A1D7276681C` |
| caller-heap v4 reproducer | `out/cs-t0/build-native-join-manager-this-copy-move-caller-heap-v4-candidate.py` | `512F79AE91FB9EF11DA537F27B3B3209FCC44DFC233107466B8DCE1F27BD74B0` |
| corrected executable-range component | `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json` | `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981` |

The reproducer verifies the exact v4 19/24 partition, copies every row outside the frozen 24-RVA set without changing the JSON object, and disposes every frozen row exactly once.

## Two new proofs

| Callsite RVA | Proof |
|---|---|
| `0x000216EC` | v4 had already closed `sub_421680`'s SSO allocation/capacity relation, all 109 direct calls, and the other wrapper families. The final `sub_8F99C0` any-xref universe is exactly 22 code calls and zero data escapes. At 22/22 calls, the forwarded output is formed by `lea` from an EBP-local SSO object. Its internal calls at `0x8F9B08` and `0x8F9C1B` therefore cannot make the inline `this` destination intersect the main-image slot. |
| `0x000216FB` | The same complete producer proof applies to `this+firstLength`. For total length at most 15, the second span remains inside the stack SSO object; for a larger total, `sub_421680` allocates `totalLength+1` before both copies. |

The machine artifact stores all 22 forwarder call VAs and the local/capacity evidence VAs under each row's `v5Proof`.

## Remaining 22

Counts by inherited reason class:

- `CALLER_PARAMETER_DOMAIN_OR_SPAN_OPEN`: 11
- `HEAP_ALLOCATION_CAPACITY_RELATION_OPEN`: 2
- `POINTER_OR_CONTAINER_LOAD_UNRESOLVED`: 8
- `RAW_REGISTER_OR_STACK_SLICE_OPEN`: 1

The principal retained countermodels are:

- `0x00008A62/0x00008B2A`: `newCount=1, oldCount=2` allocates four bytes and copies eight. Both complete callers were checked and neither proves `oldCount<=newCount`.
- `0x0001513A/0x000155A4`: fixed 5000-element buffers are followed by limit checks that log and continue; no upstream parser invariant bounds the emitted count.
- `0x0001AAB0/0x0001AEB8`: a failed destination-capacity check only logs, so the subsequent source-count copy is still reachable.
- `0x001CBEB5`: all four erase-iterator producers were traced to their receiver vector, but the complete begin/end writer set remains open.
- `0x003534FE/0x0035352E`: all four insert iterators are receiver begin/end iterators; helper-mediated writers of the member vector at `+0x1754/+0x1758/+0x175C` remain open.
- `0x00657E0A/0x00657E94`: the BufferedStreamBase/BufferedStream virtual slot-6 dispatch universe remains type-open.
- `0x00658338/0x006583C2`: all 30 direct calls and 41 non-vtable address references were enumerated. The 41 references are devirtualization comparisons, but vtable slot `0xB9ED10` still permits an untyped indirect caller.
- pointer/container rows retain their exact constructor/writer-set countermodels; no local heap or container-shape observation was treated as a complete points-to proof.

## Raw call `0x0033570B`

This row remains `UNKNOWN`. The filtered vector is initialized empty at `0x73536E/0x735374`, and every append at `0x735478-0x73548A` is conditional on filtering predicates. No upstream invariant excludes a nonempty source whose filter result is empty.

For that reachable state, `begin=end`; the zero-count branch skips division, leaves `edx=begin`, and `0x735700-0x73570B` computes `end-(begin+4)`, which underflows. Only a dominating proof that filtered `end-begin>=4` can close this row.

## Artifacts and reproduction

| Artifact | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-copy-move-caller-heap-v5-candidate.json` | `5D46EB95A7575EDC6384B4D15D6031C7F78945041FF98216AB1F8AB63DCEF6EA` |
| `out/cs-t0/build-native-join-manager-this-copy-move-caller-heap-v5-candidate.py` | `E8E78C9AD6D6E07411E6CA1E0287190AA4E1D41D97E73A3454F58AFEC18FF48D` |

Create-new mode fails if the output exists:

```powershell
python out\cs-t0\build-native-join-manager-this-copy-move-caller-heap-v5-candidate.py
```

Deterministic replay against the existing artifact:

```powershell
python out\cs-t0\build-native-join-manager-this-copy-move-caller-heap-v5-candidate.py --verify-existing
```

Verified replay result: `newClosed=2`, `cumulativeClosed=21`, `remainingUnknown=22`.

## Safety and scope

Only existing JSON, documentation, and read-only IDA decompilation/disassembly/xrefs were consulted. The IDB identity remained the frozen `isaac-ng.exe` image (`SHA-256 1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`, base `0x400000`, image size `0x8FE000`). The game was not started; no process or debugger was opened; the IDB was not modified. Production code, tests, v1-v7 artifacts, indexes, and prior evidence were not modified.
