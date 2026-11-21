> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R06 / MT-R07 / MT-R08 residual successor v2 REVERSE handoff

## 1. Role, scope, and result

- Role: `REVERSE`.
- Static target: frozen `isaac-ng.exe`, SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- In scope only: MT-R06, all five MT-R07 rows, and the two still-open MT-R08 rows `0x00661577` / `0x0068E3DB`.
- Explicitly excluded: MT-R01, MT-R02–R05, MT-R09–R10, and already-closed MT-R08 / `0x00689F63`.
- No game was started, no process was opened, no debugger/x32dbg was used, and the IDB was not modified.
- Candidate result: **2 new `STATIC_CONFIRMED_NO_OVERLAP`, 6 retained `UNKNOWN`**.
- This is a create-new REVERSE candidate. It is not eligible for frontier composition until an independent `TEST` role audits it.

## 2. Create-new artifacts

| File | SHA-256 |
|---|---|
| `out/cs-t0/build-manager-this-mt-r06-r07-r08-successor-v2.py` | `77F25526AB358E78B6BBA582B8BC6CE25BB720775121A5CB32ADCA5ACD587C2F` |
| `out/cs-t0/native-join-manager-this-mt-r06-r07-r08-successor-v2.json` | `BF70EFE07CA135B00309791FE3DF1DB206ABD9BAD167175B3D2653A3940B160E` |

The builder binds the three v1 candidates and the v34 checkpoint by SHA-256, binds the frozen PE identity/layout, validates all eight exact call bytes and rel32 helper targets, and emits machine-readable row verdicts plus frozen proof atoms.

## 3. Compressed verification ledger

Only the requested essential checks were run:

```powershell
python out\cs-t0\build-manager-this-mt-r06-r07-r08-successor-v2.py
python out\cs-t0\build-manager-this-mt-r06-r07-r08-successor-v2.py --verify-existing
python out\cs-t0\build-manager-this-mt-r06-r07-r08-successor-v2.py --self-test-mutation
```

Observed results:

```text
CREATED ...\native-join-manager-this-mt-r06-r07-r08-successor-v2.json
SHA256 BF70EFE07CA135B00309791FE3DF1DB206ABD9BAD167175B3D2653A3940B160E
PASS verify-existing ...\native-join-manager-this-mt-r06-r07-r08-successor-v2.json
PASS core mutation rejected
```

No Cargo build, whole-suite test, repeated verification, or runtime sampling was performed.

## 4. IDA connection and tools used

`ida-pro-mcp/server_health` returned ready state for:

