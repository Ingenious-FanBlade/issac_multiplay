# tx-04-05 静态确认与主智能体审阅

日期：2026-10-24。范围：[实现方案](../design/tx-04-05-implementation-plan.md)第 10 节 T1–T8。按用户要求，由 GPT-6 Luna / max 子智能体逆向，主智能体读取原始指令、官方源码与原型后审阅；主审也补充了独立 IDA 取证。

本次完成静态分析及方案修订，没有实现功能、构建、启动游戏、注入或执行测试。tx-04-05 整体仍待实现，唯一一次整体 E2E 尚未执行。没有计算/校验哈希，没有新增项目 gate。

## 1. 证据范围与结论口径

- 目标是 v1.9.7.12.J273 的脱壳副本；本轮 IDB 的 imagebase 为 `0x400000`。本文地址均为 **RVA**，IDA 中的 VA 要加该基址。
- 官方源码以本地 `F:\projects_isaac\REPENTOGON-1.1.3` 为准。旧 `D:\projects\REPENTOGON` 和 tx-04-03 的源码版本只保留历史语境。
- `STATIC_CONFIRMED` 表示所列二进制控制流/指令；`SOURCE` 表示官方源码或原型的具体行为；`DESIGN` 表示根据这些事实选择的实现办法。均不等于运行时观察。
- [原始证据目录](../../../artifacts/repentogon/tx-04-05-static/README.md)保存子智能体导出、结论及更正记录、主智能体[独立 IDA 证据](../../../artifacts/repentogon/tx-04-05-static/primary-ida-evidence.txt)和[源码片段](../../../artifacts/repentogon/tx-04-05-static/source-evidence.txt)。Hex-Rays 的错误类型、`None` 或 bad-SP 警告不用于确定 ABI；关键判断回到汇编。

主审结论：采纳经修正的 T1–T4、T7、T8 事实；T5 由主审补齐 Windows 事件来源；T6 按具体路径保留已证明的前后关系，不采纳“回调即所有玩家最终定位完成”的推断。候选文件误配、宏搜索遗漏和草稿中的错误归属均保留更正记录。

| 项目 | 静态结论 | 实现落点 |
| --- | --- | --- |
| T1 | Odd helper 有三处 Player 更新，只有 `0x2F9559` 是普通模拟点 | 使用该调用点 shim 标记，不能只看 Odd owner |
| T2 | controllerIndex 对应设备 id，原生 lookup 比较 `device+0x08` | 唯一 id 匹配；不按 vector 下标绑定 |
| T3 | action 查询不消费边沿；Poll 之后还有方向派生 | 保存原 query；设备公共更新成功返回后采样 |
| T4 | 菜单输入 Hook 可以短路，内层观察可能缺席 | 使用控制台原调用区间；不伪造设备差值 |
| T5 | Console、PauseScreen、失焦有不同分支，均可阻断普通模拟 | 分开记原因；实际普通入口决定 tick |
| T6 | 已对齐切房 API 的 animation→过渡对象 `+0x1C`；特殊分支确有回调后 Player 定位 | 普通场景显式选 animation=0；回调只失效绑定，不证明最终位置或可立即 apply |
| T7 | `+0x162C` 为 byte；`+0x1618` 为方向 int；速度为两个 float | 按真实宽度只读观测 |
| T8 | 移动 Scalar、射击 Pressed、炸弹 Triggered 走三条不同查询链 | 三种 query 均保留；边沿按 tick 唯一，重复查询幂等 |

## 2. T1：普通 Odd 调用点与 Hook 边界

`STATIC_CONFIRMED`：helper `0x2F9340–0x2F9708` 内，Player 的 vtable `+0x0C` 有三处调用：

| 调用点 | 返回点 | 路径 |
| --- | --- | --- |
| `0x2F93F9` | `0x2F93FC` | `Game+0x1B83C` 对应的特殊状态分支 |
| `0x2F949C` | `0x2F949F` | `Game+0x1BA78` 对应的另一特殊状态分支 |
| `0x2F9559` | `0x2F955C` | 控制台、综合停止条件和失焦检查之后的普通循环 |

