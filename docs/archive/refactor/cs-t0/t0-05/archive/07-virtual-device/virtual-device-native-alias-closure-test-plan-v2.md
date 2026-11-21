> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST preseal v2

## 1. Retry authority and immutable lineage

This append-only preseal authorizes one fresh retry of the compressed `NATIVE-ALIAS-CLOSURE` acceptance sequence. It does not reopen the failed v1 sequence and does not broaden work into factory/rollback, final-release/thread, implementation, or runtime activity.

The retry is bound to these immutable inputs:

| Role | Path | SHA-256 |
|---|---|---|
| Original TEST preseal | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v1.md` | `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342` |
| Verifier used by failed Group 1 | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v5/verify_virtual_device_ownership_lifecycle_decision_v5.py` | `87F67B4DAE37B5A445E7C21634D6395D01F02651D8812930F415FF4EBA1007BF` |
| Decision frozen for failed Group 1 | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json` | `07570DD050586DED75243521C0902BB568063EE14A279F8E8D6E288AFD208C62` |
| Failed TEST result | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v1.md` | `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C` |

The native evidence remains frozen:

- `cs-t0/native-alias-universe-v1.json`: SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-reverse-v1.md`: SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Parameterized negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57` and exactly two test methods.
- Fixed PE remains Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

## 2. Exhaustive repair budget

Only the following implementation delta is authorized before retry execution:

1. In the failed verifier at line 931, change the final token sequence from `)):` to `):`, removing exactly the unmatched closing parenthesis.
2. Apply only the mechanical SHA/binding propagation required by that one-byte verifier correction and by this v2 retry preseal. This may update the verifier's frozen retry-preseal/result lineage constants and the v5 decision artifact bindings/hashes, but it must not change any evidence semantics, graph reconstruction, profile, count, verdict, promotion rule, permission, CLI behavior, test case, wrapper, runner, manifest, REVERSE handoff, or historical v1–v4 artifact.

The repaired verifier and propagated decision must be newly SHA-frozen before Group 1. The decision must bind this v2 preseal and the v1 failure result without replacing or weakening the original v1 lineage. Any non-mechanical code or evidence change invalidates this preseal and requires another append-only TEST plan.

No test, verifier, or `objdump` invocation is authorized during the repair/re-freeze phase. Static inspection and hashing are allowed.

## 3. Acceptance semantics retained from v1

All substantive acceptance conditions from the v1 preseal remain unchanged and are incorporated by its bound hash:

- Manager receiver dispatch and returned `{object,control}` pair transfer are separate fixed points.
- Universe completeness is separate from drainability and native-owner-through-drain.
- The legal alias result is either a complete fixed point or conservative `UNKNOWN` with the first exact unclosed edge.
- Candidate counts, self-reported booleans, and offline lifecycle evidence cannot substitute for graph/set reconstruction.
- `dynamicAliasUniverseClosed=true` is legal only when both fixed points close and every required omitted/duplicate/unclassified/unresolved set is empty.
- `VIRTUAL_DEVICE_OBJECT_OWNER` can be promoted only when universe completeness, pre-remove alias drainability, and native owner through drain are all proven.
- `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, all permissions false, and all CS-T0-05-06/T0-05/T0-06 unlocks false remain invariant.
- The sealed evidence verdict remains `UNKNOWN`; a PASS verifies faithful conservative publication, not implementation readiness.

## 4. Retry publication identities

The v1 create-new path remains absent and retired:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5.json`

The retry must use this distinct absent create-new path:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v1.json`

The final retry TEST report, created only after all four retry groups pass, is:

`docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v2.md`

Both retry paths must be `ABSENT` immediately before Group 1. The v1 failure report is immutable and must not be edited or replaced.

## 5. Exactly four retry groups, each once

After the repaired verifier and propagated decision are frozen, TEST executes exactly these four groups once and in order. Any failure immediately stops later groups; no group is rerun under this v2 preseal.

1. **Internal fixed-PE / anchor / independent alias oracle — one invocation.** Use the unchanged internal runner with the fixed PE, bound `objdump`, and repository root. Require exit `0`, `verificationStatus=PASS`, disk reconstruction of the complete receiver/pair sets and frontiers, alias verdict `UNKNOWN`, `dynamicAliasUniverseClosed=false`, and no output write.
2. **Public CLI create-new — one invocation.** Use the unchanged PowerShell wrapper with the fixed PE, bound `objdump`, repository root, and the retry output path above. Require exit `0`, `verificationStatus=PASS`, create-new publication, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE`.
3. **Public CLI verify-existing — one invocation.** Invoke the same wrapper without `-Output`. Require exit `0`, `verificationStatus=PASS`, `outputWritten=false`, and no repository/bound/generated evidence change, measured using path, length, UTC write time, and SHA-256 before and after. The Group 2 output must remain byte-for-byte unchanged.
4. **Single parameterized negative module — one invocation.** Execute the unchanged module once. It must expose only `test_semantic_mutations` and `test_write_boundaries`; all cases remain subtests sharing the one temporary fixture.

No second invocation is permitted for diagnosis or confirmation. If a group fails, record its exact exit code/error in an append-only retry failure result; do not resume the sequence.

## 6. Final retry report requirements

Only if all four groups pass, create `virtual-device-native-alias-closure-test-results-v2.md` using create-new semantics. Record:

- all four exact commands, each exit code, and concise result summary;
- repaired verifier and propagated decision SHA-256 values;
- retry verification output path and SHA-256;
- v1 plan/result lineage plus manifest/REVERSE hashes;
- `UNKNOWN` alias verdict, `dynamicAliasUniverseClosed=false`, precise first unclosed edge, and no promotion/unlock;
- confirmation that Group 3 changed neither repository evidence nor Group 2 output;
- the explicit omitted-test/runtime list below.

## 7. Explicit prohibitions

Do not run or delete the v4 offline lifecycle harness, any v1–v4 test suite/report, Cargo tests, the full T0 build, Debug/Release production Native matrices, historical broad regression, duplicate self-test, the game, IDA, x32dbg, a process probe, passive/active canary, IPC, hook, injection, input mutation, or world mutation.

Do not edit production DLL code, offline lifecycle C++, build scripts, the wrapper, internal runner, negative module, native alias manifest, REVERSE handoff, v1 preseal, v1 failure result, or any v1–v4 artifact. Test compression applies only to this new retry sequence.

## 8. Pre-execution state

At creation of this v2 preseal, no retry group has run. The retry verification JSON and v2 TEST result are absent. The only recorded execution is the single failed Group 1 under v1, and it must never be represented as a successful or completed oracle run.
