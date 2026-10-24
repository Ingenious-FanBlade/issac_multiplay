# T0-03R Loop Envelope 静态研究

状态：`V5_LOOP_GATE_PASS / T0-04_BLOCKED_BY_EVIDENCE`。冻结 ledger 仍保持采集前的 `BLOCKED_BY_EVIDENCE` 与原 SHA，不追写后续实机结论；由七份 V5 source artifact 派生的 loop gate 已为 7/7 `PASS`。首轮只对固定 Win32 构建执行 SHA-256、PE 检查、离线反汇编和完整 `.text` direct-call 搜索。随后在用户已启动并自行附加 x32dbg 的 PID 20224 上执行了 attach-only module/memory read；该阶段没有设置任何形式的断点，没有 pause/step/run-to，没有 patch/write/register modification，也没有操作游戏或 Mod。静态机器可读基线仍以 `ledger/isaac-cs-t0-loop-ledger.json` 为准，实机门结论以 create-new matrix/gate 为准。

## 固定构建与结论边界

- `isaac-ng.exe` SHA-256：`C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`。
- `.text` SHA-256：`197175428117EC352324F22FA44CF42D60511F0A4887BF6DC5FECFBF9A6EE758`。
- PE32 x86，machine `0x014C`，timestamp `0x69E6E3A7`，preferred image base `0x00400000`。
- 工具：GNU objdump 2.44。RVA 均从 objdump 显示的 VA 扣除 image base，且只登记实际复核过的指令位置。

离线 callgraph 已经给出“顶层候选循环 → broad update → render → back edge”的结构，但不能证明线程身份、运行时每轮次数或 Win32/SDL pump 的归属。因此它足以指导被动探针设计，不足以开放主动输入抑制/注入。

## Loop envelope 候选

| 角色 | RVA / 边 | 静态事实 | 分类 |
|---|---:|---|---|
| CRT caller | `0x6EFBB9 -> 0x531050` | 直接调用，caller 清理 3 个栈参数 | `STATIC_CONFIRMED` |
| top-level app candidate | `0x531050` | 包含初始化、重复区和退出路径 | `STATIC_CANDIDATE` |
| loop header | `0x531231` | timing/sample 代码的重复区头 | `STATIC_CANDIDATE` |
| update driver | `0x5312EC -> 0x5AB6D0` | 每轮位于 render 前 | `STATIC_CANDIDATE` |
| broad update | `0x5AB6F2 -> 0x554CD0` | caller 先 push `1`；callee 在 `0x555599` 以 `ret 4` 返回 | `STATIC_CANDIDATE` |
| render owner | `0x5312F1 -> 0x5555C0` | 紧随 update driver | `STATIC_CANDIDATE` |
| back edge | `0x53135B -> 0x531231` | 条件分支回到重复区头 | `STATIC_CONFIRMED` |

`0x554CD0..0x555599` 只是 broad update owner，不能命名为完整事件循环。完整候选 envelope 的上层 owner 是 `0x531050`，而是否每个 top-level iteration 都恰好对应一个 Authority tick 仍需实机事件关联。

Win32 imports 中存在 `PeekMessageW`（IAT RVA `0x718714`）、`TranslateMessage`（`0x718650`）和 `DispatchMessageW`（`0x7185B8`）。三个 IAT disk thunks 的四字节值均已固定到账本并由 `loop-inspect` 复核。目前保留以下互不合并的候选：

- `PeekMessageW` callsites：`0x65C101`、`0x68148E`、`0x6814C3`。
- `0x681489` 区域在 `PeekMessageW` 后经寄存器调用 `TranslateMessage`（`0x6814A9`）和 `DispatchMessageW`（`0x6814B0`），再于 `0x6814C3` 继续 poll。
- generic pump candidate 入口 `0x65E660`；其 body anchor 为 `0x65E670`，在 `0x65E677` 载入 `PeekMessageW` IAT，并于 `0x65E68A` 通过两字节 register-indirect `call` 调用，因此真实返回 RVA 是 `0x65E68C`，不是先前误写的 `0x65E690`。

首个 READY 实机窗口已经把 generic pump 与 top-level update 动态关联：35,988 个 loop iteration 中，返回 RVA `0x65E68C` 恰好出现 35,988 次，全部在同一 main thread 的 `UPDATE_ENTER..UPDATE_EXIT` 内；有消息时，`TranslateMessage` 返回 `0x65E6D6`、`DispatchMessageW` 返回 `0x65E6E1`、下一次 `PeekMessageW` 返回 `0x65E6F0`，三者又各恰好出现 5,425 次并保持该顺序。因此 `0x65E660` 可晋级为 `RUNTIME_CORRELATED_LOOP_LOCAL_GENERIC_PUMP`。这仍不把每个 Peek/MSG 等同于 Authority tick，也不排除独立 SDL/UI pump；`0x681489` 等其它候选继续保持 `UNKNOWN_CALLGRAPH`。

## 输入结论修订与 action oracles

T0-03 的 `0x554F84 -> 0x55B310` 不再称为 physical sample candidate。`0x55B310` 的静态行为更接近 controller assignment，但具体 owner 和完整语义仍不足，故新账本把它写成 **controller-assignment `UNKNOWN`**。

`0x5551D8` 是条件字段复制：遍历对象，将 `+0x344/+0x348` 写到 `+0x33C/+0x340`，并清除 `+0x175`。没有 concrete InputDevice、controller ownership 或 callback 证据能把这些字段命名为 input apply，故字段语义保持 `UNKNOWN`。

已确认的 action oracle 只作为静态定位锚点：

| RVA | 已确认事实 | 不能据此声称 |
|---:|---|---|
| `0x44D740` | action `0..31` 经过 jump table 返回名字；可见 Left/Up/Right/Down、Shoot* 等字符串 | 它不是采样边界，也不证明 callback 时序 |
| `0x500F50` | action `0..34` 映射为 online-frame bit `0..14` 或 `0xFF` | compact bit 不是 T0-04 canonical wire 格式 |
| `0x500CA0` | 循环查询 action `0..31`；抽象 device vtable `+0x30` 返回 bool，`+0x38` 返回 scalar；构造现有 16B online frame | 它只证明 OriginalOnline encoder 的调用点，不能单独证明 offline apply 边界 |
| `0x510980` | OriginalOnline 输入推进/消费，使用 `0x500F50` 并写 `NetInputDevice` current/edge/axis 字段 | 不能外推为 offline/local-coop 的通用 apply 边界 |

attach-only 静态/内存交叉核对已经定位 Keyboard 与 Gamepad vtable、共享 query ABI 和当时的 keyboard owner；后续 305/305 coherent controller RPM capture 已补齐 stable active-XInput unique-owner snapshot。仍必须把 `MC_INPUT_ACTION` 的 action `0..11` 与 raw query 关联。UI/pause/map/session action `12+` 不进入观察或拟拦截域。

## Attach-only InputDevice 证据

MSVC RTTI 给出三个可验证类型：`InputDeviceBase`（vtable RVA `0x782758`）、`InputDeviceGamepad`（`0x782C80`）和 `InputDeviceKeyboard`（`0x7A1678`）。输入管理器不是指针槽，而是固定对象 `moduleBase+0x857B18`；其 `+0x08/+0x0C/+0x10` 是 8 字节 shared-pointer 元素的 begin/end/capacityEnd。`0x61F240` 在锁内按下标复制元素，`0x61FDF0` 在锁内追加元素。

Keyboard 和 Gamepad 的 vtable `+0x30/+0x34/+0x38` 均指向相同的 `InputDeviceBase` 实现：

| slot | target RVA | ABI | 具体语义 |
|---:|---:|---|---|
| `+0x30` | `0x61E8D0` | `thiscall`，ECX=`InputDeviceBase*`，栈上一个 `uint32 logicalAction`，AL 返回 bool，`ret 4` | 检查 enabled/range；cached logical magnitude 达到 `this+0xD4` 阈值即 true，否则遍历最多四个 binding 并调用 primitive vfunc `+0x24` |
| `+0x34` | `0x61E970` | 同一参数，AL 返回 bool，`ret 4` | 比较 current/previous cached magnitude 与阈值，并回退到 primitive vfunc `+0x28`，对应 logical triggered/rising-transition 查询 |
| `+0x38` | `0x61EA20` | 同一参数；x87 ST0 返回 float，`ret 4` | 以 cached logical magnitude 为初值，遍历最多四个 binding，调用 primitive vfunc `+0x2C` 并取最大值 |

因此三者不是“同一个 physical sample”：`+0x30` 是逻辑按下判定，`+0x34` 是逻辑 rising-transition 判定，`+0x38` 是逻辑标量强度。设备差异位于 primitive slots：Keyboard 的 `+0x24/+0x28/+0x2C` 直接读取键盘状态表；Gamepad 走通用 button/axis 实现。`0x500CA0` 的 `InputDevice*` 则来自 OriginalOnline `NetInputDevice+0x378` shared pointer，不能外推成 offline owner。

