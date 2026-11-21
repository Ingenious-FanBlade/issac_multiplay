> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R09 EntityList REVERSE handoff v1

## Result

MT-R09 was replayed against the frozen x86 PE only.  The final candidate is
`out/cs-t0/native-join-manager-this-mt-r09-entity-list-v3.json` and retains both
rows as `UNKNOWN` (`0 STATIC_CONFIRMED_NO_OVERLAP / 2 UNKNOWN`).  It therefore
adds zero rows to the v30 positive frontier and does not create a successor.

The frozen Wave A public envelope is bound at SHA-256
`4A54A94A72BC3D0FC8F296966B2335B49304A635D13DB95C1ACA78DAF085558A`.
The rejected v1 and v2 candidates are deliberately preserved.  v2 corrected
the serialized producer-union consistency; v3 changes only the two frozen
hexadecimal-string field types required by the public contract.

## Current-PE findings

- `EntityList+0x70` is a current-PE four-field `EntityList_EL` object, not an
  offset collision: flag `+0x00`, data `+0x04`, capacity `+0x08`, size `+0x0C`.
  Thus the scratch count is `EntityList+0x7C`.
- Current semantic functions were classified read-only at: copy constructor
  `0x00417510`, Destroy `0x004175B0`, Reset `0x00418230`, QueryRadius
  `0x0041A790`, QueryCapsule `0x0041AB50`, and QueryType `0x0041AF60`.
  The candidate binds their relevant whole-function/basic-block bytes.
- IDA xref census: copy constructor `5` xrefs / `4` caller functions; Destroy
  `708` xrefs (`239` call-type and `469` function-offset references) / `154`
  caller functions; Reset `3` xrefs / `3` callers.  The large Destroy universe
  remains semantically unclassified and therefore open.
- The REP signatures uniquely relocate only Reset and the `EntityList_EL` copy
  constructor.  QueryType, QueryRadius, QueryCapsule, Destroy and Untie are
  `NO_USABLE_SEED`.  In particular, Untie has no signature-bound vanilla
  current-PE function; its external declaration cannot be used for promotion.
- `sub_41C490` receives `EntityList+0x70` as an already-offset `EntityList_EL*`
  and writes through it.  This is a concrete counterexample to a parent-direct-
  store-only scan.  Its capacity diagnostic returns, but the store and size
  increment continue.
- The destination arena at `EntityList+0x80` has a current-PE allocation extent
  of `0x20000` bytes and capacity `0x8000` elements.  No hard invariant proving
  `scratchCount<=0x8000` was found: source/scratch push paths diagnose overflow
  and continue.  The retained minimum countermodel is count `0x8001`, copy
  length `0x20004`, which exceeds the arena extent.  Consequently neither
  destination/source span closure nor either target-slot non-overlap closes.

The first gap for both rows remains
`ENTITYLIST_SOURCE_COUNT_HARD_BOUND_LE_0X8000_WRITER_UNIVERSE_OPEN`; the
already-offset/lifetime fixed point and current-PE Untie relocation are also
unresolved.

## Deterministic replay

The REVERSE builder was run once in create-new mode and twice in
verify-existing mode:

```text
python out/cs-t0/build-manager-this-mt-r09-entity-list-v3.py
PASS mode=create-new task=MT-R09 verified=0 unknown=2 mutations=16/16

python out/cs-t0/build-manager-this-mt-r09-entity-list-v3.py --verify-existing
PASS mode=verify-existing task=MT-R09 verified=0 unknown=2 mutations=16/16

python out/cs-t0/build-manager-this-mt-r09-entity-list-v3.py --verify-existing
PASS mode=verify-existing task=MT-R09 verified=0 unknown=2 mutations=16/16
```

The deterministic LF serialization digest printed by all three runs was
`1CFA03066DD0261062F74E617BF74A7802C060514AD2B8F20851BDF6833595A6`.
Physical file hashes are:

- builder v3:
  `C01083039FA5AB31F0C80EA1E902B80C4452A5C460E06744B53FB02B045C9530`
- candidate v3:
  `10045BD2C00B7DE845807DE7DF6FFE566265DA05ED1AC443C9E325D60251F445`
- preserved rejected candidate v2:
  `7A546881B783A806F0CCF484CD4109828EB5976D39D7B9DB365228EEEADD0288`
- preserved rejected candidate v1:
  `202FF50472D7FADE6A83F132F167A1AAAEFDFF3B93EC28A035B971E79854ED60`

## Role and permissions

This handoff and candidate were authored by the REVERSE role.  No independent
TEST audit was performed or imported into the candidate.  The game was not
started, no process was opened, no DLL/module was injected or loaded, the REP
offline scanner was not run, and the IDB, production code, prior evidence,
status/index and positive frontier were not modified.
