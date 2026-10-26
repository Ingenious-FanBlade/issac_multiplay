# tx-04-07 ST5：Baby 标志静态逆向

## 范围与证据口径

本记录仅针对当前共享 IDB `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，输入映像为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase 为 `0x400000`。下文地址均同时给出 VA 与 RVA，计算式为 `RVA = VA - 0x400000`。IDA 的函数名和局部变量名只用于定位；结论以原始指令、字节、xref 和官方 `.zhl` 签名为依据。IDB 未作命名、注释、类型或补丁修改。

更正记录：早期导出曾把 VA `0x6F0AF0` 标成 `Game::constructor`；按完整官方签名核对后，真正签名匹配在 VA `0x6ED4B0`。已将 `0x6F0AF0` 改为中性的 ExecuteStartGame Prepare 标签。此更正只影响函数标签，`ExecuteStartGame → 0x6F0AF0 → 0x6F0770 → ResetState` 的清位数据流不变。

状态标签：`STATIC_CONFIRMED` 表示本 IDB 的原始指令/xref 直接支持；`INFERRED` 表示由控制流上下文与官方源码枚举推断；`SOURCE` 仅说明官方源的声明/枚举映射；`UNKNOWN` 标记当前静态证据没有闭合的范围。

## Predicate 与三个 Join 调用点

**`STATIC_CONFIRMED` — 标志位置。** `PlayerManager::CoopBabiesOnly` 位于 VA `0x9910A0` / RVA `0x5910A0`。机器码先从绝对地址 `0xBE49F4` 读出对象指针，再读取 `[pointer+0x25AC0]`，随后 `and eax, 0x10` 并返回布尔结果。因此本次 Join predicate 使用的 Baby 判定位为对象低 dword `+0x25AC0` 的 bit 4（mask `0x10`）。这里不依赖 IDA 的 `n187` 符号名。

函数完整导出见 `PlayerManager_CoopBabiesOnly.json`。对该函数入口的代码 xref 清单恰有三处，均位于 `PlayerManager::Join` 的函数体 VA `0x991360` / RVA `0x591360`：

| Predicate 调用 VA / RVA | 调用前可从原始控制流确认的条件 | Predicate 返回后的分支 |
| --- | --- | --- |
| `0x9915AE` / `0x5915AE` | `cmp [ebx+0x48],esi` 后 `jnz 0x991670`，故相等；`cmp dword ptr [ebx+0x50],0` 后 `jl 0x991670`，故非负。通过两项检查后调用。 | `test al,al`; 为零跳到 `0x9915F3`，非零继续检查当前 vector 长度。 |
| `0x99174C` / `0x59174C` | `eax < 0x40`（`0x99170E` 比较，`jnb 0x991828`）；局部字节为零（`0x991717`，否则跳 `0x991833`）；`0x99172C` 的 helper 返回非零；`sub_9911C0` 返回值非负（`0x99173C` 调用、`0x991741` 测试，负值跳 `0x991E08`）。 | 返回非零时调用 VA `0x994340` / RVA `0x594340`，随后跳 `0x991E08`；返回零走后续路径。 |
| `0x991E67` / `0x591E67` | 循环比较 `edi` 与 `esi` 后，`edi < esi` 会回跳 `0x9913D4`；通过后令 `edi=ebx+0x14`，局部计数为 4；`[edi-8]` 的 `test` 结果不得为负（否则去 `0x9925EA`）；`[edi]` 不得小于零（否则也去 `0x9925EA`）。 | 返回非零跳 `0x9925E4`；返回零继续到传入常数 `0x14` 的 helper 检查。 |

上表只把比较和分支写成原始条件，不给 `ebx`、`edi`、helper 或局部变量附加未经证实的语义。三处调用及其前后完整指令都在 `PlayerManager_JoinPass.json` 中。

## 切房置位与普通阶段切换

**`STATIC_CONFIRMED` — 切房置位。** `Level::ChangeRoom` 位于 VA `0x739DE0` / RVA `0x339DE0`。函数体内 VA `0x739E60` / RVA `0x339E60` 的原始指令为：

```asm
8B 81 C4 5A 02 00    mov eax, [ecx+25AC4h]
83 89 C0 5A 02 00 10 or dword ptr [ecx+25AC0h], 10h
89 81 C4 5A 02 00    mov [ecx+25AC4h], eax
```

该基本块从函数前段顺序落入；置位前没有条件跳转，随后才比较 `[edi]` 并分支。因此只要执行到该 `ChangeRoom` 函数体，这条路径会无条件 OR 入 bit 4，同时原样保存/写回 `+0x25AC4`。完整函数见 `Level_ChangeRoom.json`。

**`STATIC_CONFIRMED` — `Level::Init` 不清 Baby 位。** 函数 VA `0x73EB10` / RVA `0x33EB10` 在 VA `0x73EB5E` / RVA `0x33EB5E` 读 `[ecx+0x25AC0]` 到 `eax`；VA `0x73EB64` / RVA `0x33EB64` 执行 `and dword ptr [ecx+0x25AC4],0xFFFFEFFF`，清的是相邻 dword 的 `0x1000` 位；VA `0x73EB6E` / RVA `0x33EB6E` 将未改动的 `eax` 写回 `[ecx+0x25AC0]`。这不是 Baby bit 的清位。

**`STATIC_CONFIRMED` — `Level::SetNextStage` 清的是另一位。** VA `0x7410A2` / RVA `0x3410A2` 的指令为 `and dword ptr [esi+25AC0h],0FFF7FFFFh`，mask 清除 `0x00080000`，不影响 bit 4。该函数完整导出见 `Level_SetNextStage.json`；它的直接调用清单没有 `Game::ResetState`。

**`UNKNOWN` — 普通阶段切换的全局闭包。** 已核对的 `Level::Init` 保留 `+0x25AC0` 原低 dword，已核对的 `Level::SetNextStage` 仅清另一位；这两项足以否定“仅因相邻调用 NextStage/Init 就能认定 Baby 位被重置”。但这里没有穷尽所有可能的间接回调执行和别名写入，故不把有限函数清单扩成“所有普通换层路径绝不会清位”的绝对结论。完整 `.text` 直接偏移访问清单在 `GameField_25AC0_all_accesses.json`，共 88 条；其中地址形成后传入 helper 的项目没有做无限间接写闭包。

## 初始化、Continue 保存与恢复

**`STATIC_CONFIRMED` — ExecuteStartGame 初始化路径会先清位。** `Manager::ExecuteStartGame` 在 VA `0x932AE0` / RVA `0x532AE0` 调用 IDA 自动命名为 `sub_6F0AF0` 的函数；该函数在 VA `0x6F0B36` / RVA `0x2F0B36` 调用初始化子程序 VA `0x6F0770` / RVA `0x2F0770`；后者在 VA `0x6F07A1` / RVA `0x2F07A1` 调用 `Game::ResetState`。`Game::ResetState` 入口为 VA `0x6F08D0` / RVA `0x2F08D0`，其 VA `0x6F0977` / RVA `0x2F0977` 原始指令 `mov dword ptr [ecx+25AC0h],0` 将该完整低 dword 清零，包含 Baby bit 4。完整官方 `Game::constructor` 字节签名另匹配 VA `0x6ED4B0` / RVA `0x2ED4B0`；它不是 `0x6F0AF0`，本结论不把两者混为一谈。

`Manager::ExecuteStartGame` 在构造调用之后到 VA `0x932DD9` / RVA `0x532DD9` 检查 `[edi+0x4AD11]`。为零时跳到 `0x932EF6`；非零路径继续到 VA `0x932E7E` / RVA `0x532E7E`，以 `startGame=1` 调用 `Game::RestoreState`（VA `0x6F43A0` / RVA `0x2F43A0`）。任务上下文把该非零分支标为 Continue；二进制证据本身确认的是上述字节条件和调用参数。

**`STATIC_CONFIRMED` — SaveState 包含 Baby bit。** `Game::SaveState` 位于 VA `0x6F51C0` / RVA `0x2F51C0`。VA `0x6F5286` / RVA `0x2F5286` 将记录数设为 `0x35`（53）；循环索引从 0 到 52。循环在 VA `0x6F52B4` / RVA `0x2F52B4` 读取 `[Game + (index>>6)*8 + 0x25AC0]`，结合 `+0x25AC4` 的位测试得到布尔值，并在 VA `0x6F52D0` / RVA `0x2F52D0` 写到 `GameState+0x0E+index`。index 4 对应低 dword bit 4，保存字节位置是 `GameState+0x12`。

**`STATIC_CONFIRMED` — RestoreState 按保存值置位或清位。** 循环以 `GameState+0x0C` 中的数量为上界。对每个 index，函数读取保存布尔字节；字节非零时对对应低/高 dword OR 入 mask，为零时反转 mask 后 AND 清除对应位，最后将两 dword 写回。Baby bit 对应 index 4，因此 RestoreState 的结果等于 `GameState+0x12` 中保存的布尔值。

**结论（Continue）：** 构造阶段确实先把 `+0x25AC0` 清零；随后 Continue 分支调用 RestoreState，且它会按存档把 bit 4 恢复为真或假。因此 Continue 的最终 Baby 位不是无条件清零，而是继承此次保存时的值（保存为真则恢复真，保存为假则恢复假）。这也意味着新建 Game 对象后的清零不能单独当作 Continue 的最终状态。

## 特殊 ResetState 回调与边界

**`STATIC_CONFIRMED` — ResetState 的两个直接代码调用点。** `Game_ResetState_callback_context.json` 的完整直接 xref 记录显示：VA `0x6F07A1` / RVA `0x2F07A1`（经构造初始化函数调用）和 VA `0x703473` / RVA `0x303473`（经特殊 helper 调用）。第二个调用位于 helper VA `0x703460` / RVA `0x303460` 开头。该 helper 随后调用 `Level::SetNextStage`（VA `0x7035BC` / RVA `0x3035BC`）和 `Level::Init`（VA `0x7035C5` / RVA `0x3035C5`）。这些相邻调用只证明该 helper 的特殊重开流程，不证明一般阶段切换会 ResetState。

**`INFERRED` — 第二条是 R Key 特殊回调路径。** helper 的唯一直接代码引用是 VA `0x5C7C90` / RVA `0x1C7C90` 的 thunk（原始指令 `jmp sub_703460`）。该 thunk 唯一可见 xref 来自 VA `0x5C378C` / RVA `0x1C378C`：位于分发函数的 `case 636`，把 thunk 地址放入 ECX 后在 VA `0x5C3799` / RVA `0x1C3799` 调用对象工厂 `sub_6047F0`。官方枚举把 636 映射为 `COLLECTIBLE_R_KEY`。因此可将它记录为 R Key 相关的特殊 ResetState 回调登记路径；不能据此外推普通换层都会触发。

**`UNKNOWN`** 当前导出确认了回调登记处、thunk、特殊 helper 和 `ResetState` 的直接调用 xref，但没有追完对象工厂产物在运行时的间接调用消费者；也没有用动态执行验证回调何时触发。直接 xref 清单不等于间接调用闭包。

## 官方源映射

官方源仅用于名称、签名和枚举映射；运行行为结论来自上述 IDB 指令：

- `SOURCE` — `F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\PlayerManager.zhl:44`：`PlayerManager::CoopBabiesOnly()`。
- `SOURCE` — `F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Level.zhl:5,11,38`：`Level::Init`、`Level::ChangeRoom`、`Level::SetNextStage`。
- `SOURCE` — `F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Game.zhl:2,11,14,17`：`Game::constructor`、`Game::RestoreState`、`Game::SaveState`、`Game::ResetState` 的签名/字节特征；构造函数特征在当前 IDB 唯一匹配 VA `0x6ED4B0`。
- `SOURCE` — `F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Global.zhl:1428` 与 `include\IsaacRepentance.h:1704`：`COLLECTIBLE_R_KEY = 636`。

## 可复核产物

`export_st5.py` 是可重复的只读 IDAPython 导出脚本，已通过 IDA MCP 的 `py_exec_file` 执行；本次结果为 19 个完整函数、88 条 `.text` 中包含 `25AC0h` 的指令、2 段特殊回调上下文，imagebase `0x400000`。完整函数 JSON 包括三处 predicate、Join 主函数、ChangeRoom、Init、SetNextStage、stage transition、构造签名地址、Game 初始化/ResetState、Save/Restore、ExecuteStartGame 及特殊 helper。`metadata.json` 记录 IDB 与输入映像路径。

本任务未启动游戏、未运行测试/构建/动态调试，未做哈希校验，也未设置 gate。