controller ownership 的实际 join key 是 `InputDeviceBase+0x0C`，不是 Gamepad `+0x370` 的 platform controller id。`0x55B9CC` 读取前者，`0x55BA56` 将它与 `player+0x1618` 比较。player 来源是 `*(moduleBase+0x871678)` 指向的 game root，其 `+0x1BAA8/+0x1BAAC/+0x1BAB0` 为 `Player*` vector。

PID 20224、module base `0x003B0000` 的稳定只读快照为：active input vector `[0x0D50A36C,0x0D50A374)`，唯一元素 `{Keyboard*=0x0D641454, owner=0x0AE8E7AC}`，vtable `0x00B51678`，device key `0`；game root `0x72E4D01C` 的唯一 player 为 `0x8982207C`，`player+0x1618` 同为 `0`。故当前结论是 `UNIQUE_KEYBOARD_OWNER_MATCH`。

较早一次快照短暂见到第二个 active entry：Gamepad vtable、device key `1`、platform controller id `0`、名字 `XInput Controller 1`。随后 end 从 `0x0D50A37C` 收缩到 `0x0D50A374`；容量槽中的旧指针已不属于 active range，不能作为当前 XInput owner 使用。这也直接说明 reader 必须验证 begin/end/capacity 并做双重采集。

### 独立 PROCESS_VM_READ reader 规则

reader 应先校验 PID 路径、固定 EXE SHA-256、PE machine/image size 和 main module base，再验证三个 concrete vtable RVA 以及共享 query targets 的预期字节。每个样本执行：

1. 读取 input-manager header A，验证 `begin <= end <= capacityEnd`、三者 4 字节对齐、`(end-begin)%8==0` 和有界 count；只读取 `[begin,end)`，绝不读取 capacity 尾部。
2. 对每个 entry 验证 object/owner 非空且可读，vptr 必须等于白名单 Keyboard/Gamepad/Base vtable；读取 object `+0x0C`。Gamepad 才允许读取 `+0x370/+0x374`，名字必须有严格长度上限。
3. 重读完整 entries 和 header B；任一 header、entry、vptr 或 key 不同就丢弃本次并增加 `unstableRetry/drop`，不能拼接两个世代。
4. 用相同 double-collect 读取 game-root slot、Player vector 和每个 `player+0x1618`；只在两次完全一致时按 device key 做唯一 join。`-1`、无匹配或多匹配均输出 `UNKNOWN`，不能猜测。
5. 记录 reader 自己的 QPC/sequence 和失败原因；远端地址与进程指针不得进入 canonical input 编码。

这套设计可以安全实现非侵入、best-effort coherent capture，但不能宣称真正 gapless 或 per-tick。结构没有 generation/sequence；double-collect 只能发现常见并发变化，不能排除 ABA，也会漏掉两次 poll 之间的短 edge，更不能把样本唯一归属到 `loopIteration`。因此可实现“带显式 unstable/drop 计数的 gapless-ish 诊断流”，但 T0-03R/T0-04 gate 仍不得把它当作无缺口证据。

## 被动 witness

仓库内 `test-mod/isaac-cs-t0-input-witness` 是独立测试 Mod：

- 只在 `MC_INPUT_ACTION` 的 `buttonAction` 是整数 `0..11` 时聚合计数。
- callback 所有路径显式 `return nil`，不查询输入值、不抑制、不替换、不注入。
- action `12+` 在任何记录、controller 提取和聚合之前立即返回；不会出现在日志或状态里。
- 按 action/inputHook/controller 去重，并每 60 次 `MC_POST_RENDER` 最多输出一次聚合摘要；窗口内 unique key 也有硬上限。

该目录只是待部署材料。本轮没有复制到游戏 Mods 目录、没有创建 `disable.it`、没有启用 Mod，也没有启动游戏。

## Active gate 与下一人工暂停点

冻结 loop ledger 的初始 gate 为 `BLOCKED_BY_EVIDENCE`，因此它本身永远不能解锁主动 input suppression、synthetic apply 或 loopback；后续只能由绑定 source artifacts 的派生 gate 补充实机结论。当前派生 V5 loop gate 虽已 PASS，但 raw sample/apply 被动闭环与第二次主动授权仍缺失，所以全局主动路径继续 fail closed。允许继续的工作仅包括离线静态分析、纯内存 codec/queue 单元测试和不改变回调返回值的被动观测。

第一次实机验证前必须停下，由用户连接双摇杆 XInput 手柄、手动部署并启用 witness、启动精确目标游戏，然后明确表示可以开始 passive matrix。passive 证据通过后还要再次停下，单独取得用户对 active loopback 的确认。

## 已实现并完成首个被动实机窗口的 T0-03R V1 runtime（历史）

首个独立 `T03R` IPC/runtime（V1）未复用或放宽 T0-02 `ModeLatch`。它当时以一次事务安装 20 个被动 Hook 位：11 个固定 direct-call site、Keyboard/Gamepad 两张 vtable 的 `+0x30/+0x34/+0x38` 六个 slot，以及三项 USER32 IAT。RVA `0x531231` 的原调用返回值实际位于 XMM0，wrapper 以 `__m128d` ABI 原样返回；InputDevice 分别按 bool-thiscall、triggered-thiscall 和 float-thiscall 调用原函数；Win32 三项 API 保持 stdcall。这一布局只描述历史 case04，已由后文的 V2 28 槽事务替代。

V1 事件 ring 使用独立 magic/version，容量 65536。每条事件含 native sequence、QPC、`loopIteration`、phase、thread、return RVA、original-call count；只有 action `0..11` 的 query 才携带 action、logical result、device kind 和 assignment key，`12+` 不进入 artifact。历史 analyzer 要求 native/artifact sequence 无缺口、QPC 正数且单调、至少 30 个完整有序 loop、mutation 位于 update 或显式 room/run transaction、静态候选 message pump 在主线程出现、所有 wrapper 恰好一次调用原函数，以及 ring 无覆盖、20 个 V1 Hook 完整恢复。

runtime 会拒绝任何 debugger attached 或冲突模块存在的目标。首个真实窗口已 READY 并持续到十分钟有界上限，记录 304,181 条连续事件；用户动作与游戏表现正常，随后 guard 成功卸载，外部只读复核确认 11 个 call、6 个 vtable slot 和 3 个 IAT slot 全部恢复且 Native 模块无残留。但旧 controller 把有界上限误当作错误，在 guard 恢复后没有再次 drain `HOOKS_RESTORED`，也没有保存 target before/after，故该 artifact 保持 `FAIL`，不能晋级 gate。后续 controller 将有界上限作为正常停止，并在所有可安全收尾路径显式 unload、drain 和保存恢复快照。

## 首个 passive trace 的静态反证与探针修订

`20260820T101323Z-t0-03r-loop-passive-keyboard-t0-03r-keyboard-passive-20260820-04/events.jsonl` 含 304,181 条事件。loop/update/render 与 Win32 pump 都有命中，`0x555231 -> 0x2FADC0` 的 broad-gameplay wrapper 有 17,994 对 enter/exit；但六个 InputDevice logical-query slot 与旧 entity/remove/spawn/post-update 四类候选均为零。这不能解释成“没有输入、实体或 Authority 活动”，而是同时暴露了 input sample 与 Authority envelope 的探针选点问题。

17,994 恰好是 35,988 个 top-level iteration 的一半。静态上 `0x554CD0` 在 `0x554CF6..0x554D0D` 对 owner `+0x4ABBC` 做 parity 测试，并在相应轮于 `0x554D4A` 提前返回；这与 trace 中只有隔轮才出现 `0x555231` 一致。所以下一版 analyzer 应把 full broad-update transaction 与 top-level/render iteration 分开编号，不能把 60 Hz loop iteration 自动当成 30 Hz gameplay/Authority tick。

### offline InputManager 的真实分发与 sample envelope

固定对象 `module+0x857B18` 的 vptr 由 RVA `0x233B` 写成 vtable RVA `0x782950`。该 vtable 的 `+0x74` 是 RVA `0x620FB0`：controller id 不是 `-1` 时，它先经 manager vslot `+0x28`（RVA `0x620390`）在 active shared-pointer vector 中按 `InputDeviceBase+0x0C` key 选中元素，再调用传入 callback。pressed/triggered/scalar callback 分别是 RVA `0x620940/0x6209E0/0x620AB0`，它们才对已解析的对象虚调 `+0x30/+0x34/+0x38`。

因此，稳定快照中的 Keyboard 对象如果发生这些 logical query，必然经过已 patch 的 Keyboard slots。首个 trace 零命中证明的是：该 capture 内没有 action `0..11` 的这三类 logical-query 调用；它不证明 owner 是漏 patch 的 `InputDeviceBase`。静态 direct-vptr 写只在 Base ctor/dtor、derived ctor/dtor 的临时 vptr 交换和 OriginalOnline 临时对象中发现 Base vtable，尚未发现长期 offline Base aggregate。给 Base 的三槽加 hook 可以作为非门禁诊断，但不能替代定位 sample envelope。

