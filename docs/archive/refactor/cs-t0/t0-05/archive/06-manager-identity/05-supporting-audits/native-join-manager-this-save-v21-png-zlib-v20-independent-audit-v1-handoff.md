> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 save-record v21 + PNG/zlib v20 independent audit handoff v1

## Verdict

Independent acceptance passed with no author-artifact difference:

- save-record v21: accept the negative exact-one result, `0 verified / 1 unknown`;
- PNG/zlib v20: accept the positive exact-two result, `2 verified / 0 unknown`;
- first difference: none;
- copy/move ledger increment: zero;
- all permissions and promotion flags: false.

This audit did not participate in either reverse-engineering proof and did not modify any of the six
author artifacts. It did not start the game, open/read a process, attach a debugger, or perform any
online/runtime test.

## Author three-piece hashes

| Role | Path | independently recomputed SHA-256 |
|---|---|---|
| save analyzer | `out/cs-t0/analyze-manager-this-copy-save-record-one-v21.py` | `BE65C040507D09CBA4F23C6057ACC54984CD5159DC1FB6944A81C5AC14E03F68` |
| save machine | `out/cs-t0/native-join-manager-this-copy-save-record-one-v21.json` | `A0E9612B212D338C1DC50F9AA859C99F1F14EBB62A7277007E0689026920939C` |
| save handoff | `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-copy-save-record-one-v21-handoff.md` | `A5F16015A838FC53D7EF4E5C522E65749116B7ED99C9028EECDBE36F0B5C221E` |
| PNG/zlib analyzer | `out/cs-t0/analyze-manager-this-fill-png-zlib-two-v20.py` | `4269F1DCB46D394AE2A9173B1FCE0D213A70F52517294BBF8D157ABB96A336E1` |
| PNG/zlib machine | `out/cs-t0/native-join-manager-this-fill-png-zlib-two-v20.json` | `A7292DB5DF505A7ACC8D822D95041D2220913EA5114E6F77ADD2A6AB24C2DDBE` |
| PNG/zlib handoff | `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-fill-png-zlib-two-handoff-v20.md` | `605915BB849773E8599472E1C4449D3C06B4ABA612106A6EF06A1405580D9E1B` |

Each handoff contains the analyzer/machine hashes it claims and the matching expected replay summary.

## Frozen dependency hashes

The union of all nine machine bindings was independently re-read from disk and matched:

| Path | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-copy-backing-critical-two-v10.json` | `0A5834505E79B4575A2DF53C86FE647E3F6A83E3E8FD4328369C11CC257A65FF` |
| `out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json` | `C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D` |
| `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json` | `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981` |
| `out/cs-t0/native-join-manager-this-manager-tuple-global-seal-v20.json` | `E1D42FABAF1651B01BCC1C7005A2958C0510B26F29BE361A2FE8F71AA44D58C9` |
| `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| `out/cs-t0/native-join-manager-this-bulk-memset-v4.json` | `0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C` |
| `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-whole-image-alias-handoff-v5.md` | `892640B97BEB9C727CEC90FDE7F19F86441DBBD255486C5269B70ABA909D9F28` |
| `out/cs-t0/native-join-manager-this-fill-libpng-two-v18.json` | `2AA38CF80A19BA90EA53E6E7FB1FB393FEECF6CA4A34615A664E1CD93BBC49AE` |

Target `isaac-ng.exe` also matched
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

## Save-record v21 checks

The same author script was executed without importing it into the independent auditor:

```text
PE --self-test:       PASS PE exact1 verified=0 unknown=1 candidates=42 mutations=32
PE verify run 1:     PASS PE exact1 verified=0 unknown=1 candidates=42 mutations=32
PE verify run 2:     PASS PE exact1 verified=0 unknown=1 candidates=42 mutations=32
current IDB replay:  PASS IDA exact1 verified=0 unknown=1 candidates=42 mutations=32
```

The PE and IDA modes reproduced the same existing JSON payload. The independent auditor additionally
validated all 11 target function blobs directly against the PE and against current IDA function
boundaries/bytes.

Exact scope remained `{0x001CBEB5}` and exclusion remained `{0x005B91EF}`. The four direct callers
were exactly `0x005C49FD, 0x005C4A8E, 0x00688D38, 0x00688E1F`.

The classified candidate ledger was `34` stride-`0x5DC` plus `8` parent-size-`0x20660`, with
`42/42` classified. Each frozen candidate anchor exists in the target and each belongs to a current
IDA function.

The real record layout was preserved exactly:

```text
record+0x500 = allocator vector begin
record+0x504 = allocator vector end
record+0x508 = allocator vector capacityEnd
record+0x50C = separately serialized scalar
```

Therefore the requested receiver at `record+0x504` is not a begin/end/capacity tuple: its `+0/+4/+8`
are actual end, actual capacityEnd, and the scalar. The negative status is correct.

Copy/move counts remained:

```text
before=315, acceptedNewClosure=0, after=315, total=405, remaining=90
```

The accepted global seal v20 independently agrees that this row is `UNKNOWN` and the closed count is
315.

## PNG/zlib v20 checks

The author analyzer was replayed twice against the current IDB:

```text
PASS verified=... sha256=A7292DB5DF505A7ACC8D822D95041D2220913EA5114E6F77ADD2A6AB24C2DDBE rows=2 verified=2 unknown=0 mutations=24/24 permissions=false promotions=false
PASS verified=... sha256=A7292DB5DF505A7ACC8D822D95041D2220913EA5114E6F77ADD2A6AB24C2DDBE rows=2 verified=2 unknown=0 mutations=24/24 permissions=false promotions=false
```

The independent auditor checked all 16 function blobs in both PE and current IDA, exact call bytes,
and these universes:

- PNG `sub_A93480`: eight direct/tail callers; complete nonzero member writer set has three entries
  (two exact-`S` fresh allocations and their exact-peer swap);
- zlib `sub_A955E0`: three direct callers; complete nonzero `state+0x3C` writer set has one allocation
  writer, with the bound custom allocator clearing exactly `Size`.

Both rows have empty data/indirect/split-tail/EH-forwarder sets outside the enumerated callers.

Independent arithmetic checks reproduced:

```text
PNG:  S=((uint32 precursor + 7)>>3)+1, 1 <= S <= 0x20000000
      start=0, count=S, extent=S, so 0+S<=S without wrap

zlib: a5 in [1,9], field68=1<<(a5+7)
      512 <= 2*field68 <= 131072
      start=0, count=2*field68-2, extent=2*field68
```

Both destinations are successful fresh heap allocations disjoint from the PE static slot range.
The v4/v5 source partition `V4_PNG_ZLIB_MEMBER_ALLOCATION` is exactly the same two RVAs.

## Partition, schema, and IDB audit

- copy exact partition: `{0x001CBEB5}`;
- memset exact partition: `{0x00693577, 0x006956E8}`;
- copy/memset intersection: empty;
- save exact/excluded intersection: empty;
- both JSON schema IDs, work package, `createNewOnly`, row classifications, mutation counts and handoff
  bindings matched exactly.

The IDB file SHA before all author replays was
`992A16DC789B5714C337CDA34E135EF2DDC8CFE3A4F32378673B20B071F8909F`; after the save replay, two
PNG replays and the independent IDA auditor replay it remained the same hash. No IDB mutation API was
called.

## Independent audit artifacts and reproduction

- machine: `out/cs-t0/native-join-manager-this-save-v21-png-zlib-v20-independent-audit-v1.json`
  - SHA-256 `FC05622D191C056E5D93155C880D88252A37EE7D348A0A504277D3E8E3441311`
- auditor: `out/cs-t0/audit-manager-this-save-v21-png-zlib-v20-independent-v1.py`
  - SHA-256 `A289D7ADA6104B308D80123A38A7E44961894B5E60907F2BEECD77D137569BB7`

PE reproduction:

```powershell
python out\cs-t0\audit-manager-this-save-v21-png-zlib-v20-independent-v1.py --self-test
python out\cs-t0\audit-manager-this-save-v21-png-zlib-v20-independent-v1.py --verify-existing
python out\cs-t0\audit-manager-this-save-v21-png-zlib-v20-independent-v1.py --verify-existing
```

Current IDB reproduction via `py_exec_file`:

```text
D:\projects\isaac_hook_reserach\out\cs-t0\audit-manager-this-save-v21-png-zlib-v20-independent-v1.py
```

Observed in both modes:

```text
PASS ... independent-audit save=0V/1U png=2V/0U sha256=FC05622D191C056E5D93155C880D88252A37EE7D348A0A504277D3E8E3441311 mutations=20/20 idb-unchanged=true promotions=false
```

The independent audit's 20 fail-closed mutations cover author/dependency hashes, handoff bindings,
schemas, replays, save candidate/layout/ledger/status, PNG scopes/universes/ranges/classification,
partition intersections, IDB immutability, and all permissions/promotions.
