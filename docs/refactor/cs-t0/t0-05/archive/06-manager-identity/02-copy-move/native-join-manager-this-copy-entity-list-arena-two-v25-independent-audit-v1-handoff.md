> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 EntityList arena exact-two v25 independent audit v1

## Verdict

The candidate is accepted only as a fail-closed negative package: `0 VERIFIED / 2 UNKNOWN` for `memcpy` RVAs `0x0001B072` and `0x0001B3E3`. No positive closure, row promotion, manager-this promotion, whole-image promotion, ledger change, or overall acceptance is authorized.

The independent audit used only the frozen on-disk PE. It did not import the candidate analyzer, modify any candidate artifact, start the game, open/read a target process, attach a debugger, retry the occupied shared IDA request, write an IDB, or modify production/tests/status/index files. IDA remains `TIMEOUT_NO_PASS` and was not used for the verdict.

## Candidate physical-byte bindings and replay

- Candidate analyzer SHA-256: `1FB777537502657E00FBB52EE46C13A373047D1F0BAAF3558EB7E81764E518A8`
- Candidate machine JSON physical-byte SHA-256: `B22A75A5167D64D1824D02F390EDCEBFBC254026CDE942055B0770AC45004537`
- Candidate handoff SHA-256: `CD76541235F311889F5754096123C14203F68232A1F854EFB6478504B443B47F`
- Candidate analyzer-reported canonical-LF rendering hash: `30AE156B401CDFA90D1F25C1655B4A94979B2BF4347F91408106096B59A5EDEB`

The physical JSON hash was recomputed from file bytes and intentionally differs from the canonical-LF rendering hash. Two consecutive candidate `--verify-existing` runs passed with `verified=0 unknown=2 mutations=12/12`; the candidate physical files remained unchanged.

## Independent PE reconstruction

- Both exact five-byte calls were reproduced and resolve to `memcpy` VA `0x00AF05DF`:
  - `0x0001B072`: `E8 68 55 6D 00`
  - `0x0001B3E3`: `E8 F7 51 6D 00`
- The constructor requests `0x20000` bytes, publishes the returned base at `EntityList+0x84`, sets capacity `EntityList+0x88` to `0x8000` elements, and initializes cursor `EntityList+0x8C` to zero. Exact cursor-zero stores were independently bound at VA `0x0041848B` and `0x004187C7`.
- `sub_417540` initially forms `base + 4*cursor` and preserves the requested count. If `uint32(cursor+request)>capacity`, it logs, recomputes, resets the returned pointer/origin to `base/0`, and publishes `uint32(origin+request)`. It never clamps `request`.
- A retained current-request counterexample starts at a known zero cursor with request `0x8001`: the helper returns `arena.base` and count `0x8001`, so the target `memcpy` length is `0x20004`, exceeding the `0x20000` allocation without multiplication wrap.
- Special source (`+0x9C`), active source (`+0x4C`), and scratch (`+0x7C`) push paths all compare against their nominal capacities and call `A112C0` on overflow, but then reload the count, perform the store, and increment it. Thus constructor capacities `0x100`, `0x800`, and `0x4000` are not hard runtime count bounds.
- The complete hard source-count/writer fixed point establishing `<=0x8000` was not closed. Both rows retain the first gap `ENTITYLIST_SOURCE_COUNT_HARD_BOUND_LE_0X8000_WRITER_UNIVERSE_OPEN`.

Copy/move accounting remains exactly `319/405`, with zero accepted delta and `86` remaining.

## Auditor reproduction

```powershell
python out\cs-t0\audit-manager-this-copy-entity-list-arena-two-v25-independent-v1.py --verify-existing
```

Create-new succeeded, followed by two consecutive `--verify-existing` passes. All `26/26` independent candidate mutations were rejected.

## Audit artifact hashes

- Independent auditor SHA-256: `38E3B1107ECDFF909B7C0CC82699D36F6EBCD600FB1AF9D51C8483437CB6A672`
- Independent machine JSON SHA-256: `0682BA829B3D1EE2E59731A40CEA8BF56B9E47DF6BEAD2461DE164CE905FC5F7`

All permission, promotion, mutation, and overall-acceptance switches remain false.