真实 offline physical/cache update 是另一条路径：

| 角色 | RVA / 边 | 静态事实 | 建议标签 |
|---|---:|---|---|
| manager sample owner | `0x554F7D -> 0x61FC00` | broad update 条件调用固定 InputManager；另两个 direct callers `0x621CD9/0x621CF0` 位于初始化路径 | `STATIC_CONFIRMED_INPUT_MANAGER_UPDATE` |
| per-device common update | `0x61FC97 -> 0x61F280` | 唯一 direct caller；遍历 active shared-pointer vector 后以 object 为 ECX 调用 | `STATIC_CONFIRMED_ACTIVE_DEVICE_UPDATE` |
| physical poll | vtable `+0x78` | common update 首先调用；Keyboard target RVA `0x66C650`，Gamepad target RVA `0x622940` | `STATIC_CONFIRMED_PHYSICAL_POLL_SLOT` |
| logical consumers | vtable `+0x30/+0x34/+0x38` | public pressed/triggered/scalar query，不是每帧 physical sample 的必经点 | `STATIC_CONFIRMED_OPTIONAL_QUERY` |

Keyboard `+0x78` 实现采集 OS keyboard state，写入 `self+0xE8` 的物理 snapshot；Gamepad `+0x78` 更新自己的 button/axis cache。`0x61F280` 负责轮转通用 device 状态并调用该 physical poll。所以下一版最小被动输入集合应覆盖 `0x554F7D`、`0x61FC97` 和 Keyboard/Gamepad `+0x78`，只记录 vptr、assignment key、进入/退出、返回值与同一 loop iteration；不读取具体键/button 值。manager `+0x74` 只有在 callback 已唯一归类为 logical action 且 action 为 `0..11` 时才可记录，primitive callback 的 payload 在 action mapping 冻结前不得输出，以免把 UI/session action 混入拟拦截域。原六槽可以保留为 consumer 诊断，但不能再作为“存在本地输入”的必过条件。

`0x61F280` 对 logical buffers 的真实顺序还排除了另一种错误捷径。它在 `0x61F2B5` 先把 `self+0xC0` 指向的 current float array 完整复制到 `self+0xC4` 指向的 previous array，在 `0x61F2F2` 把 current 清零，随后于 `0x61F307` 载入 vslot `+0x78`，并在 `0x61F30A` 间接调用 physical poll（返回点 `0x61F30C`）。完整函数中没有再次写 current；Keyboard/Gamepad 的 `+0x78` 实现也不写这两个 buffer。基类 vslot `+0x4C`（RVA `0x61ED50`）才是显式 `(logicalAction, float)` current setter。因此 `+0xC0/+0xC4` 更像每轮推进并清空的 logical override/injected layer，而不是 normal offline physical source：在 `0x61FC97` 原调用返回后读取前 12 项可标作 `LOGICAL_OVERRIDE_BUFFER_DIAGNOSTIC`，并严格验证 count、finite 和 `[0,1]`，但全零是合法且预期的，不能据此生成 canonical input 或通过 gate。

InputManager vtable `+0x40/+0x44/+0x48`（RVA `0x620710/0x6207B0/0x620850`）经 callback `0x620730/0x6207D0/0x620880` 直接调用 device primitive `+0x24/+0x28/+0x2C`；`+0x4C/+0x50/+0x58` 才经 callback `0x620940/0x6209E0/0x620AB0` 调 logical `+0x30/+0x34/+0x38`。这证明存在完全绕过已 patch logical slots 的 primitive dispatch；结合用户确实移动/射击但六槽为零，它是 normal offline consumer 的强候选，尚不是已唯一定位的主移动/射击 callsite。因此零命中属于 **hook 设计可能漏路径**，不代表没有 consumer。下一版若观察前一组，只能记录 primitive code 与 device key；在 binding/action ledger 唯一化以前不能把 primitive code 直接重命名为 action `0..11`。

### Authority 零命中：标准路径尝试与更早的 alternate transaction

`0x2FADC0` 开头在 `0x2FADFA` 调 RVA `0x28BBA0`；只有实际捕获 AL=true 才能证明 `0x2FAE01` 跳到 `0x2FBBAA`。首个 trace 没有 hook predicate，因此“outer wrapper 命中、旧内部点为零”本身不能证明每次都在这里 early-skip。`0x555231 -> 0x2FADC0` 必须从 gameplay Authority 降级为 `STANDARD_OR_POST_CONDITIONAL_ATTEMPT`，其进入次数不能直接计作 server/gameplay tick。

旧 mutation 选点本身也漏路径：

- `0x2FB51C -> 0x4318A0` 只处理 `root+0x1B83C` 的条件性特殊状态，不是通用 entity-update owner。
- `0x2FBB66 -> 0x147F0` 只在 `root+0x6772C` delayed-removal list 中节点倒计时为零时调用；零命中不等于无 removal。
- `0x28B3D -> 0x2FE410` 只是一个单独 spawn callsite；目标 RVA `0x2FE410` 有大量其它调用者，零命中不能代表无 spawn。
- `0x2FBBA5 -> 0x4607A0` 是标准函数 non-early tail，但此前存在多条条件返回；它不是每个 outer attempt 的必经 post-update。

`0x554CD0` 在 sample 之前还有一个不能忽略的 transaction owner。full update 在 `0x554D5E` 无条件调用 `0x559720`；callee 入口检查 owner `+0x4B130`，为零立即返回。flag 非零且内部状态分支成立时会执行以下顺序：

| 边 | 静态事实 | 保守标签 |
|---:|---|---|
| `0x554D5E -> 0x559720` | full update 的 owner attempt；入口以 owner `+0x4B130` gate | `ALT_TRANSACTION_OWNER_ATTEMPT` |
| `0x559B21 -> 0x2F8140` | exact bytes `E8 1A E6 D9 FF`；ECX=game root，栈参数依次为 owner `+0xFA4` state pointer 与 `1`；target 以 `ret 8` 清理两参，caller 不读取 EAX；callee 向 root 的 bitsets、counters 和多个状态字段回填 | `ALT_SNAPSHOT_STATE_APPLY_CANDIDATE` |
| `0x559B2C/37/42` | 对 `root+0x1BAA8` player vector 依次调用 `0x5BF530/0x5BBA30/0x5BC000` | `ALT_PLAYER_VECTOR_PREP_CANDIDATES` |
| `0x559B4D -> 0x402980` | ECX=`root+0x18300`；与标准 `0x2FBA4D` 调用相同 target | `ALT_ROOM_SIMULATION_CANDIDATE` |
| `0x559B82 -> 0x464EE0` | caller `push 1`，target 是 callback id `0x0F` 的 PostGameStart dispatcher，只读取该栈参并以 `ret 4` 返回，caller 不读取 EAX；语义为 `void __stdcall(uint32_t)`，在该分支中位于 room call 之后 | `ALT_INLINE_SIMULATION_RUN_LIFECYCLE_COMPLETION` |

该函数在 `0x559CCD` 清 owner `+0x4B130` 后返回。因此，当 alternate transaction 确实运行时，稍后 `0x5551C2` 会看到 flag 已为零并仍允许 `0x555231 -> 0x2FADC0`；同一 loop 中看到 standard outer attempt 并不能排除实际 state apply/room simulation 已在前面完成。反过来，flag 是条件状态，静态分析也不能声称首个 trace 中一定走了 alternate branch，必须用新事件证明。

完整 `.text` operand scan 只找到 RVA `0x4CF6D0/0x4EB981/0x517F8E/0x558B6E/0x558DBD/0x581289/0x5812D9/0x5AB1BD` 将该 flag 置 1，另有初始化 `0x5529D9` 和 transaction tail `0x559CCD` 清零；在 `0x554CD0` 的 sample-to-standard 顺序区间内没有置 1。这进一步支持把它视为显式 restore/replay/lifecycle 类 transaction candidate，而不是默认每 tick owner。它能否解释 case04 必须由 `0x554D5E` pre/post flag 和内部 call 命中共同决定。

从 `0x554F7D -> 0x61FC00` sample 返回到 `0x555231` 之间的 direct calls 已逐一复核。`0x554F8F`、`0x554F9E`、`0x555047`、`0x5550AD`、`0x5550D3/0x5550E4/0x5550EF`、`0x555102/0x555113`、`0x5551A9/0x5551B4/0x5551BD` 分别属于状态集合、getter、UI/session action 或 transition predicates，未发现与 `0x402980` 同级的通用 simulation owner。`0x5551D8` inline field copy 继续保持 `UNKNOWN`。因此若实际 Authority 在 `0x555231` 之前，当前最强候选是 sample 之前的 `0x559720` transaction，而不是把 `0x5551D8` 升格为 input apply。

下一版最小只读 Authority 集合应为：