普通循环的 `EBX=[Game+0x18300]` 是 **Room**，其 `+0x125C/+0x1264` 为实体列表及数量；不是 `Game+0x1BAA8` 的 PlayerManager。循环检查实体 `+0x28==1`，设置 `+0x168` 的 `0x4000` 位，再执行 Player 更新，随后调用碰撞相关处理。其他实体可走 vtable `+0x10`，也不能据此开始 Player tick。

`DESIGN`：普通点采用方式 4。`0x2F9559` 的 `FF 52 0C`（虚调用，3 字节）和 `0x2F955C` 的 `6A 00`（push 0，2 字节）合计 5 字节；候选 shim 搬移这两个完整指令，回到 `0x2F955E`。必须保留虚调用 exactly-once、后续调用的 push 0、ECX/EDX 与所需寄存器/浮点状态。shim 只建立 `OddNormal` 上下文，Authority-pre 仍在唯一 A Player 更新边界执行，previous 仍在 helper 正常返回后提交。

异常清理由外层 Odd/Player RAII 负责，使标记不能逃到下一次调用；正常/未 armed 路径照常调用原虚目标。该段尚未实现或安装，不能把静态指令预算等同于已验证的机器码。

`SOURCE`：`libzhl/HookSystem.cpp:576–666` 在原生入口与 C++ wrapper 之间另发出 `call`；普通 `_ReturnAddress()` 得到的是 ZHL 中间桩的返回点，不能当成原生 `0x2F955C`。不采用猜测 ZHL 桩帧偏移的办法。

REPENTOGON 的 `CustomCallbacks.cpp:523–534` 经宏生成真实的 Player Update Hook（callback 1160）。因此外层 Player Hook 放 `-10000`；另用同一定义的 `+10000` 内层 Hook 记录原生 body 到达。消费者为零也可能只是原生条件分支，不能独自证明 Lua 跳过。

重叠结论限定：旧 tx-04-03 已定位的 ASM 特征范围未覆盖这个五字节候选；本轮源码检索未发现针对它的显式补丁。这个范围说明不证明未知插件/动态补丁不存在。实现时仍按当前常驻补丁框架处理实际字节不符；不恢复目标字节。

## 3. T2：A 与物理设备的唯一关联

`STATIC_CONFIRMED`：InputManager 查询在 controller 参数不是 `-1` 时调用 `0x5EEC40`。该函数在 vector `manager+4` 到 `manager+8` 内逐项取 `device*`，在 `0x5EEC66` 比较 `[device+8]` 与传入 id；无匹配则返回空。`+0x0C` 是 vector capacity 终点，不是额外设备。该函数自身也包含原生锁调用，不能把它说成裸数组读取函数。

`DESIGN`：拓扑采样在已理解的主线程边界完成。A 仍取 PlayerManager 列表第 0 项，并确认 `_playerIndex==0`；用 `_controllerIndex` 唯一匹配设备 id，同时确认已支持的 Keyboard/Gamepad 类型。`-1` 表示“查询任意设备”，不具备 exact-pair 身份。重复 id、设备缺失、类型未知或对象关系变化均先中性，不选择第一个候选继续 apply。

死亡/复活还要看对象生命周期状态；同一 Player 指针保留不证明仍是同一有效绑定 generation。Player/device/id 仅作本地临时关联，不加入 canonical 网络身份。

## 4. T3：查询纯度与最终采样位置

`STATIC_CONFIRMED`：三槽分别为 `0x5ECD80`（Pressed）、`0x5ECDE0`（Triggered）、`0x5ECE50`（Scalar）。它们检查 `device+0x0D` 和 action 范围 `+0x18`，经 `+0x1C` 的映射表查询按钮，并合并 action float 状态：

