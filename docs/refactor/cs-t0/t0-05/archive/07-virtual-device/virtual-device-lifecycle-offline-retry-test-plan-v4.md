> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device lifecycle offline group-1 retry — independent TEST preseal v4

Date: 2026-09-08  
Role: independent TEST  
Phase: argument-safe environment probe and one gated offline group-1 retry  
Disposition: `READY_FOR_ARGUMENT_SAFE_PROBE`

## 1. Supersession and preserved history

Retry preseal v2 remains `NON_EXECUTABLE_SUPERSEDED`: its frozen runner could
not bind the applicable retry preseal. Retry preseal v3 corrected that cycle,
but its single authorized inline probe exited `1` at PowerShell parse time. The
outer PowerShell expanded the inner script variables, so no SDK-path read was
observed, no JSON was emitted, and no build, harness, or retry ran.

The user has explicitly authorized one newly sealed argument-safe probe and,
only after its exact PASS, one new group-1 retry. This v4 seal supersedes v3 for
that purpose. It does not erase or reinterpret either prior failure.

## 2. Exact frozen bindings

Every listed file must exist and its SHA-256 must match before the probe. The
same set must be checked again after a probe PASS and before the retry.

| Role | Path | Required SHA-256 |
|---|---|---|
| v3 ownership/lifecycle decision | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v3.json` | `6E069DD4A66D90A4122D4D1D189ED99C2CFA101357515CC6D2AEBC2A9F01E219` |
| v3 verification output | `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v3.json` | `B53896E691F3B5EAABA05CEEA9740BDB7D914D6BCF8EF12D525081F5B65964BB` |
| v3 independent TEST results | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-test-results-v2.md` | `461ACA521CFF74E843494D7924EFADCEAC639602E42C99A7BC9610EC09E9D03F` |
| lifecycle design contract | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md` | `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665` |
| independent REVERSE v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md` | `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42` |
| original offline TEST preseal | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-plan-v1.md` | `4B503EC14184A75BAE8F92BD78BEA173557E3652347744EC6BCAD99D41551B39` |
| original offline TEST failure report | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-results-v1.md` | `CD8CB7FEA99A9F891DD311F408F09C8A6EF5D3629C7AA42E7D962132D4E5F969` |
| non-executable retry preseal v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v2.md` | `D3CA7B59B94EF61519F0B0F41045C6872D1488014D0E6BE51B7CB89ACBFAB31A` |
| consumed retry preseal v3 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v3.md` | `29DB7B7F76EFC0B49BECA3B355C6201372E5A55C86AC4C45D964E0F21DAF706E` |
| parser-only probe failure report | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-probe-results-v1.md` | `F7CE7069621C84E074E3D90C60907A64DD994834D25D9450F7227C67B3AB5E02` |
| offline header | `cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp` | `0BBBA3E723079BEE2971C1A5DED6F721087DFEC1CC59B4C8064B0206914F2283` |
| offline implementation | `cs-t0/native/src/virtual_device_lifecycle.cpp` | `5091E40B532099FCA54C0B9264A582B7E7B7BC952941B577C096A7D95B177562` |
| offline harness | `cs-t0/native/tests/virtual_device_lifecycle_main.cpp` | `0D1E42E826A367118A6DE63C811317059063AD330459F4D63B4B193A7B6390D8` |
| dedicated Release/Win32 project | `cs-t0/native/IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj` | `A6B3560E79367EDC28B50D69CF06B1671AD4D0B27B4902A384C4835B281569DC` |
| v4-aware offline runner | `cs-t0/test-virtual-device-lifecycle-offline.ps1` | `144308C65DABD90403AED4964B5909814069CCF6A57EC27CCD981DD318016066` |
| argument-safe SDK probe | `cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` | `C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5` |
| unchanged production native project | `cs-t0/native/IsaacCsT0.Native.vcxproj` | `B5247DEEA646D91547BB93AB718B087CCBAD586A60C46D2C4EE7C8A9F66EDC2D` |
| unchanged complete-build entry | `cs-t0/build.ps1` | `158E08C46D53B441EF6728B281FE56D6DF539CD705C1F7229BDD7E93A790FEE4` |

Any mismatch stops the sequence. It cannot be repaired or accepted under this
seal.

## 3. Static acceptance of the two narrow changes

`cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` has no parameters. It
hardcodes only `C:\Users\114514\AppData\Local\Microsoft SDKs`, uses
`Get-Item` and `Get-ChildItem` for filesystem observation, rejects a non-folder,
and emits one compressed JSON document. It contains no write, ACL, ownership,
build, compiler, test, process, game, IPC, hook, injection, or mutation action.

The offline runner retains its existing interface and create-new controls. It
activates retry binding only when the normalized output leaf is exactly
`virtual-device-lifecycle-offline-harness-20260908-v2.json`; for that leaf it
reads
`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v4.md`
and emits its runtime SHA-256 with role `TEST_RETRY_PRESEAL`. The runner contains
the v4 path but no v4 digest, while this completed seal binds the runner digest
above. The future report can therefore bind this seal without a hash cycle.

## 4. Reserved create-new report

The sole retry output remains:

`out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json`

It was observed absent while this preseal was prepared. It must still be absent
immediately before execution. The runner's existing allowed-root, exact
`.json`, ADS, traversal, reparse-point, pre-existing-output, `FileMode.CreateNew`,
and partial-write cleanup rules remain mandatory. No other output name is
authorized.

## 5. Exactly one argument-safe environment probe

TEST may execute exactly once, from the repository root and with only the
minimal escalated filesystem context necessary to read the Windows SDK
discovery directory:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-sdk-probe.ps1
```

