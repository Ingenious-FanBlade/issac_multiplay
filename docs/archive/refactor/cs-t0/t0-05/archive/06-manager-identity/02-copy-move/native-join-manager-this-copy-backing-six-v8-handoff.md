> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this copy backing six v8 handoff

## 结论

本轮只复核冻结 v7 中精确 6 个 `VERIFIED_BOUNDED` RVA 的 backing pointer/object 来源；range inequality 直接绑定 v7，不在 v8 重证。

最终逐行结果：

| callsite RVA | helper | backing | v7 range | composite no-overlap | 主要来源 / 剩余义务 |
| --- | --- | --- | --- | --- | --- |
| `0x001CBEB5` | `memmove` | `UNKNOWN` | `VERIFIED_BOUNDED` | `UNKNOWN` | 4 个直接 caller 已穷尽；已识别 GameRoot 内嵌 vector 与两个 EBP-local vector，但 begin-pointer 的全部 constructor/writer closure 未闭合 |
| `0x003534FE` | `memmove` | `UNKNOWN` | `VERIFIED_BOUNDED` | `UNKNOWN` | 4 个直接 caller 已穷尽；3 条 local vector 路径与一条 dynamic object `+0x1754` 路径中，后者及 local writer set 尚未全部闭合 |
| `0x0035352E` | `memmove` | `UNKNOWN` | `VERIFIED_BOUNDED` | `UNKNOWN` | 与 `0x003534FE` 共用同一 receiver/source universe，alternate in-place branch 不改变 backing 义务 |
| `0x005B91EF` | `memmove` | `UNKNOWN` | `VERIFIED_BOUNDED` | `UNKNOWN` | 3 个直接 caller 均传入 GameRoot-derived base `+0x1BAA8`；constructor 直接清零已定位，但通过 receiver alias 发生的全部 vector growth/assignment writer 尚未穷尽 |
| `0x00688B74` | `memcpy` | `VERIFIED_BACKING` | `VERIFIED_BOUNDED` | `STATIC_CONFIRMED_NO_SLOT_OVERLAP` | miniz `state+0x74` 唯一非零 writer 来自 `sub_A89DE0` 的 arg0；其两个直接 caller 均先通过 `sub_A0F4E0 -> sub_A648B0 -> malloc` 分配 destination |
| `0x006957BC` | `memcpy` | `VERIFIED_BACKING` | `VERIFIED_BOUNDED` | `STATIC_CONFIRMED_NO_SLOT_OVERLAP` | png object 由 `sub_A63A10` 分配；`png+0xA4` 由独立 `malloc(0x2000)` 写入，完整 zlib caller cone 只把该字段复制到内嵌 stream `next_out` |

因此：

```text
verifiedBackingCount = 2
unknownBackingCount = 4
compositeNoOverlapCount = 2
compositeUnknownCount = 4
sixRowBackingFrontier = UNKNOWN
```

v8 没有把“已识别 receiver/object”当成 backing closure。只有 `VERIFIED_BACKING` 与冻结 v7 的 `VERIFIED_BOUNDED` 同时成立，才产生 composite no-overlap。

## 冻结绑定

Range-only evidence：

- `out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json`
- SHA-256：`C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D`

目标 IDA input：

- SHA-256：`1FBA9A37E5E8B572A95F8B92609A0BC52DA6165A219FF6B406EE5E6C7104B89C`
- image base：`0x00400000`

机器 JSON 对每行保留 `v7RangeBinding`，并显式设置 `inequalityReusedWithoutReproof=true`。

## `0x00688B74` backing closure

该行 destination 是：

```text
DWORD[miniz_state+0x74] + DWORD[miniz_state+0x8C]
```

完整来源集合：

1. `sub_A89A60` 在 `0x00A89B50/0x00A89B5E/0x00A89B83` 把 `+0x74/+0x7C/+0x8C` 初始化为 0；
2. `sub_A89DE0` 在 `0x00A89E33` 把外部 arg0 写入 `state+0x74`，在 `0x00A89E36` 写入 output-end holder，并在 `0x00A89E48` 清零 cursor；
3. `sub_A89DE0` 的完整 direct caller universe 只有 `0x00918C19` 与 `0x0091AD36`；
4. `sub_918870` 在 `0x00918BC5` 先调用 `sub_A0F4E0(Size)`，再把结果传到 `0x00918C19`；
5. `sub_91ACC0` 在 `0x0091ACE0` 先调用 `sub_A0F4E0(size)`，再把结果传到 `0x0091AD36`；
6. `sub_A0F4E0 -> sub_A648B0`，后者在 `0x00A64934` 执行 `malloc(request+4)`，并在 `0x00A64959` 返回 allocation `+4`。

