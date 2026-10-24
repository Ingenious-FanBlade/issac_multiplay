> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 standard-vector group C exact-five v14 handoff

## Verdict

This offline, fail-closed package independently replays the five group-C
`memset` rows against the canonical PE.  The result is **0 VERIFIED / 5
UNKNOWN**.  No row is counted, manager-this is not changed, and every
permission/promotion flag remains false.

The v11 exact15 artifact is frozen only as the row-local range input.  Its
UNKNOWN narrative is not adopted.  Corrected executable component v2 is bound
only for executable-layout/split-address context and does not close any vector
tuple producer.

## Exact rows and first gaps

| RVA / VA | Replayed local tuple and call relation | First unclosed frontier | Result |
|---|---|---|---|
| `0x003DC995` / `0x007DC995` | `record+0x1D4` begin/end/capacity; callers `0x7B188C`, `0x9D1695`; stride 8; grow `sub_737F40` bypasses the target | `RECORD_PLUS_1D4_PARENT_PREOFFSET_ALIAS_WRITER_FIXED_POINT_OPEN` | UNKNOWN |
| `0x003DCD16` / `0x007DCD16` | byte-vector begin/end/capacity; exact 15 direct callers; stride 1; grow `sub_7DEDB0` bypasses the target | `OWNER_PLUS_1FD4_FIRST_UNCLOSED_PARENT_ALIAS_LIFETIME` (`sub_755470` forms `owner+0x1FD4/+0x1FE0`, but the address-taken owner lifetime is open) | UNKNOWN |
| `0x005CDDEF` / `0x009CDDEF` | receiver `+0x1E79C/+4/+8`; exact 3 direct callers; constructor/reset/snapshot/grow/destructor literal families classified | `OBJECT_PLUS_1E79C_PARENT_PREOFFSET_ALIAS_FIXED_POINT_OPEN` | UNKNOWN |
| `0x005D2C27` / `0x009D2C27` | record `+0x518/+4/+8`; exact 7 direct callers; stride 16; constructor and grow anchors replayed | `RECORD_PLUS_518_ALREADY_OFFSET_ALIAS_WRITER_FIXED_POINT_OPEN` | UNKNOWN |
| `0x0061AC2C` / `0x00A1AC2C` | receiver `+0x2C` / stack staging tuple; exact 17 direct callers; stride 16 | `ADDRESS_TAKEN_SUB_85E470_RECEIVER_OBJECT_LIFETIME_OPEN` | UNKNOWN |

For every row, PE instructions establish the conditional in-place inequality
`0 < requested-old <= capacity-old`, destination `oldEnd`, length
`stride*(requested-old)`, and end publication after `memset`.  That local fact
does not exclude an unclassified producer installing a locally ordered tuple
whose begin/end are `0x00C71678` and capacity covers the four-byte manager
slot.  Each JSON row records that concrete slot-covering countermodel.

## Universe checks

- Raw whole-`.text` E8 scans reproduce the five exact direct-caller lists.
- Each target function has zero raw absolute VA and zero raw RVA occurrence.
- The full raw literal census for `0x1E79C`, `0x1E7A0`, and `0x1E7A4` is frozen
  and classified into constructor zero, destructor/free, reset, snapshot
  copy/restore, serialization reads, and the target resize.  This census does
  not exclude a no-literal already-offset or parent/pre-offset alias.
- `sub_A2C6E0` has exactly two direct callers (`0xA2C2B7`, `0xA2C3D1`); both
  zero-stack-tuple paths are classified.
- `sub_85E470` has no direct E8 caller and exactly two absolute-VA file
  occurrences (`0x4662F8`, `0x49EDF7`).  It forwards ECX into `sub_A1A730`, so
  its address-taken receiver/object lifetime is the first decisive gap.
- Frozen instruction/helper/constructor windows are SHA-256 checked before a
  document can be emitted or verified.

## Reproducer and deterministic checks

Analyzer:

`out/cs-t0/analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py`

Machine JSON:

`out/cs-t0/native-join-manager-this-fill-standard-vector-group-c-exact-five-v14.json`

Commands executed in order:

```powershell
python out\cs-t0\analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py
python out\cs-t0\analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py --verify-existing
python out\cs-t0\analyze-manager-this-fill-standard-vector-group-c-exact-five-v14.py --verify-existing
```

All three runs produced JSON SHA-256
`6866380775B9D82B2D6A2C9377D613B62FC4E843BF629C84CA5AF02966DA2A63`.
Both verify-existing runs were byte-identical.  The analyzer rejects 25
semantic mutations: per-row status, first gap, direct caller set, and
slot-countermodel mutations, plus permission, summary, target, exact-set, and
external-analyzer promotion mutations.

The bounded IDA request did not return a passing result.  It is recorded as
`TIMEOUT_NO_PASS` and is not used for the verdict.  No retry or running-program
access was performed.

## Frozen bindings

- v11 analyzer: `B9A4C746B11FAAF22B407B4124EBD30FD7F43D34F217C997EEE8150C9BB054DD`
- v11 JSON: `6580AB34B5050DEABE1FCE60E1EBDC581531A8F8C3A3498E9E3BADC2F65637F6`
- v11 handoff: `C80434F31883EBD3DE5A1799EE834647B1AC35E618ACD34BB40247F90A82C674`
- corrected executable component v2 analyzer: `A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6`
- corrected executable component v2 JSON: `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`
- target PE: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

Analyzer SHA-256:
`E2938C36A1BBC86B46072AD82C8F0520983C2196A3E1FA21DB7D82875EA1DDA8`

JSON SHA-256:
`6866380775B9D82B2D6A2C9377D613B62FC4E843BF629C84CA5AF02966DA2A63`

Final acceptance/replay remains assigned to an independent tester; this
handoff is the static reverse/create-new evidence package only.