The probe is PASS only if all of the following hold:

- exit code is `0`;
- stdout is exactly one valid JSON document with no non-JSON text;
- the JSON has identity
  `IsaacCsT0-VirtualDeviceLifecycleSdkReadProbe-v1`;
- `probeStatus` is exactly `PASS`;
- `path` is exactly `C:\Users\114514\AppData\Local\Microsoft SDKs`;
- `readOnly` is exactly `true`;
- `sampledEntryCount` is an integer in `{0,1}`;
- the probe script hash still matches Section 2.

PASS establishes only read access to the named discovery path. It is not a
build, implementation, native, or formal proof. FAIL, parse error, access error,
schema drift, hash drift, or ambiguous output consumes this one probe and stops
the sequence. No automatic rerun is permitted.

## 6. Exactly one gated group-1 retry

Only after the exact Section 5 PASS, after rechecking every Section 2 hash, and
after confirming the reserved output remains absent, TEST may execute exactly
once from the repository root:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v2.json
```

The command may use only the minimal escalated filesystem context needed for
Visual Studio SDK discovery. It may build only the dedicated `Release|Win32`
Application, run only its offline harness, write its ordinary repository-local
intermediates, and create only the reserved report. PASS or FAIL consumes the
retry; it must not be rerun under this seal.

The retry remains subject to every group-1 criterion frozen by the original
preseal: exact API and seven-state lifecycle, six strong-reference classes with
total ceiling six, external weak zero, move-only leases, all thirteen
failure/rollback cases, exact operation logs and snapshots, dedicated-project
isolation, and all-false runtime/process/game/IPC/hook/mutation facts. A PASS
report must bind this v4 seal at runtime with role `TEST_RETRY_PRESEAL`, along
with the original preseal, contract, current offline files/runner, unchanged
production project/build, and executable digest.

## 7. Conservative locks

Until the group-1 retry exits `0` and the create-new v2 report and every binding
are independently verified:

- the v4 decision candidate must not be created;
- groups 2, 3, and 4 remain `NOT_RUN` and locked;
- no offline, native, or formal status may be promoted;
- no game may be started, enumerated, opened, read, attached to, injected into,
  hooked, or mutated;
- no IPC, input/world mutation, transport/scheduler work, or active
  authorization is permitted.

After a valid group-1 PASS, mainline may create the v4 candidate in the sealed
no-cycle order. This document does not authorize groups 2–4.

## 8. Pre-execution disposition

Static review of the argument-safe probe and v4-aware runner is complete with no
blocker. Neither command in Sections 5–6 has been executed under this seal. The
reserved v2 report is absent, and the disposition is
`READY_FOR_ARGUMENT_SAFE_PROBE`.
