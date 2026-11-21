> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 final retry — independent TEST seal v4

Date: 2026-09-08  
Role: independent TEST  
Phase: Group-4-fix pre-execution seal  
Disposition: `READY_FOR_SINGLE_GROUP2_RETRY_V4`

## 1. Preserved history and retry authority

Three independent result reports preserve the prior attempts:

1. the first Group 2 rejected a prose `fixedGoal`;
2. the second Group 2 rejected the offline runner's deny-list literal as a
   false runtime dependency;
3. under retry seal v3, Group 2 create-new and Group 3 verify-existing passed,
   while the single Group 4 batch failed with 11 test failure records.

The existing PASS output from the third attempt is retained as immutable
history and must not be overwritten. The user explicitly authorized the narrow
Group-4 fixes and continuation. This seal therefore reserves a distinct
create-new output and authorizes one new Group 2, followed only on PASS by one
Group 3 and then one Group 4.

## 2. Frozen chain, current code, and outputs

| Role | Path | SHA-256 |
|---|---|---|
| candidate-pre TEST seal v1 | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-candidate-test-seal-v1.md` | `E9FE56403A1D216A2C53D392B47DF73CEE663ECBFCDAC2AFACFE872A3CDFA861` |
| first Group 2 failure report | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-test-results-v1.md` | `2E1CBE840D88EACA758138E17099873D32E144CE7C87E3BC0384D92EE78D1B7B` |
| retry TEST seal v2 | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-seal-v2.md` | `FB20102E4C00F57D0F12A1692E81CC722F7434F16DAA6D2E94EDFEC56BAC217A` |
| second Group 2 failure report | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-results-v2.md` | `AE623CB372593624AE62EF6A6F0329BC1FFBF282FD3047682CFB53306A78E2F6` |
| retry TEST seal v3 | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-seal-v3.md` | `4134C902A05814F54119C988F6C895201E387C072CABDD0AA3F54A3F00CE034F` |
| third sequence result report | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-group2-retry-test-results-v3.md` | `261A3CBC0D7514798FE78E36AEB91652E3BFAEE96385D380069945F3D761F612` |
| previous PASS output, retained and not overwritten | `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json` | `F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362` |
| previous PASS candidate, historical digest in report v3 | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79` |
| current candidate | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` | `73161422E04832566EB86B9D702C0FE6ABEBAA336FEDC86EF49F0B05DC8F04FD` |
| PowerShell read-only entry | `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1` | `BD30ADBF1994F141932807F6044C1B2810B2DC92CED5FD225BC37B44161ED170` |
| current Python disk-only verifier | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/verify_virtual_device_ownership_lifecycle_decision_v4.py` | `F89E791B42978EC72676FCBBC012F8056CC0AECA37782B8C6DB523E0FA285115` |
| current sealed parameterized negative batch | `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v4/test_virtual_device_ownership_lifecycle_decision_v4.py` | `A3E449DB545EE7B13D31DB6B23CA8A2C34CC75EDD8E3A8B17BE9EB3320E963AD` |
| final Group 1 v3 harness report | `out/cs-t0/virtual-device-lifecycle-offline-harness-20260908-v3.json` | `3FCAF96223EA34D929C9D222E7107CD4E0F4DCDC207586E3C7C1CBE31CF28A55` |
| final Group 1 retry preseal | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-lifecycle-offline-retry-test-plan-v5.md` | `3D19A0B157F7A72669A953E410445A7ACB7630470DD07F14232DF164F37C37E3` |
| hash-bound offline runner | `cs-t0/test-virtual-device-lifecycle-offline.ps1` | `B58357A3556F039B718C946D45435D7F1036A99DE8EC20E9663AED0902DCDE43` |
| argument-safe SDK probe | `cs-t0/test-virtual-device-lifecycle-sdk-probe.ps1` | `C24D0B0444773D40D8C0FFD0305243CCF77576453367D321CB0BB848340247B5` |
| fixed Win32/x86 disk PE | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

