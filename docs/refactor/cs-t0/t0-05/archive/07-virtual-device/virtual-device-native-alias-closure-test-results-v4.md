> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST results v4

## 1. Identity and frozen lineage

- Role: independent `TEST`.
- Scope: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v4.md`, SHA-256 `EE99F14C0FC2EACD970DDD8713B580ABE484E058520D3F9CD9789B459DBD0C26`.
- Earlier preseals: v1 `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342`; v2 `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757`; v3 `A965DDA601F60A92B5EBBFEED7C832BDEEF6056BDA7002FA21187233E6EB8B72`.
- Earlier failure results: v1 `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C`; v2 `BB46E7A8BDD54063A80D4717FDB669DAB8FF45A6AE6E0D343A42CFCB115DAA03`; v3 `1AECCC34D65A68ECCC39642B62C2A05B00C50638AEE9F040587F11A576D9624F`.
- Verifier used by this attempt: SHA-256 `E2F23C7D857B7834A6FFF5489F5F001871845FC428BD5943AC0460B415CAA7FE`.
- Decision used by this attempt: SHA-256 `52455293C5C05CA6B599D880EEA3047E0F3B3B082A206100ACB49E166882B4EC`.
- Native manifest remained SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- Fixed PE remained Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. v4 execution result

Exactly one v4 acceptance group was invoked:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | Internal runner returned verifier `exitCode=2`, `verificationStatus=REJECTED`, error `DISPATCH_CENSUS_SCOPE`. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root .
```

Observed runner output:

```json
{"error":"DISPATCH_CENSUS_SCOPE","exitCode":2,"outputWritten":false,"schemaVersion":1,"scope":"NATIVE_ALIAS_CLOSURE_ONLY","verificationIdentity":"IsaacCsT0-NativeAliasUniverseOracleVerification-v1","verificationStatus":"REJECTED"}
```

## 3. Exact failure diagnosis

The verifier compared `managerReceiverDispatch.censusScope` against an obsolete base-only description:

```text
.text whole-image decoded base-only indirect-call syntax call *0x28(%reg), with reg in EAX/ECX/EDX/EBX/ESI/EDI/EBP/ESP and any other objdump-emitted 32-bit base register
```

The final ECBB manifest instead seals this complete scope:

```text
Whole-.text independent receiver-dispatch census: 35,189 indirect calls partitioned into base-only, corrected bounded split, and explicit residual rows, plus a separate 3,810-row indirect-tail-transfer frontier; no syntax/count partition is asserted to close receiver or callee provenance.
```

The mismatch is verifier contract drift; it is not a manifest or native-evidence failure. This result records the failure only and does not authorize repair.

## 4. Publication and fail-closed boundary

Immediately after Group 1 failed:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v3.json`: `ABSENT`.
- `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v4.md`: `ABSENT` before this append-only failure report was created.

No verification JSON was written. The v4 Group 1 invocation is consumed and must not be rerun or resumed.

- Overall v4 TEST result: `FAIL`.
- Evidence verdict remains `UNKNOWN`.
- `dynamicAliasUniverseClosed`: not accepted by TEST.
- `formalDecision`: remains `null`.
- `workPackageStatus`: remains `BLOCKED_BY_EVIDENCE`.
- `VIRTUAL_DEVICE_OBJECT_OWNER`: not promoted.
- CS-T0-05-06 and every runtime/implementation permission remain closed.

## 5. Explicitly not run

Groups 2–4, the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, debugger, process probe, IPC, hooks, injection, input mutation, and world mutation were not run.
