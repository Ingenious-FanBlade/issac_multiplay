> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST preseal v4

## 1. Scope and immutable three-failure lineage

This append-only preseal authorizes one new `NATIVE-ALIAS-CLOSURE` acceptance retry. It preserves all three consumed Group 1 failures and does not broaden scope into factory/rollback, final-release/thread, production implementation, runtime access, or wider tests.

| Attempt artifact | SHA-256 |
|---|---|
| `virtual-device-native-alias-closure-test-plan-v1.md` | `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342` |
| v1 failed verifier | `87F67B4DAE37B5A445E7C21634D6395D01F02651D8812930F415FF4EBA1007BF` |
| v1 failed decision | `07570DD050586DED75243521C0902BB568063EE14A279F8E8D6E288AFD208C62` |
| `virtual-device-native-alias-closure-test-results-v1.md` | `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C` |
| `virtual-device-native-alias-closure-test-plan-v2.md` | `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757` |
| v2 failed verifier | `0506D66130B7E07B6954A35E7329511E45D0A47DBD7B366C1AC3BC32E9D660B4` |
| v2 failed decision | `36137960E9A36A86EDA5EAC68E6910603F4318420AA8F79394F3FF031D09C5F0` |
| `virtual-device-native-alias-closure-test-results-v2.md` | `BB46E7A8BDD54063A80D4717FDB669DAB8FF45A6AE6E0D343A42CFCB115DAA03` |
| `virtual-device-native-alias-closure-test-plan-v3.md` | `A965DDA601F60A92B5EBBFEED7C832BDEEF6056BDA7002FA21187233E6EB8B72` |
| v3 failed verifier | `501F8D39F74FC081495977E8EEB1C93BFB841AF85095245C9A7E29EC864C955C` |
| v3 failed decision | `ACCD3407114C9423BB648FF961C27AC45734D79F0900BDA3E6182333B4CF204B` |
| `virtual-device-native-alias-closure-test-results-v3.md` | `1AECCC34D65A68ECCC39642B62C2A05B00C50638AEE9F040587F11A576D9624F` |

The three prior Group 1 commands are permanently consumed and must not be rerun or presented as successful oracle executions.

## 2. Frozen evidence and unchanged test surfaces

- Native alias manifest: SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- REVERSE handoff: SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`; exactly two methods.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump`: SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

## 3. Exhaustive v4 repair budget

The only logic change authorized in `_validate_function_ranges` is to implement the manifest's already-sealed append-batch canonical order. The function-range array remains exactly 8,951 rows and must be partitioned without omission or duplication into:

1. **Core prefix:** exactly 36 rows, identical to `EXPECTED_CORE_FUNCTION_RANGE_IDS` and unchanged.
2. **Indirect census segment:** exactly 8,867 rows at indices 36 through 8,902. Every id must be `indirect-census-function-{rva:08x}`, its encoded RVA must equal the row's `rva`, and RVAs must be strictly increasing within this segment. The segment begins `indirect-census-function-00001000` and ends `indirect-census-function-007170c0`.
3. **Receiver-frontier-only segment:** exactly 48 rows at indices 8,903 through 8,950. Every id must be `receiver-frontier-function-{rva:08x}`, its encoded RVA must equal the row's `rva`, and RVAs must be strictly increasing within this segment. The segment begins `receiver-frontier-function-00047180` and ends `receiver-frontier-function-0068c600`.

The two suffix segments must have disjoint RVA sets; their union plus the core prefix must equal the complete 8,951-row array. No global RVA ordering may be imposed across the append-batch boundary. Existing disk file-offset, size, SHA-256, VA/RVA, core profile, overall canonical compact profile, and containment checks remain unchanged.

No manifest row, ordering declaration, function evidence, graph, count, profile, verdict, permission, CLI path rule, wrapper, runner, or negative test may change.

Beyond that single validator correction, only mechanical lineage/hash propagation is authorized:

- add the v3 failure result and this v4 preseal to verifier and decision bindings while retaining the complete v1–v3 lineage;
- update the decision's verifier self-binding to the repaired verifier SHA;
- update binding count/order and resulting decision SHA only as required.

The repaired verifier and propagated decision must be SHA-frozen and statically compared against this exact budget before any v4 group runs. Any other change invalidates this seal.

No test, verifier, Python import/compile, or `objdump` invocation is permitted during repair and re-freeze.

## 4. Acceptance semantics retained unchanged

All substantive v1 acceptance requirements remain in force through the bound retry lineage:

- receiver dispatch and returned `{object,control}` pair transfer are separate fixed points;
- universe completeness is independent of drainability and native owner through drain;
- complete closure or conservative `UNKNOWN` with a first exact unclosed edge are the only legal evidence results;
- candidate counts, hashes, prose, or self-reported booleans cannot replace independent PE/objdump reconstruction;
- dynamic closure requires both fixed points and all zero sets, while owner promotion additionally requires drainability and native owner through drain;
- factory, reference-count, final-release, and thread states remain inherited from v4;
- `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, every permission false, and all CS-T0-05-06/T0-05/T0-06 unlocks false remain mandatory;
- the expected sealed evidence verdict remains conservative `UNKNOWN`.

## 5. Distinct v4 publication paths

The only permitted create-new JSON path is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v3.json`

The success report, created only if all four groups pass, is:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v4.md`

Both must be `ABSENT` before Group 1. Earlier JSON paths and v1–v3 reports are immutable and retired.

## 6. Exactly four v4 groups, once each, fail-fast

After the repair and static re-freeze review:

1. **Internal oracle — one invocation.** Run the unchanged internal runner with the fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, exact first edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged PowerShell wrapper with the fixed PE, bound `objdump`, repository root, and retry-v3 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to any repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time, and SHA-256 before and after.
4. **Parameterized negative module — one invocation.** Run the unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests and one shared temporary fixture.

Any failure stops all later groups. No command may be rerun for diagnosis or confirmation under this seal. A failure requires another append-only result, distinct preseal, and distinct output.

## 7. Successful report and prohibitions

Only after all groups pass, create `virtual-device-native-alias-closure-test-results-v4.md` using create-new semantics. Record exact commands, exit codes/summaries, verifier/decision/output hashes, all failed lineage, manifest/REVERSE hashes, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict and exact first edge, no promotion/unlock, and omitted work.

Do not run or delete the v4 offline lifecycle harness, historical suites/reports, Cargo tests, full T0 build, Debug/Release Native matrices, broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not modify production DLL code, offline lifecycle C++, build scripts, manifest, REVERSE handoff, wrapper, runner, negative module, prior plans/results, or v1–v4 artifacts.

## 8. Pre-execution state

At creation of this v4 preseal, no v4 group has run. The retry-v3 JSON and v4 TEST report are absent. Only the three consumed Group 1 failures recorded in v1–v3 results have executed.
