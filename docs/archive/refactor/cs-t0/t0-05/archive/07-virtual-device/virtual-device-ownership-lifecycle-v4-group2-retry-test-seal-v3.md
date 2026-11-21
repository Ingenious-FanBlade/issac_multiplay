> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 Group 2 retry — independent TEST seal v3

Date: 2026-09-08  
Role: independent TEST  
Phase: false-positive-fix pre-execution retry seal  
Disposition: `READY_FOR_SINGLE_GROUP2_RETRY_V3`

## 1. Preserved failures and authority

Candidate-pre seal v1's first Group 2 failed on the unfrozen prose
`fixedGoal`. Retry seal v2's new Group 2 then failed on
`OFFLINE_RUNTIME_DEPENDENCY:openprocess`; the token occurred only as deny-list
text inside the offline runner itself. Both executions were consumed, neither
produced the canonical output, and Groups 3–4 were not run. Their independent
failure reports remain authoritative and are bound below.

The user explicitly authorized fixing the false positive and continuing. The
only accepted verifier-policy change is to scan files that can compile or link
into the dedicated executable while keeping the runner itself protected by its
frozen hash. The candidate was updated only to bind the new verifier digest.
This seal authorizes one new Group 2 create-new attempt; it does not erase,
rerun, or reinterpret either prior failure.

## 2. Frozen chain and current inputs

| Role | Path | SHA-256 |
|---|---|---|
| candidate-pre TEST seal v1 | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-candidate-test-seal-v1.md` | `E9FE56403A1D216A2C53D392B47DF73CEE663ECBFCDAC2AFACFE872A3CDFA861` |
| first Group 2 failure report | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-test-results-v1.md` | `2E1CBE840D88EACA758138E17099873D32E144CE7C87E3BC0384D92EE78D1B7B` |
| retry TEST seal v2 | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-seal-v2.md` | `FB20102E4C00F57D0F12A1692E81CC722F7434F16DAA6D2E94EDFEC56BAC217A` |
| second Group 2 failure report | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-results-v2.md` | `AE623CB372593624AE62EF6A6F0329BC1FFBF282FD3047682CFB53306A78E2F6` |
| original candidate, historical digest in first failure report | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `9C72C0ADA5440E4096DC93F90D6FC52197E3006005079D9932712043A6C3855A` |
| fixedGoal-corrected candidate, historical digest in second failure report | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `E787255F1ED276FC7C9D5350A9245CEB2BDED121029AD0DAA0A416EE6B6A352F` |
| current candidate with synchronized verifier binding | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79` |
| unchanged PowerShell read-only entry | `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1` | `BD30ADBF1994F141932807F6044C1B2810B2DC92CED5FD225BC37B44161ED170` |
| current Python disk-only verifier | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/verify_virtual_device_ownership_lifecycle_decision_v4.py` | `422039DC901390424EC311ED5357A9AF15F54E5ECB36BFF744CAC27644604F77` |
| unchanged sealed parameterized negative batch | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/test_virtual_device_ownership_lifecycle_decision_v4.py` | `F31404BD8B44DFE5DBA184B89D720C37FA35EB29E25E405B1B698AD0FFA0A65D` |
| final Group 1 v3 harness report | `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json` | `3FCAF96223EA34D929C9D222E7107CD4E0F4DCDC207586E3C7C1CBE31CF28A55` |
| final Group 1 retry preseal | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v5.md` | `3D19A0B157F7A72669A953E410445A7ACB7630470DD07F14232DF164F37C37E3` |
| hash-bound offline runner | `cs-t0/test-virtual-device-lifecycle-offline.ps1` | `B58357A3556F039B718C946D45435D7F1036A99DE8EC20E9663AED0902DCDE43` |
| argument-safe SDK probe | `cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` | `C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5` |
| fixed Win32/x86 disk PE | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

The historical candidate digests identify the already consumed attempts; only
the current candidate digest is a current-file precondition.

## 3. Static acceptance of the narrow scanner fix

Static review found no new blocker:

- the runtime-dependency scanner combines the exact header, source, harness,
  and dedicated project text, lowercases it, and rejects every forbidden token
  by direct substring membership;
- because the scanner does not strip comments or distinguish lexical context,
  a forbidden token inserted as executable source, linked project text, or a
  source/harness/project comment is still rejected;
- the offline runner is intentionally excluded from that content scan because
  it contains the deny-list literals, but it remains an exact frozen binding in
  both verifier policy and candidate evidence;
- the fixed scan still covers every file that can compile or link into the
  dedicated Release|Win32 executable and does not weaken production-isolation
  checks;
- the current candidate has 18 artifact bindings and all 18 match their disk
  files, including verifier SHA-256
  `422039DC901390424EC311ED5357A9AF15F54E5ECB36BFF744CAC27644604F77`;
- `fixedGoal`, the three proof axes, zero promotions, six static facts, ten
  unknowns, 18 false permissions, and all false runtime/unlock fields remain
  unchanged and conservative;
- the fixed 49-anchor Win32/x86 target profile and create-new-only policy remain
  unchanged.

The canonical output
`out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
was `ABSENT` at seal time. No verifier, test, build, game, or process operation
was executed during this static review.

## 4. One-shot retry sequence

Each group is consumed by PASS or FAIL and must not be rerun. Before each group,
recheck all current-file hashes and predecessor conditions. Any failure stops
all later groups.

### Group 2 — one create-new attempt after the scanner fix

Preconditions: every current binding in Section 2 matches, the candidate has
SHA-256 `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79`,
the fixed disk PE has the frozen digest and machine `0x014C`, and the canonical
output remains absent.

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4.json
```

Require exit `0`, `verificationStatus=PASS`,
`workPackageStatus=BLOCKED_BY_EVIDENCE`, exact candidate and target digests,
`artifactBindingCount=18`, `anchorCount=49`, `newStaticAnchorCount=13`, three
offline confirmations, zero native/formal promotions, ten retained unknowns,
and the complete false permission/runtime/unlock boundary. PASS or FAIL consumes
this Group 2 authorization.

### Group 3 — one verify-existing

Only after Group 2 passes:

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

This seal does not authorize Group 1 again, another SDK probe, any extra
self-test, Cargo, full T0 build, production Native build, Debug/Release matrix,
history/v1/v2/v3 tests, game start, process enumeration/open/read, IPC, hook,
injection, input/world mutation, or active authorization. The verifier and
negative batch remain disk-only.

Do not run Group 3 after a Group 2 failure. Do not run Group 4 after a Group 2
or Group 3 failure. Do not overwrite an existing canonical output. Any failure,
hash drift, unexpected output existence, or conservative-boundary expansion
requires a new explicit authorization and a new independent TEST seal.
