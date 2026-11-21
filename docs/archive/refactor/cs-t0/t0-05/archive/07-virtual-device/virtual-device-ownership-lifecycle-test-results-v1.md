> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle successor independent TEST results v1

Date: 2026-09-08 (Asia/Shanghai)

Role: independent TEST. TEST sealed the rules before the successor REVERSE
handoff, did not perform reverse engineering, and did not alter or rerun the v1
decision verifier, tests, or report.

## Result

- `verificationStatus=PASS`
- `workPackageStatus=BLOCKED_BY_EVIDENCE`
- `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`
- `formalDecision=null`
- `promotionCount=0`, `promotedIds=[]`
- `totalStaticCount=6`, `totalUnknownCount=10`
- `csT00506Unlocked=false`, `t005Unlocked=false`, `t006Unlocked=false`
- all 18 permission fields are `false`

PASS authenticates the conservative create-new successor and its retained
UNKNOWN verdicts. It is not a CS-T0-05-05 PASS, does not establish virtual
device implementation eligibility, and grants no runtime or active authority.

## Sealed inputs and outputs

| Item | SHA-256 |
| --- | --- |
| `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v2.json` | `3ACAFAEB608EB1794B8E16477F6FFFE64EC95442E1C6C0EBE7FDC4061E40A916` |
| approved unpacked `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-test-plan-v1.md` | `7481F05B81FE5728D104115A2CEFEFA93DC64CFB3729F732431AA05F7BD5DD32` |
| `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-reverse-v1.md` | `9CB6CC4D08DC0F75B0921D0C7B3A5A2753EB5A64930DFCDC58E62F70613A307C` |
| `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1` | `417401D1AF5DA605820005B3CA6ACAA038E8622F10E3007B5F1BFBC5A41EF329` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v2/verify_virtual_device_ownership_lifecycle_decision_v2.py` | `A06C2B164F6C100955363B5B9191732E407A17A90D0C58A052233BB092E91D43` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v2/test_virtual_device_ownership_lifecycle_decision_v2.py` | `8ED0757583003B342027C67E283E1883C808DBB652A5F6DFD093B7E0DA5A7F4C` |
| create-new `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v2.json` | `03E275D747CBFED0908F67F1400AFEA38B1AD40A07692F4710D8B3DDCAAC2682` |

The verifier confirmed all eight predecessor path/hash bindings and the fixed
PE32/x86 target. It matched 17 file-backed anchors: the ten inherited v1
anchors and seven ownership/lifecycle proof-scope anchors. Matching a local
anchor proves only target identity and the referenced local subchain; it does
not close a universal ownership or lifecycle claim.

## Promotion rejection record

### `VIRTUAL_DEVICE_OBJECT_OWNER = UNKNOWN`

The manager vector is a known long-lived strong owner, but insert notification
and remove callbacks receive shared-pair copies. The evidence does not exclude
a callback retaining a strong reference, does not close the alias/owner
cardinality, and does not identify a unique removal and final-release
authority. A local wrapper/vector path therefore cannot satisfy the sealed
owner-boundary promotion.

### `VIRTUAL_DEVICE_CREATION_DESTRUCTION = UNKNOWN`

The normal allocation, ctor, wrapper, insert, remove, last-strong destructor,
stop, and manager destruction subchains are located. They are insufficient:
the control-block-null branch can still construct `{object, null-control}` and
reach insert, while allocation/constructor member failure, early return,
partial initialization, insert/grow/callback failure, rollback, unwind or
reentrancy, and exactly-once cleanup are not universally closed. The sealed
test rejects promotion based only on ctor/dtor local shapes.

### `VIRTUAL_DEVICE_REFERENCE_COUNTING = UNKNOWN`

The statically closed mechanical subchain includes the `0x18` control-block
layout and initial `strong=1, weak=1`, all six vtable slots, shared strong
acquire/release, last-strong inline object deletion and pointer clear, weak
acquire/release, last-weak control-block deletion, and the observed vector
copy/grow/erase/stop/destructor refcount calls.

This does not close the promotion contract. Last strong release destroys the
object inline on an as-yet-unconstrained caller thread; callback-retained refs,
reader drain and rollback release are not universally closed; and no `uint16`
overflow/underflow or double-release exclusion was proved. Consequently the
successor correctly retains this item as UNKNOWN despite the mechanical ABI
subchain.

The seven non-targeted v1 items also remain exactly UNKNOWN, so
`missingEvidence` contains all original ten virtual-device gaps.

## Compressed execution record

After the final REVERSE path/hash and proof anchors were sealed, the four-group
fixture suite was run once before candidate publication:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v2\test_virtual_device_ownership_lifecycle_decision_v2.py
```

Exit code: `0`. Result: `Ran 4 tests in 0.244s`, `OK`.

The four groups covered one target/hash/three-proof-anchor batch, one
create-new successor, one deterministic verify-existing case that accepts a
conservative UNKNOWN rather than forcing promotion, and one parameterized
negative batch. The negative batch rejected ctor/dtor-only shape, a missing
owner boundary, incomplete strong/weak last release, missing failure/early
return/rollback coverage, promotion of any fixed remaining UNKNOWN, permission
or unlock escalation, wrong predecessor hash, wrong proof anchor, and output
overwrite. The suite was not rerun after candidate publication.

The fixed PE create-new command was run once:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1 `
  -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" `
  -RepositoryRoot . `
  -Output .\out\cs-t0\virtual-device-ownership-lifecycle-verification-20260908-v2.json
```

Exit code: `0`. The new JSON reported `verificationStatus=PASS`,
`workPackageStatus=BLOCKED_BY_EVIDENCE`, zero promotions, ten retained unknowns,
eight predecessor bindings, 17 anchors, all permissions false, and all unlocks
false.

The fixed PE verify-existing command was then run once without `-Output`:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1 `
  -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" `
  -RepositoryRoot .
```

Exit code: `0`. It returned the same decision hash, zero-promotion result, and
blocking state without requesting an output write.

## Read-only and omitted-work boundary

The verifier opened only disk files. No game process was enumerated or opened;
the game was not started; no IPC, debugger, injection, hook, input, virtual
device, or runtime/world mutation was created; and no active authorization was
requested or consumed. The only verifier mutation was the explicitly requested
create-new JSON. This report is the only additional TEST document.

TEST did not run the v1 suite, Cargo full tests, the full T0 build, Native
Debug/Release matrices, historical broad regression, a live canary, or a
repeated v2 candidate-stage fixture suite.
