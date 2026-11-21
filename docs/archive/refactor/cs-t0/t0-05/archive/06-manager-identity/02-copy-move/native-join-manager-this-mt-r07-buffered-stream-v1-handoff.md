> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R07 BufferedStream REVERSE handoff v1

## Result

MT-R07 was analyzed against the frozen x86 PE only.  All five exact rows remain
fail-closed `UNKNOWN`: `0` static-confirmed / `5` unknown.  No positive row is
eligible for frontier composition.

Exact scope:

- `0x00657E0A`
- `0x00657E94`
- `0x00658338`
- `0x006583C2`
- `0x00658487`

The target is PE32/I386, ImageBase `0x00400000`, SizeOfImage `0x008FE000`,
SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
The game was not started, no process was opened/read, no debugger was attached,
and the IDB was not modified.

## Closed local evidence

- All five exact instructions are boundaries in the containing functions and
  target `memcpy` at VA `0x00AF05DF` with the exact bytes sealed by the Wave E
  envelope.
- The direct E8 and relative-E9 census for the three containing methods was
  independently replayed from the frozen `.text`: `25/0` for VA `0x00A57DB0`,
  `30/0` for VA `0x00A582E0`, and `9/0` for VA `0x00A583E0`.
- Current-PE RTTI/vtable identity was advanced beyond the older untyped label:
  BufferedStreamBase primary table entry VA `0x00B9ECE0` and BufferedStream
  primary entry VA `0x00B9ED3C` both bind VA `0x00A57DB0`; the BufferedStream
  `+0x28` secondary-table read and write entries VA `0x00B9ED10` and
  `0x00B9ED18` bind VA `0x00A582E0` and VA `0x00A583E0`.
- Known base/derived construction and reset slices at VA `0x00A57BA0`,
  `0x00A58040`, `0x00A581A0`, and `0x00A58200`, plus the
  `sub_7DCCD0 -> sub_7DEDB0` vector-grow/replacement edge, are PE-byte bound in
  the candidate.  These are evidence atoms, not a claim of a complete writer
  or lifetime universe.

## Retained countermodels and first gaps

For `0x00657E0A`, `0x00657E94`, `0x00658338`, and `0x006583C2`, the minimum
first gap is the actual indirect-dispatch callsite to `a2` producer/output-span
fixed point.  RTTI identifies the vtable slots, but the type-erased/address-
taken indirect caller universe and every corresponding `a2` allocation extent
remain open.  A reachable omitted indirect caller may therefore provide an
output span shorter than `a3*a4`, or provide `a2` at/before the GameRoot slot.
The requested-byte multiplication no-wrap property is also not proven for
every caller.

For `0x00658487`, the earlier independent gap is the threshold path: it invokes
vfunc `+0x20` before requesting only `Size+0x1000` from the grow helper, but no
all-dynamic-type proof establishes `usedAfterVfunc <= 0x1000`.  In addition,
the constructor/begin/used/capacity/replacement-writer and owner-lifetime fixed
point remains open.  An omitted replacement writer can retain a short backing
or a GameRoot-slot alias while `used+Size` wraps or exceeds capacity.

Thus every row is serialized with `fullyAdjudicatedNoOverlap=false`,
`disposition=UNKNOWN`, a nonempty unresolved list, and `promoted=false`.

## Public bindings and artifacts

The candidate binds the sealed Wave E public envelope
`out/cs-t0/native-join-manager-this-mt-wave-e-candidate-envelope-v1.md`
(`D472A5ADB33E8C85152C5FCE40E03F8693F510675BCCBB3B5298A0692B876135`),
v31 plus its independent audit, the Wave D independent audit, v33, and the
public v29/v29-audit/v7 machine inputs.

- Builder:
  `out/cs-t0/build-manager-this-mt-r07-buffered-stream-v1.py`
  SHA-256 `790A8D303D54BA29278E391F13BE41995BB7C14D9ACAE5DDF88A9006F569C8E3`
- Candidate:
  `out/cs-t0/native-join-manager-this-mt-r07-buffered-stream-v1.json`
  SHA-256 `B32470D7F09B08B1F6A31B845905BF983F227AC55299A7E07A543E58C8FA1BD1`

Creation and replay commands:

```powershell
python out/cs-t0/build-manager-this-mt-r07-buffered-stream-v1.py
python out/cs-t0/build-manager-this-mt-r07-buffered-stream-v1.py --verify-existing
```

Both commands passed.  The payload was byte-stable across the one required
`verify-existing` replay.  Seven compact author mutations were rejected:
target PE, exact row order, call/helper bytes, producer-union omission,
countermodel-partition corruption, false promotion, and permission enablement.

## Role boundary

This handoff is authored by the REVERSE role.  No independent TEST audit was
performed or imported, no audit/frontier/index artifact was created, and this
document does not authorize manager-this or whole-image promotion.