1. `0x554D5E -> 0x559720`，记录 attempt 与安全可读的 pre/post `owner+0x4B130`。
2. `0x559B21 -> 0x2F8140`、`0x559B4D -> 0x402980`、`0x559B82 -> 0x464EE0`，区分 alternate state-apply、room-simulation 与 inline-simulation 的 run-lifecycle completion。
3. `0x2FADFA -> 0x28BBA0`，显式保存原调用返回 AL，标签 `STANDARD_SKIP_PREDICATE_RESULT`；并记录其 false 后直线必经的 `0x2FAE09 -> 0x308630`，标签 `STANDARD_BODY_BEGIN_ANCHOR`，从而区分 predicate skip 与后续条件返回。
4. `0x2FBA4D -> 0x402980` 与 `0x2FBBA5 -> 0x4607A0`，标签分别为 `STANDARD_ROOM_SIMULATION_CANDIDATE`、`STANDARD_NON_EARLY_TAIL`。
5. 可选共享内层 `0x403E70 -> 0x186C0`，标签 `ROOM_ENTITY_LIST_UPDATE_CANDIDATE`，但必须按外层 `0x559B4D/0x2FBA4D` 归属，不能单独当完整 Authority tick。

新 trace 只有在每个 room-simulation candidate 都唯一属于一个 full update/lifecycle transaction、alternate inline 路径满足 state apply → room simulation（含 entity-list）→ run-lifecycle completion、五个 alternate outcome completion 均只能在同一个 active owner attempt 内闭合、standard predicate 与 standard room/MC_POST_UPDATE tail 的出现一致时，才足以继续冻结 envelope。未单独 hook 的 player-prep 调用不能由相邻事件冒充已观测证据。若同一 loop 同时出现 alternate 与 standard room simulation，应先报告 double-simulation candidate 而不是自动合并。上述是当时的静态结构与 fail-closed 判据；后续 V5 passive 矩阵已满足 loop gate，但这仍不提供 raw/apply 或主动抑制与注入授权。

## 被动探针 V2 实现边界

V2 IPC 将 version 升为 2，artifact identity 升为 `IsaacCsT0-LoopEnvelopeResearch-v2`。安装事务固定为 28 个槽：23 个 direct-call site、Keyboard/Gamepad 两项 concrete `+0x78` physical-poll slot，以及三项 USER32 IAT。六个 optional logical-query slot 仍在安装前校验 vtable/COL/target ABI，但不再 patch。controller 的 before/after 文档覆盖同一 28 个实际改写槽；Native 只有全部准备、暂停线程、应用成功后才发布完整 mask，卸载仍先禁止新 ENTER、恢复全部槽、等待在途 wrapper 补齐 EXIT，再发布 `HOOKS_RESTORED`。

新增输入事件只记录 `0x554F7D` manager sample、`0x61FC97` per-device update 和 Keyboard/Gamepad `+0x78` physical poll 的 enter/exit、原调用次数、AL 返回、vtable 分类与 assignment key。它不读取 key/button/axis cache，不生成 action 或 canonical bytes；action `12+` 仍不进入任何 payload。六个 logical-query ABI 只作静态诊断，不进入 V2 事务或 physical-sample 非空门。

Authority 事件覆盖 `0x554D5E` alternate owner attempt、`0x559B21` alternate state apply、`0x559B4D` alternate room、`0x559B82` inline-simulation run-lifecycle completion、`0x555231` conditional attempt、`0x2FADFA` predicate（保存 AL）、`0x2FAE09` standard-body anchor、`0x2FBA4D` standard room、`0x2FBBA5` standard tail和嵌套 `0x403E70` entity-list update。`0x559B21 -> 0x2F8140` 的 typed passive wrapper 按已冻结的 `void __thiscall(root*, state*, uint32_t)`、callee `ret 8` ABI 安装；未记录这条边时，不能单凭 alternate room/completion 命中宣称 snapshot apply 已证明。`0x559B82` 的事件 phase 应是 `RUN_LIFECYCLE`；其“在 room 之后”的顺序由 owner 内嵌套关系表达，不能借 `POST_UPDATE` phase 冒充通用 post-update 证据。

`0x559720` 还含五个与 inline simulation 分支互斥的 run-activation outcome。完整 `.text` direct-call scan 对 `0x2F5850/0x2F72F0/0x2F6DD0/0x2F7750/0x2F5320` 各只找到下表这一条 owner caller；五个 target 都会设置 active/run 字段，并在唯一正常返回前调用 PostGameStart dispatcher `0x464EE0`，因此已不能继续当作不影响 Authority 的未知旁路：

| owner call | exact bytes / target / return | target stack ABI | 保守标签 |
|---:|---|---|---|
| `0x559A70` | `E8 DB BD D9 FF` -> `0x2F5850` -> `0x559A75` | free/callee-clean by-value frame，正常尾 `ret 0x70`；随后 `0x559A77 -> 0x559500` 为 ECX=owner、无栈参、plain `ret` | `ALT_RUN_ACTIVATION_OUTCOME_5850` |
| `0x559C18` | `E8 D3 D6 D9 FF` -> `0x2F72F0` -> `0x559C1D` | `void __stdcall(state*)`，只取 `[ebp+8]`，`ret 4` | `ALT_RUN_ACTIVATION_OUTCOME_72F0` |
| `0x559C3E` | `E8 8D D1 D9 FF` -> `0x2F6DD0` -> `0x559C43` | `void __stdcall(state*)`，只取 `[ebp+8]`，`ret 4`；来向 ECX 恰好含同一 pointer，不构成 thiscall 证据 | `ALT_RUN_ACTIVATION_OUTCOME_6DD0` |
| `0x559C7F` | `E8 CC DA D9 FF` -> `0x2F7750` -> `0x559C84` | free/callee-clean by-value frame，正常尾 `ret 0x24` | `ALT_RUN_ACTIVATION_OUTCOME_7750` |
| `0x559CA6` | `E8 75 B6 D9 FF` -> `0x2F5320` -> `0x559CAB` | free/callee-clean by-value frame，正常尾 `ret 0x68` | `ALT_RUN_ACTIVATION_OUTCOME_5320` |

`0x559C18/0x559C3E` 可用单 pointer 的 typed passive wrapper 安全记录；另外三条若直接改写 owner call，需要先冻结 `0x70/0x24/0x68` 的完整 by-value 布局，不宜把 opaque frame 猜成普通参数。更小、更安全且足以区分五条正常完成路径的方案，是改写各 target 内唯一必经的 PostGameStart dispatcher edge：`0x2F61FF E8 DC EC 16 00`（来自 `0x2F5850`，return `0x2F6204`，arg `0`）、`0x2F7721 E8 BA D7 16 00`（来自 `0x2F72F0`，return `0x2F7726`，arg `1`）、`0x2F72C9 E8 12 DC 16 00`（来自 `0x2F6DD0`，return `0x2F72CE`，arg `0`）、`0x2F7A10 E8 CB D4 16 00`（来自 `0x2F7750`，return `0x2F7A15`，arg `0`）、`0x2F580B E8 D0 F6 16 00`（来自 `0x2F5320`，return `0x2F5810`，arg `0`）。这五条均为 `void __stdcall(uint32_t)`、target `ret 4`。V2 已用一个按 return RVA 区分 outcome 的共享 typed wrapper 覆盖全部五条；analyzer 只在 pre-flag=1、同一 owner 内恰好一个 completion enter/exit 且 post-flag=0 时接受 outcome transaction。

V2 analyzer 对新 evidence class 要求非真空的三层 sample 包络；Authority 非空门只接受至少一条结构闭合的 standard transaction 或 alternate inline simulation transaction。Alternate completion-outcome 只用于证明 active owner 分支闭合，不计作 Authority simulation；`0x555231` conditional attempt、top-loop iteration、predicate、body anchor 或 lifecycle completion 单独命中均不能通过 Authority 门。旧 case04 artifact 不改写，也不能以 V1 事件重放冒充 V2 gate 输入。

## V2 keyboard trace 的 standard-owner 短路复核

`20260820T122500Z-t0-03r-loop-passive-keyboard-t0-03r-v2-keyboard-passive-01` 有 4,060 个完整 top-level loop、2,030 个 `0x555231 -> 0x2FADC0` attempt。每次 `0x2FADFA -> 0x28BBA0` 都返回 false，且 `0x2FAE09 -> 0x308630` 正常完成；但 `0x2FBA4D` standard room、`0x2FBBA5` MC_POST_UPDATE、`0x403E70` entity-list 和所有 alternate owner outcome 都为零。该证据只把 short-circuit 区间缩到 `0x2FAE0E..0x2FBBAA`，不能单凭零命中否定 `0x2FADC0` 的 standard owner 角色。

`0x308630` 读取 room/grid 与 player position，更新的是 cursor/HUD 类位置和动画状态；它不是 room/entity simulation，也不能因为每次命中而升格为 Authority。其后完整控制流如下：

