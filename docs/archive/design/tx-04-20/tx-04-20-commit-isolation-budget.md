# tx-04-20 I31—I35：只读展示、隔离、退休、提交和预算

日期：2026-11-04。同一 tx-04-20 内完成本批实现接线；按用户要求直接提交。**未构建、未运行分项测试或统一 E2E、未启动游戏**。I36—I40 待继续，I39 才执行 E01—E30 统一验收和必要修复。本页是实现／静态研究说明，不是运行通过记录；不保留推进 artifact。

## I31：只发布同一 A 的展示数据

`client_player_view.*` 在写世界前一次建立候选不可变视图，只有 `ClientPlayers::Commit` 接受 Scene 同 attempt 提交才能发布。正式入口 `ClientPlayerDisplay()` 只返回当前 A；保存旧快照的消费者继续展示前调用 `ClientPlayerDisplayCurrent()`。写前拒绝／取消保留旧 A，部分写入、失效、退休不暴露候选。

`Participant()`／`Body()`／`ActiveSlot()`／`PocketSlot()` 提供组、身体 Key／shape／role／绑定代次、存在与死亡、roster／四备份槽、全部心量和资源、四主动／四口袋与充能、五 Weapon、History、效果／变身及角色有限槽。`Captured()` 保留 Authority 原查询及参数／状态，并给出同组件图根，覆盖派生心量和显式共享语义；不调用 Client Getter 补算。相等库存／心量／History 不证明共享；Null、NotApplicable、空和失败仍分开。

新增必需子合同 `player.application_source.hud_state=player-hud-state-v1`；每身体 `application_player_special.hud` 复制**实际动态 HUD**及 24 个心量缓存、索引、颜色、闪烁、mantle。来源不使用会回退 Game 八槽的 Lua Getter。`client_player_hud_native.cpp` 用原生窄工厂建立真实 HUD 和 control pair，不调用 Player Init／HUD Update，不借 Game 内嵌槽。完整 HUD 排版与动态表现仍归 27，18／19 的窄 HUD 不外推为全部完成。

## I32：用途与全窗口隔离

`client_player_isolation.*` 分别约束构造、基 Init、最终值／资源、关系、draw、退休的主线程用途。Player 所采用路径没有已确认随机需求，允许集为空；native RNG／MT、全局随机、CRT、Lua math wrapper 在 Player 用途内遇到意外随机时，在 original 前拒绝并报错，不先消费后恢复，也不记为 original 完成。18／19 已解释的局部表现流保留原合同。

Client 准备至安全退休继续保护玩法分派、Lua callback／持久 writer及进程退出最后写入者；SFX Play／Stop 全窗口主动跳过，Authority 音画消费归 27。统一 Hook 清单中的实际 trampoline、运行时随机 wrapper 与 Lua guard 在构造前核对。Host／Off 透传；正常观察 original exactly-once 不变。

## I33：先撤边，再按实际前缀退休

Scene 撤展示立即关闭 Player 新借用。有效 Manager／context、CPU／GPU／Lua 借用退尽后，全部 Player 先撤 EntityPtr、身体／Weapon 出边和动态 HUD；再让非 Player 撤边、退休；最后 Player 撤 roster、确认无全局借用、移出 EL、配平发布计数并真 Free。分别记录 adapter／装备／状态／HUD／detach／计数／Free 前缀，每次至多释放一个身体。

未进 ctor 的 raw allocation 可回滚。ctor 正常完成、基 Init 未进的唯一对象可走真 deleting destructor；ctor／基 Init 未正常返回、FreeEntered 异常及未知释放状态保留 owner 和首因，不重入。HUD 通过真 pair release 释放，不由 Player 再释放第二次。

Player ANM 和动态心量共用的 hearts ANM 持有真实 strong copy 至 CPU drain；新增登记链在进入 copy ctor 前持有 owner。C++ 构造栈展开、copy／swap／析构不确定时记录仍可达，`ClientEntityDrawDrained()` 不会误报退尽。正常完成撤记录，不写逐帧日志。

可解释的应用失败只在**实际安全退休之后**从完整 T 自动重建一次，建立新 attempt，保留第一次失败和执行前缀。重复失败、未知 owner、退休失败、stop 和网络／回调／线程等外部故障不授权盲目重试。新 T 仍走同一 Scene 接纳／退休路径；K 的 NEVER 策略不变。

## I34：唯一 Scene 提交及准确域位

`native_source_complete` 仅表示本 T 正式同窗来源完整。Player／非 Player 全体实例、关系、资源和实际回读完成，且 Room／Grid／展示／后置引用同 attempt 完成后，才由 `ClientScene::Pump` 唯一提升 A。

