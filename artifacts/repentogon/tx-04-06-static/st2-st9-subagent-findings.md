# tx-04-06 ST2–ST9 子智能体静态确认记录

日期：2026-10-25。本文由子智能体直接复核 ST3–ST7；ST2、ST8、ST9 的部分证据来自主审并行 IDA 取证，并在对应章节明确标出。所有结论均为静态观察或有限推导，不代表运行时结果。

## 取证坐标与边界

**STATIC_CONFIRMED**：IDA MCP 当前模块为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`，`min_ea=0x401000`。IDA 元数据原样输入路径为：

`C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe`

本文地址均为 RVA，VA = RVA + `0x400000`。未读或计算哈希；未修改 IDB 类型、名称、注释、补丁或字节；未启动游戏、构建或测试。

## ST3 — Join pass

**STATIC_CONFIRMED**：Join pass 是 RVA `0x591360`（VA `0x991360`），唯一已确认直接调用点为 `Game::ProcessInput` RVA `0x2F6D21`。函数入口比较 Manager 的 Netplay vector begin/end（`+0x4AFB8/+0x4AFBC`）；不相等时跳过 Join 主体。为空时才遍历 InputManager 设备。每项取设备 `+0x08` 的 id，并与现有 Player 的 controller id、已占用 Join 行 id 比较，避免将已分配 id 再当作未加入设备。

Join 的 action wrapper 语义由 RVA `0x5EE200` 与 `0x5EE140` 的直接实现确认：两者都以 InputManager 为 this、按设备 id 查找或在 id 为 `-1` 时遍历设备；前者调用设备 `+0x24`（slot 9），后者调用设备 `+0x20`（slot 8），以 AL 返回命中。Join 在 RVA `0x591360` 中以 slot 9 的触发查询检测 action 19（加入）、20/21（角色选择方向）、14（确认）和 15（取消）；特殊 possessor 分支还要求 slot 8 的按下查询 action 8 与 10 同时为真。不能把这些调用都称作同一种“按键状态”。

状态逻辑把设备 id 放进固定 Join 行；行步长为 `0x14`，RVA `0x591E53` 从 `[EDI-8]` 取 controller id，EDI 指向该行 `+0x14` 的 selected index，`[EDI-4]` 是当前角色值。确认/取消以及方向动作仅在相应行有效、选人模式允许、设备 action 命中时推进；`Game+0x25AC0 & 0x10` 的 mode predicate 还会在选人分支短路或改道。Join pass 不保证每个显示帧调用一次，也不保证外部处于 menu 时会运行。

**DESIGN / 最小报告序列**：针对单个 K id，先在一次可进入 Join 主体的 pass 令 action 19 触发；在后续可进入的 pass 用 action 14 确认当前有效角色。action 20/21 只在需要选择其他角色时加入。按不同 pass 报告是稳定的可复核序列；静态控制流不能证明至少需要两个 pass——若同一次 pass 内 action 19 与 action 14 都可观察为真，后续行循环是否立即确认取决于进入时状态。也没有静态依据给出固定帧数。

**UNKNOWN**：除已确认 Netplay 非空门、设备分配检查、选人行与分支条件外，没有穷尽所有游戏状态、所有提前返回的语义，也没有运行时观察验证外部调用频率、设备 id 状态或 K 的实际参与行为。

## ST4 — 三处 mode predicate 补丁调用

目标函数 RVA `0x5910A0`（VA `0x9910A0`）读取 `Game+0x25AC0` 并测试 bit `0x10`，AL 返回 0/1，函数以普通 `ret` 返回；无 stack 参数。汇编路径用 EAX/AL，未用 ECX/EDX/EBX/ESI/EDI 作输出。三个原调用点均为完整 5-byte `E8 rel32`，可搬移长度按原始指令边界为 5；调用后 test 重设 flags，再由紧随的分支读取 ZF：

| 调用 RVA | 原始指令及紧随指令 | 调用上下文 | 返回后控制流 |
| --- | --- | --- | --- |
| `0x5915AE` | `E8 ED FA FF FF` `call 0x5910A0`；`0x5915B3 test al,al` | EBX=PlayerManager，ESI=key | `0x5915B5 jz` |
| `0x59174C` | `E8 4F F9 FF FF` `call 0x5910A0`；`0x591751 test al,al` | EBX=PlayerManager，ESI=key | `0x591753 jz` |
| `0x591E67` | `E8 34 F2 FF FF` `call 0x5910A0`；`0x591E6C test al,al` | EBX=PlayerManager，EDI=Join 行/selected index 地址，ECX=`[EDI-8]` 的设备 id | `0x591E6E jnz` |

**STATIC_INFERENCE**：改写这三处时，替代调用要保留 EAX/AL 的真假约定；后续 `test al,al` 会重新建立 flags，因此分支读取的是 test 结果，而不是被调用函数留下的 flags。寄存器上下文与后续数据使用以调用点汇编为准，不以旧原型寄存器布局代替。

## ST5 — 角色选择值与 Spawn 参数

角色选择行用 EDI 指向 `row+0x14` 的 selected index；行的 `[EDI-8]` 是 controller id，`[EDI-4]` 是所选 PlayerType 值。RVA `0x591E87`/`0x591F03` 分别更新 index 并从 PlayerManager `+0x70/+0x74` 指向的 int32 数组读取 Type 值，再写回 `[EDI-4]`。因此方案所问的“selected index 是否继续索引 `+0x70/+0x74`”答案为是；这里观察到的是 dword 读取，元素按 int32 PlayerType 解释与其下游 Spawn 参数用法一致。

创建处 RVA `0x591FA9` 先读 `[EDI-4]`。若值不为 `-1`，直接将它作为角色参数；只有值为 `-1` 才执行随机选择分支。随机分支根据 `(end-begin)/4 - 1` 取模，并从 begin 后一个 dword 读取；不要把随机分支外推为所有角色都经随机表生成。

RVA `0x591E53..0x591FE6` 的连续原始指令中，Spawn 前只有一条未被清理的显式参数压栈：`0x591FDF push eax`；此 EAX 是前述 selected 角色或随机 fallback 角色。`0x591FE0` 令 ECX=`PlayerManager`，`0x591FE6` 调 Spawn RVA `0x590250`。因此原生 call 入口 `[ESP]` 是 CALL 指令压入的返回地址，`[ESP+4]` 是唯一 role 参数；这里没有第二个显式 Spawn 参数。

Spawn 的 cap-reject 路径 RVA `0x590392` 和正常尾部 `0x5904B8` 均为 `retn 4`。此证据也解释原型 CALL-shim 方案的栈清理：CALL 进入 shim 时 `[ESP]` 是 shim 返回地址、`[ESP+4]` 是 role；拒绝并直接跳出该 call site 时 `add esp,8` 清掉这两项，正确反映的是 CALL-shim 栈形状，不是两个工厂参数。若入口改为 JMP，则不会有新返回地址，拒绝路径只应清 role 的 4 字节。

## ST6 — RemoveCoPlayer ABI 与副作用

**STATIC_CONFIRMED**：目标为 RVA `0x590560`（VA `0x990560`），主审另以官方 `PlayerManager.zhl` 的 `RemoveCoPlayer` 特征独立匹配。函数汇编尾部为 `retn 8`；Join 调用点 RVA `0x591E03` 在 ECX 传 PlayerManager，先压入两个 dword（`0x591DFF push ecx`、`0x591E00 push eax`），调用返回值未使用。IDB 形参名 `a3/n3` 不足以确定第二个参数的业务含义；本结论只确认 thiscall 与两个栈 dword。

函数路径可见以下清理：读取 Player controller id `+0x160C`，清理控制器颜色分配；遍历相关实体/熟悉物并按分支调用实体删除或处理 helper；清理/重置所属的绑定或实体 controller id；在 PlayerManager vector 中移除 Player 并压紧、重编号其余 Player 的 `+0x1610`；重置 Join 行状态。普通路径在 RVA `0x590D0A` 执行 `call [eax+0x28]`，即虚调用 Entity vtable `+0x28`；主审将该目标直接核为 RVA `0x2A8000` 的 `Entity::Remove`，不是虚析构。RVA `0x2A8000→0x44CA50` 另会分派 callback id 67；这是 RemoveCoPlayer 的实体移除/回调副作用，不是 InputManager RemoveDevice 的 disconnect callback。特殊分支若出现析构调用应单列，不能混同普通路径。函数还包含基于 Player Variant 等条件的分支，存在资源/效果处理。`Type=1, Variant=0` 的普通 K 不应被套用到不同 Variant 分支的效果。

**UNKNOWN**：这次静态读取不能证明所有分支中掉落物、实体 helper 的完整语义，不能断言普通 K 移除必定产生或绝不会产生某类掉落。主审报告应把“存在分支相关的游戏实体/资源副作用”与“普通 K 的具体运行时效果”分开表述。已确认设备 slot 2/0 的 ABI 也不能推导 RemoveCoPlayer 一定会调用 InputManager 移除设备；Player 删除与 InputDevice 删除是不同路径。

## ST7 — Spawn 返回前后的字段写入顺序

**STATIC_CONFIRMED**：Player 构造函数 RVA `0x34DC70` 初始化 controller index `+0x160C`、PlayerIndex `+0x1610` 及 auxiliary index `+0x1614` 为 `-1`。普通 Spawn RVA `0x5903C7` 在 Player append 前按 PlayerManager vector 当前元素个数写 `+0x1610`；空集为 0，已有 A 后创建 B 为 1。该规则是 vector count，不是设备 id 或随机角色索引。

随后 Spawn 通过 Player vtable slot 1（RVA `0x590479`）调用 Init，参数路径为 Type=1、Variant=0、SubType=selected role、seed。Player::Init RVA `0x34F6A0` 在返回前调用初始化 helper RVA `0x2A4A20`（VA `0x6A4A20`）；helper 开始处写 `this+0x28/+0x2C/+0x30`，所以 Type/Variant/SubType 在 Spawn 返回前已设置。其后 Init 经 controller setter RVA `0x39E1B0`（VA `0x79E1B0`）写 `+0x160C`：普通 Type1/Variant0 路径先使用全局默认 controller id；Variant 或 SubType 特殊条件可能给 `-1`。

**STATIC_CONFIRMED**：Join 在 Spawn 返回后继续执行 RVA `0x591FEB..0x591FFB`，将返回 Player 放入 ECX，压入设备 id `[EDI-8]` 与 flags `1` 后调用 setter RVA `0x39E1B0`。因此 K 的指定 controller id 是 Spawn 返回后被 Join 覆盖写入；不能要求在 Spawn 返回瞬间它已等于 K id。拒绝路径没有 Player 可返回或设置。

## ST2、ST8、ST9 — 主审并行证据（本子任务未重新审计）

以下事实来自主审并行只读 IDA 导出 `st2-st9-primary-ida.json` 与其报告，不冒充本子任务独立确认：

- **ST2**：InputManager vtable 为 0–46 共 47 槽；第 45 槽 `0x5EEB40` 注册、第 46 槽 `0x5EEBB0` 移除。注册会建立 +14/+18 回调路径，创建/载入按名称关联的 controller mapping profile；Manager 初始化 RVA `0x52C341` 直接 call RVA `0x4E3FC0`（IDA 名为 `___std_tzdb...`，以 call rel32 实际目标为准），所以原版断连回调不是用户进入联机后才安装。InputManager 移除先调用 Manager `+0x20/+0x24` disconnect callbacks；官方初始化回调目标 `0x4EE980` 会触及 Lobby/Netplay 对象，故 C/S 直接调用 RemoveDevice 并非无副作用隔离路径。run/menu 生命周期中的全部保留时序仍需以已追踪具体调用链为界，不能只凭未看到 teardown 就断言永存。
- **ST8**：Game::Exit RVA `0x2F6270` 在 `0x2F635E` 检查 PGD dirty、`0x2F6363` 检查 fileLoaded，随后先在 `0x2F636E` 清 dirty，再于 `0x2F639A/0x2F63A3` SaveCloud/SaveLocal；尾部 `0x2F647B` 复位 readonly。因此退出后 dirty=0 不能证明操作窗口内没有存档写入。输入 profile 路径在 `0x42A520` 创建、官方保存特征匹配 `0x42ACD5`、owner `0x42AC90`；该 profile 可进入配置序列化路径。正常 Init 尾链 `0x351A9F→0x44A7E0` 与若干 PGD sink 也已列在主审原始证据中；静态图未展示的虚调用不能据此断言无持久副作用。
- **ST9**：菜单 ControllerSelect RVA `0x4AF870` 从 InputManager 枚举设备并在 `0x4AF9AB` 调 slot 3 取名称、构造名称/id 行；任意设备的物理 action14 在 `0x4AFAED` 可把当前高亮行 id 写入全局 VA `0xB88A14`（RVA `0x788A14`）。我最初误把 IDA 自动符号 `ArgList_0` 当作栈局部；主审指出该符号可能误导后，我用原始字节 `A3 14 8A B8 00` 和 IDA operand type `o_mem(2)` / value `0xB88A14` 直接更正为绝对全局写入。故 slot26/27=false 不能阻止物理设备确认当前高亮 K。
- **ST9 风险进一步限制**：Pause/断开处理 owner RVA `0x5346B0` 入口枚举所有设备，直接读取 `device+0x48C` 与 `device+0x498`，之后查询 slot32/action14；这两个偏移超出基类 `0x3F8`。只在 ControllerSelect UI 过滤 K 仍不足以排除此路径。设计必须在通用非 gameplay / 断开处理设备枚举中过滤 K，或提供经证实安全的兼容布局；不能把 K 伪装成 Gamepad，也不能宣称 13 槽中性覆写已经消除全部 UI 风险。

## 纠错记录与 UNKNOWN 范围

1. 本子任务第一轮把 VA `0x990560`、`0x992650` 错当 RVA 请求 IDA，得到 `NO_FUNCTION`；随后根据 imagebase `0x400000` 改用 RVA `0x590560`、`0x592650` 重导到 `st2-st9-subagent-remove-raw.json`。旧错误请求保留在 `st2-st9-subagent-join-create-raw.json`，不用于函数地址结论。
2. 初始解读曾把 Spawn 入口 `add esp,8` 误解为两个显式 Spawn 参数。沿 RVA `0x591E53..0x591FE6` 的连续汇编复核后，确认只有一条显式 role push；CALL-shim 的 +8 是返回地址和 role 两项，JMP-shim 则只清 role。
3. ST9 的 `ArgList_0` 自动符号曾被误读为局部变量；原字节和 operand type/value 证明它是 RVA `0x788A14` 全局写入，以上结论已更正。
4. Join、Remove、Spawn 的 IDA 伪码存在错误/粗糙的自动形参名；如与原始汇编不符，以原始指令、参数准备和 `ret/retn` 为准。初稿曾把 RemoveCoPlayer `RVA 0x2A8000→0x44CA50` 的实体移除 callback id 67 错放进 ST2 RemoveDevice 路径；主审复核后已移到 ST6，ST2 仅保留 Manager disconnect callbacks。ST6 辅助实体/资源 helper 语义未完整反编译；ST2 teardown 的全部保留/重建状态、ST8 隐式虚调用写入、ST9 未分类非 gameplay 设备遍历均为 **UNKNOWN**。

## 子任务原始证据

- [Join/Spawn/selected helper/错误请求首轮导出](st2-st9-subagent-evidence/st2-st9-subagent-join-create-raw.json)：含 RVA `0x591360`、`0x590250`、`0x5910A0` 的伪码、指令和 xref；含后来修正的地址失败记录。
- [RemoveCoPlayer 与 helper 重导](st2-st9-subagent-evidence/st2-st9-subagent-remove-raw.json)：按正确 RVA 读取 `0x590560`、`0x592650` 等目标。
- [Join 调用点原始指令与交叉引用](st2-st9-subagent-evidence/st2-st9-subagent-callsite-raw.json)：mode、选择、Spawn/Remove 调用点指令字节。
- [Player ctor/Init/controller setter/Spawn 导出](st2-st9-subagent-evidence/st2-st9-subagent-spawn-raw.json)。
- [InputManager action wrapper 与 mode predicate 原始指令](st2-st9-subagent-evidence/st2-st9-subagent-wrappers-mode-raw.json)：直接从 IDA MCP 只读导出，包含原始 imagebase、min_ea、函数指令、三处 mode 调用点。
- 主审并行证据：`st2-st9-primary-ida.json`、`st2-st9-primary-callpaths.json`、`st2-st9-primary-source-evidence.txt`。

