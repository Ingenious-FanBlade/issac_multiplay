> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure split-decode retry TEST preseal v11

## 1. Scope and consumed v10 boundary

This append-only preseal authorizes one minimal `NATIVE-ALIAS-CLOSURE` retry after the consumed v10 Group 1 failure. It does not authorize new alias evidence, REVERSE work, factory/rollback work, final-release/thread investigation, production implementation, runtime access, or wider testing.

The immediately governing lineage is:

| Artifact | SHA-256 |
|---|---|
| TEST preseal v10 | `DDBD8D3825E1D6FF4F9103DC2EF1B9E3A1A3267EC57BCA585BE3F4A4E305838B` |
| v10 failed verifier | `33F990D557A44FF2C958970B6840EA477AA454A518528D54BCA782AF9CC39AF0` |
| v10 failed decision | `2EC6D6C9F9C8CA06325875AD45EBFB3C5764E1EA217A191D269133DD157EF499` |
| Failure result v10 | `D159E136CB3664E7B7997634B2CE639EC023CC24AE6E9825690DB9F71AFD3D1B` |

The v10 Group 1 invocation is permanently consumed. It returned outer exit code `1`, verifier `exitCode=2`, `verificationStatus=REJECTED`, `outputWritten=false`, and error `REBUILT_SPLIT_DISPATCH:SPLIT-DISPATCH-0001`. Groups 2-4 were not executed and the retry-v9 JSON remained absent.

All v1-v9 and v10 plan/result lineage frozen by the preceding preseals remains immutable and is not reopened here.

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

The v10 rejection is a textual whitespace mismatch in the verifier's split-dispatch reconstruction, not an evidence mismatch:

- manifest `splitRegisterDispatchCandidates[*].independentDecodedInstruction` and `producerChain[*].decoded` use whitespace-normalized single spaces, for example `call *%esi` and `mov 0x28(%eax),%esi`;
- the parsed `Instruction.text` retains `objdump` mnemonic/operand column-alignment spaces;
- the reconstruction compares those texts byte-for-byte and therefore rejects the first row before semantic fields are evaluated;
- instruction RVA, raw bytes, operand, recognition class, producer/call registers, transfer count, and chain structure are consistent.

Whitespace normalization is permitted only for comparing these already independently decoded instruction strings. It cannot substitute for raw-byte, RVA, operand, register, or producer-chain validation.

## 4. Exact verifier repair budget

The only authorized logic changes are within the existing `REBUILT_SPLIT_DISPATCH` reconstruction/comparison:

1. Normalize the observed split call `Instruction.text` as exactly `' '.join(text.split())` before comparing it with the manifest row's `independentDecodedInstruction`.
2. Normalize each observed producer instruction `Instruction.text` by the same exact expression before comparing or reconstructing the corresponding `producerChain[*].decoded` value.

The comparison must remain case/operand sensitive apart from whitespace collapsing. The expected manifest strings must not be rewritten, dropped, or accepted without comparison.

All call and producer RVAs, raw bytes, decoded operand, producer-chain order and cardinality, recognition class, producer vptr register, call-target register, local transfer count, classification, receiver disposition, target set, callee disposition, instruction proof, anchors, function reference, and exact reconstructed row/array equality must remain unchanged and strict.

No global parser normalization, regex change, manifest edit, broad exception handling, field deletion, graph reuse, chain shortening, classification relaxation, or acceptance bypass is authorized. The v8 instruction-span mode, v9 `objdump -w`, and v10 trampoline selector must remain unchanged.

## 5. Main-integration and exact binding budget

After this preseal is hashed, the main integrator may:

- make only the two normalization applications in Section 4;
- update the decision's verifier SHA-256 to the repaired verifier hash;
- add exactly two lineage bindings for the v10 failure result and this v11 preseal;
- mechanically update the decision binding count/order and resulting decision SHA-256.

The current 32 bindings retain their existing relative order. Insert these two rows immediately after the existing v10-preseal row and before the immutable manifest-v1 row:

1. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v10.md`, SHA-256 `D159E136CB3664E7B7997634B2CE639EC023CC24AE6E9825690DB9F71AFD3D1B`, role `V5_RETRY_V10_FAILED_TEST_RESULT`;
2. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v11.md`, SHA-256 equal to this create-new preseal's final disk hash, role `TEST_RETRY_PRESEAL_V11`.

The final decision and verifier binding specification must each contain exactly 34 rows in that order. Manifest v2 remains the active alias evidence and is not rewritten merely to bind this retry.

Before formal execution, independent TEST must perform a read-only static review proving that the verifier diff is exactly the Section 4 normalization plus the two binding constants/specifications, the decision diff is purely mechanical, all 34 paths/roles/disk hashes and their order match, every split raw/RVA/operand/chain/classification check remains present, and in-memory removal of only the v11 changes restores the v10 verifier and decision hashes. The review must not run, import, or compile the verifier and must not invoke wrapper, runner, tests, or `objdump`; it does not consume a formal group.

## 6. Acceptance semantics remain unchanged

- Manifest identity remains `IsaacCsT0-NativeAliasUniverse-v2`; the evidence verdict remains `UNKNOWN`.
- Receiver dispatch and returned `{object,control}` pair transfer remain separate fixed points.
- Universe completeness remains separate from drainability and native owner through drain.
- `dynamicAliasUniverseClosed=false`, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE` remain mandatory.
- `VIRTUAL_DEVICE_OBJECT_OWNER=UNKNOWN`; no promotion is authorized.
- Factory, rollback, reference-count, final-release, and thread conclusions remain inherited unchanged.
- Every CS-T0-05-06/T0-05/T0-06 unlock and every implementation/runtime/scheduler/transport/mutation permission remains false.

## 7. Distinct v11 publication paths

The only permitted create-new formal output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v10.json`

The v11 TEST result path is:

`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v11.md`

Both paths must be absent before Group 1. Every earlier retry output remains retired and must not be created or reused.

## 8. Exactly four v11 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the unchanged internal runner once with the fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, manifest v2 identity, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, the exact first unresolved edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged public wrapper once with fixed PE, bound `objdump`, repository root, and retry-v10 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper once without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time, and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the single unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops all later groups. No invocation may be repeated for diagnosis or confirmation under this seal.

## 9. Result record and prohibitions

After the attempt, create `virtual-device-native-alias-closure-test-results-v11.md` with create-new semantics. On success record all four exact commands, exit codes/summaries, static-review disposition, all 34 bindings, manifest/REVERSE/verifier/decision/output hashes, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, lineage, and omitted work. On failure record the consumed group and mark every later group `NOT_EXECUTED`.

Do not run the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not invoke standalone `objdump`; it may be used only through the four sealed verifier entry points.

## 10. Pre-execution state

At creation of this preseal no v11 formal group has run. The retry-v10 JSON and v11 TEST result are absent. The only formal executions on record remain the ten consumed Group 1 attempts documented by results v1-v10.
