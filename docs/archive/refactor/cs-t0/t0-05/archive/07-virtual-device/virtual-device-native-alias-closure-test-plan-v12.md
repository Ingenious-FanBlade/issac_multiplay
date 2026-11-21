> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure split rule-hit retry TEST preseal v12

## 1. Scope and consumed v11 boundary

This append-only preseal authorizes one minimal `NATIVE-ALIAS-CLOSURE` retry after the consumed v11 Group 1 failure. It does not authorize new alias evidence, broader REVERSE work, factory/rollback work, final-release/thread investigation, production implementation, runtime access, or wider testing.

The immediately governing lineage is:

| Artifact | SHA-256 |
|---|---|
| TEST preseal v11 | `E52C05C3B9936D43E05F7144FFE20B088A301347E2AFA22E6A17E2614CE8999C` |
| v11 failed verifier | `794FCB15337481E507645005E71983BBE780A7CC7378A8411BEE1B79F9B8C873` |
| v11 failed decision | `A5D2340765D145CAFBDDD5D051943F3AA351E2FD54CEBB8B1F311FF658E5D742` |
| Failure result v11 | `39425C9DC84C4CAE092CE6BA34B400E0C6222CC4A1D89EF592EA6661E9A03F13` |
| Independent split rule-hit diagnostic | `172BD3D0CE2418DB9183AC97B21B5C085C9D74BA5B8E79A7D5D3C22EFF8EF090` |

The diagnostic path is `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-split-rule-hit-reverse-v1.md`.

The v11 Group 1 invocation is permanently consumed. It returned outer exit code `1`, verifier `exitCode=2`, `verificationStatus=REJECTED`, `outputWritten=false`, and error `REBUILT_EQUIVALENT_DISPATCH_ORACLE`. Groups 2-4 were not executed and the retry-v10 JSON remained absent.

All v1-v10 and v11 plan/result lineage frozen by the preceding preseals remains immutable and is not reopened here.

## 2. Immutable evidence and test surfaces

The following artifacts must remain byte-for-byte unchanged:

- `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- `virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- `virtual-device-native-alias-closure-reverse-v2.md`, SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- `virtual-device-native-alias-closure-objdump-width-reverse-v1.md`, SHA-256 `D01912DA14DC524E320CDEE8C8BAF5456158CEB25AFB4C2C3A296FA01B36F188`.
- The independent split rule-hit diagnostic named above.
- Public wrapper, SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner, SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Compressed negative module, SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`, exactly two test methods.
- Fixed PE Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump` SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

No manifest, REVERSE document, diagnostic, wrapper, runner, negative test, production DLL, offline lifecycle source, build script, earlier result, or earlier verification output may be edited.

## 3. Sealed independent diagnosis

The completed fixed-PE reconstruction established two distinct accounting domains:

| Counter domain | directPositiveSlotLoadThenRegisterCall | localRegisterCopy | spillReload | slotAddressLeaOrAddThenMemoryCall | indexedSibCallOperand |
|---|---:|---:|---:|---:|---:|
| Intermediate state-machine events | 270 | 53 | 14 | 0 | 0 |
| Final candidate contributions | 270 | 0 | 0 | 0 | 0 |

The 53 register-copy and 14 spill-reload events are intermediate tracked transitions whose state is later killed, abandoned, or never reaches an admitted indirect call. They are not members of the final split-candidate contribution set.

The final populations remain exact:

- reconstructed split-register candidates: `270`;
- final recognition `SLOT_TARGET_REGISTER`: `270`;
- final recognition `SLOT_ADDRESS_MEMORY`: `0`;
- final recognition `INDEXED_SIB_SLOT_OPERAND`: `0`;
- every final candidate has `localTransferCount=0`;
- base-only candidates: `489`;
- core projection: `759` (`489 + 270`).

Thus the manifest's `equivalentDispatchOracle.observedRuleHits={270,0,0,0,0}` describes final-candidate contributions. The verifier currently compares it with intermediate transition diagnostics `{270,53,14,0,0}`. This is an accounting-domain mismatch, not a missing candidate, changed instruction, or alias-census discrepancy.

## 4. Exact verifier repair budget

The only authorized verifier logic change is to separate intermediate transition diagnostics from the contractual final-candidate contribution counters:

1. The state-machine scan may retain the observed `53` register-copy and `14` spill-reload events in a separately named local diagnostic accumulator, but that accumulator must not be compared with manifest `equivalentDispatchOracle.observedRuleHits`.
2. Only after `reconstructed_split` is complete, derive the contractual five-field `observedRuleHits` from each final candidate's independently reconstructed recognition class, producer chain, and local-transfer count.
3. Count `directPositiveSlotLoadThenRegisterCall` for a final direct positive-slot-load-to-register candidate; count `localRegisterCopy` or `spillReload` only when that corresponding transition appears in a producer chain that actually reaches a final candidate; count `slotAddressLeaOrAddThenMemoryCall` only for a final slot-address memory-call candidate; count `indexedSibCallOperand` only for a final indexed/SIB call candidate.
4. For the fixed PE, the derived contractual value must be exactly `{directPositiveSlotLoadThenRegisterCall:270, localRegisterCopy:0, spillReload:0, slotAddressLeaOrAddThenMemoryCall:0, indexedSibCallOperand:0}`.

