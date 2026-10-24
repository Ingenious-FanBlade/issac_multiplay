> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure manifest-v2 retry TEST results v7

## 1. Identity and frozen lineage

- Role: independent `TEST`.
- Scope: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v7.md`, SHA-256 `C2EB81220638EC37E51A2BB11C15B8BBA9F173D41E310FD17BE9D61F69743B3F`.
- Immutable v1 manifest: `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- Immutable REVERSE v1 handoff: `virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Manifest v2: `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- REVERSE v2 handoff: `virtual-device-native-alias-closure-reverse-v2.md`, SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- Verifier used by this attempt: SHA-256 `429F3A84A159D96A463F68B0C8AF027DA25E8BBDFFB952D9FF5B67B65CDDA0FD`.
- Decision used by this attempt: SHA-256 `C616EA8F5E249158C59EBB1A88469D7CEE4B43B9607DE87A3CA2B09E30F0B176`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Unchanged negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump`: SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

The six earlier Group 1 failures and their v1-v6 plan/result lineage remain consumed and immutable as enumerated by the v7 preseal.

No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. Mandatory static review

The pre-execution read-only review passed and authorized formal execution. It established:

- the v1 manifest and REVERSE v1 hashes remained frozen;
- the v1-to-v2 residual diff changed exactly the sealed 21 IDs and only their `containingFunctionRange` and `functionHashRef` provenance fields;
- all 21 rows were conservatively downgraded to `UNRESOLVED_IDA_CODE_REGION`, while ten now-unreferenced function-range rows were removed;
- all 23 compact array profiles matched their declared counts, byte lengths, and SHA-256 values;
- 40,007 manifest range references and 8,941 unique function-range anchors passed the static containment, nullable-shape, and reference-consistency scan;
- the v2 manifest, REVERSE v2, verifier, decision, and 25 ordered decision bindings matched their frozen disk hashes.

This static review did not run, import, or compile the verifier and did not invoke the runner, wrapper, tests, or `objdump`.

## 3. v7 execution result

Exactly one v7 acceptance group was invoked:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | Internal runner returned verifier `exitCode=2`, `verificationStatus=REJECTED`, error `INSTRUCTION_ANCHOR:ANCHOR-CALLBACK-NO-ARGUMENT-READ_SPAN_DECODE_JOIN`, and `outputWritten=false`. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast; therefore no before/after immutability measurement was taken. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast; the unchanged two-method module was not invoked. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root "D:\projects\isaac_hook_reserach"
```

Observed runner output:

```json
{"error":"INSTRUCTION_ANCHOR:ANCHOR-CALLBACK-NO-ARGUMENT-READ_SPAN_DECODE_JOIN","exitCode":2,"outputWritten":false,"schemaVersion":1,"scope":"NATIVE_ALIAS_CLOSURE_ONLY","verificationIdentity":"IsaacCsT0-NativeAliasUniverseOracleVerification-v1","verificationStatus":"REJECTED"}
```

The outer runner process returned exit code `1`. The invocation is consumed and must not be rerun or resumed under preseal v7.

## 4. Exact failure boundary

The oracle rejected the decoded-span join for instruction anchor `ANCHOR-CALLBACK-NO-ARGUMENT-READ` before publication. No Group 2 output was created. This result records the observed contract failure only; it does not reinterpret or repair the anchor and does not authorize changes outside a new, explicitly sealed retry budget.

## 5. Publication and fail-closed boundary

Immediately after Group 1 failed:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v6.json`: `ABSENT`.
- `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v7.md`: `ABSENT` before this create-new failure report was written.

No verification JSON was written.

- Overall v7 TEST result: `FAIL`.
- Evidence verdict remains `UNKNOWN`; TEST did not accept alias-universe closure.
- `dynamicAliasUniverseClosed`: remains false/not accepted.
- `formalDecision`: remains `null`.
- `workPackageStatus`: remains `BLOCKED_BY_EVIDENCE`.
- `VIRTUAL_DEVICE_OBJECT_OWNER`: remains `UNKNOWN` and was not promoted.
- CS-T0-05-06, T0-05, T0-06, virtual-device creation, implementation, runtime, scheduler, transport, and mutation permissions remain closed.

## 6. Explicitly not run

Groups 2-4, the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, debugger, process probe, canary, IPC, hooks, injection, input mutation, and world mutation were not run. No standalone `objdump` invocation was made.
