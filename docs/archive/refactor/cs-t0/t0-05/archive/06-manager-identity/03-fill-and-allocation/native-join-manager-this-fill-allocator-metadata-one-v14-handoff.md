> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 allocator-metadata exact-one v14 handoff

## Outcome

- Scope: only `memset` RVA `0x006EC167` in `sub_AEBE30`.
- Decision: `VERIFIED` (`1 VERIFIED / 0 UNKNOWN`).
- This exact-one result supersedes the v13 `UNKNOWN`; it does not cover any other fill/copy row and grants no manager-this or whole-image promotion.
- No game, target process, debugger, runtime mutation, production edit, test edit, prior-evidence edit, or IDB write was used.

## Required correction to the candidate fact

`VA 0x00BF8228` is **not writer-free**.  Its PE initializer is `0x1000`, but
`sub_AE8670` has the unique executable writer at `0x00AE869B` and copies the
nonzero `GetSystemInfo.SYSTEM_INFO.dwPageSize` result.  `sub_AE8670` has one
direct caller, `sub_AE9DE0@0x00AE9E54`, behind the process-init guard.

The proof therefore does not treat the disk initializer as the runtime value.
It binds the target's PE32/x86 Windows ABI (`GetSystemInfo` page size `0x1000`).
A read-only host corroboration returned `SystemPageSize=4096`,
`OSArchitecture=X64`; it is labelled corroboration, not PE-static proof.
The complete `.text` literal universe has 19 references: 18 reads and the one
writer above.

## Integer and row-local range closure

- `AED6A0` rejects unsigned allocation requests above `0x7fffffff`.
- `AE85D0` rounds every accepted ordinary request to `S<=0x80000000`.
- The `AEDB40` small-reclaim entry is guarded by request `<=0x10000`.
  Replaying every such request through `AED620` reaches queue bins `1..52`;
  the frozen `src__1` queue template gives `ArgList[2]<=0x10000`.
- With `q=0x1000`, both AEBE30 header computations produce `v17=v22=0x8000`.
  In the realignment branch they cancel around
  `a3=align_up(v17,0x400000)=0x400000`, so
  `n128<=((S+0x400000+0x7fff)>>15)=0x10080`.
- All intermediate unsigned sums stay below `2^32`:
  `v18<=0x803f8000`, final pre-shift numerator `<=0x80407fff`.
- Exact maxima:
  `n128<<15 = 0x80400000`, fill length `48*n128+88 = 3151960`,
  fill end offset `68+length = 3152028`.
- For all `N>=1`, `156+48*N <= 32768*N`; therefore the target fill is inside
  the allocation request and its count/shift arithmetic does not wrap.

## Cache and fallback extent closure

Cache chain:
`AEBE30 -> AE59B0 -> AE5D10 -> AE5BD0 -> AE9280/AE9860`.

- `needed=ceil(request/0x400000)` and the addition cannot wrap because
  `request+0x3fffff<=0x807fffff`.
- `AE9280/AE9860` only return after atomically reserving one contiguous run.
- `AE5E40 -> AE8F50` marks all tail bits in
  `[chunks,32*bitmap_words)` occupied; a successful run therefore ends at or
  before `chunks`.
- Returned address is `base+index*0x400000`, leaving
  `(chunks-index)*0x400000 >= needed*0x400000 >= request`.

Pool universe:

- `C61C80[64]` and `C61D80` are zero initialized.
- The only scalar publishers are `AE5E40` and `AE5FF0`.
- `AE5E40` has exactly two direct callers: `AE6300@AE6382` and
  `AE6400@AE6488`.  Both round to a nonzero 4 MiB multiple and obtain that
  fresh extent through `AE8140 -> AE8C00` before publishing it.
- `AE5FF0` cannot publish in this image: its sole upstream allocator call
  `AE8300@AE607E` zeros both out-parameters and returns zero unconditionally.
- `AE5A90` is the only additional absolute slot consumer; it reads the
  descriptor and releases bitmap bits, not immutable base/extent scalars.

Fallback chain:
`AE59B0 -> AE81B0 -> AE8C00 -> AE8AD0 -> AE8E90`.
Successful paths either receive a directly aligned OS-selected mapping or map
`request+alignment` and align inside it.  Wrapped zero request/alignment paths
fail.  The surviving offset plus request is non-wrapped (the sole mathematical
`2^32` equality is rejected as zero), so the returned suffix retains at least
the requested extent.

All successful OS allocations use `lpAddress=0`.  Windows allocation semantics
choose a free virtual range, so neither fallback mappings nor cache-producer
mappings can overlap the already loaded PE `.data` slot
`[0x00C71678,0x00C7167C)`.

## Writer/xref partition

- `sub_AEBE30` direct entries: `0x00AEBDD1`, `0x00AECB5B`; both force `a1=0`.
- The analyzer performs a whole-`.text` `E8/E9` pass for every function in the
  proof and an absolute-literal scan.  No analyzed function is address-taken in
  executable text, so there is no hidden split/indirect entry for these chains.
- Pool literal hits are frozen at 6 slot references and 3 count references;
  the page slot is frozen at 19 references with the corrected 18/1 role split.

## Reproduction

```powershell
python out\cs-t0\analyze-manager-this-fill-allocator-metadata-one-v14.py --verify-existing
```

Expected:

```text
classification: 1 VERIFIED / 0 UNKNOWN
mutation suite: 24/24 rejected
permissions/promotions/overallAccepted: false
```

Verification status is deliberately split into three independent facts:

- PE-disk scripted verification passed twice consecutively; both runs reported
  `1 VERIFIED / 0 UNKNOWN` and rejected `24/24` mutation sentinels.
- This turn's manual read-only inspection of the current IDB covered the target,
  integer chain, cache/fallback paths, producer/caller universes, xrefs, and
  function boundaries.  The frozen IDB hash was unchanged afterward.
- The whole analyzer script did **not** obtain an IDA-mode PASS: a prior request
  on the shared IDA instance timed out.  That timeout is recorded as
  `TIMEOUT_NO_PASS` in the machine JSON and is not represented as a PASS.

The analyzer contains a read-only IDA mode, forced to verify-existing, which
would check current `.text` bytes and function boundaries without a write call;
the existence of that mode is not evidence that the timed-out run completed.

## Artifact hashes

- Analyzer: `10BBB35A226A006363F7740A94933824D2F9E618F0F1BC36F4AE52C3A8F0098D`
- Machine JSON: `0EF108E9B68EEA47F543B25AAA7BD1707BA7404E39BF4063D6A5B805352172C1`
- Frozen IDB before replay:
  `992A16DC789B5714C337CDA34E135EF2DDC8CFE3A4F32378673B20B071F8909F`,
  length `110648172`, mtime UTC `2026-08-30T19:04:49.3098429Z`.

All permission, promotion, merge, and overall-acceptance switches remain false.
