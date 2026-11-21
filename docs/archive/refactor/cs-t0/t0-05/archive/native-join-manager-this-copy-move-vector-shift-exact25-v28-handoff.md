> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy/move vectorShift exact25 v28

## Outcome

This is a reverse-author, disk-only PE evidence package for the exact
`copyMove.groups.vectorShift` frontier frozen by v26.  All 25 call instructions
and their `memmove` rel32 targets are byte-identical in the frozen PE.  For each
row, the local destination start, source start, byte count, direction/stride,
and conditional start-plus-count/no-wrap premises were reconstructed.

Those local facts do **not** close the incoming destination/source tuple and
alias-writer fixed point.  Consequently the adjudication is fail-closed:

| Result | Count |
|---|---:|
| Local conditional transfer reconstructed | 25 |
| Fully backed `VERIFIED` | 0 |
| `UNKNOWN` | 25 |

No row is added to the accepted ledger.  Copy/move remains `319/405`, with 86
rows remaining.  This package does not promote manager-this or the whole image.

## Exact set

```text
0x00025541  0x0023BD1D  0x0029FE92  0x003534DA  0x003534F1
0x00353515  0x0038FA20  0x003D830A  0x003D839C  0x003DE616
0x004352BA  0x00509B56  0x0053044F  0x005307A3  0x00530912
0x005AE147  0x005AEEF3  0x005BA135  0x005BA5F9  0x00653E1A
0x00653EB8  0x0066DA97  0x00673CD4  0x0068B4C4  0x0069F15E
```

The transfer partition is mutually exclusive: 16 left shifts, 8 right shifts,
and one composite range repack.  The machine JSON records every exact call
byte sequence, containing owner, tuple-root description, destination/source/
count expression, stride, local 0x40-byte pre-call span hash, and conditional
allocator-extent premises.

## Static universes and first gaps

The independent PE census found 306 raw whole-`.text` direct-E8 byte candidates
to the `memmove` VA; this is deliberately a conservative raw-byte universe, not
a decoded-instruction claim.  The exact25 set is proven to be a subset.

The 25 rows occupy 21 owner roots.  For those roots, the whole-`.text` raw
rel32 census contains 168 E8 candidates and zero E9 candidates; the non-`.reloc`
exact-VA census contains five occurrences.  Owner `0x00673B60` has no raw direct
E8/E9 candidate and has an exact-VA occurrence, so address-taken/indirect
transfer remains open.

The first-gap partition is:

| First gap | Count |
|---|---:|
| Incoming destination/source tuple and alias-writer fixed point open | 22 |
| Direct-caller branch feasibility plus incoming tuple-writer fixed point open | 2 |
| Address-taken/indirect owner transfer plus tuple-writer fixed point open | 1 |

The two branch-feasibility rows are `0x003DE616` and `0x004352BA`; the
address-taken row is `0x00673CD4`.

For every row, local no-wrap/non-overlap is conditional on the destination and
source belonging to the same allocator-owned extent, nonnegative path values,
and both `start + count` results being within `capacityEnd`.  Because an
unenumerated incoming tuple/alias writer can supply a locally accepted range
covering manager slot `[0x00C71678,0x00C7167C)`, the JSON retains an explicit
slot-covering countermodel and sets the non-overlap premise to false.  This is
the minimal reason none of the 25 local positives is promoted.

The rejected-v4 narrative was neither opened nor consumed.  Selection comes
only from v26, with the corrected executable component v2 and accepted global
allocator seal v20 bound as immutable inputs.

## Reproduction

Create-new was run once, followed by two consecutive byte-identical
verify-existing replays:

```powershell
python out\cs-t0\analyze-manager-this-copy-move-vector-shift-exact25-v28.py
python out\cs-t0\analyze-manager-this-copy-move-vector-shift-exact25-v28.py --verify-existing
python out\cs-t0\analyze-manager-this-copy-move-vector-shift-exact25-v28.py --verify-existing
```

All 130 semantic mutations were rejected: 100 per-row status/gap/call-byte/
no-wrap mutations, 21 owner-universe mutations, and nine global
status/ledger/permission mutations.  The external-analysis state is recorded
only as `TIMEOUT_NO_PASS` and was not used for the verdict.

## Artifact hashes

- Analyzer:
  `8EDCA1CB4AEE7F656D4AD67C9CF947EB38872CABE7169D46463A627190C995B0`
- Machine JSON:
  `0B3A1ADFFB7ABD06C12C1F93131B3D52D8770866E93FE187325C353358BD0B8D`
- Frozen PE:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
An independently isolated tester must perform final acceptance separately.
