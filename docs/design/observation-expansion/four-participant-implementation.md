# tx-04-09 四参与者与多身体实现

日期：2026-10-30。状态：DLL、harness构建及统一功能整体验收通过；实际四参与者／八身体、指定形态与退出场景均已覆盖。用户追加的生产关闭诊断／录制和拓扑复用要求也已实现，诊断／生产配置均通过全场景续测；七身体短窗口约60 FPS，不外推为完整性能专项已完成。旧同号ASM patch历史结果不变。范围见[任务定义](four-participant-control.md)。

## 身份、连接和名单

`SessionIdentity` 预留四个互异稳定 client ID；`present` 单独表达已加入名单。Host 可以一人激活，远端依次加入，日常 tick 不等待其他参与者。离开不压缩 ordinal，也不复用该会话已退休身份；新参与者可以使用尚未用过的远端槽。希望复用退休槽时须由用户显式停止并开始新 Host，会生成新 session；不自动重建 K 或重试 Join。

ICSN 协议升级为 v2，build 为 `isaac.cs.tx-04-09`。Hello 的四 ID 数组声明会话身份容量，Ack 的 `roster_mask` 表达实际名单。三个远端显式请求 ordinal 1、2、3，sender/client 为 2、3、4。一个 socket 和线程按已锁定地址路由至三个独立 Endpoint；nonce、sequence、重传、ACK stamp、输入队列、心跳和超时互不混用。满槽、已占用／退休 ordinal、另一来源复用 nonce、旧协议和身份矛盾均不获得接管资格。

每个远端有独立 `RemoteRuntime`（K、Join、生命与退休记录）。PeerBye、超时、序列冲突和归属错误只退休对应参与者；共享 socket／线程故障才退休 Host。每人每 session 最多构造一次 K，NEVER 和主线程约束保持。

## 控制组和消费

参与者槽与身体集合分开。单组容纳最多 64 个关系节点，四组不以 `Player[4]` 表达身体；溢出显式关闭相关资格。根来自初始 Host 或本次 Join 的创建 owner：保存原生 pass 前后 roster，以新增根的角色、Join row key、自有 K 和关系闭包共同确认，所有新增身体必须得到解释。后续根切换依据成功Replace的旧新参数，或当前roster到旧根的已知关系，不以同ControllerIndex单独接纳身体。跨参与者身体重叠关闭相关组资格。

复用已核定 J273 的 `sub_player +0x1D84`、`twin +0x1E4C`、`backup +0x1E50`、`replaced +0x267C`；ControllerIndex 仅为当前设备关联，原生 PlayerIndex 重编号不改变参与者身份。backup／replaced 与输入可用身体分开。各身体经过自己的真实 PlayerUpdate，分别取得观测 ID 与读取依据；不把主身体的更新资格转赠给关系目标。录制关闭时不虚构 Entity ID，状态中的 `entity_id=0` 明确表示当前无录制身份来源。

一个参与者只有一次 Submit／去重／Resolve／End。多个原生消费者只读同一不可变意图，不全局消费第一次查询的边沿，也不按身体重新提交。原生角色的 Twin／Drop 门控、资源与冷却决定实际副作用。hologram 保留运动关系，五类动作的原生提前返回保持。暂停、加载、首输入启动和最多六 tick 重绑射击恢复规则沿用；Host 自己死亡／换形也只撤销其输入，已有 Active Authority 不等待其重新绑定。

Init／Remove／清引用／销毁／PlayerCtor／Replace／PlayerRestore 的既有清单项标为 `shared_control`，关闭 world 录制也安装这些常驻入口。只按已知身体地址撤销对应输入资格，原生作用域未返回前不重绑；异常隔离相关组。纯 world 观察仍保持原来的故障隔离。

