> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device lifecycle offline group-1 retry — independent TEST preseal v5

Date: 2026-09-08  
Role: independent TEST  
Phase: one repaired, hash-bound offline group-1 run  
Disposition: `READY_FOR_SINGLE_GROUP1_V3`

## 1. Supersession and narrow repair

The v4-sealed group-1 retry built and ran the dedicated Win32/x86 harness but
failed one of 16 cases. The sole failure was
`CALLBACK_MOVE_ONLY_KIND_AND_RELEASE: callback move/release order mismatch`.
All 13 failure/rollback cases passed. The runner stopped before report
publication, so the reserved v2 report remained absent and no v4 runtime
preseal binding was observed in a report.

After explicit user authorization, mainline made only these two narrow changes:

1. In the offline harness, it removed the obsolete callback
   `ContainsOrdered` check whose expected suborder contradicted the already
   present exact full-operation-log equality. The independent exact source-log
   and target-log comparisons remain in `RunCallbackMoveCase`; the lifecycle
   implementation, API, snapshots, reference accounting, failure injection,
   and 13 rollback paths are unchanged.
2. In the runner, it reserved a distinct v3 output leaf and changed the
   runtime retry-preseal path to this future completed v5 file. Its build,
   validation, safety, isolation, and create-new behavior is otherwise
   unchanged.

Static review found no blocker. This v5 seal supersedes v4 only for the one new
group-1 execution authorized below; all prior plans and results remain immutable
bound history.

## 2. Exact frozen bindings

Every file below must exist and its SHA-256 must match immediately before the
one run. Any mismatch stops the sequence and requires another review and seal.

| Role | Path | Required SHA-256 |
|---|---|---|
| v3 ownership/lifecycle decision | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v3.json` | `6E069DD4A66D90A4122D4D1D189ED99C2CFA101357515CC6D2AEBC2A9F01E219` |
| v3 verification output | `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v3.json` | `B53896E691F3B5EAABA05CEEA9740BDB7D914D6BCF8EF12D525081F5B65964BB` |
| v3 independent TEST results | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-test-results-v2.md` | `461ACA521CFF74E843494D7924EFADCEAC639602E42C99A7BC9610EC09E9D03F` |
| lifecycle design contract | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md` | `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665` |
| independent REVERSE v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md` | `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42` |
| original offline TEST preseal | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-plan-v1.md` | `4B503EC14184A75BAE8F92BD78BEA173557E3652347744EC6BCAD99D41551B39` |
| original SDK-access build failure | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-test-results-v1.md` | `CD8CB7FEA99A9F891DD311F408F09C8A6EF5D3629C7AA42E7D962132D4E5F969` |
| non-executable retry preseal v2 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v2.md` | `D3CA7B59B94EF61519F0B0F41045C6872D1488014D0E6BE51B7CB89ACBFAB31A` |
| parser-probe retry preseal v3 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v3.md` | `29DB7B7F76EFC0B49BECA3B355C6201372E5A55C86AC4C45D964E0F21DAF706E` |
| parser-only probe failure report | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-probe-results-v1.md` | `F7CE7069621C84E074E3D90C60907A64DD994834D25D9450F7227C67B3AB5E02` |
| argument-safe retry preseal v4 | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v4.md` | `82E66F63A75142BB959A4543AC0FB1BB235BE5325314871660B3F0D903848938` |
| v4 group-1 retry failure report | `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-results-v2.md` | `B390CCA287E261A278261ADEE833ED8E420872E445124529D5235332EE282DF5` |
| unchanged offline header | `cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp` | `0BBBA3E723079BEE2971C1A5DED6F721087DFEC1CC59B4C8064B0206914F2283` |
| unchanged offline implementation | `cs-t0/native/src/virtual_device_lifecycle.cpp` | `5091E40B532099FCA54C0B9264A582B7E7B7BC952941B577C096A7D95B177562` |
| repaired offline harness | `cs-t0/native/tests/virtual_device_lifecycle_main.cpp` | `81B7FB16A350B50346DCF21FE470ECC64686C12B58CF76B17C103D3D09AD7C7C` |
| unchanged dedicated Release/Win32 project | `cs-t0/native/IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj` | `A6B3560E79367EDC28B50D69CF06B1671AD4D0B27B4902A384C4835B281569DC` |
| v5-aware offline runner | `cs-t0/test-virtual-device-lifecycle-offline.ps1` | `B58357A3556F039B718C946D45435D7F1036A99DE8EC20E9663AED0902DCDE43` |
| unchanged argument-safe SDK probe | `cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` | `C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5` |
| unchanged production native project | `cs-t0/native/IsaacCsT0.Native.vcxproj` | `B5247DEEA646D91547BB93AB718B087CCBAD586A60C46D2C4EE7C8A9F66EDC2D` |
| unchanged complete-build entry | `cs-t0/build.ps1` | `158E08C46D53B441EF6728B281FE56D6DF539CD705C1F7229BDD7E93A790FEE4` |

