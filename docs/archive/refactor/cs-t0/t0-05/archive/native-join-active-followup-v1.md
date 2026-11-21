> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 原生 Join 有界补核纪要 v1

- 日期：2026-08-31。
- 性质：**create-new 静态逆向补充，整体仍为 BLOCKED_BY_EVIDENCE**。本版不产生 active arm、token、claim、Hook 或实机资格，也不是独立测试验收。
- 新结论：已检查的原生 ordinary join 控制/设备路径，静态支持第 2 节的独立限域 prejoin 判据；不能再笼统称“没有独立限域依据”。持久化无污染仍是首要实际阻断。
- 版本边界：生产 preflight 仍绑定原 static v1，原有七项缺口不因本文自动删除；完整 prejoin 准入、时间稳定性及 runtime 契约没有升级或通过自动门。本版不得作为可触发权限。
- 本次只读取 IDA 数据库和仓库，并创建本文；未修改 IDB、目标 PE、旧证据、生产代码或验收条件。未启动/附着游戏，未枚举/打开目标进程，未使用断点、补丁、注入、实机输入或测试入口。

## 1. 目标与不可变证据

本轮使用 `mcp__ida_pro_mcp__survey_binary` 和 `py_eval`，后者只读取反编译、函数指令、虚表项及原始字节。IDA survey 再次确认 x86、preferred base `0x00400000`、image size `0x008FE000` 和 approved-unpacked SHA。所有下列地址都是 **RVA**；绝对文件路径和运行时身份不入本文。

| 证据 | SHA-256 |
| --- | --- |
| canonical target | `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30` |
| approved-unpacked target / 当前 IDA 数据库标识 | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| [static v1](../../../../../../cs-t0/coop-native-join-static-v1.json) | `CCB9F206F0E08067724DCF4D0B7297E1D0537FE612F2B2BE9B5024CCEFB67DEB` |
| [reverse v1](native-join-active-reverse-v1.md) | `71041323B3BD446CF006F19971E0C3543423A85770FCB992C95B2FE122E4EE28` |

原 static/reverse v1 保持逐字节不变，不将其中 `UNKNOWN` 原地改判。本版 `STATIC_CONFIRMED` 仅表示构建上的具体指令、调用关系与列明前提下的条件路径；不代表已出现该运行时状态。本轮没有新增 `RUNTIME_OBSERVED`。

## 2. 独立限域 prejoin：有静态依据，运行时资格尚未冻结

### 精确 scope

待后续新版本冻结的判据是：同 generation 稳定且可读的 GameRoot、PlayerManager、InputManager、onlineRoot 及已知 backend；单一 P0 Keyboard、唯一未分配 concrete Gamepad B；canonical-zero roster 三指针、onlineFrame 为零、无 NetInputDevice；原生 selector 为已证 ordinary 分支。selector 模式条件仍为 `root+0x26548 & 0x10 == 0`、`root+0x26584 == 0`、`root+0x26630 == 0`，并保留无其他 pending selector、对象/vector 唯一性等原限制。

此 scope 只论证：在这些前提成立时，选定的原生 local join 正向控制/设备路径不需要创建或使用 OriginalOnline session、ready gate、channel 或 NetInputDevice。不要求改写 T0-02 ModeLatch，不把已知 Steam backend 的存在判为在线激活，也不要求没有与实验无关的 Steam 背景活动。持久化提交及 Lua 扩展副作用另受第 3、4 节约束。

它不依赖尚未发生的 `46→49→51` 后见证；旧 passive backend-screen 例外仍保持原 scope。后续实际 UI 覆盖之前以及两次自然 manager pass 中，必须有独立实现与测试证明上述条件确实成立且未漂移。

### 本次闭合的两个具体 lookup

**STATIC_CONFIRMED：InputManager 的指定 controller lookup 是本地设备向量查找。** InputManager vtable 为 `0x7827F4`，`+0x28` 的槽 `0x78281C` 指向 `0x620390`。query dispatcher `0x620FB0` 的指定 controller 分支通过该槽取 device/shared-control pair，而不是经 online roster 取得设备。

`0x620390` 锁定 InputManager 自身 mutex（`this+0x14`），遍历 `this+0x08/+0x0C` 界定的 pair 向量，stride 为 8，按 `device+0x0C` 匹配 controller key；命中后经 `0x502D60` 复制 pair。未命中则清空结果；随后解锁。此函数没有 OnlineManager/backend 查找。它不能替代准入中的唯一性检查，因为原生查找返回第一个匹配。