退出先关闭对应输入、排空当前回调；保存属于该组且确实在主roster的身体快照。移出K后按快照中的真实PlayerIndex升序处理，每次调用前复读roster；由原生cleanup递归移除的身体直接跳过。不能先移除高索引twin再移除根：twin的Index已为-1但引用尚未清空时，原生会递归重复移除并使vector长度下溢。不向未知第二vector或库存备份盲调移除；清理不确定时保留常驻对象与具体错误，使相关组保持中性。

## 静态研究及主审

用户指定的 GPT-6 Luna max 完成只读研究，主智能体复读本地 Wiki／REP 源码并直接审阅 J273 `SpawnCoPlayer2`、Bomb 反编译。

| 点位／来源 | 静态事实与实施含义 |
| --- | --- |
| `LuaPlayerManager.cpp:242`、`:258` | `GetPlayers()` 枚举主 roster；`GetEsauJrState(0..3)` 是独立备份槽，不是额外参与者 |
| `GetPlayerSlotFromController` RVA `0x5911C0` | 四个 controller row；不能解释为 Host 外再加四人 |
| `SpawnCoPlayer2` RVA `0x590250` | 主 roster 普通分支容量 64，与四个连接身份不同；原生特殊 `-1` 分支的第二 vector 不纳入当前控制列表 |
| `InitTwin` RVA `0x3C8370`、`SetControllerId` RVA `0x39E1B0` | twin 可进主 roster；设备 key 会向关联对象传播，但同 key 不构成活动归属证明 |
| `IsHologram` RVA `0x3D1C90`、Movement RVA `0x372540` | hologram 与库存 backup 不同；保留运动关系，副作用消费者保持其原生跳过 |
| Bomb RVA `0x37A210` | 原生按当前实体处理动作 8，读取 twin 的冷却／资源条件，不在 handler 内额外递归调用 twin Bomb；保持一份组意图，由原生逻辑决定身体结果 |
| `RemoveCoPlayer` RVA `0x590560`、cleanup RVA `0x351C10` | 主审追加核对后修正初读结论：cleanup按条件递归处理twin、sub及`+0x1E50`备份；按实际索引升序并逐次复读roster，避免递归重复移除。该路径不清manager的Esau Jr槽 |

| Esau Jr helper RVA `0x3A3A40` | 当前主动道具槽 `+0x1574+0x20*slot` 为703时，`+0x1590+0x20*slot` 是一基备份槽；对应 `Game+0x1BAA8+0x80+4*(slot-1)`，不是参与者或PlayerIndex |
| `ReplacePlayer` RVA `0x595E10` | 返回前清空双方临时 `+0x267C`；成功返回参数保留控制根替换来源，后续稳定入口再核定，失败不转移 |
| `SwapForgottenForm` RVA `0x3C2470` | 普通16/17交换同两个对象的状态和类型，不交换对象地址；非roster关联身体须实际正常进入更新和consumer，下一完整tick才取得输入资格 |

| Drop RVA `0x39C300` | `0x39C359`查询action11 Triggered，`0x39C36C`调用Forgotten交换；使用既有Drop consumer。交换在另一形态没有健康时被原生拒绝，夹具需准备真实Soul健康，不能把拒绝误判为输入未投递 |

以上补充由同一Luna max子智能体研究，主智能体直接复核反编译；两份退休崩溃dump的异常地址、访问地址及EDX=-4也由主智能体复读。公开API与静态指令不替代下述运行时验收。

Esau Jr 全局四槽是原生run状态。静态追踪未找到原生退出路径中的释放API；本实现保留其原生槽至run teardown，不手工清指针或析构备份，不承诺退出后回收四槽容量。退休后的备份不是参与者，也没有输入资格；E2E包含持有备份的Peer独立退出及其余组继续运行。原生槽容量限制如实保留。

## 操作与统一整体验收

命令为 `cs host <port> [default_remote_character]`、Join 前 `cs role <ordinal> <character>`、`cs stop`；Rust `remote` 新增 `--ordinal 1..3`。UI 分别显示参与者数、身体数及三个 Peer 的状态。状态增加 `participant.<ordinal>.*`、逐身体角色／原生索引／观测 ID／活动资格及逐组边沿进度；旧 a/b 状态保留首个远端的兼容视图。离线比较可用显式 `left_entity/right_entity {local_id,use_generation}` 配对多身体，不以同 ordinal 把多个身体合成一个对象；旧单身体配对继续解释。

