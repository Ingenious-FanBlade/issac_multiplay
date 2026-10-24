> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 candidate — independent TEST seal v1

Date: 2026-09-08  
Role: independent TEST  
Phase: candidate-pre publication seal  
Disposition: `READY_FOR_V4_CANDIDATE_PUBLICATION`

## 1. Frozen review inputs

The independent TEST role completed static review before the v4 candidate
existed. The following files are now sealed for candidate publication and the
single groups 2–4 sequence:

| Role | Path | SHA-256 |
|---|---|---|
| PowerShell read-only entry | `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1` | `BD30ADBF1994F141932807F6044C1B2810B2DC92CED5FD225BC37B44161ED170` |
| Python disk-only verifier | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/verify_virtual_device_ownership_lifecycle_decision_v4.py` | `319B138A2215266531BB366227503419478C8F233A7CCB8C820FDE3508CCD180` |
| sealed parameterized negative batch | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/test_virtual_device_ownership_lifecycle_decision_v4.py` | `F31404BD8B44DFE5DBA184B89D720C37FA35EB29E25E405B1B698AD0FFA0A65D` |
| final group-1 v3 harness report | `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json` | `3FCAF96223EA34D929C9D222E7107CD4E0F4DCDC207586E3C7C1CBE31CF28A55` |
| final group-1 retry preseal | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v5.md` | `3D19A0B157F7A72669A953E410445A7ACB7630470DD07F14232DF164F37C37E3` |
| argument-safe SDK probe | `cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` | `C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5` |

At sealing time both the candidate
`cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` and canonical
verification output
`out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
were absent. Any change to a sealed file or digest before execution invalidates
this seal and stops the sequence.

## 2. Static acceptance

The wrapper only resolves Python and forwards `-GameExe`, `-RepositoryRoot`, and
optional `-Output` to the disk-only verifier. The verifier does not enumerate or
open a game process, create IPC, install a hook, load a game module, inject,
send input, or mutate runtime/world state.

Static review confirmed:

- exact frozen bindings for the final v3 group-1 report, v5 retry preseal, SDK
  probe, offline header/source/harness/project/runner, REVERSE v2, v3 decision
  and verification, production project, and build entry;
- create-new output resolution occurs before verification, only under
  `out/cs-t0` or `artifacts/cs-t0`, and publication uses exclusive creation with
  own-partial cleanup; verify-existing has no output destination;
- the target is the fixed Win32/x86 PE at
  `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`,
  SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`,
  machine `0x014C`;
- the target profile contains 49 exact anchors: 36 inherited anchors plus 13
  REVERSE-v2 anchors, with immutable identity, RVA, kind, bytes/pointer target,
  and proof role;
- all 16 harness cases have independent full-trace SHA-256 gates; all 13 failure
  identities also retain explicit terminal-snapshot and ordered rollback gates;
- lifecycle layout/source checks freeze the namespace, exact constructors and
  four public lifecycle APIs, `noexcept`, move-only leases, private admission,
  non-mutating `Snapshot()`, six strong classes/ceiling six, external weak zero,
  port-mediated release, production isolation, and forbidden runtime
  dependencies;
- the three proof rows are exactly
  `offlineImplementationStatus=OFFLINE_IMPLEMENTATION_CONFIRMED`,
  `nativeEvidenceStatus=UNKNOWN`, and `formalStatus=UNKNOWN`;
- `promotionCount=0`, six inherited static facts and ten retained unknown facts
  are exact; `formalDecision=null`, status remains
  `BLOCKED_BY_EVIDENCE`, and implementation eligibility remains false;
- all 18 permissions, all unlocks, and all runtime/process/game/IPC/hook/input/
  world mutation fields are present and false; the offline prototype remains
  `OFFLINE_NOT_LINKED_NOT_LOADED` and `virtualDeviceImplemented=false`.

The negative batch is one command over isolated temporary fixtures. It directly
covers PE hash/machine and anchor metadata; binding shape/role/hash; offline file
layout and production isolation; exact API/constructor/namespace/lease/source
contracts; every one of the 13 failure rows with identity, failure snapshot,
final snapshot, thread token, reference delta, and rollback-order mutations;
three-axis/design/candidate-as-native promotion attempts; necessity, permissions,
unlocks and missing false fields; create-new path/ADS/existing-output rules;
post-create write-failure cleanup; mandatory junction escape rejection;
verify-existing file-set stability; and forged-upgrade rejection before output.

No test or verifier was executed during this review. The batch's local temporary
files and junction are test fixtures only; they do not touch a game process or
runtime.

## 3. One-shot groups 2–4

Mainline may first create the v4 candidate exactly once and bind the sealed
wrapper, verifier, negative batch, final group-1 report, and all other exact
artifacts required by the verifier. TEST then executes only the following
sequence. Each group is allowed once; PASS or FAIL consumes that group, and any
failure stops later groups without a rerun.

### Group 2 — create-new production verification

Precondition: the candidate exists, every sealed binding matches, and the
canonical output is still absent.

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4.json
```

Require exit `0`, `verificationStatus=PASS`,
`workPackageStatus=BLOCKED_BY_EVIDENCE`, exact candidate and PE digests,
`anchorCount=49`, `newStaticAnchorCount=13`, three offline confirmations, zero
native/formal promotions, ten retained unknowns, and the complete conservative
boundary.

### Group 3 — verify-existing

Only after group 2 passes:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot .
```

Require exit `0`, the same substantive verification result and decision digest,
`outputWritten=false`, `outputPath=null`, and no created, removed, or changed
repository file.

### Group 4 — sealed parameterized negative batch

Only after group 3 passes:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v4\test_virtual_device_ownership_lifecycle_decision_v4.py
```

This is one batch invocation, not one process per mutation. Require exit `0`
with every negative subcase rejected before requested output publication and all
temporary fixtures cleaned up.

## 4. Locked omissions and stop rules

No Cargo suite, complete T0 build, production Native build, Debug/Release matrix,
historical regression, v1/v2/v3 verifier suite, duplicate self-test, game start,
process enumeration/open/read, IPC, hook, injection, input/world mutation, or
active authorization is permitted.

Do not run group 3 after a group-2 failure, or group 4 after a group-2/group-3
failure. Do not rerun any group under this seal. A failure is preserved and
reported; further work requires new explicit authority and a new TEST seal.