| gate / RVA | false/normal continuation | active outcome | 保守分类 |
|---|---|---|---|
| `0x2FB3BD`，`root+0x264F4` | `<=0` 到 `0x2FB3E6` | `>0` 时递减，`0x2FB3D0 -> 0x5A2B30` 后直接 `ret 0x2FB3E5` | fixed-delay/hold early return，语义 `UNKNOWN` |
| `0x2FB3E6`，`root+0x1D520` | zero 到 `0x2FB4BC` | nonzero 更新该状态和 `root+0x23A74`；随后按 `root+0x24ECC` 到 common epilogue 或 `ret 0x2FB4BB` | modal/transition branch；不是 steady room tick |
| `0x2FB4BC`，`root+0x1D654` | zero 到 `0x2FB4E1` | `0x2FB4CB -> 0x457450` 后 `ret 0x2FB4E0`；target 引用 `Loading`/`Idle` 资源并维护内部状态机 | loading-state early return |
| `0x2FB4E1`，`root+0x1BA78` | zero 到 `0x2FB50D` | `0x2FB4F0 -> 0x52F1C0`，随后 `0x2FB4F7 -> 0x5A2B30` 并 `ret 0x2FB50C` | room-lifecycle FSM，含独立 room simulation，见下文 |
| `0x2FB50D`，`root+0x1B83C` | zero 到 `0x2FB561` | 特殊状态 update/prep 后 `0x2FB539 -> 0x2F0070`；AL=false 到 common epilogue，AL=true 清计数后 `ret 0x2FB560` | conditional special-state branch |
| `0x2FB561`，`root+0x1C034` | zero 到 `0x2FB5DD` | `0x2FB5A2 -> 0x5ACA90` 更新有限状态；其后可能继续、到 common epilogue，或 `0x2FB5C7 -> 0x5B6840` 后 `ret 0x2FB5DC` | animation/transition FSM |
| `0x2FB5DD`，`root+0x26778` predicate | false 到 `0x2FB69B` | true 时更新该对象；invalid/incomplete state 到 common epilogue，否则 `0x2FB619 -> 0x60F550` 建临时颜色/状态并 `ret 0x2FB69A` | special visual/transition outcome |
| `0x2FB69B`，`root+0x25954` | zero 到 `0x2FB6A8` | nonzero 直接到 common epilogue | lifecycle/special-state suppressor |
| `0x2FB6AE/0x2FB811`，`root+0x26598` | nonpositive 到 `0x2FB837` | positive 时按 `root+0x265B8` 递减；若仍 positive，`0x2FB821 -> 0x5A2B30` 后 `ret 0x2FB836` | timed transition hold；不是已证实的 focus flag |
| `0x2FB837..0x2FB8DF`，`root+0x23A74` | zero/finished 到 manager check | active 时更新并清 room list；`0x2FB8C9 -> 0x5B7650` 返回 true 则到 common epilogue | queued transition branch |
| `0x2FB8DF`，manager vector / `root+0x24ECC` | pending manager 或 state zero 到 deep entry `0x2FB912` | manager empty 且 state nonzero 时 `0x2FB8FC -> 0x4EF990` 后 `ret 0x2FB911` | pending run/selection state early return |
| deep entry `0x2FB912` | housekeeping 后 `0x2FBA4D -> 0x402980`，再到 `0x2FBBA5 -> 0x4607A0` | 无额外 early return | steady standard room simulation + MC_POST_UPDATE |

`root+0x1BA78` 分支暴露了 V2 的一个真实 loop-envelope 缺口。`0x52F1C0` 是有限状态机；在其内部 `0x52FC77 E8 04 2D ED FF -> 0x402980`（return `0x52FC7C`）以 ECX=`global root+0x18300` 调用与 standard tail 相同的 room target，随后调用 `0x558ED0` 和 player-vector prep，再返回 outer early-return 路径。因此 `0x2FBA4D` 不是 `0x2FADC0` 内唯一的 room mutation 边。该内层边的冻结 ABI 是 `void __thiscall(Room*)`、callee plain `ret`，保守标签应为 `STANDARD_ROOM_LIFECYCLE_SIMULATION_CANDIDATE`，phase 应为 `ROOM_LIFECYCLE`；它不能冒充 steady gameplay tick，也不能用 MC_POST_UPDATE 缺失否定其 lifecycle transaction。

该阶段 artifact 没有记录上述 gate 的 branch identity。用户已经明确游戏失焦会自动暂停，而开始/结束 capture 的人工交互必然可能把前台切回 Codex；现有 USER32 事件又没有保存 message id 或 foreground-window identity。因此该 trace 同时符合“捕获期间一直处于自动暂停/transition short-circuit”和“用户动作发生在 capture 窗口之外”，不能证明稳定 focused gameplay 走了另一套未定位 Authority owner。当时的下一步是先在不改变 hook 集合的情况下做一次游戏全程保持前台、由有界计时自动结束的 passive 重跑；后续 V3/V5 foreground captures 已完成该验证，未扩大 Authority mutation 集合。

若前台重跑仍然 standard room 为零，才增加下列 typed **branch census** 锚。它们只用于说明 short-circuit 原因；除 `0x52FC77` 外不得计作 Authority mutation：

| caller exact bytes -> target -> return | frozen ABI | 诊断标签 |
|---|---|---|
| `0x2FB3D0 E8 5B 77 2A 00 -> 0x5A2B30 -> 0x2FB3D5` | `void __thiscall(root+0x1DA04)`，plain `ret` | `STANDARD_DELAY_HOLD_EXIT` |
| `0x2FB47B E8 C0 B3 2B 00 -> 0x5B6840 -> 0x2FB480` | `void __thiscall(root+0x23A74)`，plain `ret` | `STANDARD_MODAL_BRANCH`；调用本身不区分其后的两种 exit |
| `0x2FB4CB E8 80 BF 15 00 -> 0x457450 -> 0x2FB4D0` | `void __thiscall(root+0x1D650)`，plain `ret` | `STANDARD_LOADING_EXIT` |
| `0x2FB4F0 E8 CB 3C 23 00 -> 0x52F1C0 -> 0x2FB4F5` | `void __thiscall(root+0x1BA78)`，plain `ret` | `STANDARD_ROOM_LIFECYCLE_BRANCH` |
| `0x52FC77 E8 04 2D ED FF -> 0x402980 -> 0x52FC7C` | `void __thiscall(Room*)`，plain `ret` | `STANDARD_ROOM_LIFECYCLE_SIMULATION_CANDIDATE` |
| `0x2FB539 E8 32 4B FF FF -> 0x2F0070 -> 0x2FB53E` | `bool __thiscall(root+0x1B83C)`，plain `ret` | `STANDARD_SPECIAL_STATE_RESULT` |
| `0x2FB5A2 E8 E9 14 2B 00 -> 0x5ACA90 -> 0x2FB5A7` | `void __thiscall(root+0x1C034, bool)`，callee `ret 4`；caller arg=`0` | `STANDARD_TRANSITION_FSM_BRANCH` |
| `0x2FB5E3 E8 28 CC 13 00 -> 0x438210 -> 0x2FB5E8` | `bool __thiscall(root+0x26778)`，plain `ret` | `STANDARD_SPECIAL_VISUAL_PREDICATE` |
| `0x2FB619 E8 32 3F 31 00 -> 0x60F550 -> 0x2FB61E` | result pointer `__thiscall(outColor*, uint32_t)`，callee `ret 4`；caller arg=`0xFF000000` | `STANDARD_SPECIAL_VISUAL_OUTCOME` |
| `0x2FB821 E8 0A 73 2A 00 -> 0x5A2B30 -> 0x2FB826` | `void __thiscall(root+0x1DA04)`，plain `ret` | `STANDARD_TIMED_HOLD_EXIT` |
| `0x2FB8C9 E8 82 BD 2B 00 -> 0x5B7650 -> 0x2FB8CE` | `bool __thiscall(root+0x23A74)`，plain `ret` | `STANDARD_QUEUED_TRANSITION_RESULT` |
| `0x2FB8FC E8 8F 40 1F 00 -> 0x4EF990 -> 0x2FB901` | `void __thiscall(root+0x24ECC)`，plain `ret` | `STANDARD_PENDING_STATE_EXIT` |

这些 direct-call replacement 均可在保持原 ECX/栈参数、恰好调用一次 original 并保存 AL/返回值的 typed passive wrapper 中安全记录；但一次性安装全部只适合“前台重跑仍失败”的诊断版本。当前最小验证动作仍是零新增 hook 的前台 passive 重跑。

## V2 前台 PASS 与 V3 room-lifecycle 补充门

`20260820T123537Z-t0-03r-loop-passive-keyboard-t0-03r-v2-keyboard-foreground-02` 在用户把游戏恢复并保持前台后得到 `PASS`：233,460 条 artifact/native sequence 连续事件、10,864 个完整有序 loop、10,864 组 `InputManager → device update → physical poll` 包络，以及 5,432 条严格闭合的 `predicate=false → body → standard room → entity-list → MC_POST_UPDATE` transaction。所有事件位于同一 main thread，ring overwrite 为零，28 个被改写位置的 before/after SHA 与逐项 bytes 完全相同。此前后台窗口稳定停在 body 后的现象因此保留为与 auto-pause/transition short-circuit 一致的阴性样本，不解释为另一套 steady Authority owner。

