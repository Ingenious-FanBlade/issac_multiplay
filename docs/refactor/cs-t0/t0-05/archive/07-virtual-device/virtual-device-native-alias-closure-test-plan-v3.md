> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST preseal v3

## 1. Retry lineage and scope

This append-only preseal authorizes one new retry of `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE`. It preserves both earlier failed attempts and does not authorize factory/rollback, final-release/thread work, production implementation, runtime access, or broader regression testing.

The complete failed TEST lineage is frozen as follows:

| Role | Path | SHA-256 |
|---|---|---|
| Original preseal v1 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v1.md` | `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342` |
| v1 failed verifier | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v5/verify_virtual_device_ownership_lifecycle_decision_v5.py` at v1 failure | `87F67B4DAE37B5A445E7C21634D6395D01F02651D8812930F415FF4EBA1007BF` |
| v1 failed decision | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json` at v1 failure | `07570DD050586DED75243521C0902BB568063EE14A279F8E8D6E288AFD208C62` |
| v1 failure result | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v1.md` | `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C` |
| Retry preseal v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v2.md` | `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757` |
| v2 failed verifier | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v5/verify_virtual_device_ownership_lifecycle_decision_v5.py` at v2 failure | `0506D66130B7E07B6954A35E7329511E45D0A47DBD7B366C1AC3BC32E9D660B4` |
| v2 failed decision | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json` at v2 failure | `36137960E9A36A86EDA5EAC68E6910603F4318420AA8F79394F3FF031D09C5F0` |
| v2 failure result | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v2.md` | `BB46E7A8BDD54063A80D4717FDB669DAB8FF45A6AE6E0D343A42CFCB115DAA03` |

Both failed Group 1 invocations are consumed permanently. Neither may be characterized as an oracle run or retried under its former seal.

## 2. Frozen evidence and unchanged executable surfaces

- Native alias manifest: `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- REVERSE handoff: `virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Parameterized negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`; it remains exactly two test methods.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump`: SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

## 3. Exhaustive v3 repair budget

The sole semantic/code repair authorized before v3 execution is to make the long `schemaAdditions` comparison one syntactically complete parenthesized condition:

1. At current verifier line 831, introduce the overall opening delimiter as `if (` and place the existing first comparison inside it without altering that comparison or any subsequent operand.
2. At the end of that same condition, restore the current terminal `):` to `)):` so the inner final string-grouping parenthesis and the new outer `if (` are both closed.

Equivalently, the condition must have this structure:

```python
if (
        additions["censusRowFunctionProvenanceRule"] != {
            # existing unchanged operands
        }
        # existing unchanged `or` chain
        or additions["domainSpecificCanonicalOrderRule"] != (
            # existing unchanged string
        )):
    raise ContractError("PROVENANCE_SCHEMA_ADDITIONS_DRIFT")
```

No comparison, string, schema key, manifest rule, graph reconstruction, count, profile, verdict, permission, path rule, runner, wrapper, or negative case may change.

Beyond that structural syntax fix, only mechanical lineage/hash propagation is authorized:

- bind this v3 preseal and the v2 failure result in the verifier and decision while retaining the complete v1 and v2 lineage;
- update the verifier self-binding in the decision to the repaired verifier SHA;
- update binding counts/order and the resulting decision SHA as mechanically required.

The manifest, REVERSE handoff, wrapper, runner, negative module, v1/v2 plans and results, all v1–v4 artifacts, and production/offline/build files remain immutable. The repaired verifier and propagated decision must be SHA-frozen and statically reviewed against this exact delta before any v3 group runs. Any other modification invalidates this preseal.

No test, verifier, Python import/compile, or `objdump` invocation is permitted during repair and re-freeze.

## 4. Acceptance semantics inherited unchanged

The complete substantive contract of v1, transitively bound through v2, remains in force:

- Manager receiver dispatch and returned `{object,control}` pair transfer are independent fixed points.
- Universe completeness remains distinct from alias drainability and native owner through drain.
- A legal evidence result is full closure or conservative `UNKNOWN` with the first exact unclosed edge.
- Counts, canonical hashes, candidate-authored prose, or self-reported booleans cannot replace independent disk graph/set/frontier reconstruction.
- `dynamicAliasUniverseClosed=true` requires both fixed points and all required zero sets; owner promotion additionally requires drainability and native-owner-through-drain.
- Factory, reference-count, final-release, and thread conclusions remain inherited from v4.
- `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, every permission false, and all CS-T0-05-06/T0-05/T0-06 unlocks false remain mandatory.
- The expected evidence verdict is conservative `UNKNOWN`; PASS means the decision faithfully publishes that evidence, not that implementation is allowed.

## 5. Distinct v3 publication paths

The v1 and retry-v1 JSON paths remain retired and absent. The only allowed create-new output for this seal is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v2.json`

The success report, created only if all four v3 groups pass, is:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v3.md`

Both paths must be `ABSENT` immediately before Group 1. Existing outputs and v1/v2 TEST reports are immutable.

## 6. Exactly four v3 groups, once each and fail-fast

After the repaired verifier and decision are frozen and statically approved, execute exactly these groups in order:

1. **Internal fixed-PE / anchor / independent alias oracle — one invocation.** Run the unchanged internal runner using the fixed PE, bound `objdump`, and repository root. Require exit `0`, `verificationStatus=PASS`, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, an exact first unclosed edge, `outputWritten=false`, and no repository publication.
2. **Public CLI create-new — one invocation.** Run the unchanged PowerShell wrapper with the fixed PE, bound `objdump`, repository root, and retry-v2 output path. Require exit `0`, `verificationStatus=PASS`, `outputWritten=true`, `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper without `-Output`. Require exit `0`, `verificationStatus=PASS`, `outputWritten=false`, and no change to repository/bound/generated evidence or the Group 2 output, measured by path, length, UTC time, and SHA-256 before and after.
4. **One parameterized negative-module invocation.** Run the unchanged unittest module once. It must expose only `test_semantic_mutations` and `test_write_boundaries`; all cases remain subtests over its one shared temporary fixture.

Any failure stops all later groups. No invocation may be repeated for diagnosis or confirmation under this v3 seal. A failure must be recorded append-only and requires another distinct seal and output for any future retry.

## 7. Successful v3 report requirements

Only after all four groups pass, create `virtual-device-native-alias-closure-test-results-v3.md` using create-new semantics. Record exact commands, exit codes and summaries; repaired verifier and decision hashes; retry-v2 JSON path/hash; all v1/v2 lineage; manifest/REVERSE hashes; Group 3 before/after immutability measurements; the two negative methods; `UNKNOWN` verdict and first edge; no promotion/unlock; and the omitted-test/runtime list.

## 8. Explicit prohibitions

Do not run or delete the v4 offline lifecycle harness, any v1–v4 suite/report, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, IDA, x32dbg, process enumeration/read, canary, IPC, hook, injection, input mutation, or world mutation.

Do not modify production DLL code, offline lifecycle C++, build scripts, manifest, REVERSE handoff, wrapper, runner, negative module, historical artifacts, or either earlier plan/result. Test compression applies only to this v3 sequence.

## 9. Pre-execution state

At creation of this preseal, no v3 group has run. The retry-v2 JSON and v3 TEST report are absent. The only executions on record are the two consumed Group 1 syntax failures documented by v1 and v2 results.