- current/previous 指针为 `+0xC8/+0xCC`，元素为 float；阈值位于 `+0xD4`。
- Triggered 检查按钮上升沿或 `current > threshold && previous <= threshold`，不写回/清除这些值。
- Scalar 合并映射按钮的值和 current；只读 `+0xC8` 数组会漏掉映射按钮，不能替代完整 action 查询。
- Keyboard 下层 Pressed/Triggered 为 `0x633CA0/0x633CD0`，直接读取键盘 current `0x7E6B08` 与 previous `0x7E6DC8`。原先“状态源为推断”的条目在该路径上已有指令依据。
- Gamepad 下层 `0x5ECB80/0x5ECC40`、Scalar `0x632FF0` 及 fallback `0x5ECD00` 不消费设备状态；其 tagged-button 分支的 `0x5ED190` 只填调用方输出缓冲。不是对任意未知 vtable 的纯度保证。

`STATIC_CONFIRMED`：公共更新 `0x5EC900` 在 Poll 前复制 current→previous 并清空 current；`0x5EC98A` 调 Poll。Poll 返回后，`0x5EC9A3–0x5ECA50` 还会查询摇杆并生成方向位。因此 **采样放在公共更新成功、正常返回之后**，且需要同一 Manager/InputManager/device scope 内本次 Poll 成功的依据。Poll wrapper 不提交 input，也不改 Authority 快照。

保存原始三槽函数用于采样，wrapper 用 Resolve 覆写消费查询。Scalar 的设备返回 ABI 是 x87 float；不能照搬高层 Manager Scalar 的 XMM0 返回 ABI。

## 5. T4：屏蔽的可观察范围

`STATIC_CONFIRMED`：InputManager 的 Pressed `0x5EE140`、Triggered `0x5EE200` 分别调用设备第 8/9 槽；Scalar 真正入口为 `0x5EE2E0`，调用第 10 槽。指定 id 的查询经 T2 lookup，`-1` 才走全部设备分支。

`SOURCE`：`LuaMenuManager.cpp:191–207` 使用 `mask && super()`；`ModsMenuStuff.cpp:130–135` 在搜索时直接返回 false。内层 Hook 因而可能根本未调用。不能记录成“设备返回 false”，也不能为了补齐对照再调一次原生查询。

`SOURCE`：ImGui 的 `windowProc_hook` 释放已跟踪的按键，然后把 Console state 设为 2；关闭时清回 0。这是浮层的暂停来源，与主菜单 mask 不是同一个机制。首版记录既有控制台原调用返回区间，PauseScreen/加载另记原因。两个用于设备差值比较的额外 InputManager Hook 不纳入本轮方案。

## 6. T5：控制台、暂停菜单与失焦

`STATIC_CONFIRMED`：

1. Console 方法 `0x287020` 会更新消息/动画状态，再返回控制台是否阻断。Game 在 `0x2F6D69` 调它，真时跳到 `0x2F7A39`；Odd 在 `0x2F9358` 调它，真时跳到 helper 尾部。不能额外补调这个带写入的方法。
2. 暂停菜单使用 `Game+0x2363C`。Game 在 `0x2F775B` 调 `0x58ED50`，真时 `0x2F7762 → 0x2F7A39`，不执行 `0x2F78DF` 的普通 Room 更新。Odd 的 `0x2F92A0` 综合停止谓词也检查同一 PauseScreen；两处早期特殊 Player 更新依然不能凭外层 Odd 计 tick。
3. SDK `Manager::_optionsConfig=+0x2A178`，`OptionsConfig::_enablePauseOnFocusLost=+0x5F`，即 Manager `+0x2A1D7`。Game 的 `0x2F76C9–0x2F76F8` 在焦点字节 `+0x4AFAA` 为 0、该选项为真、原生相关容器为空且 PauseScreen state=0 时，调用 `0x58ED80` 打开暂停；该函数在 `0x58EE29` 写 state=1。随后走上述暂停检查。
4. Odd 的 `0x2F94FC–0x2F9518` 对同一焦点/选项条件提前返回，不进入普通 Player 循环。焦点 setter `0x581AC0` 将回调参数是否非零写入 Manager `+0x4AFAA`；它在 `0x5821E2/0x582206` 登记，焦点分发函数 `0x5E4910` 向该回调列表传入布尔值。
5. 主审继续补齐了 Windows 来源：窗口初始化 `0x5E4C8C` 把 `0x5E4910` 安装到窗口对象 `+0x1F4`；WndProc `0x629010` 处理消息 6（WM_ACTIVATE），由 LOWORD(wParam) 与最小化标志 HIWORD(wParam) 决定有效激活状态，`0x62912D` 调该回调并传入真，`0x629194` 在失去激活时传入 0。至此消息→回调→Manager 焦点字节→Game/Odd 失焦暂停分支的静态链已有依据。子智能体草稿保留的“Windows 事件入口未闭合”被这份后续主审证据补充；实际消息、焦点值和运行窗口仍待唯一 E2E 观察。