**STATIC_CONFIRMED：join 尾部的 online-device 查询在空 roster 下不是正向依赖。** 尾部 `0x3BE3A6 → 0x50AC70` 以 roster header 为 owner。该 helper 在 begin/end 为零时，两个计数检查均跳过设备遍历，输出诊断并返回 null；caller 的 `0x3BE3AD` 随即跳过 NetInputDevice 虚调用，继续普通尾部。日志调用的存在不等于“零 I/O”，但不要求在线设备、会话或 ready gate。

因此，原 `0x5BAC6D` local-body guard、已证 ordinary selector/创建/assignment 路线，加上这两项 lookup，足以为上述限域原生路径提供静态依据。本次没有发现一个仍要求非空 roster、NetInputDevice 或原联机 ready gate 才能成功继续的已检查入口。这个结论不扩展为“所有回调及整个游戏没有网络调用”。

### 可独立复核的原始锚点

下表中的 `EXACT_DWORD` 是虚表数据，其余均从完整指令起点读取；虚表中的地址值按目标 preferred base 编码。

| RVA | 原始字节 | 证据 |
| --- | --- | --- |
| `0x5BAC6D` | `8B 15 9C 16 C7 00 8B 82 D8 B3 04 00 3B 82 DC B3 04 00 0F 85 EF 08 00 00` | roster begin/end 不等时跳过 local body 至 `0x5BB574` |
| `0x78281C` | `90 03 A2 00` | EXACT_DWORD：InputManager `+0x28 → 0x620390` |
| `0x782868` | `B0 0F A2 00` | EXACT_DWORD：InputManager `+0x74 → 0x620FB0` |
| `0x62110C` | `FF 50 28` | 指定 controller 的 lookup 调用 |
| `0x6203E2` | `8B 47 08 8B 57 0C 3B C2 74 12` | 本地 pair vector begin/end |
| `0x6203F0` | `8B 08 39 79 0C 74 34 83 C0 08 3B C2 75 F2` | device ID 比较与 stride 8 |
| `0x62042F` | `E8 2C 29 EE FF` | 命中后复制 pair，callee `0x502D60` |
| `0x50ACC0` | `85 C0 0F 84 80 00 00 00` | 空 roster 跳过主设备查找 |
| `0x50AD99` | `85 C0 0F 84 B8 00 00 00` | 空 roster 跳过诊断设备遍历 |
| `0x50AE68` | `33 C0 5F 5E 5B 8B E5 5D C2 04 00` | helper 返回 null，thiscall `ret 4` |
| `0x3BE3A6` | `E8 C5 C8 14 00 85 C0 0F 84 4F 02 00 00` | 查找结果为 null 时跳至 `0x3BE602` |

仍为 `UNKNOWN` 的不是“必须等未来 passive witness”，而是将此 scope 变成可执行准入的完整契约：精确 field/owner 绑定、跨 pass 采样与 generation 生命周期、读失败和漂移拒绝，以及独立 runtime 验证。原 static v1 的 prejoin 缺口仅能由新的版本化契约及其独立验证处理，本文不直接升级它。

## 3. 候选只读条件能排除什么，不能排除什么

### 低 coins 并非所有 achievement 条件的共同前提

reverse v1 已证 `0x3597E0` 中 coins≥55 可到 achievement 2。本次补核发现同一函数另一条路径：coins≤0，且 `root+0x2654C & 0x800 != 0`、`root+0x269EB == 0`、`root+0x269E9 == 0`、P0 `+0x202C == 0` 时，`0x35992B` 调 `0x529A20(persistent,583)`。它仍受前面的 ordinal/模式 guard 及 setter 自身 guard 约束。

| RVA | 原始字节 | 证据 |
| --- | --- | --- |
| `0x3598EC` | `85 C9 7F 52` | coins>0 跳过此 achievement 分支 |
| `0x3598F8` | `81 E1 00 08 00 00 0B C1 74 40` | 历史 flag `0x800` 的条件 |
| `0x359902` | `80 BE EB 69 02 00 00 75 37` | `root+0x269EB` guard |
| `0x35990B` | `80 BE E9 69 02 00 00 75 2E` | `root+0x269E9` guard |
| `0x359914` | `80 BB 2C 20 00 00 00 75 25` | P0 `+0x202C` guard |
| `0x359923` | `68 47 02 00 00 8D 49 14 E8 F0 00 1D 00` | achievement ID `0x247=583`，owner 为 onlineRoot `+0x14` |

`0 < coins < 55` 可同时排除这两条局部 coins 分支；仅称“低 coins”不够。也可通过相应 flag 为零排除第二条。但这些是局部充分条件，尚未证明它们在所有前序初始化和尾部调用中保持不变。

### 尾部属性重算还有六处直接 achievement setter