结果分开报告 `player_bodies_applied`、`player_relations_applied`、`player_resources_applied`、`player_domain_applied`。完整 Entity 位需要同 scope／revision 的完整 Player owner、成员全集及非 Player 合同；player 域还包含其关系／History 责任。旧单 Isaac 支撑、历史代表、候选及独立事件不授予完整域位；其余域不随之置位，不发送网络 Applied ACK。

目录 `native_apply=true` 表示实现路径已接入，与 `runtime_acceptance=I39-not-run` 分开；历史阶段和静态研究内的 false／UNKNOWN 不追溯改写。

## I35：独立预算、缓存和成本

`replica/player_limits.h`／`client_player_budget.*` 分别计 4 参与者、每组 64 节点、256 身体、roster／非 roster、全局 4 备份槽、各身体库存／效果／History／costume／Weapon／槽／序列／动态 HUD、资源层／selector、引用、计划和字节／节点。足以容纳四组八活动身体及所需非活动对象；超限具名失败，不截断。

保留 64 Mi 节点；来源编码独立上限 32 MiB，资源槽 4096、层 65536、引用 262144、Player 计划成本 65536。原容器细限不缩小，另有 1048576 聚合容器项和 256 MiB 自有值保守估算。图展开取消旧的 1M 节点窄限，独立计展开字节；Entity／Player 合并命令再核容量。写前预算拒绝保留 A。

同 T 图索引／子值一次解析，安装和 draw 复用不可变值；稳定资源沿原 resource 服务复用，生命周期变化按 Scene 代次立即撤借用。HUD 不逐帧扫描 JSON。E2E 聚合 source／parse／structure／links-check／draw／retire 的次数、累计、峰值耗时；显式 status 另读 OS working set／private bytes，与模型估算分开。未运行便没有实测成本，不声明实时 10 Mbps 能力。

## 必要逆向与主智能体审阅

按用户要求由一个 **GPT-6 Luna max** 子智能体只读研究，主智能体独立复读采用点位。先本地原版 EntityPlayer／HUD 和 REP PlayerHUD／History Wiki，再官方 ZHL／`Lua_PlayerGetPlayerHUD`，最后才用 J273 IDA 核定私有 owner／ABI／清理；无 IDB 修改、动态实验或哈希校验。

下列为 VA，RVA=VA−0x400000：

| 点位 | 主审采用／纠错 |
| --- | --- |
| 74DC70→6A3940、6A4A20 | ctor／基 Init 直接路径无可见 RNG 需求；不调用 74F6A0 Player Init。 |
| 74E8DB／74E8E5、74F34C—74F375、A95225 | +20A8/+20AC 是两 word owner pair，SDK 只命名首 word；不能只写裸指针。 |
| 751390→7D3EA0、402670 | 完整 Player Init 会产生 ptr=null/control!=null；control 初值 0x00010001。本方直接 factory 输出优化仅用于 ctor→基 Init 两 word 均零的对象。 |
| 77BD40／77BDA7—77BE00、980020 | parented variant0 动态 HUD 路径；factory ECX=HUD，栈(outPair*,Player*)，ret8；真实分配 0x6E0 HUD／0x18 control，索引初始−1。原临时 copy／swap／release 最终留一引用；本方空槽直接取得初始唯一引用。不将所有 parented Player 都命名 Strawman。 |
| 6EC180、8326C0、7D2B20、7D33A0、6EC2F0 | 真构造／清缓存／pair release／强引用归零／真 HUD 析构。control 自身减计数及释放，BECA10 是原条件下可选回调，不能要求其非空。 |
| 78573F—78579C、83AC80、834930、412AC0 | 非空动态 HUD且layout1/3才画身体旁心量；读心24、+A颜色、+C..E闪烁、+6A0 mantle及共享 hearts ANM，helper可换资源/帧。+207D为已安装 IsCoopGhost，412AC0为sin而非随机。hearts ANM纳入整帧强借用。 |
| 74E970／74F260→6A8000→84CA50 | deleting destructor／析构／Remove／callback67，general Lua guard拦玩法回调，unref仅清引用。未把直接无SFX外推所有子析构；Play／Stop全窗口拒绝覆盖该不确定调用面。 |

静态实现不代替 I39 的真实后态／画面／隔离验收。I36 错误／故障观测、I37 构建／产品边界、I38 编排、I39 验收、I40 真实结果交接仍待执行；未声明 Mod／内部 Baby／possessor 等保持具名范围。
