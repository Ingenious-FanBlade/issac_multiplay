> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 EntityList arena exact-two v25 handoff

## Outcome

- Scope: only `memcpy` RVAs `0x0001B072` and `0x0001B3E3` in `sub_41AF60`.
- Decision: `UNKNOWN` (`0 VERIFIED / 2 UNKNOWN`).
- The helper interpretation from v4 is corrected, but the required source-count bound is not closed; neither row is promoted.
- No game, target process, debugger, runtime mutation, production edit, test edit, prior-evidence edit, or IDB write was used.

## Corrected helper semantics

The full `sub_417540` slice is bound at RVA `0x00017540`, length `103`, SHA-256
`E96167DB9FCCFFADFE4A3542F0CB9141D8D70B27BC0D60D47A07AB2736FDA18E`.
It first returns `base + 4*cursor` and the requested element count.  When
`cursor + request > capacity`, it calls `A112C0`, recomputes the comparison,
and, if it still overflows, changes the returned pointer to `base` and the
origin cursor to zero.  It then stores `origin + request` as the new cursor.

Consequently, cumulative cursor growth is not an additional extent premise:
an overflowing slice restarts at the arena base.  The helper also does not
clamp the request.  For the constructor's `0x8000`-element, `0x20000`-byte
arena, each target row is in extent exactly when its request is at most
`0x8000` elements.

## Writer-universe gap

- The exact target rows copy the query scratch count at `EntityList+0x7C`.
- The scratch list is allocated for `0x4000` pointers; both query paths clear
  its count before filtering.
- Its push path calls `A112C0` after a capacity failure but then still performs
  the store and increments the count.
- The special and active source-list push paths in `sub_418500` have the same
  log-and-continue behavior.  Their constructor capacities (`0x100` and
  `0x800`) therefore do not constitute hard count bounds.
- The duplicate guard at `entity+0x170` is bound, but the inspected PE evidence
  does not close a hard live-entity/source-count bound of at most `0x8000`.

The first unresolved premise is
`ENTITYLIST_SOURCE_COUNT_HARD_BOUND_LE_0X8000_WRITER_UNIVERSE_OPEN`.
The retained countermodel is a source count of `0x8001` or more with every
entry satisfying the filter: the scratch count can exceed `0x8000`, the helper
returns the arena base without reducing the request, and `memcpy` can exceed
the `0x20000`-byte allocation.  Exact PE evidence has not contradicted that
countermodel, so slot non-overlap is also not discharged.

## Bound universes

- Exact `sub_417540` raw-rel32 call RVAs:
  `0x0001AA79`, `0x0001AE81`, `0x0001B03F`, `0x0001B3B2`, `0x002B8367`.
- Exact `sub_417930` constructor raw-rel32 call RVA: `0x003E94A8`.
- The analyzer additionally binds the constructor/list initialization, both
  query regions, `sub_418500`, `sub_418230`, and the frame cursor reset.
- The two target call bytes are respectively `E8 68 55 6D 00` and
  `E8 F7 51 6D 00`.

The shared IDA request remained occupied by an older read-only request.  The
machine artifact records `TIMEOUT_NO_PASS`; no IDA PASS is claimed.

## Reproduction

```powershell
python out\cs-t0\analyze-manager-this-copy-entity-list-arena-two-v25.py --verify-existing
```

Two consecutive PE replays passed with `0 VERIFIED / 2 UNKNOWN`; the semantic
mutation suite rejected `12/12` mutations.  The machine artifact's canonical
LF rendering hash is
`30AE156B401CDFA90D1F25C1655B4A94979B2BF4347F91408106096B59A5EDEB`.
The physical JSON uses Windows line endings; its file-byte SHA-256 is listed
below.

## Artifact hashes

- Analyzer:
  `1FB777537502657E00FBB52EE46C13A373047D1F0BAAF3558EB7E81764E518A8`
- Machine JSON file bytes:
  `B22A75A5167D64D1824D02F390EDCEBFBC254026CDE942055B0770AC45004537`

Accepted copy/move closure remains `319/405` (86 rows remain).  All permission,
promotion, merge, and overall-acceptance switches remain false.
