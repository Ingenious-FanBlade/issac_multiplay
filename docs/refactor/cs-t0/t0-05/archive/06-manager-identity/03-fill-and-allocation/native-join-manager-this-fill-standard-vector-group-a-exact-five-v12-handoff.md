> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 standard-vector group A exact5 v12 handoff

## Scope and frozen inputs

This is a create-new, read-only successor for only these five v11 rows, in order:

`0x00308B5A`, `0x00325160`, `0x003271E2`, `0x003272FC`, `0x00330FE1`.

The other ten v11 standard-vector rows and all previously accepted components are excluded. The machine artifact records an empty component intersection. Frozen bindings:

- `out/cs-t0/analyze-manager-this-fill-standard-vector-fifteen-v11.py` — `B9A4C746B11FAAF22B407B4124EBD30FD7F43D34F217C997EEE8150C9BB054DD`
- `out/cs-t0/native-join-manager-this-fill-standard-vector-fifteen-v11.json` — `6580AB34B5050DEABE1FCE60E1EBDC581531A8F8C3A3498E9E3BADC2F65637F6`
- target `isaac-ng.exe` — `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

No game, debugger, process open/read, runtime probe, network test, or IDB mutation was used.

## Exact row adjudication

| RVA | Result | Closure / first gap |
|---|---|---|
| `0x00308B5A` | `UNKNOWN` | The three direct callers select at least two concrete member families (`GameRoot+0x6774C` and another receiver `+0x1E790`). Their combined constructor/init/copy/destructor writer universe is the first unclosed boundary. |
| `0x00325160` | `STATIC_CONFIRMED_NO_OVERLAP` | Unique `n298+0x2A404` object member `+0x118`. Constructor stores `(0,0,0)`; fixed-19 first call must grow and bypass the target; successful grow publishes an allocator-owned 76-byte logical range; reset retains size 19 and takes equality; destructor clears only after the final equality call. Target `memset` is unreachable. |
| `0x003271E2` | `UNKNOWN` | The selected heap record's `+0x0C` tuple allocation/constructor/copy universe is the first unclosed boundary. Helper-local allocation is insufficient to close the incoming record backing phi. |
| `0x003272FC` | `UNKNOWN` | Both direct callers resolve to the `n298+0x2A404` root tuple, but the remaining root-receiver/address-formation helper universe is not fully classified. A slot-address old-backing model remains. |
| `0x00330FE1` | `STATIC_CONFIRMED_NO_OVERLAP` | Same unique object, member `+0x44`. Constructor stores `(0,0,0)`; fixed-16 first call must grow and bypass the target; successful grow publishes 64 bytes; reset retains size 16 and takes equality; destructor clears after final use. Target `memset` is unreachable. |

The result is exactly **2 VERIFIED / 3 UNKNOWN**. The three UNKNOWN rows retain explicit slot-covering countermodels. No batch promotion was made.

## Positive-row proof details

For `+0x118`, the tuple constructor stores are `0x724E24/0x724E2E/0x724E38`; `sub_731F40` installs fresh begin/end/capacityEnd at `0x731FDA/0x731FDF/0x731FE8`. Its count is capped at `0x3FFFFFFF`, so `4*capacity <= 0xFFFFFFFC`; fixed 19 is exactly 76 bytes. The only full global-member displacement hits are one begin load and one end load in `sub_4362A0`; there is no capacity hit, store, or address escape.

For `+0x44`, constructor stores are `0x724D79/0x724D80/0x724D87`; `sub_732020` installs fresh begin/end/capacityEnd at `0x7320BA/0x7320BF/0x7320C8`. The same max-size guard proves non-wrapping `4*capacity`; fixed 16 is exactly 64 bytes. Whole-text scanning finds 23 begin-literal and 21 end-literal hits, all loads; the two effective-address formations are loop-local reads in `sub_75F0E0`; there is no capacity literal or metadata store/callee escape. Same-small-offset stores in `sub_72CFF0` are record-local `ESI` fields and not this root member.

`sub_4149D0` is the terminal vector clear: it frees the allocator backing and then zeros all three tuple fields at `0x414A02/0x414A0B/0x414A12`. Allocation failure in either grow helper occurs before tuple installation, so no half-published or stale backing reaches either target site.

## Reproducer and verification

PE replay (run twice):

```powershell
python out\cs-t0\analyze-manager-this-fill-standard-vector-group-a-exact-five-v12.py --verify-existing
```

Both runs returned:

```text
PASS mode=PE existing=out\cs-t0\native-join-manager-this-fill-standard-vector-group-a-exact-five-v12.json sha256=4D4D3174AA7189394AA7B260583813487A50483856642A0228F2B4FCE9E3B44A verified=2 unknown=3 mutations=31
```

IDA read-only replay in the frozen IDB:

```python
p = r"D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-fill-standard-vector-group-a-exact-five-v12.py"
g = {"__file__": p, "__name__": "ida_replay"}
exec(compile(open(p, "rb").read(), p, "exec"), g, g)
```

Result:

```text
PASS mode=IDA existing=out\cs-t0\native-join-manager-this-fill-standard-vector-group-a-exact-five-v12.json sha256=4D4D3174AA7189394AA7B260583813487A50483856642A0228F2B4FCE9E3B44A verified=2 unknown=3 mutations=31
```

The analyzer checks the PE hash, PE32 image layout, exact signatures, 13 function-byte hashes, raw E8 caller universes, literal-hit universes, exact partition, schema, permissions, and 31/31 fail-closed semantic mutations. In IDA mode it additionally checks every recorded function start and size against the IDB.

Frozen IDB after replay: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`, SHA-256 `992A16DC789B5714C337CDA34E135EF2DDC8CFE3A4F32378673B20B071F8909F`, length `110648172`, last write UTC `2026-08-30 19:04:49`.

## Decision boundary

All permissions, promotions, and `overallAccepted` are `false`. This artifact does not unlock T0-05-05 and does not authorize runtime work. The two positive rows are individually consumable only after independent review; the three UNKNOWN rows must remain frozen at the gaps above.
