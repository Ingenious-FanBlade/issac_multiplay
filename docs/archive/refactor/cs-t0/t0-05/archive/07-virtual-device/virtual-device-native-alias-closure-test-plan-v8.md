> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure instruction-span retry TEST preseal v8

## 1. Scope and frozen failure boundary

This append-only preseal authorizes one minimal `NATIVE-ALIAS-CLOSURE` retry after the consumed v7 Group 1 failure. It does not authorize new REVERSE work, evidence changes, factory/rollback work, final-release/thread work, production implementation, runtime access, or wider testing.

The immediately governing lineage is:

| Artifact | SHA-256 |
|---|---|
| TEST preseal v7 | `C2EB81220638EC37E51A2BB11C15B8BBA9F173D41E310FD17BE9D61F69743B3F` |
| v7 failed verifier | `429F3A84A159D96A463F68B0C8AF027DA25E8BBDFFB952D9FF5B67B65CDDA0FD` |
| v7 failed decision | `C616EA8F5E249158C59EBB1A88469D7CEE4B43B9607DE87A3CA2B09E30F0B176` |
| Failure result v7 | `74E6A66FB9DB2DE84FDFBB4E7A648A8491B16A2B2EC0F0CD8A70AC02CA864ECA` |

The v7 Group 1 invocation is permanently consumed. It returned outer exit code `1`, verifier `exitCode=2`, `verificationStatus=REJECTED`, `outputWritten=false`, and error `INSTRUCTION_ANCHOR:ANCHOR-CALLBACK-NO-ARGUMENT-READ_SPAN_DECODE_JOIN`. Groups 2-4 were not executed and the retry-v6 JSON remained absent.

All v1-v6 plan/result/failing-verifier/failing-decision lineage already enumerated by preseal v7 remains immutable and transitively bound; it is not reopened by this plan.

## 2. Immutable evidence and tooling

The following inputs must remain byte-for-byte unchanged:

- `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- `virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- `virtual-device-native-alias-closure-reverse-v2.md`, SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- Public wrapper, SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner, SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Compressed negative module, SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`, exactly two test methods.
- Fixed PE Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump` SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

No manifest, REVERSE handoff, wrapper, runner, negative test, production DLL, offline lifecycle source, build script, earlier result, or earlier verification output may be edited.

## 3. Sealed static diagnosis

The v7 rejection is a verifier field-semantics mismatch, not a new alias-evidence defect:

- every `instructionSpan` row retains exact contiguous RVAs, raw bytes, and per-instruction `independentDecodedInstruction` values;
- joining the independently decoded span instructions exactly matches the containing row's `independentDecodedInstruction` for every top-level instruction anchor with a span;
- only top-level anchor `ANCHOR-CALLBACK-NO-ARGUMENT-READ` has a `decoded` value that is semantic prose rather than a verbatim per-instruction join;
- every other top-level instruction span matches both fields;
- receiver graph-edge spans and `crossFunctionEvidenceSpans` use `decoded` as a verbatim instruction join and must continue to match it strictly.

Therefore this retry must not change the manifest or weaken raw-byte, contiguity, independent-decode, graph-edge, or cross-function evidence checks.

## 4. Exact verifier repair budget

The only authorized logic change is inside the existing verifier's `_validate_instruction_span` contract and its existing call sites:

1. Continue to validate every span item's schema, starting RVA, contiguity, raw bytes against the fixed PE, and `independentDecodedInstruction` against independent disassembly.
2. Continue to require the joined raw bytes to equal the containing row's `rawBytes`.
3. Continue to require the verbatim joined independent disassembly to equal the containing row's `independentDecodedInstruction` whenever that field is present.
4. For calls made while validating top-level `anchors.instructionAnchors`, do not interpret the containing row's semantic `decoded` field as a verbatim instruction join. The field must remain present/unchanged according to the frozen manifest row; this exception does not permit it to replace independent disassembly.
5. For receiver graph-edge spans and every `crossFunctionEvidenceSpans` row, retain the existing strict requirement that `decoded` and the applicable independent decode equal the verbatim joined instructions.