前台 PASS 不覆盖 `root+0x1BA78` room-lifecycle FSM。完整控制流复核冻结两条补充边：owner `0x2FB4F0 E8 CB 3C 23 00 → 0x52F1C0`（return `0x2FB4F5`）和 nested simulation `0x52FC77 E8 04 2D ED FF → 0x402980`（return `0x52FC7C`），二者均为无栈参数、caller 不消费返回寄存器的 `void __thiscall`。Nested simulation 每次 owner invocation 至多一次，正常完成时必经精确 callsite `0x403E70` 的 entity-list update；owner 返回后 outer standard path在 `0x2FB50C` 提前结束，明确跳过 steady `0x2FBA4D/0x2FBBA5`。

因此 V3 把 IPC version 和 artifact identity 升级，安装事务固定为 30 个槽：25 个 direct-call site、2 个 concrete physical-poll vtable slot 和 3 个 USER32 IAT。Analyzer 只把同一 main thread/loop/conditional attempt 内的 `lifecycle owner enter → nested simulation enter → exactly one 0x403E70 entity-list pair → simulation exit → owner exit` 计作显式 `ROOM_LIFECYCLE` Authority transaction；owner-only step 合法但不计 Authority，且 lifecycle simulation 不得与同 loop 的 steady standard 或 alternate simulation 混合。V2 artifacts保持不可覆盖历史证据，不能进入 V3 gate。

### V3 首次捕获的 bootstrap arming race

首次 V3 keyboard foreground 尝试生成不可覆盖的失败 artifact `20260820T131548Z-t0-03r-loop-passive-keyboard-t0-03r-v3-keyboard-foreground-01`。事件严格为 `NativeInitialized → HooksInstalled → Failure(errorCode=6) → HooksRestored`；失败发生在安装完成后 545 QPC ticks，且 `mainThreadId=0`、`loopIteration=0`、从未出现 `LoopBegin`。30 个目标的 before/after SHA 均为 `BBFA82704E0A13682BF59C363EB8808C16B2D9F8A07E4E5832ECFC373E35BB9F`，最终 mask 为零，没有残留 hook 证据。

这不是已知主线程与当前线程不一致的动态证据。安装事务可以在一个既有 loop iteration 中段完成；原实现只允许 `LoopClockHook` 冻结主线程，却把冻结前由其它 wrapper 命中的 `UNESTABLISHED` 状态错误归类为 `KNOWN_MISMATCH`。修正后的 bootstrap 规则固定为：主线程尚未冻结时，所有非 Clock wrapper（包括 USER32 message wrapper）只调用原函数一次，不发布 phase 事件也不报告 WrongThread；仅下一次 `LoopClockHook` 可 CAS 冻结主线程并发布 iteration 1。冻结之后的真实线程不匹配仍立即 fail-closed。若整个捕获没有 `LoopBegin`，analyzer 仍必须因缺少非真空 loop/Authority 证据而失败。

### V3 room transition FAIL 与 V4 过渡 transaction

bootstrap 修正后的 keyboard artifact `20260820T133127Z-t0-03r-loop-passive-keyboard-t0-03r-v3-keyboard-foreground-02` 得到可重放 `PASS`：173,964 条连续事件、7,943 个完整 loop、7,943 组 keyboard physical-poll 包络和 3,791 条 steady standard transaction，30 槽完整恢复。首个 phase 事件严格为 iteration 1 的 `LoopBegin`；两个 V3 lifecycle FSM hook 在稳态房间内均为 armed non-hit。

随后 room artifact `20260820T134130Z-t0-03r-loop-passive-room-t0-03r-v3-room-transition-01` 正确 `FAIL`。唯一异常位于 loop 1838：`ConditionalAttempt → predicate false → body → NewRoom pair → EntityList pair → ConditionalAttemptExit`。EntityList 原始 enter/exit 深度正常，但当时 standard room、alternate room 与 V3 lifecycle simulation depth 全为零，因此 `authority-owned` 与 semantic owner cardinality 的 `balanced-depth` 同时失败。64,921 条事件连续、3,044 个 loop 完整、无 failure/overwrite，30 槽 before/after 相同，所以这不是日志缺口或恢复问题。

固定 EXE 控制流将缺失 owner 唯一缩到另一条门房过渡链：

`0x2FB226 → 0x2F7AC0 → 0x2F802A → 0x344940 → 0x345842 → 0x33E0A0 → 0x33F5B9 NewRoom → 0x34584D Room::Update → 0x403E70 EntityList`。

`0x2FB226` bytes `E8 95 C8 FF FF`，target `0x2F7AC0`，return `0x2FB22B`；全 `.text` 只有这一处 direct caller，ECX=root、无栈参数、callee plain `ret`，保守冻结为 `void __thiscall` transition owner。`0x34584D` bytes `E8 2E D1 0B 00`，target `0x402980`，return `0x345852`；ECX=`root+0x18300` Room*、无栈参数、callee plain `ret`。它在 special transition/load apply 中位于唯一 NewRoom callback 之后，正常完成必经现有 exact `0x403E70` entity-list call。

V4 因此新增显式 `transition owner → NewRoom pair → nested simulation → entity-list pair → simulation exit → owner exit` transaction。不能把任意 `NewRoom` 后的裸 EntityList 或任意 ConditionalAttempt 内的 EntityList 泛化为合法 Authority。新增两槽使实际 hook 数达到 32；IPC version 升为 4，artifact identity/evidence class 升为 V4，shared hook mask 改用 64-bit storage，并以 `0x00000000FFFFFFFF` 表示 32 槽完整安装。旧 V1–V3 artifact 仅保留为不可覆盖历史证据，不能直接进入 V4 gate。

### V4 普通跨门 FAIL 与 V5 room-change 锚修正

V4 的上述链是由 V3 阴性 trace 提出的静态假设，不是运行时命中事实。首次 V4 普通跨门 artifact `20260820T174153Z-t0-03r-loop-passive-room-t0-03r-v4-room-transition-01` 将该假设明确证伪并作为不可覆盖失败证据保留：57,130 条事件连续、2,768 个 loop 全部闭合、32 槽 mask 为 `0x00000000FFFFFFFF`、无 overwrite/failure 且 before/after SHA 同为 `4F73B1FE90B405750A8507E848B6ADDE8F76D60DC78D509667FE755258CB8113`。然而事件 63–66 全为零，说明 `0x2FB226` 和 `0x34584D` 均为 `ARMED_NONHIT`。

唯一异常位于 loop 1425：`ConditionalAttempt → predicate false → body → NewRoom pair → EntityList pair → ConditionalAttemptExit`。全 artifact 有 933 对 EntityList，却只有 932 对 standard room/tail，正好多出的这一对既没有 standard、alternate、V3 lifecycle，也没有 V4 transition owner。Analyzer 因此正确拒绝 `authority-owned`、semantic `balanced-depth` 和 room `scenario-evidence`；不得为通过而把 ConditionalAttempt 或裸 NewRoom 泛化成 Authority owner。

重新扫描唯一 NewRoom owner `0x33E0A0` 的全部 direct callers，并沿 `0x2FADC0` 的真实普通门状态分支向下追踪后，实际固定构建链为：

`0x2FB51C → 0x4318A0 transition-state update → 0x430950 helper → 0x430BBF → 0x2FE2F0 room-change apply → 0x2FE371 → 0x33FC80 → 0x3405E8 → 0x33E0A0 → 0x33F5B9 NewRoom → 0x2FE37C Room::Update → 0x403E70 EntityList`。

其中 `0x2FB50D` 先检查 `root+0x1B83C`，`0x2FB516` 把该 state object 放入 ECX：

| V5 passive edge | 精确 ABI | 保守证据标签 |
| --- | --- | --- |
| `0x2FB51C E8 7F 63 13 00 → 0x4318A0 → return 0x2FB521` | `void __thiscall(root+0x1B83C)`；无栈参数；callee plain `ret` | `STATIC_CONFIRMED_ORDINARY_DOOR_ROOM_CHANGE_OWNER` |
| `0x2FE37C E8 FF 45 10 00 → 0x402980 → return 0x2FE381` | ECX=`root+0x18300` Room*；无栈参数；callee plain `ret`；caller 不消费 EAX | `STATIC_CONFIRMED_ORDINARY_DOOR_ROOM_CHANGE_SIMULATION_CANDIDATE` |

作为额外静态路径证明，`0x430BBF E8 2C D7 EC FF → 0x2FE2F0 → return 0x430BC4` 以 ECX=root、两个栈参数调用 `ret 8` owner；参数来自 transition state `+0x14/+0x18`，具体字段语义保持 `UNKNOWN`。V5 不需要为此再增加第三个槽：外层 `0x2FB51C` owner 和内层 `0x2FE37C` simulation 已能把既有 NewRoom/EntityList pair 严格嵌套进同一 `ROOM_LIFECYCLE` transaction。

