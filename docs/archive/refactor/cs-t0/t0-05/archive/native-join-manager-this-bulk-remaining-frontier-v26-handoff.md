> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this bulk remaining frontier v26

## Outcome

This disk-only partition artifact freezes the exact remaining bulk frontier
after the independently accepted Theora exact11 result:

| Helper partition | Closed | Total | Remaining |
|---|---:|---:|---:|
| `memcpy` / `memmove` | 319 | 405 | 86 |
| `memset` | 221 | 242 | 21 |

The 107 retained rows are all `UNKNOWN`.  This artifact is a fail-closed work
split, not a positive range proof, whole-image promotion, or manager-this
promotion.

## Exact disjoint work sets

Copy/move exact86:

- caller-heap: 17
- local-hard: 7
- vector-grow: 23
- vector-shift: 25
- string: 14

Memset exact21:

- standard-vector group A residual: 3
- standard-vector group B: 5
- standard-vector group C: 5
- non-vector residual: 8

The non-vector residual is exactly:
`0x000148AA`, `0x0001D1D9`, `0x0024CF7A`, `0x0029FA85`,
`0x003DE449`, `0x005CF2B3`, `0x0062C316`, `0x006EC167`.

The machine JSON contains the full sorted RVA lists.  The builder proves that
the groups are internally disjoint, that copy/move and memset do not overlap,
and that the ledgers sum to 405 and 242 respectively.  It also binds the exact
accepted caller-heap reductions, Theora copy rows, retained string/vector/hard
sets, and the independent allocator rejection.

## Reproduction

```powershell
python out\cs-t0\build-manager-this-bulk-remaining-frontier-v26.py --verify-existing
```

Two consecutive replays passed; all 10 aggregate/set/status/permission
mutations were rejected.

## Artifact hashes

- Builder:
  `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07`
- Machine JSON:
  `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