上述原生判断中出现原版容器，只用于解释既有原生控制流。C/S 不读取/复用这些容器决定自己的 session、输入或队列；继续使用实际普通调用点与自身上下文。

`DESIGN`：控制台为假不等于未暂停，暂停菜单也不必使控制台返回真。分别记录 Console/PauseScreen/focus/loading，恢复时刷新输入历史与 generation。没有普通入口的轮次不推进 clock。不能单凭顶层 wrapper 次数或墙钟断言 tick。

## 7. T6：new-room 回调与重绑时序

`STATIC_CONFIRMED`：`0x44B930` 的唯一调用点是大型房间加载函数 `0x3385F0` 内的 `0x339719`。回调前 `0x3396D0–0x339717` 对新 Room 实体列表做条件更新，其中可调用实体 vtable `+0x0C`。原生恢复/初始化和特殊更新不能当成 Authority tick。

主审独立 xref 看到 `0x3385F0` 的五处调用，分别在三个 owner：`0x339DE0` 的 `0x33A739`，`0x33EB10` 的 `0x33FA2B/0x33FA3F`，`0x3415E0` 的 `0x341A45/0x341AA3`。回调 owner 返回后仍有原生工作；例如门切房包装 `0x2FA0F0` 在调用该链后又于 `0x2FA17C` 更新 Room。故回调不是“全部玩家位置已经最终稳定”的全路径契约。

主审还追到一条确定的回调前位置写入链：`0x3395A7` 检查 `Game+0x25B8C < 2` 时，`0x3395B6` 调 PlayerManager `0x592A80`；它在 `0x592C8D` 逐个调用 Player `0x39EA20`。后者在 `0x39ED5A/0x39ED66` 写 Player `+0x33C/+0x340`，随后 `0x39ED73` 清速度；PlayerManager 的 `0x592EEB/0x592EF3` 还可能按相对偏移调整其他 Player 位置。该**有条件路径**在 `0x339719` 回调之前，不能扩大为普通门切房、恢复等全部路径的最终位置契约。这个补充比子智能体草稿中“尚未找到位置写入”的范围更具体，旧草稿保留。

`STATIC_CONFIRMED`：主审对齐了公开 API 的完整参数链。Lua 注册 `0x47AD08/0x47AD0E` 将原生 `Game::StartRoomTransition=0x2F9710` 交给桥接函数 `0x49EEB0`；桥接按 Lua 第 2–6 个栈参数读取 room、direction、animation、player、dimension。正常发起分支在 `0x2F9AD4` 调 `0x423990`，ECX 为 `Game+0x1B83C` 的 RoomTransition，第三个栈实参为 animation。setter 在 `0x4239D5` 读取该实参，在 `0x423B05` 写 `this+0x1C`。因此该字段是本链的 animation/type，和官方 `RoomTransition::_mode=+0` 分开。IDA 将 setter 误命名为 std stacktrace 函数，其名称和 bad-SP 伪代码签名不能用于判断真实功能或 ABI。

