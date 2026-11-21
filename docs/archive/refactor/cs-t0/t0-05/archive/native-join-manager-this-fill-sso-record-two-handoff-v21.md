> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 SSO/record exact-two handoff v21

## Decision

Read-only static analysis of proof group `V4_SSO_AND_RECORD_APPEND_CLOSURE` is complete at the current evidence boundary:

| RVA | VA | owner | result |
|---|---|---|---|
| `0x000148AA` | `0x004148AA` | `sub_414870` | `UNKNOWN` |
| `0x00025A3F` | `0x00425A3F` | `sub_4259B0` | `VERIFIED_STATIC_RECORD_EXTENT_NO_SLOT_OVERLAP` |

Exact result: `1 verified / 1 unknown`. `overallAccepted=false`. This artifact grants no row, whole-image, or manager-this promotion and authorizes no merge by itself.

## Frozen boundary

- v3 SHA-256 `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` binds both input rows as `UNKNOWN`;
- rejected v4 SHA-256 `0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C` is used only to prove that `V4_SSO_AND_RECORD_APPEND_CLOSURE` is exactly these two RVAs; none of its narrative or closure booleans is factual proof;
- range audit v5 SHA-256 `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` binds the same exact-two insufficient set;
- corrected whole-image v2 SHA-256 `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981` and handoff v5 SHA-256 `892640B97BEB9C727CEC90FDE7F19F86441DBBD255486C5269B70ABA909D9F28` bind target SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`, image base `0x00400000`, executable range `[0x00401000,0x00B18000)`, and slot `[0x00C71678,0x00C7167C)`.

## Row `0x000148AA`: conditional SSO bound, concrete writer gap remains

`sub_414870` receives a 24-byte MSVC SSO string object: the first 16 bytes are inline storage or a heap pointer, `+0x10` is size, and `+0x14` is capacity. At the target, the exact arguments are:

- receiver storage: inline `this` when `capacity<16`, otherwise heap pointer `[this]`;
- destination: `storage+oldSize`;
- value: sign-extended byte `Val`;
- count: `Size`.

The target is only on the unsigned branch `Size <= capacity-oldSize`. For a valid SSO object this proves `capacity>=oldSize`, `oldSize+Size<=capacity`, and the following NUL write at `oldSize+Size` is also in range. The growth branch is separate: it rejects `Size>0x7fffffff-oldSize`, selects capacity at most `0x7fffffff`, allocates `newCapacity+1`, and reaches a different memset.

The common lifecycle is byte/hash bound: `sub_40CF50` copy-constructs inline or allocates `cap+1`; `sub_40CCD0` assigns while preserving that representation; `sub_40D040` frees heap storage and restores `{size=0,cap=15,inline[0]=0}`. The exact `sub_414870` xref universe is:

`{0x00414545 tail, 0x004D17FF, 0x0051E323, 0x0082AF51, 0x008C90D4, 0x008CD5F2, 0x0091F3B0, 0x009E9E0F}`.

`sub_414520` is the only tail forwarder and has exactly `0x0047F2FE/0x0047F3BE`. The stack-local paths, the `92`-byte record `+20` string path, the exact `0x4D088` online-root `+0x4B0F0` path, and the direct/data/bridge invocation universe of the by-value string in `sub_9E9DB0` were enumerated. The `0x008CD5F2` call passes `Size=0xffffffff`; the maximum-size guard diverts it before this in-place target.

First missing gap: `0x0091F3B0` passes a `UserProfileBase`-derived `this+0x0C` SSO field from the unique `0x0091E4B7` caller. Its destructor is known (`sub_91DB20 -> sub_40D040`), but an exact all-constructor/all-assignment/all-alias-writer universe for `this+0x0C` across every derived-object creation/copy path was not closed. Therefore the incoming SSO invariant is not a concrete-object theorem for that path, and the row remains `UNKNOWN`.

## Row `0x00025A3F`: exact 104-byte record append

`sub_4259B0` receives a vector tuple `{begin,end,capacityEnd}` with stride `104` (`0x68`). It creates a stack source at `EBP-0x70`; the function reserves 108 usable bytes from that address to the cookie boundary, and `ECX=26; rep movsd` copies exactly 104 bytes. Thus source range `0+104<=108` is closed.

Two append paths converge before the target:

1. If `end!=capacityEnd`, the function copies 104 bytes to `end` and advances `end` by 104.
2. If `end==capacityEnd`, the unique call `0x00425A2F` enters `sub_426640`. That helper caps capacity at `0x02762762=floor(UINT32_MAX/104)`, so the largest multiplication is `0x02762762*104=0xFFFFFFD0`. It allocates `104*newCapacity`, copies the 104-byte source record and every old whole record, frees the old exact-capacity allocation, and installs aligned begin/end/capacity pointers at `0x004267C5/C9/D3`.

After either path, let the appended record be `[newEnd-104,newEnd)`. The target clears `newEnd-92` for `88` bytes, hence within-record `start=12`, `count=88`, `end=100`, and `12+88=100<=104`. The following DWORD store is at record offset `8`.

### Tuple writer and caller closure

- constructor `0x006F172A` forms `GameRoot+0x1BBE0`, and `0x006F1733/39/40` zero begin/end/capacity;
- `sub_426640` is the sole grow installer;
- `sub_4254B0` moves only whole 104-byte records and decrements end by exactly 104;
- `0x006F5379` and `0x0090CB60` only reset end to begin;
- `sub_425430 -> sub_4265D0` frees exact backing extent and zeros the tuple; `sub_AFB599` is its only tail/EH forwarder;
- no tuple writer adopts or replaces the backing pointer with foreign storage.

The complete target-function xref universe is `{0x00425850,0x00425B34,0x006FDEE0,0x0078FFF9,0x009184E5}` with no data, indirect, split-tail, or EH callers. `sub_4257B0` has the unique `0x006FBA5D` caller. `sub_425AC0` has exactly 27 calls; every receiver is `GameRoot+0x1BBE0` directly or through `sub_65D090`, whose exact three-call universe returns that same member. The reproducer also binds all 58 raw `E0 BB 01 00` occurrences in `.text` and all 54 decoded `+0x1BBE0` instructions.

The target is reachable only after a record exists. The first append allocates backing storage through `sub_40CF00`; subsequent writers preserve that ownership. The destination is therefore vector heap backing, disjoint from the PE static slot mapping.

## Artifacts and replay

- analyzer: `out/cs-t0/analyze-manager-this-fill-sso-record-two-v21.py`
  - SHA-256 `7FA5C8DE7949FFDAB4C06F1CD071561FC18724DCBA4BE8D9B1268DCB29D43DB3`
- machine report: `out/cs-t0/native-join-manager-this-fill-sso-record-two-v21.json`
  - SHA-256 `2244FE2E8CDC9CDA2012D309E1DD6D3998E988D94AD19C717990C7F868DE3EF5`

Ordinary PE mode was run twice:

```text
PASS mode=pe created=out/cs-t0/native-join-manager-this-fill-sso-record-two-v21.json sha256=2244FE2E8CDC9CDA2012D309E1DD6D3998E988D94AD19C717990C7F868DE3EF5 rows=2 verified=1 unknown=1 mutations=24/24 permissions=false promotions=false overallAccepted=false
PASS mode=pe verified=out/cs-t0/native-join-manager-this-fill-sso-record-two-v21.json sha256=2244FE2E8CDC9CDA2012D309E1DD6D3998E988D94AD19C717990C7F868DE3EF5 rows=2 verified=1 unknown=1 mutations=24/24 permissions=false promotions=false overallAccepted=false
```

The same analyzer was then replayed inside the approved IDA database, read-only, and verified byte-identical output:

```text
PASS mode=ida-read-only verified=out/cs-t0/native-join-manager-this-fill-sso-record-two-v21.json sha256=2244FE2E8CDC9CDA2012D309E1DD6D3998E988D94AD19C717990C7F868DE3EF5 rows=2 verified=1 unknown=1 mutations=24/24 permissions=false promotions=false overallAccepted=false
```

The 24 fail-closed sentinels cover frozen hashes, target/range/scope, both call bytes, row-1 status/gap/xrefs/conditional range, row-2 source/record/grow/writer/caller/displacement/slot facts, exact classification, permissions, promotions, and `overallAccepted=false`.

## Safety ledger

`gameStarted=false`, `debuggerAttached=false`, `processOpened=false`, `processRead=false`, `idbModified=false`, `productionModified=false`, `testsModified=false`, `priorEvidenceModified=false`, `runtimeMutationAllowed=false`, `promotionAllowed=false`, `overallAccepted=false`.

No game, debugger, process read, runtime probe, or online test was used.
