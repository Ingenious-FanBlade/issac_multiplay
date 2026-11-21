> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy/move local-hard exact5 v28 handoff

## Scope and disposition

- Role: reverse/static evidence producer only. Independent acceptance remains assigned to another testing role.
- Fixed target: `isaac-ng.exe`, PE32/I386, image base `0x00400000`, `SizeOfImage=0x008FE000`, SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Direct predecessors:
  - bulk remaining frontier v26 builder/JSON/handoff: `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07`, `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF`, `DF537CDFFE9889BE11556D13AB24493A52B80376A19240945F62763604CCD4C5`;
  - hard-eleven v10 analyzer/JSON/handoff: `E1DFDD540A4997B5CA45284BD6F02DF690D16AAFC866B8AF378C7EB0A035CD9E`, `631F67A92DE248214DE0A26AA91BDC436E102044115C5029BB6ED083BE9B6BA1`, `058F966126F0D9D31CF0E22C9DA89809D7E58381EF9907C4BA29BFDE7F53BC22`.
- Exact partition: `0x002EE04F`, `0x004FCD78`, `0x0061D0B4`, `0x00663CFC`, `0x006936B7`.
- Explicit exclusions: EntityList `0x0001B072`, `0x0001B3E3`; Theora `0x006DCB75`, `0x006DCCF5`, `0x006E1030`, `0x006E103C`. Target/exclusion intersection is empty.
- Result: **0 VERIFIED / 5 UNKNOWN**. No row, manager-this component, whole image, state, index, permission, or promotion was accepted or modified.

## Per-row first gaps

| RVA | Reconstructed local fact | First unclosed premise | Disposition |
| --- | --- | --- | --- |
| `0x002EE04F` | `memcpy` writes `uint32(4*n4-4)` into a 32-byte stack object; safety needs `1<=n4<=9` and no multiply wrap. The direct writer at `0x002E19E2` is `min(input,8)`. | `0x003B1400` copies `source+0x2094` into `this+0x5C0`; that source writer fixed point is not closed. The PE census has 22 direct write/increment sites and 28 positive LEA aliases. | `UNKNOWN`; retained `n4=10` countermodel. |
| `0x004FCD78` | Sort shift is locally `dest=a1+4`, `count=current-a1`; the owner has calls at `0x004F7080` plus recursion at `0x004FCD00/15`. | The sole non-recursive call loads begin/end from the caller `EDI` tuple at `0x004F706E`; all tuple producers and backing ownership are not closed. | `UNKNOWN`. |
| `0x0061D0B4` | Sort shift uses the persistent 16-byte vector tuple at object `+0x2C/+0x30/+0x34`. Constructor zeroing, destructor zeroing, local end updates, and grow helper `0x0061CEA0` were reconstructed. The grow helper caps element count at `0x0FFFFFFF`, so `count*16<=0xFFFFFFF0`. | The class-level copy/move, split/address-taken alias, callback, and lifecycle writer fixed point for the persistent tuple is open. `sub_A1A730` has 17 raw direct callers. | `UNKNOWN`. |
| `0x00663CFC` | Allocation at `0x006932C0` is `uint32(((width*channels)*bitDepth+7)>>3)+1`; row count is `uint32((currentWidth*uint8(bitDepth*channels)+7)>>3)`. There is no local capacity comparison. | Width is published through `0x006634C8 -> 0x006634E4 -> 0x006911F3 -> 0x00691381` from a caller-supplied descriptor, and its complete validation/source fixed point is open; the allocation products/add have no local overflow guard. | `UNKNOWN`. |
| `0x006936B7` | Byte-aligned Adam7 pixel move and its relative index order were reconstructed; it writes within the same row only conditionally on a valid row allocation and non-wrapping offsets/additions. | It inherits the same `A932C0` wrapped row backing and external width-source gap; pixel offset/loop no-wrap premises also remain open. | `UNKNOWN`. |

The retained PNG arithmetic model is concrete but not claimed reachable until upstream validation is closed: `bitDepth=8`, `channels=4`, `originalWidth=0x08000000` makes the allocation expression wrap to one byte; Adam7 step 8 gives `currentWidth=0x01000000` and a row count of `0x04000000` bytes. This is a fail-closed countermodel, not runtime evidence.

## Artifacts

- Analyzer: `out/cs-t0/analyze-manager-this-copy-move-local-hard-exact-five-v28.py`
  - SHA-256 `234ADEF93C1E0875D9945602030AAC450BFB8E3DC8F50E3E10DE08F3E0D23AF5`
- Machine report: `out/cs-t0/native-join-manager-this-copy-move-local-hard-exact-five-v28.json`
  - SHA-256 `90346CCDFDFCFC3B1825A7A3666C66FBFE191A999F300E354EE09034799F04E2`

The analyzer is mode-independent: ordinary PE mode reconstructs and binds the report; an IDA-hosted replay rechecks the same row/anchor bytes read-only. The shared external IDA analyzer had already timed out, and the parent explicitly directed no retry. Therefore this handoff records `TIMEOUT_NO_PASS`; it does not claim scripted IDA PASS.

## Reproducer and verification

From repository root:

```powershell
python out\cs-t0\analyze-manager-this-copy-move-local-hard-exact-five-v28.py
python out\cs-t0\analyze-manager-this-copy-move-local-hard-exact-five-v28.py --verify-existing
python out\cs-t0\analyze-manager-this-copy-move-local-hard-exact-five-v28.py --verify-existing
```

Observed output for create-new and both consecutive verify-existing runs:

```text
PASS PE localHard exact5 verified=0 unknown=5 n4Writers=22 n4Aliases=28 mutations=34
```

- PE create-new: PASS.
- Consecutive PE verify-existing #1: PASS.
- Consecutive PE verify-existing #2: PASS.
- Semantic mutation sentinels: 34/34 rejected, covering set/partition drift, false row promotion, erased first gaps, false writer/source closure, PNG/n4/this+44 closure, ledger promotion, fabricated IDA PASS, permission changes, binding drift, and call-byte drift.
- External IDA scripted replay at handoff: `TIMEOUT_NO_PASS`, not retried.
- Runtime/game/process/debugger use: none.
- IDB mutation: none.

## Independent acceptance boundary

The independent tester should recompute all three artifact hashes, run two fresh `--verify-existing` passes, independently mutate each first-gap and promotion predicate, confirm exact5/exclusion partition disjointness, and keep all permissions/promotions/`overallAccepted` false. This reverse role does not perform or claim that independent acceptance.
