> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this reachable memset exact-range independent audit v5

## Decision

**NOT_ACCEPTED**.

The frozen v4 artifact is structurally reproducible, but it does not provide
the row-local range evidence required to promote its exact 78 formerly
`UNKNOWN` rows.  The independent audit accepted only the 11 rows whose
`memset` is statically non-executable.  The other 67 executable rows remain
insufficient; one of them, RVA `0x0066AFC0`, contains a directly contradicted
object-source/extent premise.

Therefore this audit does not accept the reachable-`memset` partition, does
not permit whole-image alias promotion, and does not permit manager-this
promotion.

## Frozen inputs and outputs

| Artifact | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| `out/cs-t0/native-join-manager-this-bulk-memset-v4.json` | `0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C` |
| `out/cs-t0/analyze-manager-this-bulk-memset-v4.py` | `81C1FB99E8600D722A930C32C67910F17FDF68506BFAF45B30C456475AB1A101` |
| `out/cs-t0/analyze-manager-this-bulk-memset-range-audit-v5.py` | `7DFE2FD1C80E0047D6713C2099306D6B4D611B1D891FA288DF00821728F43BFD` |
| `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |

The read-only IDA session was bound to observed executable SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`,
image base `0x00400000`, and `.text` range
`[0x00401000,0x00B18000)`.

## Structural and reproducer checks

The audit independently confirmed:

- v3 `UNKNOWN` RVAs and v4 `newlyClosedSinceV3` RVAs are exactly equal,
  with cardinality 78;
- all 164 inherited closed rows are object-equal;
- the inherited gap partition is exactly `64/13/1`;
- all v4 permission values remain false.

The existing v4 reproducer was replayed without creating or changing its
output:

```text
PASS existing=out\cs-t0\native-join-manager-this-bulk-memset-v4.json sha256=0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C closed=242 new=78 unknown=0 overlap=0 inherited_closed_unchanged=164
```

That replay proves structural determinism only.  The reproducer copies one
shared narrative per proof family and sets completeness booleans; it does not
calculate or verify, per executable row, the object/array producer, start
offset, fill-count upper bound, available extent, complete caller set, and
`offset + count <= extent` inequality.

## Independently accepted non-executable rows

The following 11 rows need no storage-range certificate because the fill call
cannot execute in the complete local call universe:

- `0x0001CC2C`: all three callers pass `a1==a3` and `a2==a4`, making the
  outer mismatch predicate false;
- `0x00436AE6`, `0x00436B38`, `0x00436B92`, `0x00436BD2`, `0x00436C21`,
  `0x00436C72`, `0x00436D51`, `0x00436E2A`: the two XML roots enter with
  `(a3,Size)=(0,0)`, the complete recursive forwarders preserve the pair,
  and every fill requires `Size>0`;
- `0x006E5F42`, `0x006E6164`: the stored-byte predicates in
  `sub_AE5E40` and `sub_AE5FF0` contradict the respective branch conditions.

These local acceptances do not make exact78 acceptable.

## Directly contradicted premise: `0x0066AFC0`

V4 identifies the receiver of `memset@0x00A6AFC0` as a 2,664-byte GLFW
TLS/static object.  Read-only IDA shows otherwise:

1. `sub_A6A9B0` obtains `v9` from `psub_837010(808, allocator)` and clears
   exactly `0x328` bytes.
2. On failure it calls `sub_A6AF90(v9)`.
3. The complete five direct callers of `sub_A6AF90` pass the same GLFW window
   identity family via `this[47]`, `v6`, the `i_3` window list, `v9`, and
   `dword_C7999C`.
4. The actual fill is offset 624, count 68, extent 808, so
   `624 + 68 = 692 <= 808`.

The actual interval is bounded, but v4's producer and extent premise is false.
It must be replaced by the 808-byte window-object constructor/caller proof;
an inequality that happens to remain true cannot validate a false object
identity claim.

## Additional concrete open premises

- `0x006E6F3B`: `sub_AE6EE0` has two direct calls from `sub_AED000` **and**
  a data xref at `0x00BF8000`, where it is the first allocator dispatch-table
  entry.  V4 neither enumerates the indirect table consumers nor proves that
  every invocation supplies the matching block metadata whose `+24` capacity
  covers `buf`.
- `0x00686BD1`: the count is
  `byte_BA5964[n16] + decoded_bitfield`.  V4 states `Size<=460` without
  proving the `n16` domain, table-value maximum, decoded bit width, or the
  row-local inequality against `10992-(10532+i)`.

