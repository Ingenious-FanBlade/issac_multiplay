> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 Group 2 retry — independent TEST seal v2

Date: 2026-09-08  
Role: independent TEST  
Phase: corrected-candidate pre-execution retry seal  
Disposition: `READY_FOR_SINGLE_GROUP2_RETRY`

## 1. Supersession and authority

The candidate-pre TEST seal v1 authorized one Group 2 execution. That execution
was consumed and failed because the candidate used prose rather than the frozen
`fixedGoal` enumeration. The independent failure report preserves that result,
and Groups 3–4 were not run.

The user then explicitly authorized fixing that failure and continuing. The
corrected candidate now contains the exact frozen value:

`ENDPOINT_B_MUST_REMAIN_OPERATIONAL_WITHOUT_A_SECOND_PHYSICAL_CONTROLLER_ON_AUTHORITY`

This seal does not erase or reinterpret the first failure. It grants one new
Group 2 create-new attempt against the corrected candidate. Group 3 and Group 4
remain conditional on every earlier group in this new sequence passing.

## 2. Frozen retry inputs

| Role | Path | SHA-256 |
|---|---|---|
| candidate-pre TEST seal v1 | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-candidate-test-seal-v1.md` | `E9FE56403A1D216A2C53D392B47DF73CEE663ECBFCDAC2AFACFE872A3CDFA861` |
| consumed Group 2 failure report | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-test-results-v1.md` | `2E1CBE840D88EACA758138E17099873D32E144CE7C87E3BC0384D92EE78D1B7B` |
| original candidate, historical digest preserved by the failure report | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `9C72C0ADA5440E4096DC93F90D6FC52197E3006005079D9932712043A6C3855A` |
| corrected current candidate | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `E787255F1ED276FC7C9D5350A9245CEB2BDED121029AD0DAA0A416EE6B6A352F` |
| PowerShell read-only entry | `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1` | `BD30ADBF1994F141932807F6044C1B2810B2DC92CED5FD225BC37B44161ED170` |
| Python disk-only verifier | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/verify_virtual_device_ownership_lifecycle_decision_v4.py` | `319B138A2215266531BB366227503419478C8F233A7CCB8C820FDE3508CCD180` |
| sealed parameterized negative batch | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/test_virtual_device_ownership_lifecycle_decision_v4.py` | `F31404BD8B44DFE5DBA184B89D720C37FA35EB29E25E405B1B698AD0FFA0A65D` |
| final Group 1 v3 harness report | `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json` | `3FCAF96223EA34D929C9D222E7107CD4E0F4DCDC207586E3C7C1CBE31CF28A55` |
| final Group 1 retry preseal | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v5.md` | `3D19A0B157F7A72669A953E410445A7ACB7630470DD07F14232DF164F37C37E3` |
| argument-safe SDK probe | `cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` | `C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5` |
| fixed Win32/x86 disk PE | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

Before this seal was written, read-only review confirmed that the corrected
candidate's 18 artifact bindings all match their current files. The wrapper,
verifier, negative batch, final harness report, retry preseal, and SDK probe
still match candidate-pre seal v1. Any digest drift before a group starts stops
the sequence.

## 3. Corrected-candidate static acceptance

The corrected candidate has SHA-256
`E787255F1ED276FC7C9D5350A9245CEB2BDED121029AD0DAA0A416EE6B6A352F`.
Static review found no new blocker:

- `fixedGoal` now equals the frozen enumeration and
  `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`;
- `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`,
  `implementationEligibility=false`, and
  `virtualDeviceImplemented=false` remain exact;
- the three proof rows remain offline-confirmed but native/formal `UNKNOWN`,
  with `promotionCount=0`, six inherited static facts, and ten retained unknown
  facts;
- the seven states, four APIs, six strong-reference classes, maximum strong
  count six, external weak count zero, and all 13 rollback IDs remain exact;
- all 18 permissions and every runtime/process/game/IPC/hook/input/world/unlock
  field remain present and false;
- the verifier profile remains frozen at 49 anchors: 36 inherited plus 13 new;
- the offline implementation remains `OFFLINE_NOT_LINKED_NOT_LOADED` and cannot
  be treated as native or formal evidence.

The canonical output
`out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
was absent at sealing time. Create-new publication is therefore eligible for
one retry, subject to the hash prechecks below.

## 4. One-shot retry sequence

Each group is consumed by either PASS or FAIL and must not be rerun. Before
each group, recheck every frozen digest applicable to that group and its
preconditions. A failure stops all later groups.

### Group 2 — one corrected-candidate create-new retry

Preconditions: every Section 2 current-file binding matches; the corrected
candidate has the frozen new digest; the fixed disk PE has the frozen digest
and machine `0x014C`; and the canonical output is absent.

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4.json
```

Require exit `0`, `verificationStatus=PASS`,
`workPackageStatus=BLOCKED_BY_EVIDENCE`, exact candidate and target digests,
`artifactBindingCount=18`, `anchorCount=49`, `newStaticAnchorCount=13`, three
offline confirmations, zero native/formal promotions, ten retained unknowns,
and the complete false permission/runtime/unlock boundary. Any failure is
preserved; no retry is authorized by this seal.

### Group 3 — one verify-existing

Only after the retry Group 2 passes:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot .
```

Require exit `0`, the same substantive result and candidate digest,
`outputWritten=false`, `outputPath=null`, and no created, removed, or changed
repository file.

### Group 4 — one sealed parameterized negative batch

Only after Group 3 passes:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v4\test_virtual_device_ownership_lifecycle_decision_v4.py
```

This is one batch invocation. Require exit `0`, every sealed negative subcase
rejected, no requested forged output published, and temporary fixtures cleaned
up.

## 5. Locked boundary

This seal does not authorize Group 1 again, any extra self-test, Cargo, full T0
build, production Native build, Debug/Release matrix, history/v1/v2/v3 tests,
game start, process enumeration/open/read, IPC, hook, injection, input/world
mutation, or active authorization. The verifier and tests remain disk-only.

Do not run Group 3 after a Group 2 failure. Do not run Group 4 after a Group 2
or Group 3 failure. Do not overwrite an existing canonical output. Any failure,
hash drift, unexpected output existence, or conservative-boundary expansion
requires a new explicit authorization and a new independent TEST seal.
