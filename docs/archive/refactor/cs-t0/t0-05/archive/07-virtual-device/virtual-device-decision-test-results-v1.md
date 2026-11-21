> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device decision independent TEST results v1

Date: 2026-09-08 (Asia/Shanghai)

Role: independent TEST. The TEST role did not perform or reinterpret the
REVERSE work. The finalized REVERSE handoff was accepted only as an exact
path/SHA-256 predecessor binding.

## Result

- `verificationStatus=PASS`
- `workPackageStatus=BLOCKED_BY_EVIDENCE`
- `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`
- `formalDecision=null`
- `csT00506Unlocked=false`, `t005Unlocked=false`, `t006Unlocked=false`
- all 18 permission fields are `false`

PASS means that the create-new blocking conclusion is authentic and complete.
It does not mean that CS-T0-05-05 passed, does not establish
`REQUIRED_ELIGIBLE`, and grants no runtime, active, mutation, or implementation
authority.

## Sealed inputs and outputs

| Item | SHA-256 |
| --- | --- |
| `cs-t0/coop-virtual-device-decision-v1.json` | `1F95C4E5EB3D538C4EF0C33BB51706453CDF9C30E162DA1AB27AE9D42ACD16DE` |
| approved unpacked `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-static-reverse-v1.md` | `DCD5581CE7901DB6812248834A7CD7F249B81086BECF5EA69002531C18F6AE11` |
| `cs-t0/verify-coop-virtual-device-decision.ps1` | `63C8FD849D08E7D5C2DC95F4662A762472A93F6333A1D2C2861F5A449F3A1595` |
| `cs-t0/tests/virtual_device_decision/verify_virtual_device_decision.py` | `3EED83A75EE525B8E3C415DD567D8AAF401827AC615267444C2C35DCC3D664B4` |
| `cs-t0/tests/virtual_device_decision/test_virtual_device_decision.py` | `5F859E91275DDD8BE996668D753F6C555BA7129BD9E3E6D9686C0E2CD5ECE40A` |
| create-new `out/cs-t0/virtual-device-decision-verification-20260908-v1.json` | `1AA108103B31945CAAF9F7C8DB7D5979E1E3F8DE8FEF4F433E5B8C082E771AB0` |

The candidate binds exactly 13 predecessor path/hash pairs. The verifier
confirmed PE32/x86 identity and 10 file-backed static anchors, exactly six
`STATIC_CONFIRMED` rows, and exactly ten `UNKNOWN` rows. `missingEvidence`
equals those ten unknown identifiers.

## Compressed execution record

The final candidate-stage four-group suite was run once:

```powershell
python -B .\cs-t0\tests\virtual_device_decision\test_virtual_device_decision.py
```

Exit code: `0`. Result: `Ran 4 tests in 0.244s`, `OK`.

The four groups were: one fixed target/predecessor/anchor batch; one
create-new blocked-decision check; one deterministic read-only verify-existing
check; and one parameterized negative batch. The negative batch rejected
forged `NOT_REQUIRED`, premature `REQUIRED_ELIGIBLE`, permission escalation,
05-06 unlock escalation, a wrong predecessor hash, and overwrite of an
existing output.

The real fixed PE create-new command was run once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-decision.ps1 `
  -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" `
  -RepositoryRoot . `
  -Output .\out\cs-t0\virtual-device-decision-verification-20260908-v1.json
```

Exit code: `0`. The new JSON reported `verificationStatus=PASS`,
`workPackageStatus=BLOCKED_BY_EVIDENCE`, `formalDecision=null`, 13 predecessor
bindings, 10 static anchors, six confirmed rows, ten unknown rows, all
permissions false, and all unlocks false.

The real fixed PE verify-existing command was run once without `-Output`:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-decision.ps1 `
  -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" `
  -RepositoryRoot .
```

Exit code: `0`. It returned the same decision hash and blocking state and did
not request an output write.

## Read-only and omitted-work boundary

The verifier opened only disk files. No game process was enumerated or opened;
the game was not started; no IPC, injection, hook, input, virtual device, or
runtime/world mutation was created; and no active authorization was requested
or consumed. The only verifier mutation was the explicitly requested
create-new JSON. This independent report is the only additional TEST document.

As required by the compressed plan, TEST did not run Cargo full tests, the full
T0 build, Debug/Release Native matrices, historical broad regression, a live
canary, or a repeated candidate-stage self-test.