The implementation may express this distinction with one explicit mode/boolean parameter or an equivalently narrow call-site distinction. It must not key behavior on a broad error catch, silently skip `independentDecodedInstruction`, special-case arbitrary IDs outside the top-level-anchor domain, or weaken any receiver edge/cross-span check.

No other verifier logic, frozen constant, evidence reconstruction, graph/frontier equality, nullable-range handling, path/write boundary, or verdict rule may change except mechanical hash/binding propagation described below.

## 5. Main-integration and binding budget

After this preseal is hashed, the main integrator may:

- apply only the Section 4 verifier repair;
- append bindings for the v7 failure result and this v8 preseal while preserving all existing bindings and their order;
- update the decision's verifier SHA-256 to the repaired verifier hash;
- mechanically update the decision artifact-binding list/count and the resulting decision SHA-256.

The frozen decision binding order becomes the current 25 rows with the following two lineage rows inserted immediately after the existing v7-preseal row and before immutable manifest v1:

1. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v7.md`, SHA-256 `74E6A66FB9DB2DE84FDFBB4E7A648A8491B16A2B2EC0F0CD8A70AC02CA864ECA`;
2. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v8.md`, SHA-256 equal to this create-new preseal's final disk hash.

The final decision must contain exactly 27 ordered bindings. The verifier's binding specification must use the same paths, hashes, and order. Manifest v2 remains the active alias evidence and is not rewritten merely to bind this retry.

Before formal execution, independent TEST must perform one read-only static review showing that the verifier diff is limited to Section 4 plus the two binding constants, the decision diff is purely mechanical, all 27 disk hashes/order entries agree, Python continuation/parentheses/names are coherent by text inspection, and removing only the v8 logic/lineage changes restores the v7 verifier and decision bytes/hashes. This review must not run, import, or compile the verifier and does not consume a formal group.

## 6. Acceptance semantics retained unchanged

- Manifest identity remains `IsaacCsT0-NativeAliasUniverse-v2` and the alias verdict remains `UNKNOWN`.
- Receiver dispatch and returned `{object,control}` pair transfer remain separate fixed points.
- Universe completeness remains separate from alias drainability and native owner through drain.
- `dynamicAliasUniverseClosed=false`, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE` remain mandatory.
- `VIRTUAL_DEVICE_OBJECT_OWNER` remains `UNKNOWN`; no promotion is authorized.
- Factory, rollback, reference-count, final-release, and thread conclusions remain inherited unchanged.
- Every CS-T0-05-06/T0-05/T0-06 unlock and every implementation/runtime/scheduler/transport/mutation permission remains false.

## 7. Distinct v8 publication paths

The only permitted create-new formal output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v7.json`

The v8 TEST result path is:

`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v8.md`

Both paths must be absent before Group 1. The retry-v6 output remains retired and must not be created or reused.

## 8. Exactly four v8 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the unchanged internal runner once with the fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, manifest v2 identity, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, the exact first unresolved edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged public wrapper once with the fixed PE, bound `objdump`, repository root, and retry-v7 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper once without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time, and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the single unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops all later groups. No invocation may be repeated for diagnosis or confirmation under this seal.

## 9. Result record and explicit prohibitions

After the attempt, create `virtual-device-native-alias-closure-test-results-v8.md` with create-new semantics. On success record all four exact commands, exit codes/summaries, static-review disposition, manifest/REVERSE/verifier/decision/output hashes, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, lineage, and omitted work. On failure record the consumed group and mark all later groups `NOT_EXECUTED`.

Do not run the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not invoke standalone `objdump`; it may be used only through the four sealed verifier entry points.

## 10. Pre-execution state

At creation of this preseal no v8 formal group has run. The retry-v7 JSON and v8 TEST result are absent. The only formal executions on record remain the seven consumed Group 1 attempts documented by results v1-v7.
