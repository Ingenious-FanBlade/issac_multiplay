> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Native join admission v3 independent test results v1

## Result

The version 3 independent gate completed every required check and exited 1.
Its local structural and integrity checks passed, while fresh-B source
preservation remained `UNKNOWN`.  The whole synchronous proof also remained
`UNKNOWN`; no plan, progress, runtime, reader, injection or live authority was
created.

- formal result: `FAILED_FRESH_B_SOURCE_PRESERVATION_PROOF`
- `freshBStatus`: `UNKNOWN`
- `localChecksStatus`: `PASS_LOCAL_CHECKS_ONLY`
- `overallAccepted`: `false`
- formal exit code: `1`
- required checks executed: `true`
- structural checks: 7 PASS
- semantic checks: 1 `COMPLETED_WITH_UNRESOLVED`
- integrity checks: 7 PASS

The authoritative completed report is
`cs-t0/coop-native-join-admission-verification-v3-attempt-v2.json`, SHA-256
`E098CEBC954413B095143D45439F6ACFD98C3ACA8AC5E6CB7D7057CBD173C870`.
Its supervising log is
`out/cs-t0/native-join-admission-v3-verifier-attempt-v2.log`, SHA-256
`E5C1195232E58F977A4DBA5B39C2CE84A683DF2F7F445FDA67547B2FDA9AF86F`.

## Independent checks

The contract suite ran 24 cases.  It rejected version/phase drift, capability
expansion, missing source channels, stale Init v4 references, promotion of a
partial fresh-B result into the whole synchronous proof, and fresh-B closure
claims without all four machine-level closure flags.

The synthetic fresh-B model ran 47 cases.  Its accepted fixtures stay within
`SYNTHETIC_FRESH_B_SOURCE_PRESERVATION_ONLY`; definition identity, rather than
the final byte value alone, detects write-then-restore, wide overlap, aliasing,
missing or reordered consumers, second iterations, unknown callees, and
allocator null/alias paths.  Model acceptance is not native proof.

The submitted-evidence checker ran 52 adversarial cases.  It accepts a legal
`UNKNOWN` package and rejects missing or mismatched source/consumer reverse
maps, descriptive owners in machine identity fields, bad publication or alias
ranges, false writer disjointness, intervening observations hidden by restore,
constructor writes attributed to allocator zeroing, and allocator identity
claims with unresolved null/alias conditions.  It also freezes the distinct
`CANONICAL_EMPTY_OR_EXPLICIT_ZERO_LOOP` and
`EXPLICITLY_ZEROED_INLINE_RANGE` classifications.

The public inspector suite ran 6 groups / 71 assertions.  A complete fixture,
the repository and a byte-identical executable copy produced the same
`BLOCKED_BY_EVIDENCE` report with exit 1.  Bad targets, duplicate parameters,
raw path aliases, overwrite attempts, source/snapshot destinations, DOS/ADS
aliases, trailing-dot/space parents, junctions and sealed input mutations were
rejected without a report.  The published blocked report is
`out/cs-t0/native-join-admission-v3-inspection-attempt-v1.json`, SHA-256
`41D68C8B29F3B25AA9E152F9E8B0CF8B27CCD2180AA5584DCCABE9B16E47EC04`.

## Disk reconstruction and local facts

The verifier reconstructed the frozen 92-function v2 set and all v3 rows from
the approved PE bytes with the pinned decoder.  The combined comparison used
45 Init v4 rows, 47 cache/P0 rows and 62 Init v5 rows (45 repeated frozen
entries plus 17 additions):

- 47,694 decoded instructions
- 5,276 calls
- 19,679 writes
- 366 exits
- all declared instruction/call/write/exit sets matched the disk bytes
- 796 independent raw control/effect items remained unresolved

All 190 submitted consumers hit independently decoded instructions: 151
reconcile calls, 8 direct formula points, 14 callback calls and 17 tail
external calls.  The 151 formula texts, widths and signedness matched the
sealed cache/P0 inventory.  The source mapping was frozen as submitted (147
all-source, one two-source and three single-source entries) but was not treated
as a machine dependency proof.

Thirty mandatory definition-writer heads matched independent bytes, including
the four-iteration active-slot constructor loop, both halves of each Init slot,
the register-derived extra-map and byte-vector writes, and the byte write at
`0x00355B67` for the secondary inventory suppression field.  The Effects empty
branch at `0x00530A73/0x00530A77` still reaches the direct call at
`0x00530A7C -> 0x00530980`.  Both publication points were dword writes; their
object identity and path ordering remain unproved.

These facts are local membership and regression checks.  They do not establish
consumer-specific reaching definitions, alias exclusion, publication order or
callee composition.

## Preserved unresolved obligations

The conservative semantic review recorded 604 unresolved facts:

- 17 source-completeness obligations
- 190 consumer dispositions
- 190 machine-semantic formula obligations
- 2 publication points
- 70 writer dispositions
- 56 writer owner/offset semantic bindings
- 1 allocator proof
- 4 execution segments
- 1 global alias exposure
- 65 callee-effect summaries
- 8 proof frontiers

The submitted structural package separately retained 3,880 unresolved
function, edge, summary and propagation items.  In particular, allocator
zeroing is not claimed (`zeroedRanges` is empty), allocator null/disjointness is
unknown, dynamic resize/loop bounds remain unknown, global post-publication
alias writers are not excluded, callback/tail effects are not composed, and
exceptional/native indirect control remains open.

## Integrity and retained failures

The explicit v3 dependency audit covered 4 document nodes, 86 edges and 74
bound leaves.  It found no self-binding or cycle and did not reinterpret opaque
historical relative paths.  The completed verifier froze 381 input bindings,
including the formal v2 baseline of 316 inputs, all 174 protected existing
sources, 164 snapshots, 149 original-baseline files, 3 production bindings and
the 3 mutable indices.  Plan v10 and progress v4 remained current and all three
indices were unchanged.

The first formal run is deliberately retained.  An earlier development command
had created five untracked Python cache files inside frozen test directories;
the immutable pre/post checks correctly failed.  The caches were removed to
restore the frozen baseline and no old source file was edited.  The retained
failure report is `cs-t0/coop-native-join-admission-verification-v3.json`,
SHA-256
`A72B4A4D04E82C844E5E1AABE8B7A5F4A821531C531D423295D199B86ACB2D85`;
its log is `out/cs-t0/native-join-admission-v3-verifier-attempt-v1.log`,
SHA-256
`B32CC16EA78232E5899D793206518E80FF2566B7FB81025D511511012E9B5C7E`.
The 52-case final schema development record is
`out/cs-t0/native-join-admission-v3-evidence-schema-attempt-v4.json`, SHA-256
`4C9FF8626A795D61D45880A82EE5FCDBB5DBE0CB34EE6AEBB3D9D4B602CF7769`.

No runtime was observed, no process was inspected, and no production build or
native reverse conclusion was produced by this independent test role.