ordinary join 的 `0x3BC740` 尾部在 `0x3BE60A` 调 `0x363570`，原始字节为 `E8 61 4F FA FF`。本次按该函数的实际指令枚举到六处直接 `0x529A20` 调用，而非依据名称猜测。其 owner helper `0x21800` 返回 onlineRoot `+0x14`，与原 persistent owner 相同。

| 调用 RVA | 原始字节 | achievement ID | 局部条件摘要 |
| --- | --- | ---: | --- |
| `0x36E0C5` | `E8 56 B9 1B 00` | 144 | item/state 聚合计算值≥1 |
| `0x36E1E3` | `E8 38 B8 1B 00` | 19 | 另一 item/state 聚合计算值≥1 |
| `0x36E5A8` | `E8 73 B4 1B 00` | 58 | 转换状态分支中的状态 byte 非零 |
| `0x36E5BF` | `E8 5C B4 1B 00` | 370 | 与上项同一状态分支 |
| `0x36F2E0` | `E8 3B A7 1B 00` | 355 | 遍历累计值≥5 |
| `0x36FBA1` | `E8 7A 9E 1B 00` | 361 | 对应属性 float≥3.0 |

这些条件不是 coins 或 persistent `+0x484` 的检查；它们也不表示默认 B 必然写入。要排除它们，需绑定允许角色/物品/转换/属性的进入状态，以及此前调用对该状态的影响，或证明正常原生建立且稳定的 setter 只读保护。不能以“B 初始空装备”代替数据流证明。该函数还在 `0x36F07B`、`0x37034B` 递归调用自身，必须纳入所选状态分支的闭合分析。

### 无 player-init callback 不等于无属性回调

`0x363570` 有 13 处到 `0x463E90` 的调用。helper 固定 callback ID=8，并检查 Lua registry bit `0x100`，与原 player-init ID=9、mask `0x200` 不同；本文将其称为属性/cache 回调，确切证据是 ID、mask 和调用阶段。

| RVA | 原始字节 | 证据 |
| --- | --- | --- |
| `0x463EC4` | `C7 45 DC 08 00 00 00` | callback ID=8 |
| `0x463ED4` | `8B 01 F7 00 00 01 00 00 74 2C` | `**registry & 0x100`，为零跳过派发 |
| `0x463EF1` | `E8 1A 04 01 00` | 派发到 `0x474310` |

13 个调用点均为 `CALL rel32 → 0x463E90`：

| 调用 RVA | 原始字节 |
| --- | --- |
| `0x363F85` | `E8 06 FF 0F 00` |
| `0x3656DB` | `E8 B0 E7 0F 00` |
| `0x367106` | `E8 85 CD 0F 00` |
| `0x367997` | `E8 F4 C4 0F 00` |
| `0x368623` | `E8 68 B8 0F 00` |
| `0x3692B2` | `E8 D9 AB 0F 00` |
| `0x369C6A` | `E8 21 A2 0F 00` |
| `0x36AC3D` | `E8 4E 92 0F 00` |
| `0x36AEF1` | `E8 9A 8F 0F 00` |
| `0x36F221` | `E8 6A 4C 0F 00` |
| `0x36F809` | `E8 82 46 0F 00` |
| `0x36FB6C` | `E8 1F 43 0F 00` |
| `0x36FFDD` | `E8 AE 3E 0F 00` |

原生 registry 同时不存在 bit `0x100/0x200` 可排除这两个 helper 的派发，但还不能证明整个 Init/tail 无其他 callback。未分析闭合的 `0x465340` 在尾部已有三处具体调用（`0x3BC9F1/0x3BDE08/0x3BE5E7`）；必须先解明其实际语义/guard，不把它自动归入“无 player-init callback”。不得通过覆盖回调或写 registry 来制造无污染条件。

### +0x484=0、native readonly 和 no-save 的适用边界

| 候选条件 | 已证局部效果 | 尚未覆盖 |
| --- | --- | --- |
| 空 roster 时 persistent `+0x484` 低16位为零 | 排除 reverse v1 中消费该计数并在 `0x3BE3A0` 直接写回的特定分支 | 其他统计/direct write、前序改变计数、其他 item/回调路径 |
| 原生建立并保持 persistent `+1 != 0` | `0x529A20` 自身拒绝，包括上述已列 achievement setter | 不检查此 byte 的 direct writes、统计写、磁盘/Cloud 入口及生命周期暂时清零 |
| registry mask `0x200 == 0` | 排除 player-init helper 派发 | mask `0x100` 属性/cache 回调及其他 callback |
| 原生 no-save 状态 | 必须先具体化为真实字段、建立路径和生命周期，再按每个 sink guard 判断 | 不能从 UI 名称推出所有内存写、GameState save、Cloud/Steam 都被禁止 |

