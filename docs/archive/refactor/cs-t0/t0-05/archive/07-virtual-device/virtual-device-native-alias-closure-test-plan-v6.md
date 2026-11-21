> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST preseal v6

## 1. Scope and immutable five-failure lineage

This append-only preseal authorizes one new `NATIVE-ALIAS-CLOSURE` retry. It preserves the five consumed Group 1 failures and does not authorize factory/rollback, final-release/thread investigation, production implementation, runtime access, or wider testing.

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

Paths for the plan/result hashes above are the correspondingly numbered files under `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/`. All five former Group 1 invocations are permanently consumed.

## 2. Frozen evidence and unchanged execution surfaces

- Native alias manifest: `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- REVERSE handoff: SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`; exactly two test methods.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump`: SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

The manifest and every REVERSE-authored row remain immutable. In particular, the explicit unresolved function-range rows are conservative evidence and must not be rewritten as identified ranges.

## 3. Exhaustive v6 repair budget

The only logic correction authorized in the verifier is nullable candidate `functionRange` handling in `_function_range_shape` plus application of that same helper to split candidates:

1. Keep the existing exact key set `status`, `startRva`, `endRvaExclusive`, `size`, and `analysisLabel`.
2. For `status == "IDENTIFIED"`, retain the existing checks without relaxation: parse `startRva`/`endRvaExclusive`, require `size == end - start`, and require the candidate instruction to lie within the half-open range.
3. For `status == "UNRESOLVED_IDA_CODE_REGION"`, accept only when `startRva`, `endRvaExclusive`, `size`, and `analysisLabel` are all JSON null, then return without RVA conversion. Any non-null payload or any other status must fail closed.
4. Call `_function_range_shape` for every `splitRegisterDispatchCandidates` row using that row's `functionRange` and `instructionRva`, so the same identified/unresolved contract covers both the base-only and corrected split domains.

The known final-manifest nullable population is exactly one base-only dispatch candidate and two split-register dispatch candidates, each with `status=UNRESOLVED_IDA_CODE_REGION` and all four remaining fields null. This count is a static-review expectation, not permission to replace row-by-row validation with a count or boolean.

No other helper, graph reconstruction, census rule, schema, count, profile, manifest string, evidence row, verdict, permission, output rule, wrapper, runner, or negative case may change.

Beyond the exact logic above, only mechanical lineage/hash propagation is authorized:

- bind the v5 failure result and this v6 preseal in verifier and decision while retaining the complete v1–v5 lineage;
- update the decision's verifier binding to the repaired verifier SHA;
- update binding order/count and resulting decision SHA only as mechanically required.

Any additional implementation modification invalidates this preseal.

## 4. Mandatory static review before execution

After the bounded repair and mechanical re-freeze, independent TEST must perform a read-only static review before any formal group. It must not run/import/compile the verifier, invoke the wrapper/runner, execute tests, or invoke `objdump`; it does not consume a formal group.

The review must:

- enumerate every `_function_range_shape` call and confirm base direct seeds, base dispatch candidates, pair seeds, and split dispatch candidates all use the intended identified/unresolved contract;
- confirm exactly one base dispatch and two split dispatch rows carry the permitted all-null unresolved shape, and that no identified row contains a null range member;
- prove the helper does not call `_rva` before branching on `status`, rejects partially populated unresolved rows, and preserves the original identified half-open containment/size checks;
- confirm the complete v1–v5 failure lineage, artifact-binding order/count, and every bound disk SHA;
- statically inspect Python continuation, brackets, names, and the unchanged manifest string/count/schema/profile/graph/frontier/decision equalities affected by the edit;
- prove by in-memory reverse transformation that removing only the v6 nullable-range repair and the two new lineage bindings restores verifier SHA `17DDA23D7151E8DB4D0F0C2FC476548FFDB79316F807254451C0328E227393A9` and decision SHA `AAA2828273356DA4D2A4932747EF15CED4BF682EF2028C1855BF5E4C220C6960`.

If any mismatch is found, formal execution remains sealed and a new plan is required after an explicitly budgeted repair.

## 5. Acceptance semantics retained unchanged

All substantive v1 acceptance rules remain binding through the complete retry lineage:

- receiver dispatch and returned `{object,control}` pair transfer are separate fixed points;
- universe completeness remains separate from alias drainability and native owner through drain;
- complete closure or conservative `UNKNOWN` with an exact first edge are the only legal evidence results;
- self-reported counts/booleans and candidate prose cannot replace independent disk graph/set/frontier reconstruction;
- dynamic closure requires both fixed points and every required zero set, while owner promotion additionally requires drainability and native owner through drain;
- factory, reference-count, final-release and thread states remain inherited from v4;
- `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, every permission false, and every CS-T0-05-06/T0-05/T0-06 unlock false remain mandatory;
- the final native evidence verdict remains conservative `UNKNOWN`.

## 6. Distinct v6 publication paths

The only permitted create-new output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v5.json`

The success report, created only if all four groups pass, is:

`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v6.md`

Both must be absent before Group 1. All earlier JSON destinations and v1–v5 TEST reports are immutable and retired.

## 7. Exactly four v6 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the unchanged internal runner with fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, exact first edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged wrapper with fixed PE, bound `objdump`, repository root and retry-v5 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops every later group. No invocation may be repeated for diagnosis or confirmation under this seal.

## 8. Successful report and prohibitions

Only after all four groups pass, create `virtual-device-native-alias-closure-test-results-v6.md` using create-new semantics. Record exact commands, exits/summaries, verifier/decision/output hashes, complete failure lineage, manifest/REVERSE hashes, static-review disposition, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, and omitted work.

Do not run or delete the v4 offline lifecycle harness, historical suites/reports, Cargo tests, full T0 build, Debug/Release Native matrices, broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not modify production DLL code, offline lifecycle C++, build scripts, manifest, REVERSE handoff, wrapper, runner, negative module, or prior plans/results.

## 9. Pre-execution state

At creation of this preseal, no v6 formal group has run. The retry-v5 JSON and v6 TEST report are absent. The only executions on record are the five consumed Group 1 failures documented by results v1–v5.
