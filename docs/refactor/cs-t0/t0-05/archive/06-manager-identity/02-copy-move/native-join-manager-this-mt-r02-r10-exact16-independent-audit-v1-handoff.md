> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02–R10 exact16 independent TEST audit v1 handoff

## Outcome

The append-only exact16 audit passes with two new positive rows and fourteen retained `UNKNOWN` rows:

- `STATIC_CONFIRMED_NO_OVERLAP`: `0x0061F2C2`, `0x0068E3DB`.
- `UNKNOWN`: `0x00008A62`, `0x00008B2A`, `0x0001513A`, `0x000155A4`, `0x0033570B`, `0x00657E0A`, `0x00657E94`, `0x00658338`, `0x006583C2`, `0x00658487`, `0x00661577`, `0x0001B072`, `0x0001B3E3`, `0x002EE04F`.
- MT-R04 is excluded from the audit. No R04 verdict, mutation, or promotion was emitted. Its rows `0x0001AAB0` and `0x0001AEB8` remain in the expected successor frontier.

This TEST audit did not read or import any REVERSE builder. It did not modify any candidate, prior evidence, status file, or index.

## Artifacts

| File | SHA-256 |
|---|---|
| `out/cs-t0/audit-manager-this-mt-r02-r10-exact16-independent-v1.py` | `9AF1CC9EC2FA8AAE066035EDAACE7B88C2D20994E3194CFCFD39891F9B65D4EF` |
| `out/cs-t0/native-join-manager-this-mt-r02-r10-exact16-independent-audit-v1.json` | `064079AB81F4FD18079209444E4718242A04B08C602810133B7FE4212FC8ABDB` |

The audit binds the five per-task UNKNOWN successors, corrected MT-R06/R07/R08 v3 candidate, v32 frontier and audit, and the v34 completion checkpoint and audit. It independently verifies the frozen PE identity/layout, all 16 exact call bytes and rel32 helper targets, 187 unique candidate proof atoms, 39 non-builder declared bindings, candidate permissions, and the reconstructed frontier diff.

## Independent positive verdicts

### MT-R06 / `0x0061F2C2`

The first REVERSE v2 mixed the vtable-only and appended-function denominators. TEST independently found the drift and rejected that wording. The append-only v3 correction is exact:

```text
4 vtables x 38 slots = 152 raw entries
75 unique vtable methods, 18 non-stack +0xC0/+0xC4 hits
+ 5 constructor/path functions, including Base ctor's 2 writes
+ 1 target non-virtual function, with 3 reads
= 81 scanned functions and 23 hits
```

The independently reconstructed pointer-writer set is exactly `sub_A1E050`, `sub_A1E210`, and `sub_A1E2C0`. The other hits are reads. The four RTTI-backed InputDevice types are `InputDeviceBase`, `NetInputDevice`, `InputDeviceGamepad`, and `InputDeviceKeyboard`.

`sub_A1E210` populates the 16-byte-element source tuple through vtable slot `+0x14` before making two equal-length allocations. The copy length is `4*((end-begin)>>4)`, so a valid 32-bit vector cannot overflow the four-byte multiplication. For nonzero length, the two simultaneously live allocations are distinct and each covers the transfer; zero length is vacuous. The heap intervals cannot be the static manager-this slot.

### MT-R08 / `0x0068E3DB`

TEST independently decompiled `sub_A8E1B0`, `sub_AAC3E0`, and both direct caller sites. `cap<0` bypasses the memmove. Otherwise the outer routine first resets `used=consumed=0`; the parser only advances `consumed` after a complete header/body fits in `used-consumed`, while recovery selects an in-buffer `memchr` result or exactly `used`. This inductively preserves `0<=consumed<=used`.

The shift therefore uses the nonnegative length `old_used-consumed`. Its source ends at `old_used`, its destination ends at the new used count, and refill commits only when `used+read<=cap`. Both shifted intervals remain in the live malloc/realloc block and outside the manager-this slot. Source/destination overlap is intentional `memmove` behavior.

## Retained UNKNOWN boundaries

| Task / rows | First independently retained boundary |
|---|---|
| MT-R02 / `0x00008A62`, `0x00008B2A` | `oldCount=2,newCount=1` remains a legal open case: allocation covers 4 bytes while the copy requests 8. |
| MT-R03 / `0x0001513A`, `0x000155A4` | The `5000` diagnostic rejoins. The minimum open count is 5001, exceeding the respective 420000/540000-byte fixed backing. |
| MT-R05 / `0x0033570B` | The feasible conjunction `auxiliaryCount>0 && filteredCount==0` is not closed; it yields a `0xFFFFFFFC` subtraction result. |
| MT-R07 / five rows | Caller-owned output span and `a3*a4` no-wrap are not universal; the write row also leaves `oldExtent+Size+0x1000` no-wrap open. |
| MT-R08 / `0x00661577` | `Size=0x10001` reaches the fixed `0x10000` ring before the post-copy overflow report. Direct caller count is 2; the wrapper has 3 callers. |
| MT-R09 / two rows | The distinct roots `(+0x94,+0x9C)` and `(+0x44,+0x4C)` each still admit the minimum open source count `0x8001`; the diagnostic continues. |
| MT-R10 / `0x002EE04F` | `n4=10` requests 36 bytes for a 32-byte stack destination. Of 22 direct writers, 19 are locally safe and 3 value writers plus 28 LEA aliases remain open. |

No dynamic scene is needed for this audit. A future R08 reproduction would require the specific malformed serialized `Size=0x10001` stream path, but such a sample is unnecessary for retaining `UNKNOWN` and cannot prove a universal positive.

## Successor frontier expectation

The audit does not create or edit the frontier. It emits the exact expected composition data for main integration:

- base: v32, SHA-256 `A336FF06F05A11CFF97CE937BF0E8CEE030E498B3E7C401A811021EFB665EFF3`;
- remove only `0x0061F2C2` and `0x0068E3DB` from copy/move remaining;
- copy/move: `321/405 -> 323/405`, remaining `84 -> 82`;
- memset unchanged: `225/242`, remaining `17`;
- whole ledger: accepted `548/647`, remaining `99`;
- R04 rows remain in the 82-row copy/move successor list;
- manager-this, whole-image, and overall promotion permissions remain false.

## Compressed verification ledger

Only the requested essential tests were run:

1. One successful create-new audit: PASS, output SHA-256 `064079AB81F4FD18079209444E4718242A04B08C602810133B7FE4212FC8ABDB`.
2. Exactly one verify-existing: PASS, byte-identical SHA-256.
3. Six core in-memory mutations rejected: exact-row omission, fake promotion, writer-universe omission, countermodel removal, frontier/R04 drift, and permission enablement.
4. One wrong-candidate-hash probe: rejected before write; probe output was absent both before and after.

No Cargo test, full T0 build, broad suite, repeated self-test, game start, process open/read, debugger attach, runtime sample, or IDB mutation was performed.
