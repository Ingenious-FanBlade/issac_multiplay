> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R06 double-buffer REVERSE candidate v1 handoff

## Result

- Task: `MT-R06 {0x0061F2C2}`.
- REVERSE disposition: `0V/1U`; the row remains `UNKNOWN` and is not eligible
  for frontier composition or manager-this/whole-image promotion.
- Candidate schema:
  `cs-t0.manager-this.mt-r06-double-buffer.positive-candidate.v1`.
- Frozen target: x86 PE, ImageBase `0x00400000`, SizeOfImage
  `0x008FE000`, SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- No game, process, debugger, runtime mutation, injection, or IDB mutation was
  used.
- This document and candidate were authored by the REVERSE role. No
  independent TEST audit was performed or imported.

## Artifacts

- Builder: `out/cs-t0/build-manager-this-mt-r06-double-buffer-v1.py`
  - SHA-256:
    `7647F4174EBAE9DDCCE5A8A5E975C0DB8318F13E9F8B9ABEB4C5AD741CA91B5E`
- Candidate: `out/cs-t0/native-join-manager-this-mt-r06-double-buffer-v1.json`
  - SHA-256:
    `A3A167CD597CD6BE0D3B6496E48ADADA2F43275E731AE9897FEBB94A0A6B08E5`
- Public serialization contract:
  `out/cs-t0/native-join-manager-this-mt-wave-d-candidate-envelope-v1.md`
  - SHA-256:
    `89DB52BC84CF91C322CA5F78B74AC843442DC95D8FFE395783FFE32E5E54CF10`

## Current-PE evidence retained

The candidate binds the local call and seven Wave D proof categories directly
to frozen-PE byte slices.

- `0x0061F2C2`: `E8 18 13 0D 00`, direct call to `memcpy` at
  `0x00AF05DF`, inside function RVA `0x0061F280`.
- At the call, `[this+0xC0]` (`this[48]`) is the source and
  `[this+0xC4]` (`this[49]`) is the destination. The byte count is
  `4 * ((this[5] - this[4]) >> 4)`.
- InputDeviceBase constructor RVA `0x0061E050` initializes the source tuple
  and clears both buffer slots. Bound slot stores are:
  - `0x0061E0F5`: clear `this[48]`;
  - `0x0061E0FF`: clear `this[49]`.
- Init RVA `0x0061E210` derives both allocation sizes from the same source
  tuple at initialization time and stores allocated pointers at:
  - `0x0061E244`: `this[48]`;
  - `0x0061E26A`: `this[49]`.
- Cleanup RVA `0x0061E2C0` frees and clears both known slots; bound clear
  stores are `0x0061E30C` and `0x0061E335`.
- The direct E8 census for function VA `0x00A1F280` contains only callsite
  VA `0x00A1FC97`; the relative-E9 census is empty.
- Base/Gamepad source population is represented by function RVA
  `0x0061E430`; Keyboard population is represented by RVA `0x0066C480`.
  Both read global template vector families rooted at `0x00C799E8` or
  `0x00C799F8`.

These facts improve the previous `DOUBLE_BUFFER_POINTER_WRITER_SET_OPEN`
boundary by identifying the base constructor, init-time allocation relation,
cleanup, local call argument mapping, and the one direct caller. They do not
establish a class-wide lifetime invariant.

## First unresolved boundary and retained counterexample

The minimum first gap is the class-specific fixed point for the four observed
InputDevice families (Base, Net, Gamepad, and Keyboard): all direct, indirect,
address-taken, replacement, and offset-collision writers of `this[48]`,
`this[49]`, and their readable/writable extents were not closed. The global
template vectors at `0x00C799E8` and `0x00C799F8` also have an open
writer/lifetime and post-init immutability fixed point.

The retained fail-closed counterexample is:

1. initialization allocates both known buffers from the current tuple length;
2. an omitted class-specific writer replaces `this[49]` with a shorter or
   unrelated backing, or an omitted source-tuple writer increases
   `this[5]-this[4]` without reallocating both buffers;
3. RVA `0x0061F2C2` recomputes the larger length and calls `memcpy` without a
   capacity check.

Consequently the source/destination extents, tuple order/no-wrap, slot48-slot49
disjointness, owner lifetimes, and GameRoot-slot non-alias are not proven. All
eight public R06 countermodels remain unresolved; no positive claim is made.

## Replay and author-side mutation checks

Commands executed:

```powershell
python out\cs-t0\build-manager-this-mt-r06-double-buffer-v1.py
python out\cs-t0\build-manager-this-mt-r06-double-buffer-v1.py --verify-existing
```

Both runs emitted the same candidate SHA-256
`A3A167CD597CD6BE0D3B6496E48ADADA2F43275E731AE9897FEBB94A0A6B08E5`.
The builder observed create-new once and one byte-identical verify-existing
replay. Its six compact author mutations rejected target-PE drift, exact-row
drift, writer-union omission, fabricated destination extent, false promotion,
and permission escalation.

The author-side checks are not an independent audit. TEST must independently
recompute the public evidence and verdict without reading or importing this
builder.
