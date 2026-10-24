> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure manifest-v2 retry TEST preseal v7

## 1. Scope and immutable six-failure lineage

This append-only preseal authorizes one new `NATIVE-ALIAS-CLOSURE` retry using create-new manifest and REVERSE artifacts. It preserves the six consumed Group 1 failures and does not authorize factory/rollback, final-release/thread investigation, production implementation, runtime access, or wider testing.

| Attempt artifact | SHA-256 |
|---|---|
| Preseal v1 | `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342` |
| v1 failed verifier | `87F67B4DAE37B5A445E7C21634D6395D01F02651D8812930F415FF4EBA1007BF` |
| v1 failed decision | `07570DD050586DED75243521C0902BB568063EE14A279F8E8D6E288AFD208C62` |
| Failure result v1 | `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C` |
| Preseal v2 | `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757` |
| v2 failed verifier | `0506D66130B7E07B6954A35E7329511E45D0A47DBD7B366C1AC3BC32E9D660B4` |
| v2 failed decision | `36137960E9A36A86EDA5EAC68E6910603F4318420AA8F79394F3FF031D09C5F0` |
| Failure result v2 | `BB46E7A8BDD54063A80D4717FDB669DAB8FF45A6AE6E0D343A42CFCB115DAA03` |
| Preseal v3 | `A965DDA601F60A92B5EBBFEED7C832BDEEF6056BDA7002FA21187233E6EB8B72` |
| v3 failed verifier | `501F8D39F74FC081495977E8EEB1C93BFB841AF85095245C9A7E29EC864C955C` |
| v3 failed decision | `ACCD3407114C9423BB648FF961C27AC45734D79F0900BDA3E6182333B4CF204B` |
| Failure result v3 | `1AECCC34D65A68ECCC39642B62C2A05B00C50638AEE9F040587F11A576D9624F` |
| Preseal v4 | `EE99F14C0FC2EACD970DDD8713B580ABE484E058520D3F9CD9789B459DBD0C26` |
| v4 failed verifier | `E2F23C7D857B7834A6FFF5489F5F001871845FC428BD5943AC0460B415CAA7FE` |
| v4 failed decision | `52455293C5C05CA6B599D880EEA3047E0F3B3B082A206100ACB49E166882B4EC` |
| Failure result v4 | `14CE50755357D4B9189119371DEA5C008B4AD755453142A24A047EE41FF2E3C2` |
| Preseal v5 | `E48633F4A81B2C4493A09AF982467B96511DECED3C8F58B87C688C31186D4BD2` |
| v5 failed verifier | `17DDA23D7151E8DB4D0F0C2FC476548FFDB79316F807254451C0328E227393A9` |
| v5 failed decision | `AAA2828273356DA4D2A4932747EF15CED4BF682EF2028C1855BF5E4C220C6960` |
| Failure result v5 | `A642D254894F9A2BA2705DC3176072C18D5594AD70A92BADAAC9B240A31915D3` |
| Preseal v6 | `181E10814431CBF83E8E07D4DB8D5556F3A0B2C3F16975F06A379F731A3C3411` |
| v6 failed verifier | `525A0AC8CB721FE2DED5D553CF5FCF9B11F4372E8FEC01B189725B2F27865960` |
| v6 failed decision | `6B7429596B6D136B126C9B67F15D5A7DEFBFA72155C5C657A8E89ED6F8303B1A` |
| Failure result v6 | `9AAA26107786F9D2504828348C93A5E1C4BC4C8707D2417D3ADA0C5D1E2A0277` |

Paths for the plan/result hashes above are the correspondingly numbered files under `docs/refactor/cs-t0/t0-05/07-virtual-device/`. All six former Group 1 invocations are permanently consumed.

## 2. Immutable v1 baseline and create-new v2 surfaces

The following baseline artifacts are immutable and must remain byte-for-byte unchanged:

- `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.

The independent REVERSE role must create, never overwrite:

- `cs-t0/native-alias-universe-v2.json`, identity `IsaacCsT0-NativeAliasUniverse-v2`.
- `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-reverse-v2.md`.

Both v2 paths must be absent before REVERSE publication. The v2 manifest must bind this preseal, the immutable v1 manifest, and the immutable REVERSE v1 handoff by path and SHA-256. REVERSE v2 must record the exact per-row disposition and the IDA/PE evidence used.

The fixed evidence surfaces remain:

- Public wrapper SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Compressed negative module SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`, exactly two test methods.
- Fixed PE Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump` SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

## 3. Exact 21-row REVERSE adjudication set

The only residual rows whose provenance disposition may be changed in manifest v2 are this exact canonical-ID set, in existing residual-array order:

1. `RESIDUAL-INDIRECT-CALL-00037`
2. `RESIDUAL-INDIRECT-CALL-00316`
3. `RESIDUAL-INDIRECT-CALL-00317`
4. `RESIDUAL-INDIRECT-CALL-00318`
5. `RESIDUAL-INDIRECT-CALL-03856`
6. `RESIDUAL-INDIRECT-CALL-04923`
7. `RESIDUAL-INDIRECT-CALL-28609`
8. `RESIDUAL-INDIRECT-CALL-28611`
9. `RESIDUAL-INDIRECT-CALL-30704`
10. `RESIDUAL-INDIRECT-CALL-30705`
11. `RESIDUAL-INDIRECT-CALL-30706`
12. `RESIDUAL-INDIRECT-CALL-30707`
13. `RESIDUAL-INDIRECT-CALL-30708`
14. `RESIDUAL-INDIRECT-CALL-31081`
15. `RESIDUAL-INDIRECT-CALL-31082`
16. `RESIDUAL-INDIRECT-CALL-31083`
17. `RESIDUAL-INDIRECT-CALL-32100`
18. `RESIDUAL-INDIRECT-CALL-34312`
19. `RESIDUAL-INDIRECT-CALL-34313`
20. `RESIDUAL-INDIRECT-CALL-34314`
21. `RESIDUAL-INDIRECT-CALL-34315`

This set was statically derived from v1 as every residual row with `containingFunctionRange.status=IDENTIFIED` whose `instructionRva` is outside the claimed half-open `[startRva,endRvaExclusive)` range. It is an exact set: no missing, extra, duplicate, reordered, or substituted row is allowed.

## 4. v2 evidence repair contract

For each of the 21 rows, independent REVERSE must use read-only IDA navigation together with fixed-PE raw/function evidence to choose exactly one conservative disposition:

- **Correct identified range:** publish the actual containing function range with non-null `startRva`, `endRvaExclusive`, positive `size=end-start`, correct file offset and SHA-256, nonempty IDA name, and a `functionHashRef` resolving to the matching `anchors.functionRanges` row. The instruction RVA must be inside that exact half-open range.
- **Unresolved code region:** if a correct containing function cannot be established, set `containingFunctionRange` to the exact unresolved shape with `status=UNRESOLVED_IDA_CODE_REGION` and `queriedInstructionRva` equal to the row instruction RVA, and set `functionHashRef=null`. The row remains present as unresolved frontier evidence.

Containment must not be weakened, widened by an invented range, inferred from counts, or bypassed. No affected row may be dropped or treated as safe. The row `id`, instruction RVA, raw bytes, independent decode, operand, edge kind, source, prospective destination, callee disposition, reason, affected proof, inline anchor reference, residual-array position, and unresolved alias verdict remain unchanged.

After adjudication REVERSE must rebuild only the mechanically affected evidence:

- add, remove, or replace `anchors.functionRanges` rows only as demanded by the 21 adjudications, retaining canonical batch ordering, unique IDs/RVAs, correct disk hashes and exact range containment;
- recompute residual identified/unresolved provenance totals and every dependent `actualCounts` value without changing the 34,430-row residual cardinality;
- recompute canonical compact JSON byte lengths/SHA-256 values for every changed array, including at minimum `functionRanges` and `residualIndirectCallFrontier`;
- update manifest-v2 identity/version and its new lineage bindings, plus only hashes/counts/provenance summaries mechanically affected by those changes;
- document a one-to-one 21-row disposition table in REVERSE v2.

Every non-target residual row, all tail rows, direct/vtable/pair seeds, receiver and pair nodes/edges/frontiers, service/thread/import/Manager-escape evidence, zero-set inputs, fixed-point booleans, first unresolved edges, drainability, owner-through-drain, and final verdict must retain the v1 evidence meaning. The required result remains `UNKNOWN`; `dynamicAliasUniverseClosed=false`; no promotion, permission, or unlock may occur.

## 5. Main integration budget

After REVERSE publishes and hashes the two create-new v2 artifacts, the main integrator may update only the existing v5 verifier and v5 decision artifacts as required to consume manifest v2:

- change the manifest path/identity/schema and frozen profile/count/provenance expectations to the exact v2 values;
- bind manifest v1, REVERSE v1, manifest v2, REVERSE v2, failure result v6, and this preseal while retaining the complete v1–v6 TEST lineage;
- preserve the v6 strict identified/unresolved range helper and validate every v2 identified range with exact containment and every unresolved row with its exact null/reference contract;
- update reconstructed residual rows so their provenance is independently checked against the fixed PE and exact v2 range table; never copy a provenance disposition without validation;
- mechanically update the decision manifest/reverse references, alias evidence copy, artifact-binding order/count, verifier SHA, and resulting decision SHA.

The wrapper, internal runner, compressed negative module, production DLL, offline lifecycle code, build scripts, v1 manifest, REVERSE v1, earlier plans/results, and earlier verification outputs must not change. Any other logic or evidence change invalidates this preseal.

## 6. Mandatory static review before formal execution

Independent TEST must perform a read-only static review after v2 integration and before any formal group. This review must not run/import/compile the verifier, invoke the wrapper/runner, execute tests, or invoke `objdump`; it does not consume a formal group.

The review must establish:

- v1 manifest and REVERSE v1 still match their frozen hashes, while manifest v2 and REVERSE v2 are create-new and hash-bound;
- the v1-to-v2 residual provenance diff touches exactly the 21 IDs above and only the allowed fields, plus the mechanically required function-range/profile/count/binding/identity updates;
- all 21 rows have either a correct disk-bound containing range or the exact unresolved shape, and a whole-manifest static scan finds zero identified residual, tail, base-frontier, service, Manager-escape, pair seed/graph/frontier, or other instruction-backed rows outside their claimed half-open ranges;
- no identified function range contains null members, no unresolved row carries an identified-range hash/ref, and no row was dropped or duplicated;
- all changed profile byte lengths/SHA-256 values, provenance totals, array cardinalities, graph/frontier identities and ordering, manifest verdict and decision inheritance are internally consistent;
- verifier/decision syntax, names, nullable range handling, v2 path/identity/schema, artifact-binding order/count, and all disk hashes are exact;
- in-memory reverse transformation demonstrates that removing only the v7 integration/lineage changes restores the bounded v6 verifier and decision hashes, after accounting for the intentional create-new v2 evidence dependency.

Any mismatch keeps formal execution sealed and requires a new explicitly budgeted plan.

## 7. Acceptance semantics retained unchanged

- Receiver dispatch and returned `{object,control}` pair transfer remain separate fixed points.
- Universe completeness remains separate from alias drainability and native owner through drain.
- Complete closure or conservative `UNKNOWN` with an exact first edge remain the only legal evidence outcomes.
- Counts, self-reported booleans, candidate prose, and offline lifecycle results cannot substitute for independent disk graph/set/frontier validation.
- Factory, reference-count, final-release and thread statuses remain inherited from v4.
- `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, every permission false, and every CS-T0-05-06/T0-05/T0-06 unlock false remain mandatory.
- `VIRTUAL_DEVICE_OBJECT_OWNER` must remain `UNKNOWN` because this retry only repairs provenance validity; it does not close the nonempty alias frontiers, drainability, or native owner through drain.

