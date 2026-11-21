> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure retry TEST results v2

## 1. Identity and lineage

- Role: independent `TEST`.
- Work package: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing retry preseal: `virtual-device-native-alias-closure-test-plan-v2.md`, SHA-256 `E8822CD967A8DB58191C174D2DA2A8E0901D5185FB7E163D5B9BFD8AB4AC2757`.
- Original preseal: `virtual-device-native-alias-closure-test-plan-v1.md`, SHA-256 `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342`.
- Prior failure result: `virtual-device-native-alias-closure-test-results-v1.md`, SHA-256 `7F86F4E487B5BF8761E917B0AE5633C496C89E71FFE5580A6F0D0DD2E2DBC06C`.
- Verifier used by this failed retry: SHA-256 `0506D66130B7E07B6954A35E7329511E45D0A47DBD7B366C1AC3BC32E9D660B4`.
- Decision used by this failed retry: SHA-256 `36137960E9A36A86EDA5EAC68E6910603F4318420AA8F79394F3FF031D09C5F0`.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. Retry v2 execution result

Exactly one retry-v2 acceptance group was attempted:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | Importing the verifier failed before oracle execution with `SyntaxError: invalid syntax` at line 846. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root .
```

Observed failure:

```text
File "...\verify_virtual_device_ownership_lifecycle_decision_v5.py", line 846
    "anchorRefs", "functionHashRef"] or
                                       ^
SyntaxError: invalid syntax
```

The retry-v2 repair removed the unmatched final parenthesis but did not add an outer opening parenthesis around the long `schemaAdditions` conditional. When the locally open list closes at line 846, the trailing `or` is no longer inside any implicit-continuation delimiter. The structurally correct repair requires an outer `if (` and the matching final close; this result records the failure but does not authorize that repair.

## 3. Publication boundary

Immediately after Group 1 failed, these paths were confirmed absent:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v1.json`: `ABSENT`.
- `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v2.md`: `ABSENT` before this append-only failure report was created.

No verification JSON was written. The oracle did not execute, so this run provides no positive evidence verdict. The sealed native manifest remains conservatively `UNKNOWN`; TEST did not accept alias closure or owner promotion.

## 4. Fail-closed disposition

- Overall retry-v2 TEST result: `FAIL`.
- `formalDecision`: remains `null`.
- `workPackageStatus`: remains `BLOCKED_BY_EVIDENCE`.
- `dynamicAliasUniverseClosed`: not accepted by TEST.
- `VIRTUAL_DEVICE_OBJECT_OWNER`: not promoted.
- CS-T0-05-06 and all implementation/runtime permissions remain closed.
- The retry-v2 four-group sequence must not be rerun or resumed.
- A further retry requires a new append-only TEST preseal, new frozen hashes, and a distinct create-new path.

## 5. Explicitly not run

Groups 2–4 were not run. The v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, debugger, process probe, IPC, hooks, injection, input mutation, and world mutation were also not run.
