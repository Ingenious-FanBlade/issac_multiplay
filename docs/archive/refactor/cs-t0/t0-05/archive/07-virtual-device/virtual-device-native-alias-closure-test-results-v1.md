> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure TEST results v1

## 1. Identity and scope

- Role: independent `TEST`.
- Work package: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v1.md`, SHA-256 `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342`.
- Verifier at the failed run: `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v5/verify_virtual_device_ownership_lifecycle_decision_v5.py`, SHA-256 `87F67B4DAE37B5A445E7C21634D6395D01F02651D8812930F415FF4EBA1007BF`.
- Decision at the failed run: `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json`, SHA-256 `07570DD050586DED75243521C0902BB568063EE14A279F8E8D6E288AFD208C62`.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. Sealed execution result

Exactly one acceptance group was attempted under the v1 preseal:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | Importing the verifier failed before oracle execution with `SyntaxError: unmatched ')'` at verifier line 931. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root .
```

Observed failure:

```text
File "...\verify_virtual_device_ownership_lifecycle_decision_v5.py", line 931
    "independently compact-profile-bound")):
                                          ^
SyntaxError: unmatched ')'
```

The syntax defect is in the long `schemaAdditions` conditional: its final comparison ends in `)):` although that conditional has no matching outer opening parenthesis. This result records the failure only; it does not authorize or apply a repair.

## 3. Publication and mutation boundary

After the failure, both intended publication paths were confirmed absent:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5.json`: `ABSENT`.
- `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v1.md`: `ABSENT` before this append-only failure report was created.

No verification JSON was written. Group 1 did not reach the alias oracle, so this execution produces no positive alias verdict and does not change the sealed manifest verdict `UNKNOWN`. Groups 2–4 remain unconsumed only in a separately sealed retry; they must not be appended to or resumed under the v1 preseal.

## 4. Fail-closed disposition

- Overall v1 TEST result: `FAIL`.
- `formalDecision`: remains `null`.
- `workPackageStatus`: remains `BLOCKED_BY_EVIDENCE`.
- `dynamicAliasUniverseClosed`: not accepted by TEST.
- `VIRTUAL_DEVICE_OBJECT_OWNER`: not promoted.
- CS-T0-05-06 and all runtime/implementation permissions remain closed.
- The v1 four-group sequence must not be rerun.
- Any retry requires a new TEST preseal, newly frozen hashes, and a distinct create-new output path.

## 5. Explicitly not run

The following were not run: Groups 2–4, the v4 offline lifecycle harness, Cargo tests, the full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, the game, a debugger, process probes, IPC, hooks, injection, input mutation, and world mutation.