The verifier must still reconstruct and compare all 270 candidate rows one-to-one, including canonical order, RVA, raw bytes, normalized independent decode, operand, recognition class, producer chain, producer/call registers, local-transfer count, classification, receiver disposition, target set, callee disposition, proof, anchors, range, and hash reference. The 489 base rows, 759 core projection, 34,430 residual rows, and exact base/split/residual partition must remain independently checked.

No manifest edit, counter constant substitution, count-only closure, candidate deletion, intermediate-state suppression, partition relaxation, graph reuse, broad exception handling, or acceptance bypass is authorized. The v8 instruction-span mode, v9 `objdump -w`, v10 trampoline selector, and v11 split-decode normalization must remain unchanged.

## 5. Main-integration and exact binding budget

After this preseal is hashed, the main integrator may:

- implement only the accounting-domain separation and final-candidate derivation in Section 4;
- update the decision's verifier SHA-256 to the repaired verifier hash;
- add exactly three lineage bindings for the v11 failure result, independent split diagnostic, and this v12 preseal;
- mechanically update the decision binding count/order and resulting decision SHA-256.

The current 34 bindings retain their existing relative order. Insert these three rows immediately after the existing v11-preseal row and before the immutable manifest-v1 row:

1. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v11.md`, SHA-256 `39425C9DC84C4CAE092CE6BA34B400E0C6222CC4A1D89EF592EA6661E9A03F13`, role `V5_RETRY_V11_FAILED_TEST_RESULT`;
2. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-split-rule-hit-reverse-v1.md`, SHA-256 `172BD3D0CE2418DB9183AC97B21B5C085C9D74BA5B8E79A7D5D3C22EFF8EF090`, role `INDEPENDENT_SPLIT_RULE_HIT_DIAGNOSTIC`;
3. `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v12.md`, SHA-256 equal to this create-new preseal's final disk hash, role `TEST_RETRY_PRESEAL_V12`.

The final decision and verifier binding specification must each contain exactly 37 rows in that order. Manifest v2 remains the active alias evidence and is not rewritten merely to bind this retry.

Before formal execution, independent TEST must perform a read-only static review proving that the verifier diff is exactly the Section 4 accounting separation plus the three binding constants/specifications, the decision diff is purely mechanical, all 37 paths/roles/disk hashes and their order match, candidate-level and residual-partition checks remain present, and in-memory removal of only the v12 changes restores the v11 verifier and decision hashes. The review must not run, import, or compile the verifier and must not invoke wrapper, runner, tests, or `objdump`; it does not consume a formal group.

## 6. Acceptance semantics remain unchanged

- Manifest identity remains `IsaacCsT0-NativeAliasUniverse-v2`; the evidence verdict remains `UNKNOWN`.
- Receiver dispatch and returned `{object,control}` pair transfer remain separate fixed points.
- Universe completeness remains separate from drainability and native owner through drain.
- `dynamicAliasUniverseClosed=false`, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE` remain mandatory.
- `VIRTUAL_DEVICE_OBJECT_OWNER=UNKNOWN`; no promotion is authorized.
- Factory, rollback, reference-count, final-release, and thread conclusions remain inherited unchanged.
- Every CS-T0-05-06/T0-05/T0-06 unlock and every implementation/runtime/scheduler/transport/mutation permission remains false.

## 7. Distinct v12 publication paths

The only permitted create-new formal output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v11.json`

The v12 TEST result path is:

`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v12.md`

Both paths must be absent before Group 1. Every earlier retry output remains retired and must not be created or reused.

## 8. Exactly four v12 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the unchanged internal runner once with the fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, manifest v2 identity, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, the exact first unresolved edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged public wrapper once with fixed PE, bound `objdump`, repository root, and retry-v11 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper once without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time, and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the single unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops all later groups. No invocation may be repeated for diagnosis or confirmation under this seal.

## 9. Result record and prohibitions

After the attempt, create `virtual-device-native-alias-closure-test-results-v12.md` with create-new semantics. On success record all four exact commands, exit codes/summaries, static-review disposition, all 37 bindings, manifest/REVERSE/diagnostic/verifier/decision/output hashes, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, lineage, and omitted work. On failure record the consumed group and mark every later group `NOT_EXECUTED`.

Do not run the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not invoke standalone `objdump`; it may be used only through the four sealed verifier entry points.

## 10. Pre-execution state

At creation of this preseal no v12 formal group has run. The retry-v11 JSON and v12 TEST result are absent. The only formal executions on record remain the eleven consumed Group 1 attempts documented by results v1-v11.
