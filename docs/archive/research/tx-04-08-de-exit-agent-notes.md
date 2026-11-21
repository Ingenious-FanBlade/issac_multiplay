# J273 main 退出前置延后窗口：静态记录

范围：仅静态阅读 J273 main close-byte 判断、前置条件、相关窗口消息路径与 SDK 声明；未启动游戏，未运行测试、探测或校验。

## 结论

存在可继续评审的前置延后候选：将 `_main` 中 `0x90AFC4–0x90AFCD` 的十字节 `cmp + jnz` 整体转到常驻 stub。它位于唯一已确认的普通 `Isaac::Shutdown` 调用之前。Stub 执行原 CMP 一次后，窗口未请求关闭时走原循环目标 `0x90AF41`；关闭请求存在但 world 未 Closed 且期限未尽时也走 `0x90AF41`；world Closed 或期限耗尽时跳到原 Shutdown 调用 `0x90AFCE`。这不需要改 GLFW 对象或猜 should-close 函数 ABI。方案在静态控制流上可行，尚未实现或执行验证。

策略检查必须有界且非阻塞，不得等待、重入 Manager 或改变原生调用线程/栈语义。若 stub 调用 helper，应保存 GPR/EFLAGS 与 x87/SSE 状态，并使用 16-byte 对齐的 FXSAVE/FXRSTOR 区域；helper 的决策要在恢复上下文后仍可用。SDK 搜索未发现 RequestExit、PollEvents、MainLoop、WindowShouldClose 或 SetWindowShouldClose 声明；main 的 close 读取是内联指令。

## main 控制流和字节证据

`_main` 起点 `0x90AC60`，大小 `0x3F1`。主循环先调用 Manager 更新 `0x90AF08`、render `0x90AF0D`，之后：

```asm
0x90AF20  mov eax, dword_BAE270
0x90AF25  cmp byte_BE65A5, 0
0x90AF2C  jnz 0x90AFC4
0x90AF32  push 0
0x90AF34  push 10001h
0x90AF39  call sub_A2F820
0x90AF3E  add esp, 8
0x90AF41  test bl, bl
...
0x90AFC4  cmp byte ptr [eax+8], 1
0x90AFC8  jnz 0x90AF41
0x90AFCE  call sub_982C90 ; Isaac::Shutdown
0x90AFD3  call sub_A2AD80
0x90AFD8  call CoUninitialize
```

补丁窗口为 `[0x90AFC4,0x90AFCE)`，长度 10 字节，原始指令字节为 `80 78 08 01 0F 85 73 FF FF FF`：4-byte CMP 和 6-byte near JNZ，边界完整。原 CMP 检查 `[EAX+8] == 1`：不等则跳回 `0x90AF41`，等于 1 则落到 Shutdown。窗口前的 `0x90AF20` 从 `dword_BAE270` 加载 EAX；`0x90AF25` 只读全局，`0x90AF2C` 直接跳到 AFC4，中间没有 call 改写 EAX。IDA xref 显示到 AFC4 的代码边只有 AF2C；到 `0x982C90` 的直接调用只有 AFCE（函数 main）。这只排除其他普通直接调用，不排除异常或进程级退出路径。

AF41 首条是 `test bl,bl`，会重置 EFLAGS；AFCE 的无参数 Shutdown call 未见依赖 CMP flags。尽管如此，helper 方案仍应保守保存上下文。无需 close 时可直接复现原分支；close 时根据 world/期限只选择原两个目标。stub 不应在主线程上等待 world 线程。

## byte_BE65A5 的含义边界

`byte_BE65A5` 不是 close request。静态反编译中，`sub_A2AB60` 在 `0xA3935F` 检查该值；值为零时走 `sub_A3B130` 初始化和 `sub_A3A060` 显示设备枚举，只有两者成功且后者非空时在 `0xA39406` 写入 1。main 在此值为零时绕过 close-byte 判断，转去错误报告/循环路径。因此只称其为 graphics/display setup availability gate；其更精确的产品语义未证明。

## 窗口关闭路径

`sub_A29010` 的 `WM_CLOSE` 分支约在 `0xA2939D–0xA293BC`：读取 GLFW close callback，设置 `window+8=1`，有 callback 则调用后返回。消息泵 `sub_A2A390` 的 `WM_QUIT` 分支约在 `0xA2A3CB–0xA2A3E6`：遍历窗口列表，对每个窗口设置同一 `window+8=1`，并调用回调。消息泵由 `sub_92E3E0` 更新路径调用（`0x92E67C`）；main 的 close 检查随后在 `0x90AFC4`。

