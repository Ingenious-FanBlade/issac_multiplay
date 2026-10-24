> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 — final independent TEST acceptance report v1

Date: 2026-09-08  
Role: independent TEST  
Work package: `CS-T0-05-05`  
Final TEST disposition: `PASS_WITH_FORMAL_DECISION_BLOCKED`

## 1. Final frozen identities

| Artifact | SHA-256 |
|---|---|
| v4 candidate `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `73161422E04832566EB86B9D702C0FE6ABEBAA336FEDC86EF49F0B05DC8F04FD` |
| v4 PowerShell verifier entry | `BD30ADBF1994F141932807F6044C1B2810B2DC92CED5FD225BC37B44161ED170` |
| v4 Python verifier | `F89E791B42978EC72676FCBBC012F8056CC0AECA37782B8C6DB523E0FA285115` |
| v4 sealed negative batch | `A3E449DB545EE7B13D31DB6B23CA8A2C34CC75EDD8E3A8B17BE9EB3320E963AD` |
| final retry seal v4 | `91D62717A094CBB2D4FC860AD3683D64263CDB355731957F10602B1BF5C14282` |
| independent REVERSE v2 handoff | `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42` |
| fixed Win32/x86 PE | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

Before the final Group 2 run, retry seal v4's 11 frozen inputs matched, all 18
candidate artifact bindings matched, the prior PASS output was unchanged, and
the distinct retry-v1 output did not exist.

## 2. Group 1 — final offline harness v3

The final sealed Group 1 command was executed once under retry preseal v5:

```powershell
pwsh -NoProfile -File .\cs-t0\test-virtual-device-lifecycle-offline.ps1 -Configuration Release -Output out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json
```

Result:

- verification status: `PASS`
- configuration/platform: `Release|Win32`
- harness report:
  `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json`
- harness report SHA-256:
  `3FCAF96223EA34D929C9D222E7107CD4E0F4DCDC207586E3C7C1CBE31CF28A55`
- executable SHA-256:
  `90E0D1BDCE0B7B07CF98626C27D4D61C1F64E98436A5B85D17E6CFB11B33AE58`
- cases: `16/16 PASS`
- assertions: `332`
- rollback cases: `13/13 PASS`
- lifecycle states: `7`
- strong-reference classes: `6`
- maximum simultaneous strong references: `6`
- maximum external weak references: `0`

The report binds retry preseal v5 SHA-256
`3D19A0B157F7A72669A953E410445A7ACB7630470DD07F14232DF164F37C37E3`.
The harness was an isolated x86 application and was not linked into or loaded by
the production game DLL.

## 3. Groups 2–4 — final retry seal v4

### Group 2 — create-new production verification

The following command was executed exactly once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4-retry-v1.json
```

Result:

- process exit code: `0`
- verificationStatus: `PASS`
- outputWritten: `true`
- output SHA-256:
  `048634971F408F30E2A06552C96A5AC3ACF5850BBAE5D1FF2D0321656FE0FCA4`
- decision SHA-256:
  `73161422E04832566EB86B9D702C0FE6ABEBAA336FEDC86EF49F0B05DC8F04FD`
- artifact bindings: `18`
- anchors: `49` total, `13` new
- offline confirmations: `3`
- native static promotions: `0`
- formal static promotions: `0`
- retained unknown facts: `10`

The earlier PASS output
`out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
was preserved without overwrite at SHA-256
`F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362`.

### Group 3 — verify-existing

Only after Group 2 passed, the following command was executed exactly once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot .
```

Result:

- process exit code: `0`
- verificationStatus: `PASS`
- outputWritten: `false`
- outputPath: `null`
- repository file count before and after: `162521`
- repository path/length/UTC-write-time manifest SHA-256 before and after:
  `2E9F3AE2B8FE1150D5361732AAF68E744CD21F76F156FA13267814A927854D85`
- old output SHA-256 before and after:
  `F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362`
- retry-v1 output SHA-256 before and after:
  `048634971F408F30E2A06552C96A5AC3ACF5850BBAE5D1FF2D0321656FE0FCA4`