## Exact insufficient RVA set (67)

Every executable row below lacks a row-local, evidence-bound range certificate.
A proof-family narrative, an address-domain label, or zero observed overlap was
not accepted as a substitute.

| Proof family | RVAs |
|---|---|
| `V4_ALLOCATOR_BLOCK_USABLE_SIZE` | `0x006E6F3B`, `0x006ED13B` |
| `V4_ALLOCATOR_SEGMENT_METADATA_EXTENT` | `0x006EC167` |
| `V4_CALLER_CAPACITY_VECTOR_CLOSURE` | `0x0001D1D9`, `0x0024CF7A`, `0x0029FA85`, `0x003DE449` |
| `V4_CHECKSUM_FOUR_BYTE_TAIL` | `0x00019701`, `0x005CACD4`, `0x005D8384` |
| `V4_FIXED_640_BYTE_DESTINATION` | `0x0062C9CF` |
| `V4_FIXED_MEMBER_OBJECT_UNIVERSE` | `0x0034FC5D`, `0x005ADB94`, `0x005CF2B3` |
| `V4_FORMAT_BUFFER_CAPACITY_CLOSURE` | `0x0062C316` |
| `V4_FRESH_VECTOR_EXACT_ALLOCATION` | `0x0068DF56` |
| `V4_GLFW_TLS_OBJECT_EXTENT` | `0x0066AFC0` |
| `V4_LIBPNG_EXACT_STRUCT_ALLOCATION` | `0x006630F1`, `0x00664114` |
| `V4_MINIZ_COMPRESS_STATE_EXTENT` | `0x00686514`, `0x00686524`, `0x00686621`, `0x006866EC`, `0x00686BD1` |
| `V4_MINIZ_DECOMPRESS_STATE_EXTENT` | `0x00687822`, `0x0068783A`, `0x0068857A`, `0x0068858A`, `0x00688AD7` |
| `V4_N298_LARGE_LAYOUT_CLOSURE` | `0x005C7A15`, `0x005C7BAD`, `0x005C7C8D`, `0x005C7E7A`, `0x005C7EC6`, `0x005C7ED6` |
| `V4_PERSISTENT_DATA_CALLER_AND_RECEIVER_CLOSURE` | `0x0051B6A7`, `0x005262DD`, `0x005262F0`, `0x00526342`, `0x00526352`, `0x005263B5` |
| `V4_PNG_ZLIB_MEMBER_ALLOCATION` | `0x00693577`, `0x006956E8` |
| `V4_SSO_AND_RECORD_APPEND_CLOSURE` | `0x000148AA`, `0x00025A3F` |
| `V4_STANDARD_VECTOR_TUPLE_CLOSURE` | `0x00308B5A`, `0x00325160`, `0x003271E2`, `0x003272FC`, `0x00330FE1`, `0x00331054`, `0x003310C4`, `0x0033112F`, `0x003350C6`, `0x0033685C`, `0x003DC995`, `0x003DCD16`, `0x005CDDEF`, `0x005D2C27`, `0x0061AC2C` |
| `V4_THEORA_STATE_AND_PLANE_GEOMETRY` | `0x006DCB1E`, `0x006DEF41`, `0x006DF552`, `0x006E10CD`, `0x006E10DE`, `0x006E168B`, `0x006E181F` |

For each of these rows a replacement evidence artifact must bind, rather than
merely assert, all of the following: object/array source; byte start offset;
maximum fill count; available extent; every direct, indirect, split-tail, and
forwarded caller; and the final arithmetic inequality.  This includes the
requested `n298`/`n60`/`sub_9262B0`, codec, mimalloc, container, and
caller-parameter cases.

## JSON semantic mutation checks

The v5 verifier executed ten in-memory negative cases.  All 10 were rejected:

1. remove a newly closed row;
2. duplicate a newly closed row;
3. change a newly closed RVA;
4. change the site-instance function RVA;
5. change a fill-length expression;
6. enable a permission;
7. modify an inherited closed row;
8. clear a completeness flag;
9. change the summary count;
10. inject an incomplete range certificate containing only an asserted
    inequality.

The last case is important: adding a new label does not satisfy the range
contract unless it binds every required premise.

## Boundaries

No game was started; no process was opened or read; no debugger was attached;
no injection, network operation, IDB write, production edit, test edit,
v1-v7 edit, or index/status/plan edit occurred.  All permission flags in the
audit remain false.
