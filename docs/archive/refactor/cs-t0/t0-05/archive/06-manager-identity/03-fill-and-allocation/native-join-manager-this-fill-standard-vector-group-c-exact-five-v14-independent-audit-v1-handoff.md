> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 standard-vector group C exact5 v14 independent audit v1

## Independent decision

- Role: independent tester; this role did not author the v14 candidate.
- Candidate analyzer, JSON, and handoff were read-only throughout the audit.
- Audit result: **PASS_CANDIDATE_NEGATIVE_EXACT5_FAIL_CLOSED**.
- Accepted disposition: **0 VERIFIED / 5 UNKNOWN**.
- Accepted positive closure count: `0`.
- `overallAccepted`, row promotion, manager-this promotion, and whole-image promotion all remain `false`.
- No game, process, debugger, runtime read, IDB mutation, production/test modification, status/index update, or candidate modification occurred.

## Candidate identity and immutability

| Candidate artifact | SHA-256 before and after audit |
| --- | --- |
| `out/cs-t0/analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py` | `E2938C36A1BBC86B46072AD82C8F0520983C2196A3E1FA21DB7D82875EA1DDA8` |
| `out/cs-t0/native-join-manager-this-fill-standard-vector-group-c-exact-five-v14.json` | `6866380775B9D82B2D6A2C9377D613B62FC4E843BF629C84CA5AF02966DA2A63` |
| `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-fill-standard-vector-group-c-exact-five-v14-handoff.md` | `73FFC3D7F920A90FF8855B71CF9D7B25AC7D0F528DA3FFBF23CFB637FDBD39F6` |

The candidate's five frozen v11/component-v2 bindings were also recomputed and matched. The target PE is PE32/I386, image base `0x00400000`, `SizeOfImage=0x008FE000`, SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

## Independent PE findings

The auditor does not import the candidate analyzer. It independently reads the PE and verifies each exact call, `memset` target, grow-transfer call/target, local tuple anchor, end-publication anchor, raw whole-text E8 owner census, and selected literal/address-taken censuses.

| RVA | Independent local result | Independently retained first gap | Verdict |
| --- | --- | --- | --- |
| `0x003DC995` | Stride-8 in-place resize; exact raw direct callers `0x007B188C`, `0x009D1695`; grow transfer to `0x00737F40` bypasses the target. | `RECORD_PLUS_1D4_PARENT_PREOFFSET_ALIAS_WRITER_FIXED_POINT_OPEN`: both direct callers form `+0x1D4`, but no-literal interior/pre-offset aliases and all record-lifetime writers are not excluded. | UNKNOWN |
| `0x003DCD16` | Stride-1 resize; exact raw E8 set has 15 entries; grow transfer to `0x007DEDB0`. | `OWNER_PLUS_1FD4_FIRST_UNCLOSED_PARENT_ALIAS_LIFETIME`: `0x007575FB/0x00757627` form owner `+0x1FD4/+0x1FE0`, but the parent lifetime and no-literal aliases remain open. | UNKNOWN |
| `0x005CDDEF` | Stride-8 receiver `+0x1E79C` resize; exact raw E8 set has 3 entries; grow transfer to `0x009D88A0`. | `OBJECT_PLUS_1E79C_PARENT_PREOFFSET_ALIAS_FIXED_POINT_OPEN`: the complete raw literal census cannot exclude an already-offset or parent/pre-offset pointer that contains no displacement literal. | UNKNOWN |
| `0x005D2C27` | Stride-16 record `+0x518` resize; exact raw E8 set has 7 entries; grow transfer to `0x009D8960`. | `RECORD_PLUS_518_ALREADY_OFFSET_ALIAS_WRITER_FIXED_POINT_OPEN`: direct callers and local constructor/grow anchors do not close record-array/pointer aliases. | UNKNOWN |
| `0x0061AC2C` | Stride-16 receiver `+0x2C` resize; exact raw E8 set has 17 entries; grow transfer to `0x00A1CEA0`. | `ADDRESS_TAKEN_SUB_85E470_RECEIVER_OBJECT_LIFETIME_OPEN`: `sub_85E470` has zero raw E8 callers, two absolute-VA file sites (`0x4662F8`, `0x49EDF7`), and forwards ECX to `sub_A1A730`; its receiver lifetime remains open. | UNKNOWN |

For every row, the PE proves only the conditional range:

```text
0 <= oldCount < requestedCount <= capacityCount
destination = begin + stride*oldCount
length = stride*(requestedCount-oldCount)
end = begin + stride*requestedCount <= capacityEnd
```

This does not prove that every incoming non-null tuple is allocator-owned. Until each first gap closes, the locally ordered tuple may still begin at `0x00C71678` with sufficient numeric capacity to cover `[0x00C71678,0x00C7167C)`. Thus retaining UNKNOWN is required.

The independent raw census also reproduced every occurrence of displacements `0x1E79C`, `0x1E7A0`, and `0x1E7A4`. The audit explicitly does not promote that literal census into a decoded split/address-taken/return/indirect fixed point.

## Verification and mutation results

Candidate verification, executed consecutively:

```powershell
python out\cs-t0\analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py --verify-existing
python out\cs-t0\analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py --verify-existing
```

Both runs passed with JSON SHA-256 `6866380775B9D82B2D6A2C9377D613B62FC4E843BF629C84CA5AF02966DA2A63`, `verified=0`, `unknown=5`, candidate mutations `25/25`, all permissions false, and `TIMEOUT_NO_PASS`.

Independent auditor verification:

```powershell
python out\cs-t0\audit-manager-this-fill-standard-vector-group-c-exact-five-v14-independent-v1.py
python out\cs-t0\audit-manager-this-fill-standard-vector-group-c-exact-five-v14-independent-v1.py --verify-existing
python out\cs-t0\audit-manager-this-fill-standard-vector-group-c-exact-five-v14-independent-v1.py --verify-existing
```

Create-new and both consecutive verify-existing runs passed. The independent mutation suite rejected **31/31** changes: per-row status, first-gap, caller-universe, fixed-point, and call-byte mutations plus target, aggregate status, permission, external-analyzer, promotion, and exact-set mutations.

The external analyzer remains `TIMEOUT_NO_PASS`. It was not retried and was not used for the verdict; no PASS is fabricated.

## Independent artifacts

- `out/cs-t0/audit-manager-this-fill-standard-vector-group-c-exact-five-v14-independent-v1.py`
  - SHA-256 `F6C570220B91910F7D6992D62F884A8E716612304E2329539F012E201E298837`
- `out/cs-t0/native-join-manager-this-fill-standard-vector-group-c-exact-five-v14-independent-audit-v1.json`
  - SHA-256 `572463AB3A496F672C3CAEA1734B25AA7A76109F044440C5F38F09A39A4CF235`

No candidate discrepancy was found. This acceptance is strictly limited to the negative `0 VERIFIED / 5 UNKNOWN` package and authorizes no state/index change or manager-this promotion.
