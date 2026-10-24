> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R06 / MT-R07 / MT-R08 successor v3 count-correction handoff

## Outcome

Independent TEST correctly identified that v2 mixed two different census populations. The row verdicts are unchanged, but v2's phrase “81 unique methods in the four complete vtables” is inaccurate.

V3 is append-only and separates the populations explicitly:

- `uniqueVtableMethodCount = 75`
- `additionalConstructorPathFunctionCount = 5`
- `targetFunctionCount = 1`
- `scannedFunctionCount = 81`
- hit partition: `18 vtable-method hits + 2 Base-constructor hits + 3 target-function read hits = 23`

The v2 builder, candidate, and handoff are preserved unchanged.

## Artifacts

| File | SHA-256 |
|---|---|
| `out/cs-t0/build-manager-this-mt-r06-r07-r08-successor-v3.py` | `4A500EAFAB9B16CADA4D2E9005D0A1D891FB0AF73E6AA9773188F3FCF975A276` |
| `out/cs-t0/native-join-manager-this-mt-r06-r07-r08-successor-v3.json` | `E204077A0A6C922BA1163F8CC4247594079E6116D41C99D2470E5E4FFA5568F8` |

V3 binds the v2 builder, candidate, and handoff by SHA-256, retains the v2 public frozen-PE proof atoms, and adds a machine-readable correction block and partitioned R06 census.

## Read-only reproduction

IDA Pro MCP remained connected to:

- IDB: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`
- image base: `0x00400000`
- auto-analysis / Hex-Rays: ready

The recomputation used the four vtables specified by v2:

- `0x00B78030` (`NetInputDevice`), 38 slots
- `0x00B82758` (`InputDeviceBase`), 38 slots
- `0x00B82C80` (`InputDeviceGamepad`), 38 slots
- `0x00BA1678` (`InputDeviceKeyboard`), 38 slots

Their 152 raw function-pointer entries deduplicate to exactly 75 functions. Scanning only these 75 functions for decoded, non-stack displacement operands `+0xC0/+0xC4` returns 18 hits.

## Exact set difference

The original v2 analysis script had explicitly appended these six functions before reporting its 81/23 totals:

| VA | Function | Population | C0/C4 hits |
|---|---|---|---:|
| `0x009015B0` | `sub_9015B0` | Net constructor | 0 |
| `0x00901930` | `sub_901930` | Net constructor | 0 |
| `0x00A1E050` | `sub_A1E050` | Base constructor | 2 writes |
| `0x00A21980` | `sub_A21980` | Keyboard construction path | 0 |
| `0x00A21D40` | `sub_A21D40` | Gamepad construction path | 0 |
| `0x00A1F280` | `sub_A1F280` | target non-vtable method | 3 reads |

Therefore:

```text
75 unique vtable functions
+ 5 additional constructor/path functions
+ 1 target non-vtable function
= 81 scanned functions

18 vtable-method hits
+ 2 Base-constructor writes
+ 3 target-function reads
= 23 scanned hits
```

TEST's `20`-hit reproduction is also correct for its stated population: 75 vtable functions plus only `sub_A1E050` gives `18 + 2 = 20`.

## Writer conclusion

The count correction does not change the writer fixed point:

- vtable population writers: `sub_A1E210`, `sub_A1E2C0`;
- explicit non-vtable writer: `sub_A1E050`;
- `sub_A1F280` contributes three reads, not writes;
- the other four construction-path functions contribute no `+0xC0/+0xC4` hit.

The complete pointer-writer function set remains exactly:

```text
sub_A1E050
sub_A1E210
sub_A1E2C0
```

## Verdicts and integration boundary

Verdicts remain identical to v2:

- `STATIC_CONFIRMED_NO_OVERLAP`: `0x0061F2C2`, `0x0068E3DB`
- `UNKNOWN`: `0x00657E0A`, `0x00657E94`, `0x00658338`, `0x006583C2`, `0x00658487`, `0x00661577`

V3 remains a REVERSE candidate requiring independent TEST review. No game/process/debugger was used, the IDB was not modified, and no index, frontier, v34, or v35 file was edited.

Essential builder commands used for the create-new cycle:

```powershell
python out\cs-t0\build-manager-this-mt-r06-r07-r08-successor-v3.py
python out\cs-t0\build-manager-this-mt-r06-r07-r08-successor-v3.py --verify-existing
python out\cs-t0\build-manager-this-mt-r06-r07-r08-successor-v3.py --self-test-mutation
```

The one permitted create-new, one verify-existing, and one core mutation run returned `CREATED`, `PASS verify-existing`, and `PASS core count mutation rejected`. After TEST requested the exact public alias names (`uniqueVtableMethodCount`, `additionalConstructorPathFunctionCount`, `targetFunctionCount`, `scannedFunctionCount`, and `hitPartition`), those metadata-only fields were added synchronously to builder and JSON without changing the population sets or row verdicts. To preserve the one-verify budget, REVERSE did not run a second verify; independent TEST should use the public `--verify-existing` command against the final hashes above. No Cargo/full build or broad test suite was run.