The historical candidate digest identifies the already consumed sequence; only
the current candidate digest is a current-file precondition.

The new reserved create-new output is:

`out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4-retry-v1.json`

It was `ABSENT` when this seal was created.

## 3. Static acceptance of the Group-4 fixes

Independent static review found no new blocker:

- decision-only validation now invokes the exact target-contract validator, so
  missing or substituted `newStaticAnchorIds` is rejected before a fixture can
  be accepted;
- the namespace gate requires exactly one real declaration line matching
  `namespace isaac_cs_t0::virtual_device {`, rather than relying only on a
  substring that could survive elsewhere in the header;
- the exact invalid-lease release guard must occur exactly twice in the source,
  so removing either ReaderLease or CallbackLease validity check is rejected;
- the rollback-order test accepts either the explicit rollback-order error or
  the earlier independent full-trace digest rejection;
- candidate-as-native and lifecycle-contract tests only widened their expected
  error expressions to include the verifier's existing, more specific rejection
  codes; they did not weaken the verifier;
- the current candidate binds the current verifier and negative batch digests,
  and all 18 candidate artifact bindings match their disk files;
- the candidate remains conservative: frozen `fixedGoal`, three offline-only
  confirmations, native/formal `UNKNOWN`, zero promotions, six static facts,
  ten unknowns, 18 false permissions, and all false runtime/unlock fields;
- the old PASS output still has SHA-256
  `F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362`
  and records previous candidate digest
  `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79`.

No test, verifier, build, game, or process operation was executed during this
review.

## 4. One-shot execution sequence

Each group is consumed by PASS or FAIL and must not be rerun. Before each group,
recheck all current-file hashes and predecessor conditions. Any failure stops
all later groups.

### Group 2 — one distinct create-new verification

Preconditions: every current binding in Section 2 matches; the current candidate
has SHA-256 `73161422E04832566EB86B9D702C0FE6ABEBAA336FEDC86EF49F0B05DC8F04FD`;
the fixed disk PE has the frozen digest and machine `0x014C`; the retained PASS
output remains unchanged; and the new retry-v1 output is absent.

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4-retry-v1.json
```

Require exit `0`, `verificationStatus=PASS`,
`workPackageStatus=BLOCKED_BY_EVIDENCE`, exact current candidate and target
digests, `artifactBindingCount=18`, `anchorCount=49`,
`newStaticAnchorCount=13`, three offline confirmations, zero native/formal
promotions, ten retained unknowns, and the complete false permission/runtime/
unlock boundary. PASS or FAIL consumes this Group 2 authorization.

### Group 3 — one verify-existing

Only after Group 2 passes:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot .
```

Require exit `0`, the same substantive result and current candidate digest,
`outputWritten=false`, `outputPath=null`, both verification outputs unchanged,
and no created, removed, or changed repository file.

### Group 4 — one corrected parameterized negative batch

Only after Group 3 passes:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v4\test_virtual_device_ownership_lifecycle_decision_v4.py
```

This is one batch invocation. Require exit `0`, all ten unittest methods and all
parameterized negative subcases accepted, every forged candidate rejected, no
requested forged output published, and temporary fixtures cleaned up.

## 5. Locked boundary

This seal does not authorize Group 1 again, another SDK probe, any extra
self-test, Cargo, full T0 build, production Native build, Debug/Release matrix,
history/v1/v2/v3 tests, game start, process enumeration/open/read, IPC, hook,
injection, input/world mutation, or active authorization. The verifier and
negative batch remain disk-only.

Do not run Group 3 after a Group 2 failure. Do not run Group 4 after a Group 2
or Group 3 failure. Do not overwrite either verification output. Any failure,
hash drift, unexpected output existence, or conservative-boundary expansion
requires a new explicit authorization and a new independent TEST seal.
