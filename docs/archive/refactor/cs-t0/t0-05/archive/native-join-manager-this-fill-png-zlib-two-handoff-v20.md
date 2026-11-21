> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 PNG/zlib member allocation exact-two handoff v20

## Decision

The two remaining rows in rejected-v4 proof group `V4_PNG_ZLIB_MEMBER_ALLOCATION` are now independently closed by row-local, read-only static evidence:

| RVA | VA | owner | result |
|---|---|---|---|
| `0x00693577` | `0x00A93577` | `sub_A93480` | `VERIFIED_STATIC_EXACT_ALLOCATION_NO_SLOT_OVERLAP` |
| `0x006956E8` | `0x00A956E8` | `sub_A955E0` | `VERIFIED_STATIC_EXACT_ALLOCATION_NO_SLOT_OVERLAP` |

Exact result: `2 verified / 0 unknown`.  This artifact permits merging only these two rows into the memset partition.  It does not promote the whole-image alias result or manager-this.

## Frozen-source boundary

- range-audit v5 is authoritative for the exact insufficient set; its `V4_PNG_ZLIB_MEMBER_ALLOCATION` array is exactly these two RVAs;
- rejected v4 is used only for proof-group set membership; none of its narrative or boolean closure flags is proof here;
- corrected whole-image v2 and handoff v5 bind the corrected executable range `[0x00401000,0x00B18000)` and reachable callsite membership;
- accepted `fill-libpng-two-v18` is bound only as a method-shape comparison, never as factual evidence for these rows.