| animation / `this+0x1C` | 回调/加载 owner 返回后的定位事实 |
| --- | --- |
| `0` | `RoomTransition::ChangeRoom=0x4254B0` 本体的该分支没有直接 Player 坐标 store；DoorNestedOwner 仍在回调后调用 Room update，不能排除间接改变 Player 位置 |
| `11` | 从 PlayerManager 列表取得 Player，并在 `0x425B75/0x425B7F` 直接写其坐标 |
| `20` | 维度 2 条件路径从 PlayerManager 列表取得 Player，在 `0x425E8A/0x425E92` 写坐标；之后 `0x425F26/0x425F2E` 的另一批写入来自 type=3、variant=238 的实体查询，不是 Player 列表 |
| `3` | 条件路径在 `0x42601A` 对选定 Player 调坐标 helper `0x3C3C40`，该 helper 有直接坐标 store；之后 `0x426144/0x42614C` 的批量写入同样来自 type=3、variant=238 查询，不能称为批量 Player 定位 |

`RoomTransition::Update=0x426430` 本体没有直接 `+0x33C/+0x340` 访问，这只描述本体，不穷举它的间接调用。子智能体补充中把两批 type=3 实体写入归给 PlayerManager 的说法由主审撤回，更正与原始推导分别保存在 [T6 补充](../../../artifacts/repentogon/tx-04-05-static/T6-supplement.md)和[更正记录](../../../artifacts/repentogon/tx-04-05-static/candidacy-corrections.md)。

`DESIGN`：S9 测试夹具显式传全参数 `Game():StartRoomTransition(targetRoom, direction, 0, nil, -1)`，使正常发起的过渡选择 animation=0；setter 将负 dimension 取为当前维度。原生仍可因状态拒绝过渡，或在 animation=0 的特定目标判断下将 room 改为 -16，因此记录请求值与实际房间身份，不把函数调用本身当成切房成功。这里没有调用 API 或运行夹具。

`UNKNOWN`：尚无证据把所有出生、门切房、恢复与特殊房间路径的最终 Player 位置写入统一归到回调前或后。报告不以函数/回调命名补造这个契约；本轮实现不依赖它。

`DESIGN`：回调正常返回只做失效标记，撤销当前 A 绑定、清旧输入，不在回调内重绑或 apply。其所在 Manager 轮次内继续中性；至少等该轮 owner 退出、后续 Manager 轮次出现无阻断的普通 Authority 入口，再以新房间身份、唯一 exact pair 和该轮新鲜采样重绑。实际 loading 状态未退出时继续中性。这样即使后续原生重新定位，也不会用旧 generation 修改中途对象。

`ExecuteStartGame` 每 Manager 帧都调用；Door/FSM 的周期性 wrapper 进入也不证明发生加载。上下文分类与实际生命周期事件必须分开，否则每帧失效会清空 lead=2 队列。

## 8. T7：观测字段的宽度与含义

| 字段 | 直接依据 | 允许的描述 |
| --- | --- | --- |
| Player `+0x1618` | 官方 `_moveDirection` 为 int；移动 consumer `0x3728D5` 写 -1，随后按分量写方向值 | 移动方向枚举；不能按 float 或 Vector 读 |
| Entity `+0x360/+0x364` | 官方 `_velocity` 为 Vector；移动 consumer `0x3725B3` 等有 `movss` 写入 | 速度 x/y，只读且检查有限值 |
| Player `+0x1624/+0x1628` | 射击 consumer `0x3794B0/0x3794B8` 使用 `movss` 写入 | 两个射击相关 float 分量 |
| Player `+0x162C` | `0x3794C0 mov [edi+162Ch],al`；同函数局部 flag 由清零/置 1 分支产生 | 一字节射击标志，Q16 地址映射得到实证；不外推为方向 float 或完整蓄力状态 |

当前字段只在有效对象/generation 和对应正常更新后观测，读到相同数值不证明构造历史或所有武器的状态都相同。

## 9. T8：consumer 的三类查询

