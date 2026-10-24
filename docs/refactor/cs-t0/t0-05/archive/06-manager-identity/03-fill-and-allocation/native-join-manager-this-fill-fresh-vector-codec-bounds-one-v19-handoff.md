> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 fresh-vector codec bounds exact-one v19 handoff

## Outcome

The sole v17/v18 UNKNOWN row at callsite RVA `0x0068DF56` is statically closed as
`VERIFIED_FRESH_VECTOR_CODEC_PRODUCER_AND_UINT32_SUM_BOUND`.

This is a read-only exact-one component. It does not promote the whole-image alias result or the
manager-this conclusion by itself. All permission and promotion flags remain `false`; the game was
not started, no debugger/process was opened, and the IDB was not modified.

## Frozen bindings

- target `isaac-ng.exe`: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- v17 UNKNOWN machine: `707A6C727C330A95B5A93B9C927284F38EFDB76B7569F5BBD2EF02A175A5AE08`
- v18 UNKNOWN machine: `10AA255A0ACECBF5DA88ACDD62D2C0A1DD62A53546C2B0D7564E01CD738DCDA4`
- corrected whole-image v2 machine: `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`
- corrected whole-image v2 analyzer: `A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6`

## Complete object/field producer universe

The receiver is `theoraplayer::VideoClip_Theora`, vtable VA `0x00BA60DC`. Its nine frozen virtual
entries are `A8BCF0, A8C6D0, A8CA10, A8D0A0, A8D350, A8D580, A8D950, A8DA40, A8E050`.

- The complete executable vtable-literal xref universe is `0xA8C778` and `0xA8C812`.
- `sub_A8C700` allocates exactly `0x528` bytes and installs the vtable. The only executable literal
  for this constructor is `0xA620B4`, where `sub_A61F20` registers the Theora/`.ogv` factory.
- The second vtable literal is the destructor reset in `sub_A8C7E0`; there is no second producer,
  copy constructor, or callback object producer.
- The embedded helper call universe is frozen exactly: `A8E530 <- A8CA45`, `AACB50 <- A8CF14`,
  `AACF20 <- A8C84A/A8CF26/AACF0E`, `AAD330 <- A8CF2D/A8DB3B`, and
  `AAD3B0 <- A8D8EB/A8E11B`.

The complete writer counts are:

| Field | Meaning | Complete writes |
|---|---|---|
| `this+0xA8` | channels | 2 direct object writes: `A8C764` ctor zero; `A8CFE9` copy from `this+0x40C` |
| `this+0x43C` | DSP `pcm_current` / end | 9: `A8E5E3`, `AACB8C`, `AACDB9`, `AAD375`, `AAD842`, `AAD875`, `AAD950`, `AAD9FD`, `AAD31C` |
| `this+0x440` | DSP `pcm_returned` / start | 9: `A8E5E3`, `AACB8C`, `AAD352`, `AAD83F`, `AAD84D`, `AAD95D`, `AAD968`, `A8D80C`, `AAD31C` |

The first and only external input in this chain is the Vorbis identification-header packet:
`sub_A8E530` passes it to `sub_AAA150` at `0xA8EAA2`. `sub_AAA150` reads channels with exactly
8 bits at `0xAAA37B..0xAAA38D` and its successful-header path rejects channels below one at
`0xAAA402`. Therefore successful initialization gives the hard, non-heuristic bound
`1 <= channels <= 255`.

## Buffer/allocation bound

The same successful identification-header path reads the two blocksize exponents with four bits and
enforces `blocksize0 >= 0x40`, `blocksize1 >= blocksize0`, `blocksize1 <= 0x2000`, and framing bit
one at `0xAAA3D3..0xAAA42B`.

`sub_AACB50` then:

- sets `dsp.pcm_storage = blocksize1` at `0xAACD43..0xAACD4C`;
- allocates each channel-pointer array as `4*channels` at `0xAACD53..0xAACD6F`;
- allocates every channel PCM buffer as `calloc(pcm_storage, 4)` at
  `0xAACD82..0xAACD93`;