有怪与无怪不会选择不同的上述外层 Room::Update callsite。`0x402980` 的正常路径在 `0x403E70` 无条件调用 `0x4186C0` 更新 `Room+0x1218` 的 entity-list；怪物存在只改变该列表的内容和内部迭代。first-visit、revisit、clear、门类型会改变进房初始化细节，但当前只能保守关联到 `0x33FC80` 使用的 descriptor `+0x40/+0x44` bits、root transition fields 及 state `+0x1C`，不能从现有静态证据冻结具体语义。

尤其 `0x345842/0x345856` 不是 monster/clear/visited 分流。前者仅在 `root+0x26584 == 0x1F` 且 `[edi] == 0x0B` 时执行，随后紧邻 `0x34584D Room::Update`；否则走 `0x345856 NewRoom` 且该函数内部没有紧邻 Room::Update。这两个比较字段继续标记 `UNKNOWN`。V5 因此只替换 V4 的两个错误槽，hook 总数仍为 27 个 callsite、2 个 physical-poll vtable slot 和 3 个 USER32 IAT slot；IPC/artifact identity 升为 V5，32-bit 数量不变且 shared mask 继续使用 u64。

### V5 首次实机窗口：安装恢复 PASS、场景证据 FAIL

首次 V5 `room` 尝试 `20260821T054422Z-t0-03r-loop-passive-room-t0-03r-v5-room-ordinary-door-canary-01` 在安装前由 Helper 返回错误码 4。Native mapping 保持 `Empty`，事件数和 hook mask 均为零，32 个目标的 before/after SHA 同为 `5DEBBE254D2C8A94211C2B55309F40078D335058AAE9F6E51A32484408890965`。该 artifact 只记录受限执行环境的 pre-install failure，不是 runtime 证据，也没有发生需要卸载的 hook。

使用新的 case-id 在允许 Helper 获取远程进程权限后，`20260821T054618Z-t0-03r-loop-passive-room-t0-03r-v5-room-ordinary-door-canary-02` 完成十分钟有界窗口。源码重放得到稳定 `FAIL`：664,152 条 artifact/native sequence 连续事件、35,993 个完整 loop、35,993 组 `InputManager → device update → keyboard physical poll` 包络、52,265 条 message-pump 事件；主线程、QPC、event contract、original-once、深度、loop order、UI 分区和 lifecycle 检查均通过。32 槽完整 mask 为 `0x00000000FFFFFFFF`，无 overwrite/failure，before/after SHA 相同且 `HOOKS_RESTORED` 唯一出现。

该窗口的全部 `ROOM_CHANGE_OWNER_*`、`ROOM_CHANGE_SIMULATION_*`、`NEW_ROOM_*`、`ENTITY_LIST_*`、standard room/tail 和 lifecycle simulation 计数均为零；只有 `authority-evidence-nonempty` 与 `scenario-evidence` 失败。事件形状为每轮 top-level update/render、每隔轮一次 `predicate=false → body` 后短路，和此前后台/自动暂停阴性 trace 相同。因此它只说明声明的房间转场没有发生，不能证明也不能证伪 `0x2FB51C/0x2FE37C`。下一次 canary 必须在 READY 后立即让游戏保持前台并实际越过普通门槛，观察到至少一条 `NewRoom` 后再停止；继续使用新 artifact，禁止覆盖或把本次 FAIL 填入 V5 gate。

### V5 普通门 transaction 实机 PASS

第三次 canary `20260821T060539Z-t0-03r-loop-passive-room-t0-03r-v5-room-ordinary-door-canary-03` 在游戏恢复前台并实际穿过普通门后得到可重放 `PASS`。Artifact 有 558,237 条连续事件、30,068 个完整 loop、90,204 条三层 input envelope 事件和 44,732 条 message-pump 事件；32 槽 mask 完整、无 overwrite/failure，停止后 before/after bytes 相同且目标进程内不再存在 T0 Native module。源码重放的 18 项检查全部通过，`eligibleForActiveGate=true`。

唯一完整 room-change transaction 位于 main thread 8292、loop 27012、sequence 499028–499035：

```text
ROOM_CHANGE_OWNER_ENTER          return 0x2FB521
  NEW_ROOM_ENTER/EXIT            return 0x33F5BE
  ROOM_CHANGE_SIMULATION_ENTER   return 0x2FE381
    ENTITY_LIST_ENTER/EXIT       return 0x403E75
  ROOM_CHANGE_SIMULATION_EXIT    original exactly once
ROOM_CHANGE_OWNER_EXIT           original exactly once
```

`0x2FB51C` owner enter/exit 在 artifact 中各出现 9 次，只有上述一次包含 nested simulation；其余 8 次是合法 owner-only attempt。这与状态机可能跨多个 transition frame 的静态契约一致。V5 由此闭合了 V3/V4 普通门缺失 owner：`0x2FB51C → 0x4318A0` 是该场景的外层 owner，`0x2FE37C → 0x402980` 是 NewRoom 后、EntityList 前的 nested Room::Update。该证据只填入 V5 七场景 gate 的 `room` 槽；完整 Authority envelope、其它六场景、稳定 XInput ownership 和 T0-04 raw/apply 仍未证明。

### V5 keyboard foreground PASS

`20260821T064433Z-t0-03r-loop-passive-keyboard-t0-03r-v5-keyboard-foreground-01` 得到可重放 `PASS`：105,441 条连续事件、4,837 个完整 loop、4,837 组 `InputManager → per-device update → keyboard physical poll` 包络，以及 2,419 条 `predicate=false → body → standard room → EntityList → MC_POST_UPDATE` transaction。全部事件属于 main thread 16876；message-pump、phase、深度、return RVA、original-once 和 sequence/QPC 检查通过，32 槽完整恢复且无 overwrite/failure。源码重放的 18 项检查全部通过，`eligibleForActiveGate=true`。

该 artifact 只记录 concrete device kind/key 和 poll bool envelope，不读取键码、button、axis 或 logical action 值；因此它证明 V5 keyboard 场景的 loop/input/Authority 包络，但不升级为 T0-04 raw sample、canonicalization 或 apply 证据。随后对 PID 12124 的只读 `input-rpm-inspect` 仍只返回 `KEYBOARD:UNIQUE_OWNER_MATCH`，active vector 中尚无可接受的 stable XInput owner；controller capture 前必须先让 Gamepad 保持 active 并完成 device key → player assignment 的唯一 join。

### Stable XInput ownership 与 V5 controller PASS

用户以手柄接管 Player 后，单点只读 inspect 首先得到 `GAMEPAD:UNIQUE_OWNER_MATCH` 与 `KEYBOARD:UNKNOWN_NO_PLAYER_MATCH`。随后 `20260821-t0-04-rpm-controller-active-p12124-v1` 在 5 秒内取得 305/305 coherent samples、0 unstable/read-error drop；每个样本均保持同一 ownership 结果。RPM analyzer 的 8 项 diagnostic check 全部通过。该证据解决当前 fixed run 的 stable XInput device key → player assignment join，但仍是 `RPM_DIAGNOSTIC_ONLY`，没有 generation/sequence，也不证明 per-tick/raw/canonical/apply。

在该 ownership 前置条件下，`20260821T065234Z-t0-03r-loop-passive-controller-t0-03r-v5-controller-foreground-01` 得到可重放 `PASS`：117,334 条连续事件、4,606 个完整 loop、键盘与 Gamepad 各 4,606 次 physical poll、2,303 条完整 standard transaction。Gamepad poll 的 device key 为 `1`；全部事件位于 main thread 16876，18/18 analyzer checks PASS，32 槽完整恢复且无 overwrite/failure。该 artifact 填入 V5 `controller` 槽，但不读取 button/axis value；ownership 语义来自独立 RPM artifact，不能由 loop trace 单独外推。

同一 Run 的 Player ownership 自动切换仍未由 loop trace 单独证明。Player assignment 的切换前后由两份独立 RPM diagnostic 固定；runtime analyzer 还要求设置消息分发后的精确双设备直接刷新形状，不能再以“同一 artifact 中看见两类 poll”替代真实切换证据。

### V5 device-switch 设置切换实机形状

切换前 `20260821-t0-04-rpm-device-switch-pre-gamepad-p12124-v1` 与切换后 `20260821-t0-04-rpm-device-switch-post-keyboard-p12124-v1` 各取得 123/123 coherent、0 drop，Player assignment 从 Gamepad unique / Keyboard no-match 严格反转为 Keyboard unique / Gamepad no-match。两者仍是 `RPM_DIAGNOSTIC_ONLY`，不具备 gapless/per-tick 语义。

中间 V5 runtime artifact `20260821T070732Z-t0-03r-loop-passive-device-switch-t0-03r-v5-device-switch-settings-controller-to-keyboard-01` 有 208,371 条连续事件、8,310 个完整 loop 和完整 32 槽恢复。旧 analyzer 的唯一失败是 sequence 74985 的 `INPUT_DEVICE_UPDATE_ENTER` 不在 `INPUT_MANAGER_SAMPLE` 深度内。完整窗口 sequence 74981–74992 实际为：

