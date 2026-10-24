> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device lifecycle offline group-1 retry — independent TEST preseal v3

Date: 2026-09-08  
Role: independent TEST  
Phase: superseding retry seal / environment probe not executed  
Scope: one environment-gated offline group-1 retry only

## 1. Why retry preseal v2 is non-executable

Retry preseal v2 required its future harness report to bind the retry seal, but
the runner frozen by v2 emitted only the original TEST preseal binding. It could
not emit a `TEST_RETRY_PRESEAL` binding, so no report produced by that frozen
runner could satisfy v2.

No environment probe, build, harness, or retry command was executed under v2.
The defect was found before execution. The runner was then narrowly patched so
that, when and only when the normalized output leaf is
`virtual-device-lifecycle-offline-harness-20260908-v2.json`, it reads and emits a
binding for this v3 retry preseal. The patch changes the runner digest, which is
an additional reason v2's exact input set can no longer authorize execution.

Retry preseal v2 is therefore `NON_EXECUTABLE_SUPERSEDED`. It remains immutable
evidence and is bound below; it is not a failed test and must not be used to
authorize any probe or retry.

## 2. Exact frozen inputs

| Role | Path | Required SHA-256 |
|---|---|---|
| original TEST preseal | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-plan-v1.md` | `4B503EC14184A75BAE8F92BD78BEA173557E3652347744EC6BCAD99D41551B39` |
| original TEST failure report | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-results-v1.md` | `CD8CB7FEA99A9F891DD311F408F09C8A6EF5D3629C7AA42E7D962132D4E5F969` |
| superseded retry preseal v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v2.md` | `D3CA7B59B94EF61519F0B0F41045C6872D1488014D0E6BE51B7CB89ACBFAB31A` |
| lifecycle design contract | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md` | `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665` |
| independent REVERSE v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md` | `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42` |
| offline header | `cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp` | `0BBBA3E723079BEE2971C1A5DED6F721087DFEC1CC59B4C8064B0206914F2283` |
| offline implementation | `cs-t0/native/src/virtual_device_lifecycle.cpp` | `5091E40B532099FCA54C0B9264A582B7E7B7BC952941B577C096A7D95B177562` |
| offline harness | `cs-t0/native/tests/virtual_device_lifecycle_main.cpp` | `0D1E42E826A367118A6DE63C811317059063AD330459F4D63B4B193A7B6390D8` |
| dedicated Release/Win32 project | `cs-t0/native/IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj` | `A6B3560E79367EDC28B50D69CF06B1671AD4D0B27B4902A384C4835B281569DC` |
| patched offline runner | `cs-t0/test-virtual-device-lifecycle-offline.ps1` | `295341E7A73B3C8C9AD931BA12E9E160BA80AD49C561487850B3443C1DFB21B5` |

Every digest must be recomputed and match before the probe and again before the
retry. Any mismatch stops the sequence and requires a new independent review
and seal.

## 3. Runtime-computed v3 binding and no hash cycle

This preseal is intentionally not self-hashed inside the runner. The patched
runner freezes only this path:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v3.md`

For the reserved v2 output leaf, the runner reads that completed file at runtime,
computes its SHA-256, and appends exactly one binding with role
`TEST_RETRY_PRESEAL`. Thus:

1. this v3 preseal binds the already-complete patched runner by the digest above;
2. the runner contains the v3 path but no v3 digest;
3. the create-new report records the runtime-computed v3 digest;
4. the later v4 candidate binds the completed report and v3 digest.

No file's content depends on its own digest, so this ordering has no hash cycle.
The report must contain the original `TEST_PRESEAL`, the design contract, this
one `TEST_RETRY_PRESEAL`, all five offline file/entry bindings, the unchanged
production project/build bindings, and its executable hash.

## 4. Distinct create-new output

The sole reserved retry output remains:

`out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json`

It must be absent before execution and may be created only with
`FileMode::CreateNew`. All original allowed-root, exact `.json`, ADS, traversal,
reparse, pre-existing-output, and partial-write cleanup gates remain mandatory.
The original v1 output and all preseal/failure artifacts remain immutable.

## 5. Exact read-only environment probe

TEST may execute exactly this probe from the repository root, using only the
minimal escalated filesystem context required to read the SDK discovery path:

```powershell
pwsh -NoProfile -Command "$sdkDiscoveryPath='C:\Users\114514\AppData\Local\Microsoft SDKs'; $null=Get-Item -LiteralPath $sdkDiscoveryPath -Force -ErrorAction Stop; $null=Get-ChildItem -LiteralPath $sdkDiscoveryPath -Force -ErrorAction Stop | Select-Object -First 1; [pscustomobject]@{probeStatus='PASS'; path=$sdkDiscoveryPath; readOnly=$true} | ConvertTo-Json -Compress"
```

The probe may only resolve and enumerate the exact named directory. It must not
write, change ACLs or ownership, invoke MSBuild/a compiler/a test, discover a
game process, or access any game/runtime state. The gate passes only with exit
code `0` and `probeStatus=PASS` for the exact path. Any other result stops the
sequence; the retry is not executed.

## 6. Exactly one retry group-1 command

Only after the probe passes and all Section 2 hashes still match may TEST execute
this exact command once from the repository root:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json
```

The retry may use only the minimal escalated filesystem context necessary for
Visual Studio Windows SDK discovery. That context does not expand the work:
the runner may build only the dedicated `Release|Win32` Application, execute
only its offline harness, write its normal repository-local intermediates, and
create only the reserved report.

The retry remains offline. It must not start, enumerate, open, read, attach to,
inject into, hook, or mutate a game process; start the game; create IPC; load a
game DLL; send input; mutate world state; use transport/scheduler paths; or
request/consume active authorization.

This retry is one-shot. PASS or FAIL consumes it. It must not be automatically
or manually rerun under this seal. A failure is preserved and reported without
another attempt.

## 7. Acceptance and successor locks

All original group-1 requirements remain in force: exact four lifecycle APIs,
seven states, six strong-reference classes with total ceiling six, external weak
zero, move-only lease ownership, all thirteen failure/rollback cases, exact
operation logs and full snapshots, Release/Win32 isolation, create-new output,
and all-false runtime/process/game/IPC/hook/mutation facts.

Until the retry exits `0`, the v2 report exists, and its bindings—including the
runtime-computed digest of this v3 preseal—are independently verified:

- the v4 candidate must not be created;
- groups 2, 3, and 4 remain `NOT_RUN` and locked;
- no offline, native, or formal proof status may be promoted.

After a valid PASS, mainline may create the v4 candidate using the original
no-cycle ordering. Groups 2–4 still require their separate sealed TEST
authorization; this document does not authorize them.

## 8. Pre-execution disposition

At publication, retry preseal v2 is superseded, the v3 environment probe and
retry have not run, the v2 create-new report does not exist, and the original
`MSB4184` failure remains bound history.
