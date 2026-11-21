> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST results v3

## 1. Identity and lineage

- Role: independent `TEST`.
- Scope: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v3.md`, SHA-256 `A965DDA601F60A92B5EBBFEED7C832BDEEF6056BDA7002FA21187233E6EB8B72`.
- Prior preseal v2: SHA-256 `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757`.
- Prior preseal v1: SHA-256 `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342`.
- Prior failure results: v1 SHA-256 `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C`; v2 SHA-256 `BB46E7A8BDD54063A80D4717FDB669DAB8FF45A6AE6E0D343A42CFCB115DAA03`.
- Verifier used by this attempt: SHA-256 `501F8D39F74FC081495977E8EEB1C93BFB841AF85095245C9A7E29EC864C955C`.
- Decision used by this attempt: SHA-256 `ACCD3407114C9423BB648FF961C27AC45734D79F0900BDA3E6182333B4CF204B`.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. v3 execution result

Exactly one v3 group was invoked:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | The internal runner reported verifier `exitCode=2` with `FUNCTION_RANGE_CENSUS_ORDER:receiver-frontier-function-00047180`. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root .
```

Observed runner output:

```json
{"error":"FUNCTION_RANGE_CENSUS_ORDER:receiver-frontier-function-00047180","exitCode":2,"outputWritten":false,"schemaVersion":1,"scope":"NATIVE_ALIAS_CLOSURE_ONLY","verificationIdentity":"IsaacCsT0-NativeAliasUniverseOracleVerification-v1","verificationStatus":"REJECTED"}
```

The public process exit code was `1` because the internal runner maps a rejected oracle to failure.

## 3. Exact failure diagnosis

The manifest's sealed `setSemantics.canonicalOrder.functionRanges` is an append-batch order: inherited core roles, then `indirect-census-function-*` ranges, then `receiver-frontier-function-*` ranges. The verifier instead treated every row after the core prefix as a single globally RVA-sorted `indirect-census-function-*` segment.

The first legal receiver-frontier-only row is array index `8903`, id `receiver-frontier-function-00047180`, RVA `0x00047180`; the preceding final census row is `indirect-census-function-007170c0`. The transition deliberately begins a new batch and restarts RVA order. The verifier therefore rejected a manifest row that matches the sealed canonical ordering.

This result records the defect only. It does not authorize a repair or alter the manifest.

## 4. Publication and fail-closed boundary

Immediately after Group 1 failed:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v2.json`: `ABSENT`.
- `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v3.md`: `ABSENT` before this append-only failure report was created.

No verification JSON was created. The v3 Group 1 invocation is consumed and must not be rerun or resumed.

- Overall v3 TEST result: `FAIL`.
- Native evidence verdict remains `UNKNOWN`.
- `dynamicAliasUniverseClosed`: not accepted by TEST.
- `formalDecision`: remains `null`.
- `workPackageStatus`: remains `BLOCKED_BY_EVIDENCE`.
- `VIRTUAL_DEVICE_OBJECT_OWNER`: not promoted.
- CS-T0-05-06 and all runtime/implementation permissions remain closed.

## 5. Explicitly not run

Groups 2–4, the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, debugger, process probe, IPC, hooks, injection, input mutation, and world mutation were not run.
