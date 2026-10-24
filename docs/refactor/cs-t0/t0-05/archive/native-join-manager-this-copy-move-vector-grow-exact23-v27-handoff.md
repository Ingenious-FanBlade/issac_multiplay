> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 copy/move vector-grow exact23 v27 handoff

## Outcome

The bulk-v26 `copyMove.groups.vectorGrow` partition was independently replayed
from the frozen PE.  All 23 rows remain `UNKNOWN`; no row is promoted.

| Result | Count |
|---|---:|
| Exact rows | 23 |
| Local compare/capacity branch reconstructed | 23 |
| Grow allocation/install chain reconstructed | 23 |
| Fully backed `VERIFIED` | 0 |
| Retained `UNKNOWN` | 23 |

The strongest positive result is local: all seven helper families enforce an
element-count limit, choose a bounded capacity, allocate through RVA
`0x0000CF00`, and install the returned allocation into begin/end/capacity.  For
each family, `elementSize * limit <= UINT32_MAX`, so the successful allocation
byte request does not wrap.

This does not prove the target rows safe.  The in-place branch first trusts the
incoming tuple fields.  Without a complete caller/object/source writer fixed
point, a foreign or malformed begin/end/capacity tuple can make that comparison
appear sufficient and bypass the fresh grow path.

## Exact helper groups

- `0x001711A0`, element size 32: 4 rows
- `0x002A0D30`, element size 4: 10 rows
- `0x002A0DC0`, element size 12: 3 rows
- `0x003099A0`, element size 8: 2 rows
- `0x003DCCD0 -> 0x003DEDB0 -> 0x003DF050`, element size 1: 1 row
- `0x003DEFC0`, element size 16: 2 rows
- `0x00515F50`, element size 1: 1 row

There are 19 distinct containing owners and 19 tuple-root descriptions.  The
machine JSON records every exact group, row, compare-to-call span hash, owner
raw rel32 caller-candidate set, and non-relocation exact-VA immediate scan.

## Per-row gap decision

Twenty-two rows have
`DEST_AND_SOURCE_TUPLE_WRITER_FIXED_POINT_OPEN`: neither the incoming
destination tuple nor the source begin/end producer universe is complete.

RVA `0x0062C746` has the stronger local result: the copy length is the explicit
byte count passed through resize, and the owner-local resize/copy span is
closed.  It still remains `UNKNOWN` with
`CALLER_DEST_TUPLE_WRITER_FIXED_POINT_OPEN`, because `a4[begin,end,cap]` is
caller supplied.  A forged capacity can bypass growth before the copy.

The PE raw caller census found direct rel32 candidate supersets and no exact VA
immediate in any non-`.reloc` section for the 19 owner entries.  Those raw hits
are not promoted to a decoded full/split/address-taken/stack/return fixed point;
the shared IDA service was unavailable.  This is explicitly recorded per owner
and is part of the fail-closed gap.

## Binding boundary

Direct frozen bindings include:

- bulk-v26 builder/JSON/handoff:
  - `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07`
  - `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF`
  - `DF537CDFFE9889BE11556D13AB24493A52B80376A19240945F62763604CCD4C5`
- negative vector-grow v6 analyzer/JSON:
  - `BD42DB3381593926F140F646E5B58D2CD614374404685B5D31C7FE0F193DCB90`
  - `DB7512C7DCD1012D40D87626FDA1A8B90F234D7A4A436C8E0C67F6E61D5090DD`
- allocator global-v20 builder/JSON:
  - `0A91926ABACB02C0DCA62F6546B1C9E735DA124D46798DF558917A8754F3F356`
  - `E1D42FABAF1651B01BCC1C7005A2958C0510B26F29BE361A2FE8F71AA44D58C9`
- corrected whole-image-range component v2 analyzer/JSON:
  - `A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6`
  - `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`

All 16 transitive binding rows embedded in bulk-v26 were also hash-checked;
their ordered identity SHA-256 is
`8F11260524D5DF5130F6A1DC20FD39D904C3FF10BA16C876193A97478DBB22C3`.

The rejected v4 file was not opened and none of its positive conclusions was
used.  Exact row selection came only from bulk-v26.  The retained v6 artifact
was used as a negative predecessor identity check, while row bytes, helper
calls, install anchors, arithmetic and caller candidates were re-read from the
fixed PE SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

## Created artifacts

- `out/cs-t0/analyze-manager-this-copy-move-vector-grow-exact23-v27.py`
  - SHA-256 `F3A0E38567A8FD87B83264CF90B4DB8CE73DFC1B0C97F8FE689D5A4CD0CB3CAB`
- `out/cs-t0/native-join-manager-this-copy-move-vector-grow-exact23-v27.json`
  - SHA-256 `D8F618EFC26866EF7B70D1831E9D49FEB5F7AC731D96E41FD1556A232692E8B0`

## Reproduction

Create run:

```text
python out/cs-t0/analyze-manager-this-copy-move-vector-grow-exact23-v27.py
PASS PE vectorGrow exact23 verified=0 unknown=23 helpers=7 owners=19 mutations=54
```

Two consecutive verify-existing runs:

```text
python out/cs-t0/analyze-manager-this-copy-move-vector-grow-exact23-v27.py --verify-existing
PASS PE vectorGrow exact23 verified=0 unknown=23 helpers=7 owners=19 mutations=54

python out/cs-t0/analyze-manager-this-copy-move-vector-grow-exact23-v27.py --verify-existing
PASS PE vectorGrow exact23 verified=0 unknown=23 helpers=7 owners=19 mutations=54
```

All 54 semantic mutations were rejected.  They cover every row's attempted
promotion, every helper's no-wrap predicate, rejected-v4 isolation, counts,
helper grouping, caller completeness, writer completeness, bindings, IDA
status, ledgers and permissions.

## IDA, runtime and promotion status

The shared IDA MCP `server_health` request timed out.  The artifact records
`TIMEOUT_NO_PASS`; it does not claim an IDA replay PASS and no IDB write was
performed.

No game or target process was started/opened/read.  No debugger, runtime probe,
production/test change, prior-evidence edit, or status/index edit was used.
All permissions and promotions remain `false`.  Copy/move counts remain
`319/405`, with accepted-new closure count `0` and remaining count `86`.

This reverse artifact intentionally does not self-declare independent test
acceptance; it is ready for the separate test role requested by the parent.