- IDB: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`
- module/input: `isaac-ng.exe`
- image base: `0x00400000`
- auto-analysis: ready
- Hex-Rays: ready

Read-only tools used: `server_health`, `analyze_batch`, `decompile`, `xref_query`, and `py_eval`. The `py_eval` work was limited to address/name enumeration, vtable decoding, decoded-instruction field-access census, and read-only byte/function inspection.

## 5. Row verdicts

| MT row / RVA | Verdict | Minimal evidence or first gap |
|---|---|---|
| MT-R06 `0x0061F2C2` | `STATIC_CONFIRMED_NO_OVERLAP` | Complete InputDevice RTTI/vtable fixed point; source tuple populated before equal-size double allocation; only ctor/init/cleanup write `+0xC0/+0xC4`; template tuple size fields have no post-init writer. |
| MT-R07 `0x00657E0A` | `UNKNOWN` | Destination is caller-owned `a2`; no all-indirect-caller proof of span `>= a3*a4`, and multiplication no-wrap is open. |
| MT-R07 `0x00657E94` | `UNKNOWN` | Same caller-output-span and multiplication gap. |
| MT-R07 `0x00658338` | `UNKNOWN` | Same gap through the secondary `StreamBase` view. |
| MT-R07 `0x006583C2` | `UNKNOWN` | Same gap through the secondary `StreamBase` view. |
| MT-R07 `0x00658487` | `UNKNOWN` | Concrete flush target is resolved, but caller product and `oldExtent + Size + 0x1000` no-wrap remain unbounded. |
| MT-R08 `0x00661577` | `UNKNOWN` | Reachable wrapper forwards a serialized 32-bit Size to a fixed `0x10000` ring; overflow report is after the memcpy. `Size=0x10001` remains a real function-level counterexample. |
| MT-R08 `0x0068E3DB` | `STATIC_CONFIRMED_NO_OVERLAP` | Entry reset plus parser/refill induction proves `0 <= consumed <= used <= cap`; both shifted intervals remain inside the malloc/realloc block and outside the static manager-this slot. |

## 6. MT-R06 static fixed point

### 6.1 Exact call and length

- `0x00A1F2C2`: `E8 18 13 0D 00` -> `memcpy` at `0x00AF05DF`.
- Function `sub_A1F280` copies from `[this+0xC0]` to `[this+0xC4]`.
- Exact length is `4 * (([this+0x14] - [this+0x10]) >> 4)`.

### 6.2 Type and vtable universe

IDA RTTI name census found the complete InputDevice family present in the image:

- `InputDeviceBase`, vtable `0x00B82758`
- `NetInputDevice`, vtable `0x00B78030`
- `InputDeviceGamepad`, vtable `0x00B82C80`
- `InputDeviceKeyboard`, vtable `0x00BA1678`

There are no other `InputDevice` RTTI type descriptors/vtables in the image. Decoding all 38 slots of these four tables produced 81 unique methods.

### 6.3 `+0xC0/+0xC4` writer closure

A decoded-instruction census over those 81 methods plus all four constructor paths found 23 non-stack `[reg+0xC0]` / `[reg+0xC4]` references. Writes are confined to:

- `sub_A1E050`: zero both pointers (`0x00A1E0F5`, `0x00A1E0FF`);
- `sub_A1E210`: allocate both pointers (`0x00A1E244`, `0x00A1E26A`);
- `sub_A1E2C0`: free then zero both pointers (`0x00A1E30C`, `0x00A1E335`).

Every other decoded hit is a read, including the target memcpy. Constructors calling `sub_A1E050` are exactly the two Net paths, the Gamepad path, and the Keyboard path (`0x009015DB`, `0x0090195D`, `0x00A21A41`, `0x00A21DF2`).

### 6.4 Order, size, and lifetime

`sub_A1E210` first dispatches vtable slot `+0x14`:

- Base / Net / Gamepad -> `sub_A1E430`, copying the standard template tuple;
- Keyboard -> `sub_A6C480`, copying the keyboard template tuple.

Only after population does it issue the two allocator calls, each with the identical byte count later used by the memcpy. The tuple has 16-byte elements in a valid 32-bit vector, so its count is at most `0x0FFFFFFF`; `4*count` cannot wrap.

Whole-XREF results close the remaining template-size gap:

- `C799E8/EC/F0` and `C799F8/FC/A00` have size-field writes only in one-time `sub_4307C0` initialization.
- Intermediate references are readers/copies in `sub_835820`, `sub_95AF30`, `sub_A1E430`, and `sub_A6C480`.
- `sub_B16DD0` is the shutdown destructor. No post-init template size writer exists.

Thus even a repeated slot `+0x14` call cannot increase the per-device tuple count after the two buffers are allocated. For nonzero size the two simultaneously live allocator blocks are disjoint and cover the same length; for zero size the copy is vacuous. Heap allocations cannot be the static manager-this slot at `0x00C71678..0x00C7167C`.

## 7. MT-R08 / `0x0068E3DB` inductive proof

### 7.1 Exact operation

- `0x00A8E3DB`: `E8 DD 24 06 00` -> `memmove` at `0x00AF08BD`.
- In `sub_A8E1B0`, the nested buffer tuple is:
  - `this+0xB0`: pointer;
  - `this+0xB4`: capacity (`cap`);
  - `this+0xB8`: used bytes (`used`);
  - `this+0xBC`: consumed prefix (`consumed`).
- The shift is `memmove(buf, buf+consumed, used-consumed)`.

### 7.2 Fixed-point invariant

1. If `cap < 0`, the branch at `0x00A8E3B4` bypasses the memmove completely.
2. If `cap >= 0`, every `sub_A8E1B0` entry zeros `used`, `consumed`, and parser scratch at `0x00A8E249..0x00A8E271`.
3. `sub_AAC3E0` computes `available=used-consumed`. It advances `consumed` only after proving a complete Ogg header/body fits in `available`. Its invalid-page recovery uses `memchr` within the remaining buffer and otherwise selects exactly `used`. Therefore it preserves `0 <= consumed <= used`.
4. Before refill, the caller subtracts `consumed`; consequently `new_used=old_used-consumed` is nonnegative. The memmove source ends at `buf+old_used`, while the destination ends at `buf+new_used`.
5. Refill grows when `cap-used < 4096` and commits `used+read` only when the result is `<= cap` (`0x00A8E46D..0x00A8E489`). This preserves `used <= cap` for the next iteration.

Hence both memmove intervals are within the live malloc/realloc buffer. They may overlap each other by design, which is why the function uses `memmove`; neither interval can overlap the static manager-this slot.

The only direct callers of `sub_A8E1B0` are `0x00A8DB55` and `0x00A8DB6B`, both in `sub_A8DA40`.

## 8. Retained UNKNOWN rows

### 8.1 MT-R07 five BufferedStream rows

RTTI contains only `BufferedStreamBase` and concrete `BufferedStream`, and the secondary-view targets are resolved:

- primary read: `sub_A57DB0` (`0x00657E0A`, `0x00657E94`);
- secondary read: `sub_A582E0` (`0x00658338`, `0x006583C2`);
- secondary write: `sub_A583E0` (`0x00658487`).

The four read memcpys write to caller-owned `a2`. Their local source-chunk bounds do not prove that every indirect Stream caller supplied an output span at least the requested 32-bit product `a3*a4`. A caller with a smaller object is still a retained counterexample.

For `0x00658487`, the apparent unknown virtual call is no longer the first gap: the one concrete type resolves primary vtable `+0x20` to `sub_A57F80`, which flushes and resets buffered `used` to zero. Nevertheless, `Size=a3*a4` and the growth expression `oldExtent+Size+0x1000` have no universal caller bound/no-wrap proof. A wrap can make the requested growth smaller than the following memcpy length. The row therefore stays `UNKNOWN`.

No dynamic scene is requested for MT-R07. Finite runtime samples cannot close the universal indirect-caller and arithmetic claims.

### 8.2 MT-R08 / `0x00661577`

- `sub_A614D0` has exactly two direct callsites: `0x00918DDA` and `0x0091AD62`.
- `sub_91AD50`, reached at `0x0091AD62`, has exactly three callers: `0x00916C7D`, `0x00917E28`, `0x0091875E`.
- At all three, a virtual stream read fills a 32-bit `Size` field; that value is forwarded to `sub_91AD50` without a `Size <= 0x10000` clamp.
- `sub_91AD50` calls `sub_A614D0(&unk_C82C10, 0x10000, Src, Size)` first. Only afterward does it test the returned count and report `"Buffer overflow"`; execution continues.
- Inside `sub_A614D0`, the power-of-two diagnostic also logs and continues. Neither diagnostic dominates the memcpy at `0x00A61577`.

The concrete assignment `Size=0x10001` therefore remains reachable in the static function-level dataflow and overruns the fixed `0x10000` destination. This is a retained, meaningful counterexample, not merely a missing enumeration artifact.

No runtime sample was taken. If reproduction is later authorized, it needs a specific malformed multiplayer/stream payload whose serialized length is `0x10001`, routed through one of `0x00916C7D`, `0x00917E28`, or `0x0091875E`. Ordinary gameplay cannot establish the universal safety property, so dynamic sampling must not be used as a positive proof.

## 9. Integration boundary

- Preserve the six `UNKNOWN` rows exactly as listed.
- Treat the two positives as candidate promotions only until independent TEST review.
- Do not import the excluded `0x00689F63` row from this artifact; v34 already owns that accepted positive.
- This handoff deliberately does not edit v34/v35, total indexes, README indexes, or the final frontier.
