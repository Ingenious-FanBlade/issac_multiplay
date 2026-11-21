> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle v4 retry — independent TEST results v3

Date: 2026-09-08  
Role: independent TEST  
Seal: `virtual-device-ownership-lifecycle-v4-group2-retry-test-seal-v3.md`  
Overall result: `FAIL_AT_GROUP4`

## 1. Sealed inputs and precheck

- retry seal v3 SHA-256:
  `4134C902A05814F54119C988F6C895201E387C072CABDD0AA3F54A3F00CE034F`
- candidate:
  `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json`
- candidate SHA-256:
  `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79`
- PowerShell wrapper SHA-256:
  `BD30ADBF1994F141932807F6044C1B2810B2DC92CED5FD225BC37B44161ED170`
- Python verifier SHA-256:
  `422039DC901390424EC311ED5357A9AF15F54E5ECB36BFF744CAC27644604F77`
- sealed negative batch SHA-256:
  `F31404BD8B44DFE5DBA184B89D720C37FA35EB29E25E405B1B698AD0FFA0A65D`
- fixed PE SHA-256:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

Before Group 2, all 14 retry-chain/current-file hashes matched, all 18
candidate artifact bindings matched, and the canonical output was absent.

## 2. Group 2 — create-new production verification

The sealed command was executed exactly once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot . -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v4.json
```

Result:

- process exit code: `0`
- verificationStatus: `PASS`
- outputWritten: `true`
- outputPath:
  `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4.json`
- output SHA-256:
  `F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362`
- decision SHA-256:
  `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79`
- artifact bindings: `18`
- anchors: `49` total, `13` new
- offline confirmations: `3`
- native static promotions: `0`
- formal static promotions: `0`
- retained unknown facts: `10`
- work package status: `BLOCKED_BY_EVIDENCE`
- formal decision: `null`
- implementation eligibility: `false`

All permissions, runtime/process/game/IPC/hook/input/world fields, and unlocks
were false.

## 3. Group 3 — verify-existing

Only after Group 2 passed, the sealed command was executed exactly once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -RepositoryRoot .
```

Result:

- process exit code: `0`
- verificationStatus: `PASS`
- outputWritten: `false`
- outputPath: `null`
- substantive result and candidate digest: unchanged from Group 2
- canonical output SHA-256 before and after:
  `F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362`
- repository file count before and after: `162518`
- repository path/length/UTC-write-time manifest SHA-256 before and after:
  `43CA37A8BD40FA284D45953EC05CC4BF1914B2B9E506FD175FB4584B23D92586`
- candidate artifact-binding manifest SHA-256 before and after:
  `3CC759C0CC7CBAD68975142D4BD3C6AC68D489EFAA2ECEB800C3D4A0770F4FBC`

No repository file was created, removed, or observed changed by
verify-existing.

## 4. Group 4 — sealed parameterized negative batch

Only after Group 3 passed, the sealed command was executed exactly once:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v4\test_virtual_device_ownership_lifecycle_decision_v4.py
```

Result:

- process exit code: `1`
- duration reported by unittest: `2.596s`
- unittest methods run: `10`
- failure records: `11`
- overall unittest result: `FAILED (failures=11)`

Methods reported `ok`:

- `test_00_sealed_fixture_is_valid`
- `test_02_binding_set_role_digest_and_frozen_artifact_rejections`
- `test_03_production_isolation_and_project_shape_rejections`
- `test_08_conservative_boundary_permission_and_unlock_rejections`
- `test_09_create_new_output_policy_rejections_are_write_free`

Four negative mutations did not raise the expected `ContractError`:

1. `newStaticAnchorIds=[]`
2. `newStaticAnchorIds=['WRONG_ANCHOR']`
3. `headerMutation='namespace'`
4. `sourceMutation='invalid_release'`

Seven mutations were rejected, but the test's accepted error expression was
narrower than the verifier's actual specific error:

1. rollback-order mutation expected `HARNESS_ROLLBACK_ORDER`, actual
   `HARNESS_CASE_TRACE_DRIFT:VECTOR_INSERT_FAILURE`
2. `candidate_as_native` expected
   `PROMOTION_ASSESSMENT|EVIDENCE_TRANSITION`, actual
   `NATIVE_REFS_VIRTUAL_DEVICE_OBJECT_OWNER_MISMATCH`
3. state mutation expected `LIFECYCLE_CONTRACT|MUST_BE_FALSE`, actual
   `STATE_MACHINE_MISMATCH`
4. API mutation expected `LIFECYCLE_CONTRACT|MUST_BE_FALSE`, actual
   `PUBLIC_APIS_MISMATCH`
5. strong-class mutation expected `LIFECYCLE_CONTRACT|MUST_BE_FALSE`, actual
   `STRONG_REFERENCE_CLASSES_MISMATCH`
6. rollback-ID mutation expected `LIFECYCLE_CONTRACT|MUST_BE_FALSE`, actual
   `ROLLBACK_FAILURE_IDS_MISMATCH`
7. thread-class mutation expected `LIFECYCLE_CONTRACT|MUST_BE_FALSE`, actual
   `THREAD_CLASSES_MISMATCH`

Group 4 was consumed by this failure and was not rerun.

## 5. Hash and side-effect closure

After Group 4:

- candidate SHA-256 remained
  `7873D6FAD59A7E5F39E697CD5D5C71F9353FF847095549194D37DF93C13EBF79`;
- verifier SHA-256 remained
  `422039DC901390424EC311ED5357A9AF15F54E5ECB36BFF744CAC27644604F77`;
- negative batch SHA-256 remained
  `F31404BD8B44DFE5DBA184B89D720C37FA35EB29E25E405B1B698AD0FFA0A65D`;
- canonical output SHA-256 remained
  `F4903C6B7E950C9A0E735D976EB443237F40AB385C370D07195B2F8B03C6A362`;
- the repository file count and metadata manifest remained `162518` and
  `43CA37A8BD40FA284D45953EC05CC4BF1914B2B9E506FD175FB4584B23D92586`.

Temporary negative-test fixtures left no observed repository side effect. No
game was started or enumerated/opened/read. No IPC, hook, injection, runtime
mutation, input mutation, world mutation, or active authorization occurred. No
unsealed test, Cargo suite, full build, Native matrix, historical regression,
or extra retry was run.

The overall retry sequence is not accepted because Group 4 failed. Further
work requires a new explicit authorization and a new independent TEST seal.
