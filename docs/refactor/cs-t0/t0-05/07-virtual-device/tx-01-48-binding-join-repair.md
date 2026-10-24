# tx-01-48：绑定、注册和 Join 阻断修复

日期：2026-10-16。用户要求检查 A/B 绑定、注册及 Join 未观察到的原因，修复后再次测试；已确认完全重启游戏并准备好隔离 Offline 初始仅普通 A。沿用已有全部操作授权。tx-01-45/46/47 的失败与未观察记录保留。

## 实现前 E2E 与失败判据

分析、修改期间不运行构建或实机试验。完成本轮实质修改后，通过统一入口一次执行构建和真实游戏 E2E，新目录为 `artifacts/cs-t0/tx-01-48-live-01`，使用新游戏进程及桌面权限，不能重新注入上轮常驻 DLL 的进程。

场景沿用 [tx-01-45 完整场景](tx-01-45-e2e.md)。首先要求观察拓扑检查、A 绑定、单次设备构造/注册、两个 Join pass 的实际 query、精确 B row 创建及 B 绑定；成功后继续双端输入、暂停/恢复、换房/死亡复活、B 冲突退休、A 继续及菜单 teardown/47 槽恢复。未能到达的场景不记 PASS。

本轮特别检查以下失败方式：

- 把 Lua 引擎存在误判成输入回调接管，仍不能进入普通输入路径。
- callback 13 已启用、mask 不可读或 Online roster/frame 非空时仍继续绑定或写世界。
- 玩家 vector/vtable/ordinal、K/pair 或物理设备身份不唯一，但被放宽成有效绑定。
- 只有 Rebind 事件而没有具体拒绝原因；失败诊断随查询大量重复导致事件覆盖。
- 设备失败后重试、Join owner 再次创建，或用手工添加 Player 替代原生 Join。
- 新事件改变 128 字节布局或重现 tx-01-46 的按值 Event 调用错误。
- 输入隔离、generation、退休和恢复失败；没有观察到的用例被写成通过。

产物包括 invocation/scenario/toolchain/build/native-disassembly 和 capture 的 events/loop-events/operations/helper/restoration/result；新增 Diagnostic 事件及报告摘要保留阶段、原因与实际值。失败不自动修改和重跑，不计算哈希。

## 原因与静态证据

tx-01-47 事件确认反复 ReadTopology 拒绝→InvalidateBoth(Rebind)，没有任何 Binding/DeviceState/Joined。源码中 TopExit 只有拓扑与 A 绑定成功后才调用 Create/Register，JoinPass 只有 Registered 后才开启，因此三项缺失有同一前置阻断。

旧 ReadTopology 用 `n298[2]==2 && dword_C71690!=nullptr` 直接拒绝。IDA 对当前 isaac-ng.exe（32-bit、base 0x400000）的静态读取显示：

- 956110/9561E0/9562D0 中，上述条件只是“可以尝试 Lua callback”的前置；只有 864C30/864D80 返回有效结果，才早于普通设备 reader 返回。
- 864C30 在 864C76 读取 Lua engine 首指针，在 864C7B 的 `F7 00 00 20 00 00` 检查该指针指向的首个 DWORD 的 `0x2000` 位；位关闭时直接返回 false。864D80 同样检查这位，然后才尝试 callback id 13、registry ref 和非 nil 返回。
- 6B5E60 通过 C71690+0x18 取得 lua_State，证实 C71690 是 Lua engine 相关对象，不能仅以其存在判定 reader 被接管。
- 6FA0D2 检查 root+0x2658A；6FADA1 以 root+0x1BAA8 为 Join receiver；9B9E5B–9B9E63 以 Main vector 大小赋 Player+0x161C。保留这些现有身份检查，没有猜写布局。

直接确认的源码错误是把必要条件当作充分条件。上轮没有记录 mode、mask 等现场值，故不能追认上轮具体拒绝分支已经被运行时证明；本轮会记录 mode、engine 是否存在、callback mask 和实际拒绝条件。

## 修复范围

仅在 mode=2、Lua engine 存在且 callback 13 的启用位打开时保守拒绝；mask 指针不可读或为空仍拒绝。位关闭时进入已理解的普通 reader 路径，不执行 Lua、不探测 callback 返回值、不改变 Mod 注册状态。Online、Main Player、ordinal、K 和 exact pair 约束继续生效。

新增只在状态/实际值改变时发布的阶段诊断：Topology、Binding、PairLookup、DeviceCreate、DeviceRegister、Join。PairLookup 和最终身份绑定分别记状态，避免同一次查询的中间成功与最终拒绝互相切换、重复刷日志。绑定和注册事件附上当前 native K/row/player/device/control 与本地身份映射信息，便于核对；这些指针只用于日志与本地关联，不进入 canonical command。

