> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure trampoline-flow retry TEST preseal v10

## 1. Scope and consumed v9 boundary

This append-only preseal authorizes one minimal `NATIVE-ALIAS-CLOSURE` retry after the consumed v9 Group 1 failure. It does not authorize new alias evidence, REVERSE work, factory/rollback work, final-release/thread investigation, production implementation, runtime access, or wider testing.

The immediately governing lineage is:

| Artifact | SHA-256 |
|---|---|
| TEST preseal v9 | `53D72424544636036CA03D874A2A4AECF1BC4C3F2C687C4307DF4E8886A244B4` |
| v9 failed verifier | `E30A7A17F5328C06A551C9E3E1E219FBD1BE634099FCED29964F1DBDF962439A` |
| v9 failed decision | `15C609C69E03EB2D5CE77626E9BAA4CAC18BB1184CB26BD668E927685DC061CE` |
| Failure result v9 | `96B99D1CC332E9ECB41E4911C7E40356E896A7DC8D88019FBE461689E9389DA6` |

The v9 Group 1 invocation is permanently consumed. It returned outer exit code `1`, verifier `exitCode=2`, `verificationStatus=REJECTED`, `outputWritten=false`, and error `REBUILT_THREAD_TRAMPOLINE_FLOW`. Groups 2-4 were not executed and the retry-v8 JSON remained absent.

All v1-v8 and v9 plan/result lineage frozen by the preceding preseals remains immutable and is not reopened here.

## 2. Immutable evidence and test surfaces

The following artifacts must remain byte-for-byte unchanged:

- `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- `virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- `virtual-device-native-alias-closure-reverse-v2.md`, SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- `virtual-device-native-alias-closure-objdump-width-reverse-v1.md`, SHA-256 `D01912DA14DC524E320CDEE8C8BAF5456158CEB25AFB4C2C3A296FA01B36F188`.
- Public wrapper, SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner, SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Compressed negative module, SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`, exactly two test methods.
- Fixed PE Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump` SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

No manifest, REVERSE document, wrapper, runner, negative test, production DLL, offline lifecycle source, build script, earlier result, or earlier verification output may be edited.

## 3. Sealed static diagnosis

The v9 rejection is a selector defect in the verifier's reconstruction, not a manifest-span defect:

- the trampoline span contains prologue instruction `push %esi` at RVA `0x0065A767`;
- the same span contains the Manager argument instruction `push %esi` at RVA `0x0065A781`;
- the verifier's unconstrained `next(push %esi)` selects the earlier prologue push;
- that selection makes the existing thread-trampoline order predicate reject with `REBUILT_THREAD_TRAMPOLINE_FLOW`;
- the manifest span, raw bytes, contiguity, and independent disassembly are correct.

The repair must distinguish the Manager argument push by dataflow position. It must not delete or bypass the order predicate and must not reinterpret the prologue push as the Manager argument.

## 4. Exact verifier repair budget

The only authorized verifier logic change is the `manager_push` selector in the existing rebuilt thread-trampoline flow:

- select the decoded `push %esi` whose instruction RVA is strictly after the already selected `manager_load` instruction;
- the selected row must therefore be the Manager argument push at RVA `0x0065A781`, not the prologue push at RVA `0x0065A767`;
- absence of a qualifying post-load push must continue to reject rather than fall back to the prologue or a guessed row.

All existing order checks must remain, including the required ordering among context load, callback load, Manager load, Manager push, and callback dispatch. All existing EDI no-write checks, register identity, callback/Manager dataflow, raw bytes, independent decode, instruction-span contiguity, function/range grounding, graph/frontier equality, and first-edge/verdict rules must remain strict.

No hardcoded acceptance bypass, broad exception handling, predicate deletion, graph reuse, new evidence, alternate target, or semantic relaxation is authorized. The v8 instruction-span mode and v9 `objdump -w` argument must remain unchanged.

## 5. Main-integration and exact binding budget

After this preseal is hashed, the main integrator may:

- make only the Section 4 `manager_push` selector change;
- update the decision's verifier SHA-256 to the repaired verifier hash;
- add exactly two lineage bindings for the v9 failure result and this v10 preseal;
- mechanically update the decision binding count/order and resulting decision SHA-256.

The current 30 bindings retain their existing relative order. Insert these two rows immediately after the existing v9-preseal row and before the immutable manifest-v1 row:

1. `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v9.md`, SHA-256 `96B99D1CC332E9ECB41E4911C7E40356E896A7DC8D88019FBE461689E9389DA6`, role `V5_RETRY_V9_FAILED_TEST_RESULT`;
2. `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v10.md`, SHA-256 equal to this create-new preseal's final disk hash, role `TEST_RETRY_PRESEAL_V10`.

The final decision and verifier binding specification must each contain exactly 32 rows in that order. Manifest v2 remains the active alias evidence and is not rewritten merely to bind this retry.

Before formal execution, independent TEST must perform a read-only static review proving that the verifier diff is exactly the Section 4 selector constraint plus the two binding constants/specifications, the decision diff is purely mechanical, all 32 paths/roles/disk hashes and their order match, the original order and EDI/dataflow predicates remain present, and in-memory removal of only the v10 changes restores the v9 verifier and decision hashes. The review must not run, import, or compile the verifier and must not invoke wrapper, runner, tests, or `objdump`; it does not consume a formal group.

## 6. Acceptance semantics remain unchanged

- Manifest identity remains `IsaacCsT0-NativeAliasUniverse-v2`; the evidence verdict remains `UNKNOWN`.
- Receiver dispatch and returned `{object,control}` pair transfer remain separate fixed points.
- Universe completeness remains separate from drainability and native owner through drain.
- `dynamicAliasUniverseClosed=false`, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE` remain mandatory.
- `VIRTUAL_DEVICE_OBJECT_OWNER=UNKNOWN`; no promotion is authorized.
- Factory, rollback, reference-count, final-release, and thread conclusions remain inherited unchanged.
- Every CS-T0-05-06/T0-05/T0-06 unlock and every implementation/runtime/scheduler/transport/mutation permission remains false.

## 7. Distinct v10 publication paths

The only permitted create-new formal output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v9.json`

The v10 TEST result path is:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v10.md`

Both paths must be absent before Group 1. Every earlier retry output remains retired and must not be created or reused.

## 8. Exactly four v10 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the unchanged internal runner once with the fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, manifest v2 identity, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, the exact first unresolved edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged public wrapper once with fixed PE, bound `objdump`, repository root, and retry-v9 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper once without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time, and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the single unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops all later groups. No invocation may be repeated for diagnosis or confirmation under this seal.

## 9. Result record and prohibitions

After the attempt, create `virtual-device-native-alias-closure-test-results-v10.md` with create-new semantics. On success record all four exact commands, exit codes/summaries, static-review disposition, all 32 bindings, manifest/REVERSE/verifier/decision/output hashes, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, lineage, and omitted work. On failure record the consumed group and mark every later group `NOT_EXECUTED`.

Do not run the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not invoke standalone `objdump`; it may be used only through the four sealed verifier entry points.

## 10. Pre-execution state

At creation of this preseal no v10 formal group has run. The retry-v9 JSON and v10 TEST result are absent. The only formal executions on record remain the nine consumed Group 1 attempts documented by results v1-v9.
