> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this reachable memset closure handoff v4

## Result

The create-new v4 machine closes exactly the frozen v3 `UNKNOWN` set:

- reachable `memset` callsites: **242**
- inherited v3 closed rows, structurally unchanged: **164**
- newly fully backed rows: **78 / 78**
- remaining `UNKNOWN`: **0**
- explicit GameRoot-slot overlap: **0**

This is the reverse-evidence half only.  It completes the reachable-`memset`
partition's static proof; it does **not** by itself authorize manager-this,
runtime testing, production changes, or any v1-v7/index/status edit.  A separate
agent must perform the skill-required independent audit.

## Frozen inputs

| Input | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| `out/cs-t0/analyze-manager-this-bulk-memset-v3.py` | `35394D352A955E36CD9DFB82D52B13BC3C4A559D6DB0A2B207D4306B96526474` |
| `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json` | `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981` |

The v4 reproducer fails closed on any binding/count/set drift.  Its exact-set
check requires the proof table to equal v3's 78 `UNKNOWN` RVAs, and its
append-only check requires every one of the frozen 164 closed callsite objects
to remain object-equal.

## Outputs

| Artifact | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-bulk-memset-v4.json` | `0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C` |
| `out/cs-t0/analyze-manager-this-bulk-memset-v4.py` | `81C1FB99E8600D722A930C32C67910F17FDF68506BFAF45B30C456475AB1A101` |

## Exact inherited gap discharge

| v3 gap class | v3 count | v4 disposition |
|---|---:|---|
| `POINTER_LOAD_MEMBER_BASE_OR_CONTAINER_POINTS_TO_OPEN` | 64 | closed producer/member/container writer universe and full forward span |
| `PARAMETER_POINTS_TO_OR_CALLER_CAPACITY_OPEN` | 13 | closed every direct caller/forwarder and its allocation/capacity premise |
| `ADDRESS_DOMAIN_KNOWN_BUT_COMPLETE_SPAN_OPEN` | 1 | closed all 17 stack-array callers and `Size+n2 <= n2*a3` |

No previously closed row was re-proved or changed.

## Proof families

| v4 proof family | Count | Closed interface |
|---|---:|---|
| `V4_STANDARD_VECTOR_TUPLE_CLOSURE` | 15 | complete begin/end/capacity writers; fresh-grow and in-place tail bounds |
| `V4_XML_INDENT_ZERO_EXECUTION` | 8 | complete roots enter with `Size==0`; recursive forwarders preserve it |
| `V4_THEORA_STATE_AND_PLANE_GEOMETRY` | 7 | 57,184-byte state plus unique plane tuple installer and stride geometry |
| `V4_N298_LARGE_LAYOUT_CLOSURE` | 6 | frozen 0x4D088 n298 allocation/writer universe; maximum end 124,542 |
| `V4_PERSISTENT_DATA_CALLER_AND_RECEIVER_CLOSURE` | 6 | all 11 receiver calls and five slice calls; maximum fixed end 3,940 |
| `V4_MINIZ_COMPRESS_STATE_EXTENT` | 5 | exact 0x2AF0 payload; maximum dynamic end 10,992 |
| `V4_MINIZ_DECOMPRESS_STATE_EXTENT` | 5 | exact 319,296-byte state; maximum written end 37,490 |
| `V4_CALLER_CAPACITY_VECTOR_CLOSURE` | 4 | all callers of four parameter-capacity helpers |
| `V4_CHECKSUM_FOUR_BYTE_TAIL` | 3 | `[state+count,state+4)` under `count<4` |
| `V4_FIXED_MEMBER_OBJECT_UNIVERSE` | 3 | all constructor/caller roots and containing-object extents |
| `V4_ALLOCATOR_BITMAP_ZERO_EXECUTION` | 2 | both allocator bitmap sites locally contradictory/unreachable |
| `V4_ALLOCATOR_BLOCK_USABLE_SIZE` | 2 | internal block base tied to its exact usable byte count |
| `V4_LIBPNG_EXACT_STRUCT_ALLOCATION` | 2 | exact `malloc(0xB8)` / `malloc(0x20C)` cleanup intervals |
| `V4_PNG_ZLIB_MEMBER_ALLOCATION` | 2 | member writer bound to exact scanline/calloc formulas |
| `V4_SSO_AND_RECORD_APPEND_CLOSURE` | 2 | SSO inline/fresh writer closure and reserved 104-byte record |
| `V4_ALLOCATOR_SEGMENT_METADATA_EXTENT` | 1 | header plus exact `48*n128+88` metadata tail |
| `V4_BIT_RANGE_ZERO_EXECUTION` | 1 | all three calls make the outer write predicate false |
| `V4_FIXED_640_BYTE_DESTINATION` | 1 | exact two-call universe; minimum heap end 872 <= 880 |
| `V4_FORMAT_BUFFER_CAPACITY_CLOSURE` | 1 | all 17 fixed-stack-array callers and full span formula |
| `V4_FRESH_VECTOR_EXACT_ALLOCATION` | 1 | same-function allocation `4*(v37+old)` bounds `4*v37` |
| `V4_GLFW_TLS_OBJECT_EXTENT` | 1 | end 692 inside the 2,664-byte TLS/static library object |

Total: **78**.

## Notable zero-execution closures

1. `memset@0x0041CC2C`: `sub_41CB90` has three direct calls.  Each passes
   `a1==a3` and `a2==a4`; the outer predicate
   `(a1!=a3 || a2!=a4)` is false.
2. XML indentation sites `0x00836AE6..0x00836E2A`: both roots pass
   `Size=0`, and the two recursive forwarders preserve the pair.  Every site is
   dominated by `Size>0`.
3. `memset@0x00AE5F42`: reaching the branch requires byte+25 nonzero,
   which was assigned `(!a4&&!a8a)`, while the call also requires `a8a!=0`.
4. `memset@0x00AE6164`: the preceding store writes `WORD[+25]=0x0100`;
   its low byte at +25 is zero, so the immediately following `byte+25==0`
   branch always bypasses the call.

## Reproduction

From the repository root:

```powershell
python out\cs-t0\analyze-manager-this-bulk-memset-v4.py
```

Expected existing-output replay:

```text
PASS existing=out\cs-t0\native-join-manager-this-bulk-memset-v4.json sha256=0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C closed=242 new=78 unknown=0 overlap=0 inherited_closed_unchanged=164
```

The machine retains every permission flag as `false`.  The pass used only
read-only IDA queries and offline replay: no game start, process open/read,
debugger attach, injection, network access, IDB write, production edit, test
edit, or v1-v7/index/status-plan edit occurred.

## Independent-audit contract

The audit agent should independently verify, at minimum:

1. all three frozen SHA bindings and both output SHA values;
2. exact equality between v3's 78 `UNKNOWN` RVAs and
   `newlyClosedSinceV3[*].callsiteRva`;
3. object equality for all 164 inherited closed rows;
4. the three inherited gap counts `64/13/1` and final counts `242/0/0`;
5. the four zero-execution predicates above;
6. representative allocation/span inequalities for PersistentGameData, n298,
   miniz, libpng, Theora, PNG/zlib, and allocator metadata;
7. that all permission values remain false and no forbidden files changed.