因此 WM_CLOSE 和 WM_QUIT 汇入相同 main close byte；WM_QUIT 不经过 WndProc。正常菜单“退出”是否必经此 byte 尚未独立追踪，不声称覆盖其他退出机制。

## Hook/ABI 与剩余审阅点

SDK 的 `libzhl/functions/Isaac.zhl:47` 声明 `static __cdecl void Isaac::Shutdown()`，但此 Hook 位于最后的 Shutdown 调用点，无法让 Manager 继续更新。`libzhl/functions/KAGE_Graphics_Manager.zhl` 中 `_GLFWwindow` 是明确不完整的布局，不应用它猜测字段或做结构 Hook。SDK 没有独立 GLFW should-close 函数声明；即使 hooks.json 支持自有 FunctionDefinition，也无法为 main 内联 CMP 制作函数入口 Hook，需代码地址 patch。

实现评审还需确认 patch 注册时序、stub 跳转/恢复细节，以及 world Closed/期限数据的线程安全读取。main 另有异常清理路径；唯一 Shutdown xref 是针对普通调用控制流的结论。本报告没有修改 manifest 或工程源文件。

## C：Esau Jr.（703）公开使用入口的静态证据

REPENTOGON SDK 的 Lua wrapper 提供了所需公开路径。`Entity_Player::UseActiveItem` 的 SDK ABI 声明为 `__thiscall void(short* resultFlags, int collectible, unsigned int useFlags, int activeSlot, int varData)`，函数字节模式在 J273 命中 `0x5B4A60`。新式 Lua 调用必须让第三参数是数字；wrapper 随后按 `UseActiveItem(item, useFlags, activeSlot, varData)` 解析参数，默认 slot 为 `-1`，slot 0 显式对应 `ActiveSlot.SLOT_PRIMARY`。因此应显式给出 `UseActiveItem(703, UseFlag.USE_OWNED, 0, 0)`；不要依赖默认 slot。

`USE_OWNED` 的公开定义是“效果由玩家拥有的主动道具触发”，数值为 `1 << 2`（`0x4`）。既然测试路径要求玩家确实持有 703，传 `USE_OWNED` 能正确标记该次使用。静态 SDK/API 资料没有证明它是进入 Replace 分支的必要位；该位是否影响 J273 内部的其他派发仍未从机器码闭合。`USE_ALLOWNONMAIN` 不属于普通 Isaac A 的必要候选条件，也不要加入。

如启动场景没有 703，可用公开 API `A:AddCollectible(703, 0, false, 0, 0)` 临时加入 slot 0：SDK Lua wrapper 的参数依次为 collectible、charge、firstTimePickingUp、slot、varData、pool，且 slot 校验范围为 0–3。`false` 对应源码注释中的 `addConsumables` 开关，避免授予该参数关联的额外消耗品；它仍调用原生 `AddCollectible`。之后用 `A:HasCollectible(703, true)` 检查真实持有（文档说明 true 忽略 Zodiac、3 Dollar Bill 等修饰来源），再按上面显式参数调用 UseActiveItem。`AddCollectible` 改的是当前 run 玩家数据，跨局持久性不在此 API 调用的静态证据范围内。

IDA 的 `sub_7A3A40`（Esau Jr. helper）先要求 `n3 <= 3`，然后用 `this + 32*n3 + 5492` 读取当前 active slot 的 collectible id；该值必须非零，且等于 703 才走 Esau Jr. 路径。对应调用点 `0x5BED63` 位于 `UseActiveItem` 函数 `0x5B4A60` 内：`0x5BED54` 检查 `[edi+0x15F4] != 0`，满足时在 `0x5BED5D`、`0x5BED5F` 分别压入 `a4=0`、`n3=0`，`0x5BED61` 将 `edi` 放入 `ecx`，并在 `0x5BED63` 调 helper。故该静态调用固定检查 slot 0；`[edi+0x15F4]` 的字段语义以及在指定公开 Lua 调用下必然非零，尚未闭合。这是最小路径剩余的关键资格未知，不应声称由公开接口层证据单独保证一定 Replace。

