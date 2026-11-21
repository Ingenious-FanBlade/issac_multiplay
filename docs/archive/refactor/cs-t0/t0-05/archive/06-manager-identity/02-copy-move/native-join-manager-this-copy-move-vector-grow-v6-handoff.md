> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy/move vector-grow v6 reverse handoff

## Outcome

This create-new reverse artifact adjudicates the exact 29 rejected-v4 rows whose
`proofClass` is `VECTOR_ASSIGN_AFTER_CAPACITY_CHECK_OR_GROW` without accepting
any rejected-v4 proof conclusion.

- exact rows: 29
- owners: 21
- fully backed static closures: 6
- retained `UNKNOWN`: 23
- confirmed manager-slot overlaps: 0
- intersection with local-hard-v5 exact21: 0
- manager-this promotion: not allowed
- runtime/live authorization: not granted

The six closed rows are:

```text
0x00030A5A 0x00030AE8 0x00030CAC 0x00030D10
0x005E88CE 0x005E8915
```

The 23 fail-closed rows are:

```text
0x00298C7A 0x00298CF5 0x0029B9AB 0x002F83B6
0x002F8B66 0x002F8E5D 0x002F921B 0x00308BCA
0x003421AC 0x003423A1 0x0035261B 0x003AF5C2
0x003B04FF 0x003B06B5 0x003DCAAB 0x004355EA
0x004D87B6 0x0050D3B4 0x0055D09A 0x005BDB80
0x005BDBDB 0x0062C746 0x00667065
```

## Inputs and fixed partitions

