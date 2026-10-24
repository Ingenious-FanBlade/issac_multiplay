# tx-04-05：game 主线程适配实现记录

后续记录：Lua 夹具、Rust 单启动 S0–S11 / V1–V10 代码已编写，并补充必要的只读诊断，见[测试代码记录](tx-04-05-test-harness.md)。下文保留适配阶段的原始状态；唯一整体 E2E 尚未执行。

日期：2026-10-24。对应[实现方案](tx-04-05-implementation-plan.md)第 3.2 节，并完成使这些模块可进入调用链所需的第 3.3 节 Hook 与第 9 节命令/状态桥接。Win32 / v143 Release 纯编译、链接成功。没有部署、加载 DLL、启动游戏、执行测试或操作存档；tx-04-05 的唯一整体 E2E 尚未执行。

## 模块与调用链

| 文件 | 职责 |
| --- | --- |
| `mod/src/game/context.*` | 固定 64 层 owner 栈；每次压栈分配单调 token，按 token 弹栈；区分 Input、Device、普通 Odd 标记及生命周期 owner；错误或异常关闭相关修改 |
| `mod/src/game/tick.*` | 堆上稳定存储的 SoloRuntime、主线程会话启动/退休、Room/Odd Authority-pre、原生返回及 TopExit 提交/发布、状态快照与每 tick trace |
| `mod/src/game/topology.*` | 主线程有界读取玩家/设备 vector；设备 id 唯一匹配、Keyboard/Gamepad 类型确认；区分房间身份、Player 生命状态及临时 exact pair |
| `mod/src/game/input_a.*` | 同一设备作用域的 Poll 依据；公共更新成功返回后采样原三槽；设备 held/射击方向历史、canonical、启动例外与统一 Submit |
| `mod/src/game/query.*` | 仅对受控 A gameplay consumer 的 12 个 action 覆写 Pressed/Triggered/Scalar；有效窗口读取 Resolve，其它受控情况中性 |
| `mod/src/game/consumers.*` | 六种 consumer 的 TLS 上下文、射击分派、Player 内外层观测、有效 generation 下正常更新后的字段观测 |
| `mod/src/game/lifecycle.*` | Console/PauseScreen/focus/loading 区间、房间/死亡/复活/设备变化失效、延后重绑、Restore/退出退休 |
| `mod/src/game/bridge.h` | 只用 C++17 值类型的 Hook 桥，SDK-facing wrapper 不包含 C++20 core 头 |
| `mod/src/game/state.h` | 主线程内部运行状态、tick 与查询汇总；B 的存储不初始化、不参加 tick |

`hooks/context.cpp` 的 CallScope 仍维护入口/正常返回计数，现在委托 game 管理 owner/token。`ops/command.cpp` 在最外层 Manager 入口执行命令，在出口调用 game 的 TopExit 后发布状态。所有游戏内存读取受主线程检查约束；网络线程只保留现有命令消息队列。

## tick 与 Hook 接线

- 普通 Room 入口要求栈顶为 Game，且不在 Start/Restore/Lifecycle/Door/NestedDoor/Join 内，实际 Room 与当前房间一致。随后统一 BeginDrain → SubmitInput → BeginSimulation。
- Odd helper 自身只建立 owner；普通调用点 shim 建立 OddNormal，只有该标记对应的首个 A Player 入口可开始 OddPlayer tick。两处早期特殊 Player 更新不开始 tick。
- Room/helper 正常返回才调用 NativeReturned 提交 previous。Manager TopExit 先 FinishPostState（Disabled）推进 completed，再 PublishCompleted 和 tick trace。异常、重入、同轮重复、错 token 或缺失返回按现有原因停止，不自动重试。
- Player Update 外层 priority=-10000，内层 priority=+10000。入口、原生子类 body 到达、正常返回但内层未到达分别计数；没有用 consumer 缺席推断 Lua 跳过。按 U4，该 tick 中未被读取的 edge 不补发。
- 六种 consumer 除受控 A 射击的明确抑制外，各自调用原函数一次。射击在无模拟窗口、绑定无效、生命周期上下文或阻断期间不调用原函数；正常 tick 的缺输入与真实松键仍执行中性消费。
- 设备 wrapper 每次仍调用原三槽函数一次，再决定是否覆写返回。未激活、非主线程、UI、其它 Player 和 action>=12 保留原值。按当前支持范围，不接管第三方 gameplay Lua 输入重写分支。

唯一清单 `mod/binding/hooks.json` 现在描述 **28 个入口 Hook、8 个设备槽与 1 个 Odd 字节补丁**。这是代码和构建清单的数量，不是实机安装观察。生成器同时输出 manifest 和 `permanent_patches.inc`；新增补丁由统一 InstallRuntimePatches 在首次主线程 Manager 入口协调安装，会话只逻辑启停。

