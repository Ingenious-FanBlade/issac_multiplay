> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure objdump-width retry TEST results v9

## 1. Identity and frozen evidence

- Role: independent `TEST`.
- Scope: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v9.md`, SHA-256 `53D72424544636036CA03D874A2A4AECF1BC4C3F2C687C4307DF4E8886A244B4`.
- Failure result v8: SHA-256 `784BBDC398A9A442E0106F74A4F1909865956D7B55A7BE280AC91DB71B726005`.
- Independent objdump-width diagnostic: SHA-256 `D01912DA14DC524E320CDEE8C8BAF5456158CEB25AFB4C2C3A296FA01B36F188`.
- Immutable manifest v1: SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- Immutable REVERSE v1 handoff: SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Active manifest v2: `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- Active REVERSE v2 handoff: SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- Verifier used by this attempt: SHA-256 `E30A7A17F5328C06A551C9E3E1E219FBD1BE634099FCED29964F1DBDF962439A`.
- Decision used by this attempt: SHA-256 `15C609C69E03EB2D5CE77626E9BAA4CAC18BB1184CB26BD668E927685DC061CE`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Unchanged two-method negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump`: SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

All v1-v8 plan/result lineage remains immutable and consumed. No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. Mandatory static review

The read-only pre-execution review passed and authorized v9 formal execution:

- the verifier change was limited to adding `-w` to the existing `run_objdump` disassembly argument list and three lineage constants/specifications;
- parser, regex, instruction-span handling, PE checks, census/order rules, and every evidence-validation rule remained unchanged;
- in-memory removal of only the v9 changes reproduced verifier SHA-256 `FBC831EF48814BF6F7A9BE2F61D0814C480F7535B77A796F67D210855AE74B51` exactly;
- the decision contained exactly 30 ordered bindings and all 30 bound disk hashes matched;
- in-memory removal of the three lineage rows and verifier-hash propagation reproduced decision SHA-256 `EDB3566785E6D02470BEFE7CC2FF47B857C34016EA60FA7176A78F7AD66C4DF3` exactly;
- `UNKNOWN`, no promotion, false permissions/unlocks, and absent retry-v8 output/result state were retained.

The static review did not run, import, or compile the verifier and did not invoke the wrapper, runner, tests, or `objdump`.

## 3. v9 execution result

Exactly one v9 acceptance group was invoked:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `FAIL` | `1` | Internal runner returned verifier `exitCode=2`, `verificationStatus=REJECTED`, error `REBUILT_THREAD_TRAMPOLINE_FLOW`, and `outputWritten=false`. |
| 2. Public CLI create-new | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast. |
| 3. Public CLI verify-existing | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast; no before/after path, length, UTC-time, or SHA-256 measurement was taken. |
| 4. Parameterized negative batch | `NOT_EXECUTED` | — | Suppressed by Group 1 fail-fast; the unchanged two-method module was not invoked. |

The single Group 1 command was:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root "D:\projects\isaac_hook_reserach"
```

Observed runner output:

```json
{"error":"REBUILT_THREAD_TRAMPOLINE_FLOW","exitCode":2,"outputWritten":false,"schemaVersion":1,"scope":"NATIVE_ALIAS_CLOSURE_ONLY","verificationIdentity":"IsaacCsT0-NativeAliasUniverseOracleVerification-v1","verificationStatus":"REJECTED"}
```

The outer runner process returned exit code `1`. This invocation is consumed and must not be rerun or resumed under preseal v9.

## 4. Exact failure and publication boundary

The oracle rejected the reconstructed thread-trampoline flow before publication. This report records the observed contract failure only; it does not diagnose, reinterpret, or repair the trampoline evidence and grants no further retry authority.

Immediately after Group 1 failed:

- `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v8.json`: `ABSENT`.
- `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v9.md`: `ABSENT` before this create-new report was written.

No verification JSON was written and there is no Group 2 output hash or Group 3 immutability result.

## 5. Conservative decision remains closed

- Overall v9 TEST result: `FAIL`.
- Manifest evidence verdict remains `UNKNOWN`; TEST did not accept alias-universe closure.
- The manifest's first unresolved edge remains RVA `0x00024037` in function RVA `0x00023CC0`, edge kind `POST_RELEASE_INDIRECT_CALLBACK_WITH_PAIR_ADDRESS`.
- `dynamicAliasUniverseClosed=false`.
- `formalDecision=null`.
- `workPackageStatus=BLOCKED_BY_EVIDENCE`.
- `VIRTUAL_DEVICE_OBJECT_OWNER=UNKNOWN`; no promotion occurred.
- All 30 artifact bindings remain evidence inputs only; they do not constitute closure.
- CS-T0-05-06, T0-05, T0-06, virtual-device creation, implementation, runtime, scheduler, transport, and mutation permissions remain closed.

## 6. Explicitly not run

Groups 2-4, the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, debugger, process probe, canary, IPC, hooks, injection, input mutation, and world mutation were not run. No standalone `objdump` invocation was made.
