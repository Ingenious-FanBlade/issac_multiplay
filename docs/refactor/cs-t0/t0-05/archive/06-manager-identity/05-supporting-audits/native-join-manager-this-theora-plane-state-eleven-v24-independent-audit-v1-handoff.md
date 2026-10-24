> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 Theora exact11 v24 independent audit v1 handoff

## Result

The fixed PE independently reproduces all eleven component rows: four `memcpy`
and seven `memset` destinations are inside the live Theora state/frame/table
allocations.  Row verdict is **11 VERIFIED / 0 UNKNOWN**.  The candidate
component is not rejected.

This is not an overall manager-this acceptance.  All permissions and promotion
flags remain false.  The unique audit-coverage gap is
`IDA_READ_ONLY_REPLAY_TIMEOUT_NO_PASS`: the shared IDA MCP request timed out
before returning even the input hash/image base, so no scripted or manual IDA
PASS is claimed.

## Independent proof highlights

- `VideoClip_Theora+0x404` has four classified stores in the bound class
  functions: constructor zero at `A8C77E`, then only `ADE980` results at
  `A8CA69`, `A8DAFD`, and `A8E0B2`.  Whole-`.text` direct-call scans bind the
  three `ADE980` producers and both `ADEBD0` consumers.  Full/split member
  literals, address-taken/indirect/return/stack aliases add no writer or scalar
  adoption path.
- State construction is `ADE980 -> ADBDD0(57184,16) -> ADD260 -> AE19B0`.
  `ADBDD0` allocates `n+align`; its returned adjustment is in `[1,align]`, so
  the requested 57184-byte usable extent remains.
- `AE19B0 -> AE1CF0(state,3)` constructs exactly three adjacent frames and all
  Y/Cb/Cr tuples.  `ADD7A0` creates scratch tuples or copies a ring tuple;
  `ADEBD0` supplies the fallback copy; `ADBE40` only reverses orientation inside
  the same rectangle.  Outbound descriptor copies do not write back.
- The apparently raw auxiliary shifts are closed by the frame allocation, not
  by an assumption that videos are small.  `AE1CF0` checks
  `3*frameBytes <= UINT32_MAX`, hence `frameBytes <= 1431655765`.
  For admitted multiple-of-16 dimensions,
  `64*total8x8Count = visiblePlaneBytes <= frameBytes`, and
  `totalMacroCount <= total8x8Count`.  Therefore both
  `state[0xDC] << 2` and `state[0xE8] << 6` are non-wrapping on a live target
  path.
- The candidate's loose `AE1060` “row may be -10” narrative was not used.
  Exact caller flow gives first-chunk correction zero (`startRow=0`).  A later
  chunk advances by 4/8 blocks before correction one, yielding minimum
  `(4-1)*8-2 = 22`.  Both 8/16-byte side fills remain inside the padded plane.
- `AE1330` imports `calloc` at `0x00B187E8`, checks `4*YmacroCount`, checks every
  allocation result before the target loop, and the nested indices end strictly
  below their allocated entry counts.

## Per-row verdict

| RVA | Helper | Verdict |
|---|---|---|
| `0x006DCB75` | `memcpy` | VERIFIED |
| `0x006DCCF5` | `memcpy` | VERIFIED |
| `0x006E1030` | `memcpy` | VERIFIED |
| `0x006E103C` | `memcpy` | VERIFIED |
| `0x006DCB1E` | `memset` | VERIFIED |
| `0x006DEF41` | `memset` | VERIFIED |
| `0x006DF552` | `memset` | VERIFIED |
| `0x006E10CD` | `memset` | VERIFIED |
| `0x006E10DE` | `memset` | VERIFIED |
| `0x006E168B` | `memset` | VERIFIED |
| `0x006E181F` | `memset` | VERIFIED |

Semantic row first gap: **none in exact11**.  Unique audit gap:
**IDA_READ_ONLY_REPLAY_TIMEOUT_NO_PASS**.

## Reproduction

Create-new was run once:

```powershell
python out\cs-t0\audit-manager-this-theora-plane-state-eleven-v24-independent-v1.py
```

Result: `PASS mode=PE`, 11/11 rows and 33/33 effective semantic mutations.

Two subsequent sequential verify-existing runs were executed with:

```powershell
python out\cs-t0\audit-manager-this-theora-plane-state-eleven-v24-independent-v1.py --verify-existing
```

Both returned the same machine-artifact hash and
`rows=11/11 mutations=33/33`.

No game, debugger, process open/read, runtime mutation, IDB modification,
candidate modification, prior-evidence modification, production modification,
or test modification occurred.

## Bindings

- Candidate analyzer:
  `6EA04FB3EF5BF2C45B111C02AB16E410EC8AF8AB15620E5F21780DFCD4D115EE`
- Candidate JSON:
  `19E14EBCB1AEC99AFE7838A1CE04F2F317473EDC0B6CC7E60CDA1F3D3956B1C3`
- Copy v10 analyzer:
  `E1DFDD540A4997B5CA45284BD6F02DF690D16AAFC866B8AF378C7EB0A035CD9E`
- Copy v10 JSON:
  `631F67A92DE248214DE0A26AA91BDC436E102044115C5029BB6ED083BE9B6BA1`
- Memset v3 analyzer:
  `35394D352A955E36CD9DFB82D52B13BC3C4A559D6DB0A2B207D4306B96526474`
- Memset v3 JSON:
  `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`
- Memset range-audit v5 analyzer:
  `7DFE2FD1C80E0047D6713C2099306D6B4D611B1D891FA288DF00821728F43BFD`
- Memset range-audit v5 JSON:
  `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- Fixed PE:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- Independent auditor:
  `9D814E0DAA8578FCC948273C73346F2FB9B4B406C18D2DB21B9D496909B96C3B`
- Independent machine JSON:
  `B16823DED0183E3099EE2ED457BF929272A673B570234CA7A9916E1270D221AE`

The candidate analyzer and JSON retain their original hashes; the auditor did
not edit either file.