| role | path | SHA-256 |
|---|---|---|
| positive baseline | `out/cs-t0/native-join-manager-this-copy-move-local-stack-v3.json` | `73905C2A1D27C12344F82147C4F89781BAC08A447D55D4C6E89D8465BB23626D` |
| negative independent audit | `out/cs-t0/native-join-manager-this-copy-move-local-stack-v4-independent-audit-v1.json` | `3937A58884C84D8A4653612F6C64980C314C191F70D9D5498653BAD06D22CC7C` |
| rejected selector only | `out/cs-t0/native-join-manager-this-copy-move-local-stack-v4.json` | `FFDAD182B2948579A2D5FE6F1BB6BEE7502400B1BD27ADE87CF9276F9FE96265` |
| disjoint exact21 | `out/cs-t0/native-join-manager-this-copy-move-local-hard-v5.json` | `D88E92E8F1D2E89072883FAC04072FD2364FA74B7F22ED02D18DDD9E6E9A68AF` |
| IDA input | `isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

The v4 selector is hash-bound and contributes only `proofClass` plus exact row
identity (`callsite`, owner, helper, call bytes, destination, source, length).
Its `status`, producer/span closure, narrative and proof conclusions are never
consumed.  The reproducer separately opens hash-bound hard-v5 only to extract
its 21 callsite identities and rejects any non-empty intersection.

## Fresh read-only reconstruction

IDA was attached only to the already-open exact `isaac-ng.exe.i64` database.
The target was x86/32 with image base `0x00400000`, text range
`[0x00401000,0x00B18000)` and data range `[0x00BF8000,0x00C9D000)`.  No IDB
write API was used.

For every row the machine artifact records:

1. destination begin/end/cap tuple root;
2. grow or resize helper and element size;
3. allocation-return flow into begin/end/cap;
4. the raw compare/resize branch governing the in-place path;
5. destination offset and copy length relation to capacity;
6. whether caller/constructor/destructor/alias tuple writers are exhaustive.

The reconstructed grow helpers are RVAs `0x001711A0`, `0x002A0D30`,
`0x002A0DC0`, `0x003099A0`, `0x003DEFC0`, `0x00515F50`, `0x005E9020`, and
resize wrapper `0x003DCCD0` through `0x003DEDB0`/`0x003DF050`.  Each allocation
path uses allocator RVA `0x0000CF00`, installs allocation as begin and end, and
installs allocation plus `elementSize*newCapacity` as cap.  Assertion/logging
calls are not used as a control-flow proof.

### Closed 0x00030A5A and 0x00030AE8

`sub_430F50` at RVA `0x000307F3` zeroes the two destination stack tuples.  A
full owner stack-operand scan finds no other pre-copy field writer.  Raw
compares at `0x00030A36` and `0x00030AC4` either establish capacity for 32
16-byte elements or call `0x003DEFC0` on the exact tuple.  Source tuples are
separately zeroed and grown by `sub_A24420` to exactly 32 elements.  Therefore
the exact copies have offset 0, length 512 and capacity at least 512.

### Closed 0x00030CAC and 0x00030D10

IDA image bytes show both global tuples
`[C799E8,C799EC,C799F0]` and `[C799F8,C799FC,C79A00]` initialized to zero.
Executable-wide direct data xrefs for all six fields were enumerated.  They are
limited to the assignment/grow path, read-only consumers, and exit destructor
`0x00716DD0` through tuple clearer `0x000B3950`; no other direct writer exists.
Raw compares at `0x00030C8C` and `0x00030CF0` grow the exact global tuple when
needed.  Their sources are the already closed 512-byte local tuples, so
`offset+length = 0+512 <= capacity`.

### Closed 0x005E88CE and 0x005E8915

Owner `0x005E86E0` has one direct caller at `0x005E7138`.  The caller invokes
initializer `sub_9E6D90` at `0x005E6F3F`, zeroing the aggregate that contains
both source tuples; a full caller stack-operand scan finds no source-tuple write
before the call.  The callee invokes the same initializer at `0x005E8764`,
zeroing both destination tuples.  Thus both exact copy lengths are zero at this
call, while the raw compare/grow paths and grow allocation returns were still
independently reconstructed.  Their ranges are empty: `0+0 <= 0`.

## Why the other 23 remain UNKNOWN

All 23 have their owner-local compare/grow/copy/end-update sequence and grow
helper reconstructed.  Row `0x0062C746` also has its resize chain
`0x003DCCD0 -> 0x003DEDB0 -> 0x003DF050` reconstructed, proving the local
`0+Size<=capacity` relation.  They remain `UNKNOWN` because the destination is
a caller-supplied tuple or an object field and the complete executable-wide
caller/constructor/destructor/alias writer universe is not closed.  Most also
derive length from an external source tuple whose begin/end writer universe is
open.  Address domain, C++ vector convention, family narrative, string anchors,
and non-blocking asserts/logging do not discharge those gaps.

## Artifacts and verification

| artifact | SHA-256 |
|---|---|
| `out/cs-t0/analyze-manager-this-copy-move-vector-grow-v6.py` | `BD42DB3381593926F140F646E5B58D2CD614374404685B5D31C7FE0F193DCB90` |
| `out/cs-t0/native-join-manager-this-copy-move-vector-grow-v6.json` | `DB7512C7DCD1012D40D87626FDA1A8B90F234D7A4A436C8E0C67F6E61D5090DD` |

Verification commands:

```powershell
python out/cs-t0/analyze-manager-this-copy-move-vector-grow-v6.py --self-test
python out/cs-t0/analyze-manager-this-copy-move-vector-grow-v6.py --verify-existing out/cs-t0/native-join-manager-this-copy-move-vector-grow-v6.json
```

Expected result:

```text
PASS exact29 closed=6 unknown=23 overlap=0 hardV5Intersection=0 mutations=87
PASS ... exact29 closed=6 unknown=23 overlap=0 mutations=87
```

The 87 semantic mutations cover selector identity, hard-v5 disjointness,
destination/source provenance, tuple-writer completeness, grow allocation
return, in-place branch, span inequality, raw atoms, UNKNOWN promotion,
aggregates and permissions.  Every mutation fails closed.

## Boundary and acceptance

No game or target process was started or opened.  No debugger was attached.  No
IDB, production code, tests, v1-v7 artifacts, index/status file, or prior
evidence was modified.  This is reverse evidence only; independent test-agent
acceptance is intentionally not self-declared by this handoff.