```text
PEEK_MESSAGE             return 0x65E68C
TRANSLATE_MESSAGE        return 0x65E6D6
DISPATCH_MESSAGE         return 0x65E6E1
PEEK_MESSAGE             return 0x65E6F0
KEYBOARD device update → physical poll → update exit
GAMEPAD  device update → physical poll → update exit
```

所有事件属于 main thread 16876、loop 2984 和同一 active update，两个 device group 的 kind/key 内部一致。这说明设置消息 handler 会绕过常规 InputManager sample，直接刷新 concrete devices。Analyzer 因而只在 `device-switch` 场景接受上述严格连续 12 事件；follow-up RVA 偏一、缺任一设备、identity 不一致或其它场景仍失败。修正后源码重放为 18/18 PASS，但采集时的历史报告保持 `FAIL`，analysis SHA-256 `EECB3B9830B78368E387D2A0A96E26A872AA9937DD9A489293913CE0E651D039`，不进入 gate。用户在该检查点暂时跳过新复测，随后已恢复并完成最终 gate 采集，见下文。

### V5 pause 实机语义与 PASS

首次 pause artifact `20260821T091050Z-t0-03r-loop-passive-pause-t0-03r-v5-pause-bracketed-gameplay-01` 有 4,032 个完整 loop，且每个 loop 都继续出现 InputManager/device/physical-poll；旧的“pause 产生 input-query 缺口”假设因此被实机否定。该 artifact 的历史 `scenario-evidence` FAIL 和 analysis SHA-256 `1C9F1B78032903C6CFE2C54D4492C95D489D5877C49BD5028D541E15B6C33650` 保持不变。

同一 trace 显示暂停窗口的保守被动特征是 steady-standard transaction 连续消失后再次恢复，而 top loop/update/render/physical-poll 持续。新 analyzer 要求缺口前后均为 coherent standard transaction、缺口至少 30 次 conditional attempt、覆盖的每个 top loop 都有 update/render/physical poll，并排除 NewRoom、Run teardown、room-change、room-lifecycle 与 alternate simulation。测试同时覆盖缺 poll、混入 lifecycle 和未夹闭尾部的拒绝路径。

新 artifact `20260821T092837Z-t0-03r-loop-passive-pause-t0-03r-v5-pause-bracketed-standard-gap-02` 直接得到 18/18 PASS：110,700 条连续事件、5,369 个完整 loop、2,685 次 standard attempt、1,854 条完整 standard transaction；两个前后夹闭缺口分别为 loop 3431–4589（580 attempts）和 2305–2805（251 attempts）。Manifest/events/analysis SHA-256 分别为 `319FDA7285D9F00083E15F9F26F9534306DE802A68AAE869252599AB0F4A8707`、`0E9E5FD7DB5188257FCD1314DA7DE5356436CE3279604F1CBA1789DAFBF2F467`、`812D51C963DC565B2605AAF02F8FEEAF4870DB171DA0FAD8C61410EB3C67B1EB`。Hook mask 为 `0x00000000FFFFFFFF`，无 overwrite/failure，before/after bytes SHA 相同。该 artifact 填入 V5 `pause` 槽；随后 menu/loading PASS 在当时将矩阵推进到 6/7，最终 device-switch PASS 再推进到 7/7。

此次还修正了 artifact 不可覆盖契约：采集侧只以 create-new 写 `analysis.json`，CLI `loop-analyze` 改为纯重放且不写源目录。历史 device-switch/pause FAIL 均按采集时原始结果恢复；用新 CLI 重放前后，各自 analysis SHA 保持不变。

### V5 menu 与 loading 实机 PASS

`20260821T151105Z-t0-03r-loop-passive-menu-t0-03r-v5-menu-run-teardown-01` 从 active Run 正常退出到主菜单，得到 54,326 条连续事件、2,795 个完整 loop、885 条完整 steady standard transaction，以及恰好一对 main-thread `RUN_TEARDOWN_ENTER/EXIT`。18/18 analyzer checks PASS，manifest/events/analysis SHA-256 分别为 `B16D303FA0FE9FCCE026E3126FEF79FF4A9FF49E89247C450404764499A91D1C`、`5EEECE7067ACE78C418DC3C92691296AA3C761B1196BBE8611D88C37C79600BC`、`9ED1564007BC2EA91D8920EF506ABEABB2A6DFD17D9549D53463522485F2CD92`。

随后保持同一目标进程存活，从主菜单 Continue 的 `20260821T151234Z-t0-03r-loop-passive-loading-t0-03r-v5-loading-menu-continue-01` 得到 103,189 条连续事件、6,321 个完整 loop、116 条完整 steady standard transaction，以及恰好一对 main-thread `NEW_ROOM_ENTER/EXIT`。18/18 checks PASS，manifest/events/analysis SHA-256 分别为 `51518025A576F4BB1A9F14E7712819F75CFDB522F83160E22A2D67F9B8177CC3`、`A4E4298A2394A0B503EB4FA80AB3810CEAEEBD373F05D9F339C7DC136492E009`、`F4E789881CDC18C16C7441A8BE22C2264FB473346B66B6E1246D91B837EDD56E`。

两次运行的 32 槽 mask 均完整，无 overwrite/failure，before/after bytes 完全一致；只读重放没有改变 analysis 文件。它们分别填入 V5 `menu` 与 `loading` 槽；在该检查点必需矩阵为 6/7，之后由最终 `device-switch` artifact 补齐。

### V5 device-switch 最终 PASS 与 7/7 gate

在最终 gate 实验前，`20260821T151933Z-...keyboard-to-controller-03` 又暴露一个与早期 12 事件 direct-refresh 不同的精确合法变体：四个 message 事件后先执行完整 `InputManager → Keyboard → Gamepad` sample，manager exit 后再次 `Peek(0x65E68C)`，再执行 Keyboard → Gamepad direct refresh，共 23 个连续事件。所有事件仍在同一 main thread/update/loop，manager 与 direct 两段 device identity 一致。Analyzer 现在仅在 `device-switch` 场景接受这两个穷举形状，并继续拒绝错误 follow-up RVA、事件缺失、乱序、identity 不一致和其它场景。该 artifact 的采集时 `analysis.json` 仍保持原始 FAIL，SHA-256 `F9473AD5D8152AC5D5CF68BCF5B348C2E308D358D695C234DD64D940E616945C`；修正后源码重放 18/18 PASS 且没有改写历史文件，因此它仍未进入 gate。

最终前置 RPM `20260821-t0-04-rpm-device-switch-gate-pre-gamepad-active-p12124-v5` 取得 123/123 coherent、0 drop，结果为 `GAMEPAD:UNIQUE_OWNER_MATCH`、`KEYBOARD:UNKNOWN_NO_PLAYER_MATCH`。中间 runtime artifact `20260821T152754Z-t0-03r-loop-passive-device-switch-t0-03r-v5-device-switch-gate-gamepad-to-keyboard-04` 直接得到 18/18 PASS：85,048 条连续事件、3,481 个完整 loop、17,409 条 input-query 包络事件、5,315 条 message-pump 事件、1,104 条完整 standard transaction。精确 12 事件 sequence 62922–62933 位于 loop 2532 的同一 main thread/update 内。Hook mask 为 `0x00000000FFFFFFFF`，无 overwrite/failure，before/after SHA 均为 `5DEBBE254D2C8A94211C2B55309F40078D335058AAE9F6E51A32484408890965`；manifest/events/analysis SHA-256 分别为 `F4B11B584FA56BC956B30ED014B8FF778095BB3A3161E92CF54C7956331B0AEB`、`37EC4976C2B4956187CFBA773C3995B17812B0F05CAAE9313AA774A7BDADC708`、`3A617EA26508F5942A90C2FE1A8866606F5FBC26F0BA703ED79A4B718243A1C3`。

切到键盘后的 RPM `20260821-t0-04-rpm-device-switch-gate-post-keyboard-p12124-v4` 同为 123/123 coherent、0 drop，结果严格反转为 `KEYBOARD:UNIQUE_OWNER_MATCH`、`GAMEPAD:UNKNOWN_NO_PLAYER_MATCH`。前后 RPM 仍是 `RPM_DIAGNOSTIC_ONLY`，只固定 assignment snapshot；runtime trace 才固定 gapless loop-local 形状，三者都不读取 button/axis/raw 值。

七份 V5 source artifact 已写入 `artifacts/cs-t0/20260821-t0-03r-v5-loop-matrix-7of7.json`，matrix SHA-256 `30F2FCE8E5E3684B19967ACFA8CC3A0D21A3ACCE6516911A157FFC71F1CFD05A`。派生 `artifacts/cs-t0/20260821-t0-03r-v5-loop-gate-7of7.json` 状态为 `PASS`、missing scenarios 为空，文件 SHA-256 `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA`。在该历史检查点，gate-local `activeInputAllowed=true` 只代表 loop 前置条件满足，T0-04 仍等待 raw/apply 与主动授权；这些后续门现已闭合。当前状态与可复用结论见 [T0-04 逆向交接](../../../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md)。