如果 helper 确实进入 703 分支且 Manager 的 4 个 backup 槽仍有空位，首次调用就会创建备份并继续替换，不需要以“先初始化、再重试”作为方案：slot0 的 backup 索引 `this+5520` 为零时，函数扫描 `n187+28362` 起的 4 个指针槽，取第一个空位，`operator new(0x2EA0)` 后通过 `sub_74DC70` 创建 Player backup，将 1-based 索引写回 slot 状态；随后进入 `LABEL_53` 取回 backup，并在 `0x7A420A` 调用 `sub_995E10` 完成替换。首次完整流程后，`n187[v71+28361]` 被改成被替换前的 live player，backup 槽记录反向 owner。若 4 个 backup 槽都占满且传入的 `a4=0`，满槽路径在 `0x7A3E18` 返回早期处理，跳过 `sub_995E10`。因此具体用例还需保证有一个空 backup 槽。

当前最小可实现候选为：临时确保 A 在 slot 0 实际拥有 703 → 做真实持有检查 → 一次调用 `A:UseActiveItem(703, UseFlag.USE_OWNED, 0, 0)`；不要默认重试。静态控制流确认首次 helper 命中可同时建立 backup 并 Replace；但 `[player+0x15F4]` 对该公开调用的资格值仍未知。主审可按该地址决定是否接受这个尚未闭合的条件，或先将 C 保留为 UNKNOWN。未启动游戏、未运行测试/探测。

### C 路径更正：原生 ControlActiveItem 消费者

上段“直接调用 `UseActiveItem(703, ...)`”是早期候选，现已由自然输入路径取代，不应作为当前实现建议。`UseActiveItem` 内部 `0x5BED54` 的 `+0x15F4` 条件控制一个内部 helper 副作用；外层 `ControlActiveItem` 在返回后另有直接调用 Esau Jr. helper 的路径。不过，`UseActiveItem` 的共享返回收尾在 `0x5C7031` 还会独立读取同一字段，并据此保留或清除低返回字节，不能把外层自然调用描述为完全绕过 `+0x15F4`。

当前 SDK Hook `Entity_Player::ControlActiveItem(int slot)` 对应 J273 `0x789E50`。原生路径读取传入 slot 的 active collectible，并把真实 collectible 与 slot 传给 `UseActiveItem`：

```asm
0x78B442  mov byte ptr [edi+2064h], 2
0x78B449  mov eax, [ecx+edi+1574h]     ; slot stride 32 + active collectible id
0x78B450  mov [edi+2068h], eax
0x78B456  mov eax, [ecx+edi+1590h]
0x78B45D  mov [edi+206Ch], eax
0x78B463  xor eax, eax
0x78B465  cmp [ebp+var_47], al
0x78B468  push 0                       ; varData
0x78B46A  setnz al
0x78B46D  push ebx                      ; activeSlot
0x78B46E  add eax, 4                    ; USE_OWNED | optional bit 0
0x78B471  push eax                      ; useFlags
0x78B472  push dword ptr [ecx+edi+1574h] ; collectible
0x78B479  lea eax, [ebp+var_48]         ; resultFlags
0x78B47C  mov ecx, edi                  ; Player this
0x78B47E  push eax
0x78B47F  call sub_5B4A60               ; UseActiveItem
0x78B484  mov ax, [eax]
0x78B487  mov word ptr [ebp+a4], ax
...
0x78B4E8  cmp byte ptr [ebp+a4], 0      ; low result byte
0x78B4EC  jz short loc_78B4F9
0x78B4EE  push [ebp+a4+1]               ; helper a4 = high result byte
0x78B4F1  mov ecx, edi
0x78B4F3  push ebx                      ; helper n3 = activeSlot
0x78B4F4  call sub_7A3A40               ; Esau Jr. helper
```

因此普通 Isaac A 的 primary slot0 在有 charge、一次 SPACE 经原生输入触发 `ControlActiveItem(0)` 时，`UseActiveItem` 的 collectible 与 active slot 都来自真实 slot0；低返回字节非零时，consumer 随即以 `n3=0` 调 Esau helper。此入口不依赖 `UseActiveItem` 内部的 `+0x15F4` 条件。