高层原生包装为 Pressed `0x52F610`、Triggered `0x52F6D0`、Scalar `0x52F7B0`；它们还有 Lua 输入回调分支。指定 controller 的正常 fallback 分别进入 InputManager `0x5EE140/0x5EE200/0x5EE2E0`，最后到设备第 8/9/10 槽。本项目不新增 gameplay 输入回调；首版不支持第三方 gameplay mod。不能据设备层覆盖声称已覆盖任意 Lua 输入重写。

| consumer | 已确认的查询与 action | 关键证据 |
| --- | --- | --- |
| 移动 | Scalar 0/1/2/3；特殊条件读 Pressed 11、Triggered 0–3 | `0x371D70 → 0x52F7B0` 分别取左右上下；`0x37388A` 的 action 表 `0x741020=[0,1,2,3]` |
| 射击 | Pressed 4–7；特殊情形读 0–3；Triggered 11 及射击方向 4–7 的特殊分支 | `0x373C2D–0x373D16`、`0x374514`；`0x376013` 的 action 表 `0x741120=[4,6,5,7]` |
| 炸弹 | Triggered 8 | `0x37A291/0x37A37B`；两处属于条件路径，不能硬编码总查询次数为 1 |
| 主动道具 | Pressed/Triggered 9 或 10；Pressed 11 作为辅助动作 | `0x389F3A/0x389F64` 及相邻分支 |
| 卡牌/药丸 | Triggered 9 或 10；Pressed 11 | `0x39BE9F`、`0x39BEC3` 等 |
| 丢弃 | Triggered/Pressed 11 | `0x39C359/0x39C5D4` |

0–3 为移动左右上下，4–7 为射击左右上下，8 炸弹，9 主动道具，10 卡牌/药丸，11 丢弃。9/10 的部分特殊分支会互选，consumer allowlist 必须保留原型已有的交叉辅助查询；不能只支持 consumer 的单个同名 action。

`DESIGN`：查询只读 Resolve。一个 edge 只进入一个 sequence/目标 tick，同一 tick 重复查询仍返回同一结果；exactly-once 用队列归属与实际炸弹效果验证，不靠“原生恰好只问一次”的假设。

## 10. 主审的额外修正与余项

主审除复核 T1–T8 外，还从原型 `Participant::Begin` 确认：它只取当前 target tick 的命令，没有命令立即中性。原方案“值不变且没有 edge 就不再提交”会让持续 held 断续，已改为每个有本轮新鲜采样的 Authority-pre 生成相应 target 的命令，只对同一轮重复发布去重。`sample` 日志仍可按变化降采样。

屏蔽时继续更新物理 held 历史，但不缓存待重放 edge；恢复时要求新轮次、新鲜样本。候选物理采样与可 apply 的绑定分开，Warmup/重绑前可收集唯一候选的新鲜样本，必须重新确认 exact pair/generation 后才允许 Submit；绑定无效的 A gameplay 查询保持中性，不能退回物理输入旁路。周期性 owner 不触发每帧重绑；死亡/复活不能只比指针。以上均维护既有统一输入和生命周期语义。

审阅纠正的候选误认包括：宏生成的 Player Update Hook 被字面搜索漏掉；Room 实体列表误称 PlayerManager；`0x5EE2C0` 被误标 Scalar（实际是 bool 取反 wrapper，Scalar 为 `0x5EE2E0`）；`0x5EED00` 被误标设备名/类型函数；一个 `0x341A45` caller 文件实际导出别的函数。原始文件保留，最终结论采用重新确认的地址与主审证据，见 artifact 更正记录。

本轮静态工作已收束，以下不宣称完成：shim 和成对 Hook 的实际安装/异常行为、设备更新与采样在实机上的实际顺序、各阻断区间的运行值、MC_PRE_UPDATE/Player skip 的实际样本、切房后的实际绑定/位置、输入效果与持久副作用。这些随 tx-04-05 全部实现完成后的唯一一次整体 E2E 观察，不在本轮试跑。所有房间路径的最终位置写入次序仍按 T6 的限定保留 UNKNOWN，不作为本轮实现依赖。