`sub_A88650` 的 4 个直接调用点 `0x00A88F7C/0x00A8905A/0x00A89995/0x00A89F13` 全部收敛回 `sub_A89DE0`。因此非零 output base 的 constructor/writer/caller closure 已完整，且来源为 allocator 返回对象，不是固定 image slot alias。

## `0x006957BC` backing closure

该行 destination 是 zlib-style stream 的 `next_out`：

```text
DWORD[z_stream+0x0C]
= DWORD[png_struct+0x78]
= DWORD[png_struct+0xA4]
```

完整来源集合：

1. `sub_A63A10` 在 `0x00A63A3B` 分配 `0x20C` bytes 的 png object，并在 `0x00A63A4E` 清零；
2. 同一 constructor 在 `0x00A63AC1` 设置 `png+0xA8 = 0x2000`，在 `0x00A63ACB` 执行 `malloc(0x2000)`，并在 `0x00A63AD8` 把结果写入 `png+0xA4`；
3. `sub_A63A10` 的完整直接 caller 只有 `sub_9E86E0` 的 `0x009E871C`；返回对象保存到 local `v37[0]` 并贯穿后续 write pipeline；
4. 可到达的 stream `next_out` writers 为 `sub_A911E0/sub_A92640/sub_A932C0/sub_A93480/sub_A94B10/sub_A63DB0`；它们只把 `png+0xA4` 复制到 `png+0x78`；
5. `sub_A957F0` 的完整 5 个直接 caller 为 `0x00A63DD5/0x00A92704/0x00A92818/0x00A93598/0x00A94B2A`；
6. 这些 caller 的完整上游链分别经 `sub_A63DB0/sub_A92640/sub_A93480/sub_A94B10`，最终全部回到 `sub_9E86E0` 持有的同一 `sub_A63A10` 返回对象。

因此 `next_out` backing 为 constructor-owned `malloc(0x2000)` buffer，不是固定 image slot alias。

## 四条 UNKNOWN 的精确边界

### `0x001CBEB5`

`sub_5CBEA0` 的 4 个直接 caller 已穷尽：

```text
0x005C49FD, 0x005C4A8E, 0x00688D38, 0x00688E1F
```

其中 `0x005C49FD` receiver 可追到 GameRoot-derived base `+0x1A280 + index*0x5DC`；`0x00688D38/0x00688E1F` 是 EBP-relative local vector。仍需闭合嵌入对象及 local vector 的所有 begin-pointer writers。

### `0x003534FE` / `0x0035352E`

共享的 4 个直接 caller 已穷尽：

```text
0x007413E3, 0x0074157B, 0x007A3A2F, 0x0082C96C
```

前三个 caller context 中有 EBP-local vector，`0x007A3A2F` 则使用 dynamic receiver `+0x1754`。`sub_753390` 自身的 growth path 可见 `sub_40CF00` allocation 与 `sub_41D150` triple install，但这不能替代进入 in-place branch 前所有既有 begin-pointer writers 的 closure。

### `0x005B91EF`

`sub_9B9150` 的完整 3 个直接 caller：

```text
0x0042A5A3, 0x006FA437, 0x0090E2D4
```

三者均以 GameRoot-derived base `+0x1BAA8` 为 receiver。`sub_6F1020` 在 `0x006F161D/0x006F1627/0x006F1631` 直接清零完整 vector triple；whole-image exact-displacement store scan 未发现其他直接字段 store。不过，程序中仍有大量先 `LEA GameRoot+0x1BAA8` 再通过 receiver alias 调用 vector helper 的路径，尚未完成全部 growth/assignment writer 分类，因此保持 UNKNOWN。

## 输出与复现

Machine JSON：

- `out/cs-t0/native-join-manager-this-copy-backing-six-v8.json`
- SHA-256：`1F45AC9ADA79174114C8C74C63F4C05CD88304DE44178A8A77A8C65DD994D3D9`

Create-new offline builder/reproducer：

- `out/cs-t0/build-native-join-manager-this-copy-backing-six-v8.py`
- SHA-256：`AB2AB24F641B7C47B2A89A89B8815622902D3247CEE82CB0776D247F11CEE047`

复核已生成 JSON：

```powershell
python out/cs-t0/build-native-join-manager-this-copy-backing-six-v8.py --verify-existing
```

Builder 会 fail-closed 校验：

- v7 SHA 必须精确匹配；
- v7 必须包含精确 6 个目标 RVA；
- 6 行在 v7 中必须全部仍为 `VERIFIED_BOUNDED`；
- 输出必须与内嵌 read-only evidence snapshot 的 deterministic rebuild 完全一致。

本轮未启动游戏、未附加调试器、未打开目标进程、未修改 IDB；未改 production、tests、v1-v7、index 或既有证据文件。
