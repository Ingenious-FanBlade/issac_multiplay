> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-T02 v31 successor independent audit v1 handoff

## Result

The independent TEST verdict is
`PASS_EXACT_V31_SUCCESSOR_ONE_POSITIVE_102_UNKNOWN`.

The v31 frontier is the exact permitted composition of v30 with the sealed
Wave A independent audit:

- removed from the copy/move UNKNOWN frontier: `0x001CBEB5` only;
- introduced into either remaining set: none;
- retained as UNKNOWN from MT-R09: `0x0001B072`, `0x0001B3E3`;
- memset frontier: byte-for-byte unchanged.

The successor is accepted as a frontier artifact. This does not promote
manager-this, whole-image raw v2, formal v7, or T0-05.

## Role isolation and bindings

The TEST validator read only the public v30 frontier, the sealed Wave A
independent audit, and the public v31 JSON. It did not read, import, or execute
`build-manager-this-bulk-remaining-frontier-v31.py`, did not use the main
builder's decision as truth, and did not modify v31.

- v30 frontier SHA-256:
  `12EC336104A9C049EE15E83E634977F2C8CAE3E11AD4AD3E9711DEBCC3D4F814`.
- Wave A independent audit SHA-256:
  `3B85E54D25888F5DA260971BB3BED0194C2032DAA313A775BEA9430BD71E73AA`.
- v31 candidate SHA-256:
  `37C23D80BF45F4162305EA174A053E0D294BD9D32B0494D4639B516C2D7551EB`.

The candidate has exactly those two predecessor/evidence bindings. A missing,
duplicate, extra builder, or wrong-hash binding is rejected.

## Independent recomputation

The TEST validator independently reconstructed the successor from v30 and the
Wave A row verdicts, then required structural equality with the public v31
JSON. The copy/move group counts changed as follows:

| Group | v30 | v31 |
|---|---:|---:|
| callerHeap | 17 | 16 |
| localHard | 7 | 7 |
| vectorGrow | 23 | 23 |
| vectorShift | 25 | 25 |
| string | 14 | 14 |
| total | 86 | 85 |

The exact set difference is
`v30.copyMove.remainingRvas - v31.copyMove.remainingRvas = {0x001CBEB5}`;
the reverse difference is empty. Each group and each flattened remaining list
is duplicate-free, group membership remains mutually exclusive, order is
stable, and both R09 UNKNOWN rows remain in `localHard`.

Coverage was also recomputed rather than copied from the candidate:

- cumulative accepted-positive count: `4 + 1 = 5`;
- copy negative coverage: `85/85`, exactly the v31 copy remainder;
- memset negative coverage: `17/17`, unchanged;
- both remaining-coverage completeness flags remain true.

The resulting ledger is:

- copy/move: `320/405`, remaining `85`;
- memset: `225/242`, remaining `17`;
- total accepted: `545/647`;
- total remaining: `102`.

The v31 frontier and final-decision objects continue to mark every remaining
row UNKNOWN, disable frontier positive promotion, and deny manager-this,
whole-image, and overall acceptance.

## Mutation and replay gates

All 59 negative mutations were rejected. Coverage includes schema and
create-new drift, extra claims, predecessor/audit binding drift, builder
binding injection, wrong supersedes identity, accepted-delta inflation,
retaining R01, dropping or duplicating an unaccepted row, group/list reorder,
copy or memset ledger drift, positive/negative coverage drift, R09 promotion,
Wave A role/target/scope/verdict/ledger mutation, final promotion flags, and
enabled permissions.

The validator self-test was created once and reproduced byte-for-byte by two
consecutive `--verify-existing` runs. Its SHA-256 is
`7A7ABB6E7231CC1526C4CA07305600BF82E3EEE09BBA66A3C9938956E270A5BF`.

The formal v31 audit was likewise created once and reproduced byte-for-byte by
two consecutive verify-existing runs. Its SHA-256 remained
`244459B2B5C47A74E1D806E237895A9C7DB896CC748E0710885CFBFAC46F314D`.

An ordinary create-new rerun correctly rejected overwrite. A run with an
incorrect v31 physical hash failed before creating its negative-test output.

## Artifacts

- Validator:
  `out/cs-t0/audit-manager-this-bulk-remaining-frontier-successor-v31-independent-v1.py`.
- Validator self-test:
  `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v31-independent-validator-v1-self-test.json`.
- Formal machine audit:
  `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v31-independent-audit-v1.json`.
- This handoff:
  `docs/archive/refactor/cs-t0/t0-05/06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v31-independent-audit-v1-handoff.md`.

No production code, runtime interface, prior evidence, status/index file, IDB,
or external checkout was modified by MT-T02.
