> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device lifecycle offline group-1 retry — independent TEST preseal v2

Date: 2026-09-08  
Role: independent TEST  
Phase: retry authorized by user / retry not executed  
Scope: one environment-gated offline group-1 retry only

## 1. Purpose and supersession boundary

The original sealed group-1 run was consumed and failed during Visual Studio
Windows SDK discovery before compilation or harness execution. The user has now
explicitly authorized continued work. This document does not erase, overwrite,
or reclassify that failure. It creates one new run identity with a distinct
create-new output.

This preseal authorizes no build or test by itself. The retry becomes executable
only after the exact read-only environment probe in Section 4 succeeds. A probe
failure consumes no implementation evidence and forbids the retry command.

## 2. Exact hash bindings

The following inputs are frozen before the environment probe and retry:

| Role | Path | Required SHA-256 |
|---|---|---|
| prior TEST preseal | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-plan-v1.md` | `4B503EC14184A75BAE8F92BD78BEA173557E3652347744EC6BCAD99D41551B39` |
| prior TEST failure report | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-results-v1.md` | `CD8CB7FEA99A9F891DD311F408F09C8A6EF5D3629C7AA42E7D962132D4E5F969` |
| lifecycle design contract | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md` | `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665` |
| independent REVERSE v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md` | `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42` |
| offline header | `cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp` | `0BBBA3E723079BEE2971C1A5DED6F721087DFEC1CC59B4C8064B0206914F2283` |
| offline implementation | `cs-t0/native/src/virtual_device_lifecycle.cpp` | `5091E40B532099FCA54C0B9264A582B7E7B7BC952941B577C096A7D95B177562` |
| offline harness | `cs-t0/native/tests/virtual_device_lifecycle_main.cpp` | `0D1E42E826A367118A6DE63C811317059063AD330459F4D63B4B193A7B6390D8` |
| dedicated project | `cs-t0/native/IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj` | `A6B3560E79367EDC28B50D69CF06B1671AD4D0B27B4902A384C4835B281569DC` |
| offline runner | `cs-t0/test-virtual-device-lifecycle-offline.ps1` | `794045BCE4DEDB5A97A2F319ABBE725ABD3BEC1FC9122C3E761B6AA7B9094A85` |

Every digest must be recomputed and match before the retry. This retry preseal
must also be bound by its final SHA-256 in the eventual retry result and v4
candidate. Any input drift requires a new review and a new preseal; it cannot be
accepted by editing or weakening this one.

## 3. Distinct create-new output

The only reserved retry output is:

`out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json`

It must be absent before the retry and is create-new only. The runner's sealed
allowed-root, exact `.json`, traversal/ADS/reparse, pre-existing target, and
failure-cleanup rules remain mandatory. The prior v1 output name and prior TEST
failure report must not be reused, overwritten, deleted, or rewritten.

## 4. Exact read-only environment gate

TEST may run exactly this read-only probe from the repository root:

```powershell
pwsh -NoProfile -Command "$sdkDiscoveryPath='C:\Users\114514\AppData\Local\Microsoft SDKs'; $null=Get-Item -LiteralPath $sdkDiscoveryPath -Force -ErrorAction Stop; $null=Get-ChildItem -LiteralPath $sdkDiscoveryPath -Force -ErrorAction Stop | Select-Object -First 1; [pscustomobject]@{probeStatus='PASS'; path=$sdkDiscoveryPath; readOnly=$true} | ConvertTo-Json -Compress"
```

The probe is read-only: it may resolve and enumerate only the named SDK
discovery directory. It must not create, modify, take ownership of, or change
ACLs on any file or directory. It must not invoke MSBuild, a compiler, a test, a
game, or process discovery.

The environment gate passes only when the command exits `0` and emits
`probeStatus=PASS` for the exact path. If access is denied, the path is missing,
or the probe otherwise exits nonzero, TEST stops. The group-1 retry must not be
attempted and groups 2–4 remain locked.

Because the original failure was caused by sandbox denial outside the workspace,
the probe may be executed with only the minimal escalated filesystem context
needed to read `C:\Users\114514\AppData\Local\Microsoft SDKs`. Approval must be
scoped to this exact read-only probe; it is not permission for a general
elevated shell or filesystem mutation.

## 5. One sealed group-1 retry

Only after Section 4 passes and all Section 2 hashes still match, TEST may run
exactly once from the repository root:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json
```

The retry must use the minimal escalated filesystem context required for Visual
Studio to read its SDK discovery path. The escalation does not broaden the
runner's scope: it may build only the dedicated
`IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj` `Release|Win32` Application and
execute only its offline harness. It may write only normal dedicated build
intermediates under the repository and the one create-new report above.

The retry remains fully offline. It must not start, enumerate, open, read,
attach to, inject into, hook, or mutate a game process. It must not start the
game, create IPC, send input, mutate world state, load a game DLL, use transport
or scheduler paths, or request/consume an active authorization.

This is exactly one retry. PASS or FAIL consumes it. There is no automatic or
implicit rerun. On failure, preserve the exact exit/output and stop. On PASS,
seal the v2 report digest before any successor work.

## 6. Acceptance and successor gates

The retry must still satisfy every group-1 requirement in the original preseal:
dedicated Release|Win32 isolation, exact API/state/reference contract, move-only
lease paths, all thirteen failure/rollback cases, exact logs and snapshots,
create-new output, and all-false runtime/process/game/IPC/hook/mutation facts.
Environment elevation is not implementation evidence and cannot convert a
failed or incomplete harness result into PASS.

Until the retry exits `0` and the v2 create-new report exists with a verified
digest:

- v4 candidate creation remains forbidden;
- group 2 remains `NOT_RUN` and locked;
- group 3 remains `NOT_RUN` and locked;
- group 4 remains `NOT_RUN` and locked;
- no offline, native, or formal proof status may be promoted.

After a valid retry PASS, mainline may create the v4 candidate according to the
original no-cycle ordering. TEST may then execute groups 2–4 only under their
separate sealed authorization. This retry preseal does not itself authorize any
of those groups.

## 7. Pre-execution disposition

At publication of this document, the environment probe and retry have not been
run, the v2 output does not exist, and groups 2–4 remain gated. The original
`MSB4184` failure remains part of the evidence chain.