本次读到原生 lifecycle 函数 `0x2F9F20` 先把 readonly 置零、执行条件统计操作，再置一：`0x2F9F4B = C6 46 15 00`，`0x2FA00F = C6 46 15 01`；ESI 为 onlineRoot，`+0x15` 即 persistent `+1`。其中有直接统计字段写和 `0x529B40` 调用。它说明只读位并非全生命周期常量；本次**未**证明它会在候选 join 窗口执行，也不将所有生命周期函数一概视为已触发反证。最小后续任务是证明所选正常场景如何建立这些保护，以及声明窗口内不会进入其临时写区间。

因此，上述只读条件可以组成下一轮具体的候选准入，但目前不存在已经证明正常可达、与 ordinary selector 条件兼容、且整个窗口无持久写的完成契约。不得写 readonly、清计数或改模式来绕过未闭合路径。

## 4. 延迟写与最小后续补证清单

本次定位到一个有具体原生输入条件的 flush caller：函数 `0x54C580` 内通过 InputManager 查询 action15、controller=-1；原返回 true 时，`0x54C91E` 调 `0x529660(persistent,1)`，随后调用 `0x58AEC0`。它说明普通菜单/退出转换可提交此前 pending persistent；并不说明每个正常帧或仅停止 Hook 就会提交。

| RVA | 原始字节 | 证据 |
| --- | --- | --- |
| `0x54C902` | `C7 45 CC 0F 00 00 00` | action15 |
| `0x54C909` | `FF D0 84 C0 0F 84 76 01 00 00` | 原始 query false 跳过 flush |
| `0x54C91E` | `E8 3D CD FD FF` | 调 `0x529660` |
| `0x54C923` | `E8 98 E5 03 00` | 后续原生转换 `0x58AEC0` |

最小进一步静态工作应围绕选定实验状态和明确的两次自然 pass、稳定观察、正常停止窗口，不扩展成全游戏 persistence 逆向：

1. **属性重算与状态来源**：`0x363570 → 0x529A20 / 0x463E90`，覆盖六 setter、13 callback 和两个递归点；追踪仅与候选普通 B 的角色、物品、转换和相关 cache flags 有关的分支。`0x3BC740` 中 `0x35F0E0` 的现有调用及 `0x465340` 三处调用是需先解析的具体上游，不预设其无副作用。
2. **正常保护状态与 direct write**：证明不靠注入修改即可建立、且符合 ordinary selector 的保护状态；检查 `0x2F9F20`/`0x529B40` 与声明生命周期的关系，封闭 readonly 暂时失效和统计直接写；将 reverse v1 的 `+0x484` 分支纳入同一状态数据流。只读位能挡住 setter，不得被当作 direct write 的护栏。
3. **声明窗口内的 flush/save 可达性**：从实际正常帧和停止入口检查 `0x54C91E → 0x529660 → 0x5292C0 / 0x528EE0 / 0x5294F0`；检查已定位的 `0x558ED0` caller 所属入口 `0x2F5320/0x2F72F0/0x2F7AC0/0x2FA0C0/0x52F1C0`，仅追踪与声明场景/停止相关者。不得把不相关 online merge 或 restart 的 xref 当成候选场景已发生的写。
4. **外部提交面**：在上面真实可达链上核对 `0x529790` 的 Steam achievement 提交、`0x528EE0` 的 Cloud 和本地/备份写入口；证明保护实际覆盖调用及 pending dirty/stat/unlock，而非只看前后文件 hash。独立限域 prejoin 不禁止背景 Steam，不等于允许本实验造成新的持久提交。

停止判据保持不变：任一必要状态只能靠写未知字段建立、相关 callback/direct write 无法排除、或无法绑定正常停止后的待提交状态，就保持 `BLOCKED_BY_EVIDENCE`，不实施主动覆盖。没有选择一个正常可达的候选状态之前，不发布“原生禁成就/禁保存即可安全”的笼统契约。

## 5. 给主线与独立测试的交接

55 槽和精确两 callback 路线继续沿用 reverse v1；本文没有增加 Hook、改变 ABI 或授权直接调用 spawn/assignment/HUD。原 static v1 与生产 preflight 的冻结关系保持不变，仍按原缺口拒绝 active。

独立测试可只读核对本文的目标 hash、RVA 与完整指令字节，且应将这些结果标为新增静态证据复核。该复核不能替代 prejoin 稳定性测试，更不能把恢复 55 槽或静态字节一致当作零持久污染。后续只有新版本契约完整绑定并通过相应独立门，才能升级资格；本纪要及已有 v1 证据均保留历史原义。
