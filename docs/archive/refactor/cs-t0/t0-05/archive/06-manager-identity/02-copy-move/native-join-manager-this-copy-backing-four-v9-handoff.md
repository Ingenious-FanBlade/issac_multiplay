> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy backing exact-four v9 handoff

## Outcome

This create-new offline artifact processes only the four rows that were `UNKNOWN` in frozen copy-backing-six-v8. It does not revisit the two v8 `VERIFIED_BACKING` rows.

| callsite RVA | v9 backing status | frozen range-v7 status | composite result |
|---|---|---|---|
| `0x001CBEB5` | `UNKNOWN` | `VERIFIED_BOUNDED` | `UNKNOWN` |
| `0x003534FE` | `VERIFIED_BACKING` | `VERIFIED_BOUNDED` | `STATIC_CONFIRMED_NO_SLOT_OVERLAP` |
| `0x0035352E` | `VERIFIED_BACKING` | `VERIFIED_BOUNDED` | `STATIC_CONFIRMED_NO_SLOT_OVERLAP` |
| `0x005B91EF` | `UNKNOWN` | `VERIFIED_BOUNDED` | `UNKNOWN` |

The exact-four result is therefore `2 VERIFIED_BACKING / 2 UNKNOWN`. Exactly two rows satisfy the composite rule. This artifact alone does not decide the complete manager-this promotion.

## Frozen bindings and scope

- `out/cs-t0/native-join-manager-this-copy-backing-six-v8.json`
  - SHA-256: `1F45AC9ADA79174114C8C74C63F4C05CD88304DE44178A8A77A8C65DD994D3D9`
  - Role: exact four-row `UNKNOWN` frontier.
- `out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json`
  - SHA-256: `C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D`
  - Role: range inequalities only; all four relevant rows are reused as `VERIFIED_BOUNDED` without re-proof.
- Explicitly excluded from analysis: v8 callsites `0x00688B74` and `0x006957BC`, because v8 already classified both `VERIFIED_BACKING`.

The reverse pass used only offline, read-only IDA queries. No game was started, no process or debugger was opened, and no IDB or existing evidence file was modified.

## `0x003534FE` and `0x0035352E`: final `VERIFIED_BACKING`

Both callsites are in-place branches of the same `sub_753390` four-byte-vector insertion helper and share one receiver/writer universe. The direct caller set remains exactly:

- `0x007413E3` and `0x0074157B` in `sub_740E10`, using the same EBP-relative local triple;
- `0x007A3A2F` in `sub_7A37C0`, using dynamic object member `+0x1754`;
- `0x0082C96C` in `sub_82C7D0`, using an EBP-relative local triple.

The local receivers are closed:

- `sub_740E10` zeros begin/end/cap at `0x00741326/0x00741330/0x00741336`. Its two inserts call `sub_753390`; its later whole-vector assignment can change begin only through `sub_6A0D30` at `0x00741684`. The local backing is released at `0x00741C74`.
- `sub_82C7D0` zeros begin/end/cap at `0x0082C8F3/0x0082C8F6/0x0082C8FC`. Its insert calls `sub_753390`; its later whole-vector assignment can change begin only through `sub_6A0D30` at `0x0082C9D7`. The local backing is released at `0x0082CB95` or `0x0082CDA1`.

The dynamic `object+0x1754` receiver is closed by the complete rendered whole-image `0x1754h/0x1758h/0x175Ch` occurrence inventory:

- constructor `sub_753DA0` zeros the triple at `0x0075431E/0x00754328/0x00754332`;
- `sub_7A37C0` forwards it directly to `sub_753390` at `0x007A3A2F`;
- `sub_7AE4B0` resets end to begin, reserves through `sub_42C880` at `0x007AE7BF`, and can append through `sub_42C920` at `0x007AE817`;
- `sub_9BD3B0` copies another `+0x1754` vector and can reserve the destination only through `sub_6A0D30` at `0x009BDBCA`;
- `sub_78F840` changes end only after an in-place erase;
- `sub_40A030` and `sub_40A1B0`, reached by the `sub_9ECDD0/sub_9ED3C0` forwarders, read or clear elements through already-owned storage and do not write the vector begin;
- the remaining rendered `0x1754` occurrences are tuple reads or use `+0x1754` as the source of an assignment into a different vector.

Every possible non-null begin writer in this closed universe is allocation-derived:

- `sub_753390` growth: `sub_40CF00` at `0x0075341F`, installed through `sub_41D150` at `0x007534AB`;
- `sub_6A0D30` reserve: `sub_40CF00` at `0x006A0D96`, installed directly at `0x006A0D9B/0x006A0D9D/0x006A0DA2`;
- `sub_42C880` reserve: `sub_40CF00` at `0x0042C8A8`, installed through `sub_41D150` at `0x0042C8C6`;
- `sub_42C920` growth: `sub_40CF00` at `0x0042C971`, installed through `sub_41D150` at `0x0042C9E3`.

`sub_40CF00` returns `sub_A0F4C0` storage directly for small allocations or an aligned address inside a larger `sub_A0F4C0` allocation. No closed begin writer imports an image address or a fixed image slot address. Thus both rows are finally `VERIFIED_BACKING`. Combined with their frozen range-v7 `VERIFIED_BOUNDED` rows, both become `STATIC_CONFIRMED_NO_SLOT_OVERLAP`.

## `0x001CBEB5`: retained `UNKNOWN`

The four direct callers remain complete: `0x005C49FD`, `0x005C4A8E`, `0x00688D38`, and `0x00688E1F`.

Two receiver families were closed:

- The `sub_686DA0` EBP-relative local vector used by `0x00688D38/0x00688E1F` is produced by `sub_82CE40`. That producer zeros its output triple at `0x0082CEC6/0x0082CECC/0x0082CED3`; inline append changes end only, while growth calls `sub_42C920` at `0x0082CFC0`. The relevant lifetime is destroyed through `sub_4149D0` at `0x00688EC8`.
- The object `+0x1FA4` vector used by `0x005C4A8E` is zeroed by `sub_753DA0` at `0x0075475B/0x00754765/0x0075476F`. Its restoring assignment in `sub_7AE4B0` can change begin only through `sub_6A0D30` at `0x007AF5AD`; `sub_7A42B0`, `sub_7D8240`, and `sub_5CBEA0` operate in place and do not replace begin.

The remaining receiver at `0x005C49FD` is formed by:

```text
sub_706780(GameRoot) + 0x1A280 + index * 0x5DC
```

at `0x005C496E/0x005C497F/0x005C4985`. A complete constructor plus alias-writer/forwarder inventory for that embedded tuple was not established. The row therefore remains `UNKNOWN`, even though its other receiver paths are allocation-closed. Frozen range-v7 cannot promote an `UNKNOWN` backing row.

## `0x005B91EF`: retained `UNKNOWN`

The direct caller set is closed: `0x0042A5A3`, `0x006FA437`, and `0x0090E2D4`, each passing the GameRoot-derived Player-manager vector at `+0x1BAA8`.

Confirmed facts:

- `sub_6F1020` zeros begin/end/cap at `0x006F161D/0x006F1627/0x006F1631`; the exact-displacement whole-image scan found no other begin store.
- `sub_9B9CD0` appends new `Player*` values. Its two capacity-growth sites, `0x009B9D82` and `0x009B9EBA`, call `sub_42C920`, whose replacement begin comes from `sub_40CF00`.
- `sub_9BA000` and `sub_9B9150` erase in place and change end only. The target `memmove` at `0x009B91EF` never writes begin.

The whole image contains a large number of `LEA/ADD` forwarders that form `GameRoot+0x1BAA8`. This pass did not finish a complete per-forwarder classification proving that every alias-mediated assignment/growth path is covered by the confirmed allocator chain. Exact-displacement stores alone are insufficient for that claim, so the row remains `UNKNOWN`. Frozen range-v7 cannot promote it.

## Artifacts and reproduction

- Machine-readable result: `out/cs-t0/native-join-manager-this-copy-backing-four-v9.json`
- Deterministic reproducer: `out/cs-t0/build-native-join-manager-this-copy-backing-four-v9.py`
- This handoff: `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-copy-backing-four-v9-handoff.md`

Reproduction is create-new-only. From the repository root:

```powershell
python out/cs-t0/build-native-join-manager-this-copy-backing-four-v9.py --output <new-path>
```

Verify the checked-in output without rewriting it:

```powershell
python out/cs-t0/build-native-join-manager-this-copy-backing-four-v9.py --verify-existing
```

Independent acceptance remains a separate test task; this reverse handoff does not self-certify that test boundary.