## 3. Reused SDK-read gate; no redundant probe

The argument-safe v4 probe already ran exactly once with approved minimal
escalation before the v4 group-1 command. Its frozen script hash was
`C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5`,
its exit code was `0`, and its sole JSON document was:

```json
{"probeIdentity":"IsaacCsT0-VirtualDeviceLifecycleSdkReadProbe-v1","probeStatus":"PASS","path":"C:\\Users\\114514\\AppData\\Local\\Microsoft SDKs","readOnly":true,"sampledEntryCount":0}
```

The subsequent v4 run also passed SDK discovery, completed the dedicated build,
and executed the harness. Its only failure was the harness assertion recorded
in the bound v2 retry results. The probe script and SDK target are unchanged.
Therefore the user-authorized v5 sequence reuses that observed PASS and does not
authorize or require another SDK probe. This reuse is only an environment gate;
it is not native, runtime, implementation, or formal proof.

## 4. Distinct create-new output and runtime seal binding

The sole new output reserved by this seal is:

`out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json`

Both the failed v2 output and this new v3 output were observed absent while this
preseal was prepared. The v3 output must remain absent immediately before the
run. Existing allowed-root, exact `.json`, ADS, traversal, reparse-point,
pre-existing-output, `FileMode.CreateNew`, and partial-write cleanup rules remain
mandatory.

For exactly this v3 leaf, the runner reads the completed path
`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v5.md`
and emits its runtime SHA-256 with role `TEST_RETRY_PRESEAL`. The runner contains
the v5 path but no v5 digest, while this completed v5 seal binds the runner
digest in Section 2. The future report can therefore bind this seal without a
hash cycle.

## 5. Exactly one group-1 v3 command

Only after read-only confirmation that every Section 2 hash matches and the v3
output remains absent may TEST execute exactly once from the repository root,
using only the minimal escalated filesystem context needed for Visual Studio SDK
discovery:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json
```

PASS or FAIL consumes this authorization. No retry is permitted under this
seal. The command may build only the dedicated `Release|Win32` Application, run
only its offline harness, write its ordinary repository-local intermediates,
and create only the reserved report.

All original group-1 criteria remain frozen: exact lifecycle API and seven
states, six strong-reference classes with total ceiling six, external weak zero,
move-only leases, all 13 failure/rollback cases, exact operation logs and full
snapshots, dedicated-project isolation, and all-false game/process/IPC/hook/input
and world mutation facts. A PASS report must contain a `TEST_RETRY_PRESEAL`
binding whose path is this v5 file and whose SHA-256 is the runtime digest of
this completed file, together with the original preseal, contract, current
offline files/runner, unchanged production project/build, and executable hash.

## 6. Conservative locks

Until this one group-1 run exits `0`, publishes the create-new v3 report, and
all report contents and bindings are independently verified:

- the v4 decision candidate must not be created;
- groups 2, 3, and 4 remain `NOT_RUN` and locked;
- no offline, native, or formal status may be promoted;
- no game may be started, enumerated, opened, read, attached to, injected into,
  hooked, or mutated;
- no IPC, transport/scheduler work, input/world mutation, or active
  authorization is permitted.

After a valid group-1 PASS, mainline may create the v4 candidate in the original
sealed no-cycle order. Groups 2–4 still require their separate authorization;
this document does not authorize them.

## 7. Pre-execution disposition

The narrow repair and runner identity change have passed static review. No build,
harness, probe, or test was executed while preparing this seal. The disposition
is `READY_FOR_SINGLE_GROUP1_V3`.
