> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device ownership/lifecycle joint proof v3 — independent TEST results

Date: 2026-09-08  
Role: independent TEST  
Scope: read-only disk-PE decision verification  
Result: verifier contract `PASS`; work package remains `BLOCKED_BY_EVIDENCE`

## 1. Sealed inputs and resulting artifacts

| Artifact | SHA-256 |
|---|---|
| TEST preseal `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-test-plan-v2.md` | `2043AF3F13F834925E3814FA6A1F929C6524709B571F7C9BC55ED37FFD28A623` |
| REVERSE handoff `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v1.md` | `5C3BEB037C03C51F42EC85E7E513FAD3D23A1F0FCCC0A1059B137A3A581B7C38` |
| Design contract `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md` | `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665` |
| Candidate `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v3.json` | `6E069DD4A66D90A4122D4D1D189ED99C2CFA101357515CC6D2AEBC2A9F01E219` |
| PowerShell verifier `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v3.ps1` | `195D16BC8DD179F8D289C137708AAC8C8954CB26A22059C38EA776D74D63BC7E` |
| Python verifier `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v3/verify_virtual_device_ownership_lifecycle_decision_v3.py` | `B9BC060D4F59041250FDCDB9C319778F246AFE413A0E8EC79B32886C11262013` |
| Sealed four-group test `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v3/test_virtual_device_ownership_lifecycle_decision_v3.py` | `30ADF30B88FD3EE67226374809E737114B7532A5FB7898359811F97CB374DBE1` |
| Create-new verification output `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v3.json` | `B53896E691F3B5EAABA05CEEA9740BDB7D914D6BCF8EF12D525081F5B65964BB` |

The fixed disk PE was `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`, Win32/x86 machine `0x014C`, SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

## 2. Single sealed fixture-suite execution

Exactly one execution was performed:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v3\test_virtual_device_ownership_lifecycle_decision_v3.py
```

Result:

- exit code: `0`
- elapsed time reported by unittest: `0.269s`
- tests: `4/4 PASS` (`Ran 4 tests`, `OK`)
- fixed PE/hash, machine, 36-anchor, 11-predecessor, and isolated contract/schema batch: `PASS`
- isolated create-new: `PASS`
- deterministic/no-write verify-existing: `PASS`
- single parameterized negative batch: `PASS`

The suite was not rerun.

## 3. Production create-new and verify-existing

### 3.1 Create-new — one execution

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v3.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v3.json
```

Result:

- exit code: `0`
- `verificationStatus=PASS`
- `outputWritten=true`
- output path: `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v3.json`

### 3.2 Verify-existing — one execution

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v3.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot .
```

Result:

- exit code: `0`
- `verificationStatus=PASS`
- `outputWritten=false`
- `outputPath=null`
- decision SHA-256 matched create-new: `6E069DD4A66D90A4122D4D1D189ED99C2CFA101357515CC6D2AEBC2A9F01E219`

Neither production command was rerun.

## 4. Verified decision result

Both production invocations reported the same substantive decision state:

- `anchorCount=36`
- `newProofAnchorCount=19`
- `predecessorBindingCount=11`
- `promotionCount=0`
- `promotedIds=[]`
- `totalStaticCount=6`
- `totalUnknownCount=10`
- `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`
- `formalDecision=null`
- `workPackageStatus=BLOCKED_BY_EVIDENCE`
- `implementationEligibility=false`
- `designContractStatus=DEFINED_NOT_IMPLEMENTED`

All five v3 assessment IDs remain `UNKNOWN`, and all five always-unknown IDs remain `UNKNOWN`. Passing the verifier means the blocking decision is internally consistent with its sealed read-only evidence; it does not make CS-T0-05-05 eligible and does not unlock CS-T0-05-06.

## 5. Permission, unlock, and runtime boundary

All 18 permission fields were present and `false`:

- `activeExperimentAllowed`
- `activeInputAllowed`
- `applyAllowed`
- `automaticLiveRetryAllowed`
- `inputMutationAllowed`
- `liveAuthorizationCreated`
- `modeLatchAllowed`
- `originalOnlinePositiveAllowed`
- `overrideAllowed`
- `passiveHookTransactionAllowed`
- `passiveObservationAllowed`
- `processEnumerationAllowed`
- `processReadAllowed`
- `runtimeMutationAllowed`
- `schedulerAllowed`
- `transportAllowed`
- `virtualDeviceCreationAllowed`
- `worldMutationAllowed`

All unlock and runtime-side-effect fields were `false`:

- `csT00506Unlocked=false`
- `t005Unlocked=false`
- `t006Unlocked=false`
- `runtimeObserved=false`
- `processAccessed=false`
- `gameStarted=false`
- `ipcCreated=false`
- `virtualDeviceImplemented=false`

TEST did not enumerate, open, read, attach to, or mutate a game process. No game, IPC, hook, injection, input, world mutation, virtual-device implementation, passive canary, or active canary was started.

## 6. Intentionally omitted work

In accordance with the compressed TEST seal, TEST did not run:

- Cargo full tests;
- the complete T0 build;
- Debug/Release Native matrices;
- historical broad regression;
- v1 or v2 fixture suites/verifiers;
- game startup or any game-process test.

No fixture suite or production verification command was repeated. No implementation, verifier, candidate, preseal, REVERSE handoff, design contract, or historical artifact was modified by this reporting step.

## 7. Independent TEST disposition

The v3 verifier contract and its create-new/verify-existing determinism checks pass. The evidence result remains conservative: zero promotions, six inherited static items, ten unknown items, no formal decision, no implementation eligibility, no permissions, and no unlock. CS-T0-05-05 therefore remains `BLOCKED_BY_EVIDENCE`.