## 验收结果

2026-10-16 仅执行一次统一入口。Rust、Native/helper 构建和初始化成功；使用用户已完全重启并准备好的 PID 35976（creation time 134366351339428419），未复用旧常驻模块。记录 731,674 条 co-op 事件，Authority 推进 11,868 tick。**本轮绑定/注册/Join 修复已得到运行时确认；完整场景结果为 NOT_OBSERVED（35 PASS、7 NOT_OBSERVED），没有自动重跑。**

| 观察 | 原生事件序号与实际值 |
| --- | --- |
| 旧条件会误拒绝本轮普通输入路径 | #9 Topology READY：mode=2、Lua engine 存在、mask=819206（0xC8006），callback 13 的 0x2000 位关闭 |
| A 绑定 | #12：ClientId=1、ordinal=0、K=0、Player=2149355636、device/control=229474676/186647124 |
| 单次创建与注册 | #15 DeviceCreate 正常且已 pin；#17 注册一次，B K=1、device/control=233937436/2151204828、strong=2 |
| 原生 Join 与 B 发布 | 两个 Join pass 实际查询记录 #22–29、#60–67；#64 精确 K=1/row=2050325228 创建 callsite；#71 B 绑定 ClientId=2/ordinal=1/Player=2063831332；#72 确认 Main A/B 与同一 owned pair |
| 双端输入及决策 | A/B Movement、Shooting 世界 witness、物理 loopback、Duplicate/Invalid/Expired/WrongGeneration/WrongSession/SequenceConflict 均有事件；缺输入中性、action 8–11 显式 edge Resolve、新 generation 首个显式 edge 均观察到 |
| 暂停/恢复与换房 | #204372/204373 为暂停恢复后重绑；用户仅确认完成换房，#327538 起 Room 事件及 #327599/327600 generation=15 重绑独立确认；不把用户口头回复作为通过依据 |
| B 冲突退休，A 继续 | #687495 SequenceConflict，tick=10570；#687498 pair 已移除且 root strong=1；#687501 A Resolve、#687506 A Movement 变化，之后继续推进 |
| 菜单 teardown 与恢复 | 用户确认回主菜单后，#731625/731626 原生 Teardown；#731627–731673 为 47 槽原始字节恢复，#731674 汇总成功；helper 正常 shutdown，pinned/nativeRestored=true；之后同一游戏进程仍在 |

nativeError=0、transportFailure=null；重复 Resolve、世界 commit、consumer apply、accepted sequence 和不一致 query 记录均为 false。没有发生第二次设备注册、Main B 创建或 Join 发布。

七项未观察到：B Pill/Card consumer 状态变化、action 10 held 查询、Loading、LoadingExit、Death、Respawn、同一 query key 的重复读取。不能据此宣称完整 T0-05 验收通过；死亡/复活本轮未实际完成，Pill/Card 缺少运行时变化证据，重复查询缺少重复样本。完整 OOM/EH/worker 并发仍不在范围内。

操作中 Authority 曾停在 tick 5386，原生 Pause 事件 #350226/350227 已使 A/B 绑定失效；第一次 conflict-b 控制因此未提交冲突且没有退休。用户确认正常游戏画面后，随后捕获 Resume、generation=19 重绑并继续推进；具体画面/焦点成因没有观察证据，不作猜测。保持同一次验收窗口，在活动 Authority 下发送后续 conflict-b 才观察到上述冲突退休。没有重建、重注入或重创建设备。

产物：[原始结果](../../../../../artifacts/cs-t0/tx-01-48-live-01/result.json)、[本轮摘要及精确事件](../../../../../artifacts/cs-t0/tx-01-48-live-01/assessment.json)、[操作记录](../../../../../artifacts/cs-t0/tx-01-48-live-01/capture/operations.jsonl)、[桌面操作补记](../../../../../artifacts/cs-t0/tx-01-48-live-01/desktop-operations.json)、[原生事件](../../../../../artifacts/cs-t0/tx-01-48-live-01/capture/events.jsonl)、[恢复记录](../../../../../artifacts/cs-t0/tx-01-48-live-01/capture/restoration.json)、[helper](../../../../../artifacts/cs-t0/tx-01-48-live-01/capture/helper.log)。同目录还保留 invocation、场景输入、构建日志、工具链信息和反汇编；未计算哈希。

原始 result.json 的 tx 仍是控制器写死的功能编号 tx-01-45；不修改该原始输出。本次执行编号 tx-01-48 由 invocation.json 和 assessment.json 明确关联。capture-only 摘要脚本不连接游戏或重新验收。