统一入口为 `scripts/accept-tx-04-09.ps1`，`-Stage Build` 与 `-Stage Run -GameRoot <目录>` 是同一整体验收的构建及实机阶段。只有全部目标实现完成后调用；失败修复后在同一 tx 继续必要覆盖，不新增单元测试或提前试跑。

用户追加生产配置要求后，同一入口增加`-Profile Diagnostic|Production`。诊断配置显式启用录制与明细，生产配置从进程启动即关闭两者；两者都使用生产拓扑复用路径，不以重复全图读取制造E2E专用正确性。生产配置不要求录制Entity ID，明确检查它为0，并继续用已核定控制关系与逐身体原生行为验证输入；查询一致性诊断关闭时的0不冒充已测量无不一致。

E2E 单次启动内覆盖一至四人、第五来源拒绝、不同方向与动作、每组四类边沿、暂停／控制台、切房／换层、房主及远端死亡复活、独立超时／Bye、四组 Jacob/Esau 的八身体、Forgotten／Tainted Forgotten／Lazarus Birthright／Flip、Esau Jr 两次切换及 teardown。双子炸弹副作用在同一整体验收中与原版透传的一次真实物理按键作具体参照，避免用队列处理次数代替实际创建结果。关闭录制后继续验证房主死亡复活，输入生命周期不依赖录制器。判断使用 DLL 的已确认身份图、三个独立远端记录、各身体原生运动／攻击观察和一组一次的处理进度，不能只看 ACK。角色的原生差异单独表达，不强迫 hologram 或 Tainted Soul 做原生禁止的攻击。

每次启动前告知操作范围并确认必要准备：第二存档、种子测试局，临时部署并恢复 DLL／UI／夹具／配置及 Download Helper 标记；harness 接管键盘，用户停止手动操作直到退出提示。沿用 D18/D20/D27 的存档变化范围，正常退出不回滚存档。

正常测试只使用 `out/tx-04-09-four-participant-temp-*` 临时材料，成功后清理。必要中断才保留恢复所需的诊断／进度；不为本 tx 创建源码快照或永久 artifact。

整体验收最终结果：统一入口最后一次退出码0，五个输入／角色窗口通过，其他生命周期与退休场景由逐状态等待确认；游戏正常退出码0。覆盖一至四参与者、第五来源拒绝、普通／双子混合、四组双子的八活动身体、不同运动与攻击、每组四类边沿、暂停／控制台、显式切房／换层、远端死亡复活、独立超时／Bye，以及普通Forgotten的16→17→16（根地址不变）、T.Forgotten的35／40身体、Lazarus Birthright／Flip、Esau Jr两次切换与持备份退出。关闭录制后Host死亡复活仍正常；补验Flip后的Lazarus双身体独立Bye、T.Forgotten双身体`cs stop`、新四人Host及Active菜单退出。最终query mismatch、trace dropped、context fault、cleanup failed均为0。双子原生物理按键参照实际创建Jacob炸弹1个、Esau炸弹1个；四个canonical组逐组完全相同。