No repository file was created, removed, or observed changed by
verify-existing.

### Group 4 — corrected parameterized negative batch

Only after Group 3 passed, the following command was executed exactly once:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v4\test_virtual_device_ownership_lifecycle_decision_v4.py
```

Result:

- process exit code: `0`
- unittest result: `OK`
- test methods: `10/10 PASS`
- reported duration: `2.213s`

The batch passed the sealed fixture and every parameterized negative category:
target/hash/anchor identity, artifact binding closure, production isolation,
source/API/lease/runtime-dependency contract, harness rollback/reference/thread
semantics, three-axis promotion rejection, conservative permissions/unlocks,
and create-new/verify-existing write boundaries. Temporary fixtures were
cleaned up, and the repository metadata manifest remained unchanged.

## 4. Preserved failed attempts

No failed attempt was hidden, overwritten, or rerun under its consumed seal:

1. `virtual-device-ownership-lifecycle-v4-group2-test-results-v1.md`, SHA-256
   `2E1CBE840D88EACA758138E17099873D32E144CE7C87E3BC0384D92EE78D1B7B`,
   preserves the original Group 2 `fixedGoal` mismatch;
2. `virtual-device-ownership-lifecycle-v4-group2-retry-test-results-v2.md`,
   SHA-256
   `AE623CB372593624AE62EF6A6F0329BC1FFBF282FD3047682CFB53306A78E2F6`,
   preserves the offline-runner deny-list false positive;
3. `virtual-device-ownership-lifecycle-v4-group2-retry-test-results-v3.md`,
   SHA-256
   `261A3CBC0D7514798FE78E36AEB91652E3BFAEE96385D380069945F3D761F612`,
   preserves the earlier Group 4 `Ran 10 / failures=11` result.

Each correction received new explicit authority and a new independent TEST
seal. The final successful sequence used a distinct create-new output.

## 5. Formal decision boundary

The three v4 proof rows remain deliberately split across three axes:

| Proof | offlineImplementationStatus | nativeEvidenceStatus | formalStatus |
|---|---|---|---|
| `VIRTUAL_DEVICE_OBJECT_OWNER` | `OFFLINE_IMPLEMENTATION_CONFIRMED` | `UNKNOWN` | `UNKNOWN` |
| `VIRTUAL_DEVICE_CREATION_DESTRUCTION` | `OFFLINE_IMPLEMENTATION_CONFIRMED` | `UNKNOWN` | `UNKNOWN` |
| `VIRTUAL_DEVICE_REFERENCE_COUNTING` | `OFFLINE_IMPLEMENTATION_CONFIRMED` | `UNKNOWN` | `UNKNOWN` |

Therefore:

- `promotionCount=0`;
- `formalDecision=null`;
- `workPackageStatus=BLOCKED_BY_EVIDENCE`;
- `implementationEligibility=false`;
- `virtualDeviceImplemented=false`;
- all 18 permissions are false;
- all runtime/process/game/IPC/hook/input/world observation or mutation fields
  are false;
- `csT00506Unlocked=false`, `t005Unlocked=false`, and `t006Unlocked=false`.

Passing the offline harness and read-only verifier confirms that the blocked
decision is internally complete and authentic. It does not promote the three
proofs to native or formal static evidence and does not unlock implementation.

## 6. Role separation and safety boundary

The independent REVERSE role produced and sealed the static native handoff
`virtual-device-ownership-lifecycle-joint-reverse-v2.md`. The independent TEST
role separately presealed acceptance conditions, reviewed the verifier/tests,
executed only the authorized compressed groups, and preserved every failure.
TEST did not perform reverse engineering or alter REVERSE evidence; REVERSE did
not author this acceptance result.

No game was started. No target-game process was enumerated, opened, read,
attached to, injected into, hooked, or mutated. No IPC, input mutation, world
mutation, runtime mutation, or active authorization occurred. No Cargo suite,
full T0 build, production Native matrix, historical broad regression, or
duplicate self-test was run.