- initializes `pcm_current = blocksize1/2` at `0xAACDB9`; the `0x70`-byte zero at
  `0xAACB8C` initializes `pcm_returned = 0`.

`sub_AAD330` uses `-1` only as a restart sentinel. `sub_AAD3B0` rejects an accepted block when
`current > returned && returned != -1` at `0xAAD3E5..0xAAD3F0`. Its first/subsequent block centers
are half/quarter-block expressions; with both block sizes bounded by `pcm_storage`, all productive
writes preserve `0 <= returned <= current <= pcm_storage`. The corrective writers only decrease
`current` or increase/cap `returned` to `current`. Finally `0xA8D80C` advances returned by
`current-returned`, making it exactly current before the next productive block.

Thus on the publisher guard path:

```text
1 <= delta = this[271]-this[272] <= pcm_storage <= 8192
1 <= channels = this[42] <= 255
published_count = delta*channels <= 8192*255 = 2,088,960
published_bytes <= 4*2,088,960 = 8,355,840
```

Both the count and byte count are strictly inside `uint32` and are tied to the exact PCM allocation
extents; this is not a “channels are usually small” assumption.

The frozen v17 bound is `1 <= v37 <= INT32_MAX`. Therefore the unchecked add at `0xA8DF39` obeys:

```text
old_count + v37
<= 2,088,960 + 2,147,483,647
= 2,149,572,607
< UINT32_MAX (4,294,967,295)
```

The remaining headroom is `2,145,390,688`; unsigned wrap is impossible.

## Artifacts

- machine: `out/cs-t0/native-join-manager-this-fill-fresh-vector-codec-bounds-one-v19.json`
  - SHA-256 `43B5C2119F985914ADCA269E1E82BA216A0553F721D84BE0C7D283864ED11B5F`
- deterministic PE analyzer:
  `out/cs-t0/analyze-manager-this-fill-fresh-vector-codec-bounds-one-v19.py`
  - SHA-256 `94AE6F381A90AF3306A248FBED8BBAF91A5C6CD477507C845728A1FC01C88B2A`
- read-only IDA reproducer:
  `out/cs-t0/ida-reproduce-manager-this-fill-fresh-vector-codec-bounds-one-v19.py`
  - SHA-256 `2E1860775941CE9CD8A6A8238524C5F9C4DDCF7F73CF0AD90C007AE2C1F99BC2`

## Reproduction and verification

From the repository root:

```powershell
python out\cs-t0\analyze-manager-this-fill-fresh-vector-codec-bounds-one-v19.py
python out\cs-t0\analyze-manager-this-fill-fresh-vector-codec-bounds-one-v19.py --verify-existing
python out\cs-t0\analyze-manager-this-fill-fresh-vector-codec-bounds-one-v19.py --verify-existing
```

The create run and both verify runs produced:

```text
PASS ... sha256=43B5C2119F985914ADCA269E1E82BA216A0553F721D84BE0C7D283864ED11B5F rows=1 verified=1 unknown=0 mutations=24/24
decision: VERIFIED published<=2088960; published+v37<=2149572607<UINT32_MAX
```

An independent verifier also replayed the analyzer and reported `1/1`, `24/24`, and the same machine
hash.

With the target IDB open in IDA 9.x, execute (read-only):

```python
exec(open(r"D:\projects\isaac_hook_reserach\out\cs-t0\ida-reproduce-manager-this-fill-fresh-vector-codec-bounds-one-v19.py", encoding="utf-8").read())
```

Observed IDA output:

```text
PASS IDA_READ_ONLY v19 writers=channels:2/end:9/start:9
PASS bounds channels<=255 delta<=8192 published<=2088960 sum<=2149572607<UINT32_MAX
PASS permissions=false promotions=false IDB_UNMODIFIED
```

The analyzer runs 24 fail-closed mutations covering all input hashes, function and instruction
anchors, vtable/constructor/helper universes, all three writer counts, the external-header source,
codec bounds, allocation extent, DSP invariant, both arithmetic conclusions, exact-one VERIFIED
classification, and false permissions/promotions.