失败与修复如实保留：沙箱内MSBuild停滞后中止，在正常Windows构建环境继续，修复C++设备字段替换及Rust实体配对接口编译错误。实机共11次启动，同一tx内连续修复续测：①启动焦点要求冲突；②夹具完整status导致游戏单行日志截断；③持续输入意外过门；④关门回调在run之前访问房间导致崩溃，增加真实run生命周期限制；⑤已开门仍可通过，增加显式关门并把资源准备位置放回房中；⑥死亡尚未成为ghost就调用复活，改为等待ghost并检查返回；⑦Lua userdata包装不等导致关系遍历溢出，按同步观察中的原生索引／InitSeed去重；⑧误把堕化魂写成36，按本地Wiki改为40；⑨Forgotten另一形态没有健康，补真实SoulHearts后切换通过；⑩原场景全部通过，但临时目录检查混用普通路径与`\\?\`前缀，已统一规范化；收尾复读第⑧／⑨次发现Lazarus退休崩溃，dump与原生递归清理链一致，修复上述移除顺序；⑪全量续测与新增退休场景通过，自动清理也成功。中间三次生成native崩溃文件（④、⑧、⑨），不改判为成功；最后一次无新增dump。

用户随后提出生产不能启用录制／重复读取等开销，完成下节修改后又在同一tx续测两次：第12次显式诊断配置、第13次生产配置的全部输入／角色窗口、生命周期、独立退休与正常退出均通过，统一入口退出码均为0。生产配置额外检查无逐tick/query/持久入口明细日志、无世界录制文件、Hook明细不可用；两种配置都实际复用了拓扑。没有新增测试编号或单元测试。

全部13次启动的部署／环境恢复检查均无错误；最终存档变化未越出D18/D20/D27约定范围。中间的额外持久文件为上述三次崩溃产生的dump及txt，已连同本次全部临时目录、对应boot日志清理；没有保留测试artifact，也没有回滚存档。历史同号材料未动。

优化前观察：60个Manager帧区间估算，录制开启的四组八身体约19 FPS，后续多形态约21—25 FPS；关闭录制后、剩余五身体的区间约60 FPS。第11次四参与者／七活动身体、录制关闭，1.5874秒内69帧，约43.5 FPS。这些场景并非严格同配，不能把差值当作录制的精确占比；它们指向观测与控制热路径的持续开销。旧实现每个consumer重复读取所有控制组，WorldPlayerAnchor／WorldRefreshAnchors还复读拓扑，ReadMemory逐字段执行VirtualQuery，并叠加录制、Lua观察与trace。上述数据保留为修改前事实。

## 生产默认与C++／Rust边界

当前原生字段、控制拓扑和录制快照由游戏进程内C++主线程读取；录制写盘在C++后台线程，Rust只做E2E输入远端、编排和离线分析。现有外部控制入口是本地`cs world ...`命令／可选控制口，不是把原生Player指针暴露给Rust的接口。后续正式同步应对接拷贝后的不可变状态快照，采集、网络分发与可选调试录制分别启停；当前tx不声称已经交付世界下行接口。

`mod/config.example.json`与C++缺省值明确关闭`world_record.enabled`、`diagnostics.enabled`及`diagnostics.repeat_topology_reads`，控制口也默认关闭。普通日志只保留低频启动、会话和错误信息；高频事件在格式化／计时／入队前过滤，停用逐查询分桶、逐Player诊断字段读取与Hook明细字符串构建。生产tick提交不依赖诊断日志成功。配置只在启动时读取；测试夹具与Rust harness不进入正式模组内容。

完整拓扑仍在必要的Manager／权威／生命周期边界确认。稳定consumer及同身体更新返回时，仅在同Manager轮次和资格epoch复用快照，同时轻量检查当前身体的类型／角色／生命／控制器／四个关系、设备可用性及全局房间／加载／暂停／焦点字段。任一变化、未知对象、阻断或生命周期作用域都不能复用，立即退回完整读取／中性；不缓存权限页或跨代对象，不转赠更新资格。调试人员只有显式启用diagnostics后才能另外打开重复全图核对。

修改后第12次（详细诊断仍启用、录制已关闭的短窗口）四参与者／七身体为91帧／1.5181秒，约59.9 FPS，该会话4977次快照复用；第13次生产配置为91帧／1.5162秒，约60.0 FPS，该会话4970次复用。完整图读取次数分别338／335，均为会话累计，不当作单窗口次数。两种配置均通过全功能续测；没有做全关卡、重负载或逐函数性能专项，不能外推稳定整局60 FPS。