J273 `UseActiveItem` 对 703 的分派可由原始两级索引复算，而非只依赖 IDA 注释：输入 collectible `703 (0x2BF)` 在 `0x5B8801` 减去 `0x11B`，索引为 `0x1A4`；`byte_5C755C[0x1A4]` 位于 `0x5C7700`，字节值为 `0x5F`；第二级跳表 `jpt_5B8820` 基址 `0x5C7398` 的槽 `0x5F` 位于 `0x5C7514`，其 dword 值为 `0x5B4E3F`。该地址同时是第一跳表的 case 93 共用块。原始分支为：

```asm
0x5B4E3F  mov eax, [ebp+var_5C8]
0x5B4E45  mov [ebp+var_5BD], al
0x5B4E4B  mov eax, [ebp+a5]
0x5B4E51  jmp loc_5BC6C3
```

该分支写入的是 `var_5BD`，它在 `0x5BC6FD` 被共用后续代码检查，用于条件性调用 `sub_7A2860`（UseItem sparkle）；它不是传回 `ControlActiveItem` 的低返回字节。`var_5C8` 的值来自 `var_5ED` 与 `[esi+0x2C]` 条件：`0x5B4B11–0x5B4B1D` 计算 `var_5ED = !(useFlags & 1)`；`0x5B4C09` 比较 `[esi+0x2C]` 与零，`0x5B4C13` 仅在相等时选择该布尔值，随后 `0x5B4C18` 保存 `var_5C8`。因此自然 primary 调用的 `useFlags=4` 且玩家 `Entity.variant=0` 时，`var_5C8=1`，703 分支把 `var_5BD=1`。

传回 `ControlActiveItem` 的两个结果字节来自另一局部值 `var_5E4`：函数在 `0x5B4ACB` 初始化其低字节为 1；共享收尾 `0x5C7028–0x5C7049` 在低字节仍非零时读取 `[player+0x15F4]`，仅当字段为 0 或 `0x2F` 时保持低字节为 1，否则清为 0；`0x5C70F0–0x5C7103` 将 `var_5E4` 写到结果指针。Lua 绑定在 `0x8502C2` 将 `GetItemState` 注册到 `sub_431860`，getter 原码为 `0x431860 mov eax,[ecx+15F4h]; 0x431866 retn`；本地 API 文档 `IsaacDocs/docs/EntityPlayer.md:1119` 将它声明为 `GetItemState()->CollectibleType`。夹具可用公开只读 `GetItemState()` 在准备后及输入前断言为 0（0 与 `0x2F` 都满足此处原生条件）；`ResetItemState()` 是独立写入 API，本路径不调用。自然 `ControlActiveItem` 在 `0x78B425`、`0x78B49D` 等位置只读 `+0x15F4`，不在此路径写它。

703 case 经 `0x5BC6C3–0x5BC6F0` 共用的 PostUseItem 回调后到达该收尾。`sub_84A020` 的第七实参指向 `var_5E4`；回调门控在 `0x84A06E` 检查 `[eax] & 8`（`eax` 在 `0x84A062` 从 `dword_BE4A0C` 取出），并可在 `0x84A163` 写结果低字节（`Discharge`）或 `0x84A1EC` 写高字节（`Remove`）。因此启用且实际存在的回调是一个可变结果分支；最终严格 Replace 证据应按原生结果验收。随后 `ControlActiveItem` 在 `0x78B4E8` 检查低字节，只有非零才于 `0x78B4F4` 调用 `sub_7A3A40`。对本用例，静态自然路径的条件为：slot0 实际持有 703、charge 足够、可选 bit0（`var_47`）为 0、`Entity.variant=0`、`GetItemState()` 为 0 或 `0x2F`，并且 PostUseItem 回调未清除低结果字节。该路径可由原生输入自然到达；本文没有运行游戏或测试。

资源表 `F:/projects_isaac/IsaacDocs/scripts/data/items.xml:731` 将 703 定义为 active item，`maxcharges="1"` 且没有 `chargetype` 属性。公开 AddCollectible 准备数据应显式设置 `charge=1`（早期写下的 `charge=0` 建议已撤回），随后只读核对真实持有、`GetActiveItem(0)==703` 与 `GetActiveCharge(0)==1`。候选执行路径是经 harness 一次 SPACE 触发原生消费者，不直接从 Lua 调 `UseActiveItem`。未启动游戏、未运行测试/探测。