`hooks/odd_patch.cpp` 在静态确认的 `0x2F9559` 搬移完整 `call [edx+0Ch]` 与 `push 0`，返回 `0x2F955E`。回调前后保存通用寄存器、EFLAGS 与 x87/MXCSR/XMM 状态；原虚调用在 shim 中恰好出现一次。没有读取 `_ReturnAddress()` 或猜测 ZHL 桩帧；正常路径弹出标记，异常路径由 Odd RAII 清理。实际指令不匹配或保护/缓存操作失败时停用相关能力，不恢复目标字节。

## A 的输入与生命周期

1. A 取 PlayerManager 列表第 0 项并确认 playerIndex==0，以 controllerIndex 唯一匹配设备 `+8` 的 id；不把 id 当 vector 下标，不使用 -1 任意设备分支。Binding 不含 control block。
2. Poll wrapper 只登记同层设备的结果。设备公共更新返回 true、同层一次 Poll 成功、处于 Manager/Input/Device 且候选唯一时，才调用保存的原 Pressed/Triggered/Scalar 读取 12 个 action。原型旧设备数组 sampler 没有移植。
3. 物理历史与 participant generation 分离。暂停、加载和绑定失效期间继续维护已知设备的 held 历史；丢弃受阻区间 edge。设备失败、消失或身份未知后，首次重见只建立 held 基线。公共更新失败时先撤销关联，防止同轮销毁/地址复用冒充旧设备。
4. 每个有本轮新鲜样本的 Authority-pre 都提交目标 tick 命令，持续 held 不因数值相同而省略。重绑后的首条用 completed+1；同一样本的连续量另以 lead=2、新 sequence、edge=0 提交。所有命令经 Participant 的 codec/字节 admission/queue/Resolve，没有房主旁路。
5. 新房回调正常返回只使绑定失效，该 Manager 轮次保持中性；之后至少在后续轮次的无阻断 Authority 入口才能重绑。还比较 stage/room/dimension、Player 的 valid/exists/dead/removed/ghost 与 exact pair，覆盖同一指针的死亡/复活。周期性 owner 进入本身不产生持续失效。
6. 失效时保留受控 A 的识别标记，仅用于中性查询，不解引用旧对象；退休的在途回调若替换了 A，也不能给新对象打开物理输入旁路。停止关闭新输入/tick，已开始的正常模拟先完成提交与发布，再退休 generation 和回到 Idle。

字段读取按 v1.9.7.12 的既有静态依据与官方 `.zhl`：移动方向为 int，速度与两个射击分量为 float，射击标志 `+0x162C` 为 byte；只在对应正常 Player 更新和相同 generation 后观测，并检查 float 有限性。不能从这些局部字段声称完整武器或世界状态一致。

## 命令、状态与记录

- `cs solo start/stop` 已接入唯一命令处理器，校验后入队，下一次最外层 Manager 入口执行。start 再次检查处于 run，使用系统随机 session id 与递增会话 generation；A client id 在会话内固定为 1，不初始化 B。
- `cs host/stop` 仍是原有占位语义，与已激活或已排队的 Solo 相互排斥，返回 busy。真实会话由 game 持有 core::Session；没有把占位 Hosting 状态当成网络或 Solo。
- 状态增加 session_state/mode、generation、completed/current tick、两种 tick 计数、A 绑定/generation、blocks、last_tick_decision、queue_depth_a、query_overrides、shooting_suppressed、odd_patch_installed 与 session_error。Idle 后保留最近一次会话统计及 mode，新的 start 重新构造整个上下文。
- trace 增加 session、binding、sample（按变化）、submit、tick、tick_reject、paused、block_interval、shooting_dispatch。每 tick 汇总六种 consumer 的三类查询次数及非零次数；Hook completed 仍表示 wrapper 正常返回，不能据它推断射击 original 未被抑制。
- 非主线程仅执行原调用、计数和原子失败通知，主线程消费后停止相关修改。上下文损坏、丢日志或原生异常也关闭本次会话；不会盲目重绑、补发旧输入或重注入。

## 构建结果与未观察部分

[构建记录](../../artifacts/repentogon/tx-04-05-game/README.md)保留配置、四次纯构建日志及生成的 Hook/字节补丁材料。初次全部适配编译链接成功；随后静态复查补充设备失败和退休对象替换处理，统一激活前后样本的新鲜度为 Manager 轮次（scope token 仅作配对），并收紧采样作用域/原查询失败处理。最终构建成功，没有新增 game 源文件 warning。构建中的依赖 warning 来自现有 RapidJSON、SDK 头和生成 wrapper 的未用参数等。这些仅为随代码修改进行的纯编译/链接，测试执行次数仍为零。

这次没有运行任何测试、游戏或构建产物，没有校验哈希。实际 Hook/shim 安装、异常展开、输入效果、各阻断区间、重绑位置与边沿语义仍待 tx-04-05 唯一整体 E2E 观察。T6 的全路径最终位置时序仍为 UNKNOWN，代码使用后续稳定 Authority 边界，不以 new-room 回调证明位置稳定。测试 mod、harness 的 S0–S11 场景与 V1–V10 汇总尚未实现；网络、B、Join 调用点修改及 world_record 不在本次范围。
