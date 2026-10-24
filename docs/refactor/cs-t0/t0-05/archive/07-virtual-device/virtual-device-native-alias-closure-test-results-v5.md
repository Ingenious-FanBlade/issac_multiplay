> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST results v5

## 1. Identity and frozen lineage

- Role: independent `TEST`.
- Scope: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v5.md`, SHA-256 `E48633F4A81B2C4493A09AF982467B96511DECED3C8F58B87C688C31186D4BD2`.
- Earlier preseals: v1 `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342`; v2 `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757`; v3 `A965DDA601F60A92B5EBBFEED7C832BDEEF6056BDA7002FA21187233E6EB8B72`; v4 `EE99F14C0FC2EACD970DDD8713B580ABE484E058520D3F9CD9789B459DBD0C26`.
- Earlier failure results: v1 `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C`; v2 `BB46E7A8BDD54063A80D4717FDB669DAB8FF45A6AE6E0D343A42CFCB115DAA03`; v3 `1AECCC34D65A68ECCC39642B62C2A05B00C50638AEE9F040587F11A576D9624F`; v4 `14CE50755357D4B9189119371DEA5C008B4AD755453142A24A047EE41FF2E3C2`.
- Verifier used by this attempt: SHA-256 `17DDA23D7151E8DB4D0F0C2FC476548FFDB79316F807254451C0328E227393A9`.
- Decision used by this attempt: SHA-256 `AAA2828273356DA4D2A4932747EF15CED4BF682EF2028C1855BF5E4C220C6960`.
- Native manifest remained SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- REVERSE handoff remained SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Fixed PE remained Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. v5 execution result

Exactly one v5 acceptance group was invoked:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | Internal runner returned verifier `exitCode=2`, `verificationStatus=REJECTED`, error `RVA_FORMAT`, and `outputWritten=false`. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root .
```

Observed runner output:

```json
{"error":"RVA_FORMAT","exitCode":2,"outputWritten":false,"schemaVersion":1,"scope":"NATIVE_ALIAS_CLOSURE_ONLY","verificationIdentity":"IsaacCsT0-NativeAliasUniverseOracleVerification-v1","verificationStatus":"REJECTED"}
```

The outer runner process returned exit code `1`. The invocation is consumed and must not be rerun or resumed under preseal v5.

## 3. Exact failure boundary

The failure occurred before publication while validating a nullable candidate `functionRange`: the final manifest retains one base-dispatch row and two split-dispatch rows with `status=UNRESOLVED_IDA_CODE_REGION` and null `startRva`, `endRvaExclusive`, `size`, and `analysisLabel`, while the verifier helper attempted unconditional RVA conversion. This failure records verifier contract handling for an explicit unresolved evidence row; it does not convert the row into resolved evidence and does not authorize an implementation change under the consumed seal.

## 4. Publication and fail-closed boundary

Immediately after Group 1 failed:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v4.json`: `ABSENT`.
- `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v5.md`: `ABSENT` before this append-only failure report was created.

No verification JSON was written.

- Overall v5 TEST result: `FAIL`.
- Evidence verdict remains `UNKNOWN`.
- `dynamicAliasUniverseClosed`: not accepted by TEST.
- `formalDecision`: remains `null`.
- `workPackageStatus`: remains `BLOCKED_BY_EVIDENCE`.
- `VIRTUAL_DEVICE_OBJECT_OWNER`: not promoted.
- CS-T0-05-06 and every runtime/implementation permission remain closed.

## 5. Explicitly not run

Groups 2–4, the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, debugger, process probe, IPC, hooks, injection, input mutation, and world mutation were not run.