All factual conclusions below are bound directly to target SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`, exact function hashes, instruction bytes, and xref universes in the reproducer.

## Row `0x00693577`: PNG scanline member

### Containing receiver and member writers

`sub_A63A10` allocates exactly `0x20C` bytes at `0x00A63A1E/0x00A63A3B` and clears exactly `0x20C` at `0x00A63A46/0x00A63A4E`.  Its unique callsite is `0x009E871C` in `sub_9E86E0`; that function retains the result as its local PNG write receiver and uses it synchronously through header/row production and cleanup.  The local receiver does not escape into the read-mode pipeline.

For this write-mode lifetime, the complete relevant `Buf+0xE0` / `Buf+0xE4` writer set is:

1. constructor zero from the whole `0x20C` clear;
2. `0x00A932F8`: `malloc(S)` into `Buf+0xE4`;
3. `0x00A9334F`: a distinct `malloc(S)` into `Buf+0xE0`;
4. `0x00A94C0B..0x00A94C21`: swap only those two exact-`S` peers.

Here both allocation sites and the target independently compute the same uint32 expression

`S = ((Buf[48] * byte(Buf+0x120) * byte(Buf+0x123) + 7) >> 3) + 1`.

The expression fields are written in the same write pipeline at `0x00A91381`, `0x00A91373`, and `0x00A91350` and remain stable between allocation and target.  Read-mode member stores `0x00AB3602/0x00AB35D8` cannot reach this local write-mode receiver.  The replacement path is therefore not an adoption of arbitrary storage: it only swaps the two fresh, equally sized allocations.

### Complete callers

The exact `sub_A93480` xref universe is seven type-19 tail calls from `sub_A63BA0` plus one type-17 normal call after the swap:

`{0x00A63BFE, 0x00A63C22, 0x00A63C38, 0x00A63C58, 0x00A63C6E, 0x00A63C8E, 0x00A63D44, 0x00A94C29}`.

Upstream closure is also exact: `sub_A63BA0` has only `0x009E8EB1`, its allocator helper `sub_A932C0` only `0x00A63BB9`; `sub_A93860` has only `0x00A63D5E`, and swap helper `sub_A94B10` only `0x00A94AC1`.  There are no data/indirect/EH/split-tail callers to the target.

### Range and separation

The target checks `Buf+0xE0 != 0` at `0x00A9354E` then performs `memset(member,0,S)` at `0x00A93577`.  The final logical shift and `+1` yield `1 <= S <= 0x20000000` even if the preceding uint32 multiply/add wraps; allocation and fill use identical wrapped precursor semantics.  Thus `start=0`, `count=S`, `extent=S`, and `0+S<=S` without arithmetic wrap.  A valid `S`-byte allocation implies `base <= UINT32_MAX-(S-1)`, excluding address wrap.  Allocation failure takes the libpng error/longjmp path and cannot execute the target with a failed pointer.

The destination is a successful fresh CRT heap allocation, while `[0x00C71678,0x00C7167C)` is owned by the PE static `.data` mapping; the ownership domains are disjoint.

## Row `0x006956E8`: zlib deflate member

### State allocation and member writers

`sub_A911E0` configures the stream with `zalloc=sub_A62F10`, `zfree=sub_A62F90`, and `opaque=Buf` at `0x00A91487..0x00A9149B`, then its unique `sub_A953B0` call at `0x00A91549` creates the deflate state.  `sub_A953B0` allocates the containing state as `zalloc(opaque,1,5816)` or default `calloc(1,0x16B8)` and stores it at `z_stream+0x1C`.

It rejects `a5` outside `[1,9]`, then sets

`field68 = 1 << (a5+7)`, so `field68 in {256,512,...,65536}`.

The sole nonzero writer of this state instance's `state+0x3C` member is `0x00A95524`, which stores `zalloc(opaque,field68,2)`.  The default branch is exact `calloc(field68,2)`.  Custom allocator `sub_A62F10` computes `Size=a2*a3`, calls `malloc(Size)`, then clears exactly `Size`: one memset for `Size<=0x8000`, otherwise `0x8000` plus `Size-0x8000`.  Here `Size=2*field68<=131072`, so the multiplication cannot wrap.

`sub_A95A10` only reads `state+0x3C`, frees it through configured `zfree`, frees the state, and clears `z_stream+0x1C`; it never adopts or replaces the member.  Superficially similar `+0x3C` stores in unrelated structure families are excluded by the exact state source and deflate consumer graph.

### Complete callers

The exact `sub_A955E0` xref universe is three ordinary calls:

`{0x00A92AA2 (sub_A92640), 0x00A93612 (sub_A93480), 0x00A955AC (sub_A953B0)}`.

All pass the same initialized `Buf+27` / z_stream receiver.  There are no data xrefs, indirect callers, split tails, or EH forwarders.

### Range and separation

At `0x00A956D4` the function first clears the final word at offsets `2*field68-2` and `2*field68-1`; at `0x00A956E8` it clears `[0,2*field68-2)`.  For the target row:

- `start = 0`;
- `count = 2*field68-2`;
- `extent = 2*field68`;
- `512 <= 2*field68 <= 131072`, hence subtraction and multiplication do not wrap;
- `0 < 2*field68-2 <= 2*field68`.

A valid contiguous `2*field68` allocation also gives `base <= UINT32_MAX-(2*field68-1)`, so the member span cannot wrap.  Allocation failure reports an error and enters cleanup; the target additionally requires the state and relevant members to be non-null.

As in row 1, successful heap ownership is disjoint from the PE static slot mapping.

## Machine artifacts and reproduction

- analyzer: `out/cs-t0/analyze-manager-this-fill-png-zlib-two-v20.py`
  - SHA-256 `4269F1DCB46D394AE2A9173B1FCE0D213A70F52517294BBF8D157ABB96A336E1`
- report: `out/cs-t0/native-join-manager-this-fill-png-zlib-two-v20.json`
  - SHA-256 `A7292DB5DF505A7ACC8D822D95041D2220913EA5114E6F77ADD2A6AB24C2DDBE`

Run the analyzer twice with read-only IDA MCP `py_exec_file` against the approved database:

```text
PASS created=out/cs-t0/native-join-manager-this-fill-png-zlib-two-v20.json sha256=A7292DB5DF505A7ACC8D822D95041D2220913EA5114E6F77ADD2A6AB24C2DDBE rows=2 verified=2 unknown=0 mutations=24/24 permissions=false promotions=false
PASS verified=out/cs-t0/native-join-manager-this-fill-png-zlib-two-v20.json sha256=A7292DB5DF505A7ACC8D822D95041D2220913EA5114E6F77ADD2A6AB24C2DDBE rows=2 verified=2 unknown=0 mutations=24/24 permissions=false promotions=false
```

The second run verifies byte equality rather than overwriting.  The 24 fail-closed mutations cover frozen-source/target hashes, corrected range, exact-two membership, call bytes, both allocation formulas, writer/replacement/custom-allocator closure, caller universes, field domain/no-wrap/member non-adoption, slot separation, classification, permissions, and promotion boundaries.

## Safety ledger

`gameStarted=false`, `debuggerAttached=false`, `processOpened=false`, `processRead=false`, `idbModified=false`, `productionModified=false`, `testsModified=false`, `priorEvidenceModified=false`, `runtimeMutationAllowed=false`, `promotionAllowed=false`.

First missing gap: none for these exact two rows.  No claim is made for any other remaining row or for final manager-this completion.