## 8. Distinct v7 formal publication paths

The only permitted create-new formal output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v6.json`

The success report, created only if all four groups pass, is:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v7.md`

Both must be absent before Group 1. All earlier JSON destinations and v1–v6 TEST reports are immutable and retired.

## 9. Exactly four v7 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the internal runner once with fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, v2 manifest identity, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, exact first edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the public wrapper once with fixed PE, bound `objdump`, repository root and retry-v6 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper once without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the single unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops every later group. No invocation may be repeated for diagnosis or confirmation under this seal.

## 10. Successful report and prohibitions

Only after all four groups pass, create `virtual-device-native-alias-closure-test-results-v7.md` using create-new semantics. Record exact commands, exits/summaries, v2 manifest/REVERSE/verifier/decision/output hashes, complete failure lineage, static-review disposition, the 21-row adjudication summary, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, and omitted work.

Do not run or delete the v4 offline lifecycle harness, historical suites/reports, Cargo tests, full T0 build, Debug/Release Native matrices, broad regression, duplicate self-test, game, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. IDA may be used only by the independent REVERSE role in read-only mode for the exact 21-row adjudication. Do not modify production DLL code, offline lifecycle C++, build scripts, the v1 manifest, REVERSE v1, wrapper, runner, negative module, or prior plans/results.

## 11. Pre-execution state

At creation of this preseal, no v7 formal group has run. Manifest v2, REVERSE v2, retry-v6 JSON, and v7 TEST report are absent. The only formal executions on record are the six consumed Group 1 failures documented by results v1–v6.
