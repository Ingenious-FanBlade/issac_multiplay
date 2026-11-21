# tx-04-21 I21—I25：正式路由、网络冷构建与原生范围提交

日期：2026-11-05，Asia/Singapore。同一 tx-04-21 继续实现 I21—I25；统一运行验收仍由全部实现完成后的 I39 执行 P2 全 E01—E26及子用例→P4 全套。本轮没有测试、游戏启动、逐项试跑或新增单元测试，不能把实现或纯构建当作实时网络运行通过。

## I21：准备与路由交接

`net/live.*` 保留稳定菜单连接。正式 `ScenePrepare` 在旧路由可靠发送，声明真实世界、下一 route、固定 manifest、绑定 generation、原生／保留域和具名能力。Client 主线程先关闭输入，撤时序／边沿／worker Intent 和冗余历史；已有原生前缀须真实退休，再接受待准备事务。

Client 在旧路由反复发送同值 `ScenePrepared`，Host 验证后保留一个有界旧路由确认重放记录并切换正式路由；Client 收到同值 echo 才切换，向主线程发布 `Prepared`。旧通道只接受该小确认，不扩大旧世界 lane 的接纳资格。早到的新路由数据依靠既有补片／完整动态重发收敛。准备超时、取消和断联走实际 Scene 退休；不能把等待身份中的占位 run／room 当作正式世界。准备只表示双方已采用 route，尚无原生 Baseline 提交。

Host 仅在自己的 Publication Active 时授予远端输入；主线程还要求 DisplayReady、新 route 的实际 TickProgress 与当前 binding。旧 InputAck、排队控制和历史输入不重新贴标签。准备期间的退休进度只在实际步骤／owner 数变化时报告。

## I22：网络 Baseline 冷应用

新增 `game/client_stream.*`，由唯一 `ClientScene` 持有。`AdmitStream` 再查 scope／route，按固定 CaptureLayouts 接纳自有值；缺依赖有界等待。`ContextReady` 只确认纯值接纳，不产生 Applied。

固定 manifest 的完整闭包到齐后，`ColdModel` 仅从网络不可变组件生成一次冷目标。Scene 比较完整 Entity／Player／Grid／Room 成员与 shape，再进入已有 18—20 的全目标预检、构造、资源、关系、回读与唯一显示提交。产品路径不读 fixture、录制、Rust 或导入文件。执行中固定在途目标，后续动态只更新尚未执行候选。

冷数据提交后释放接收仓库中的 cold-components 和 manifest 大闭包；主线程保留必要元数据与实际 A，网络内容寿命不替代 native owner 退休。

## I23：同一 Scene 的完整动态步骤

`replica/stream_materialize.*` 只沿精确不可变依赖组合，分清局部 typed table、graph_ref、普通固定槽数组和显式状态。提取主体前闭合封套内引用，合并时重编号并重定向别名；不从可变旧 A 继承省略值。

普通范围按主体编译 Entity／Player DTO；Room／Grid／Level／Run 使用局部四域 DTO，不展开十一域完整模型，也不对每个动态包调用全 Scene AdmitTarget。逐身体路径解析自己的公开字段、实际绑定和具名资源依赖；普通联合私有值与独立身体样本按真实 tick 选择，旧普通值不能回退较新的身体公开值。共享心量／货币等 Getter 保留为事实，不冒充该身体的 backing；私有最终值仍由普通联合源安装。

原生安装只在 Manager、已核 Graphics／NativeApply borrow、真实 CPU 队列退尽的完整窗口进行。全部字段／资源／关系及回读完成后才恢复显示与提交该范围；逐身体提交不推进整房 revision 或完整成员。需普通成员提交的 flag 变化在写前等待，不能借快写修改 EntityList。已开始写入的异常撤显示，保留实际步骤并交后续失败退休合同，不发 Applied。

ANM 已加载文件／图层图像 pair 可跨帧／动画复用，先检查目标动画和全部 layer／null selector，再安装精确相位并清事件 mask，不运行 Update。数量相同才用 reset=false；数量变化用已核 reset 路径。FX／ray／vein／poop 同布局直接写已有数组，静态 Image、surface 和 ANM 不每帧重建；布局或资源身份改变转 I24。地图 surface 只在具名来源事实变化时重建。

同身体快组包括 main、13个已核物理ANM槽、服装姿态／公开动态值及 head_animation，写前逐一确认资源可复用；config身份、服装顺序和15项层映射沿精确ordinary依赖。只读 HUD／History／查询根随真实提交更新，不继续暴露冷目标的旧数据。29/38 绘制跨读另一身体，单侧快源报告 gap，仍由合格完整ordinary组合更新；不把本侧见证转给backup，也不声称这两类独立快写60Hz。详见 [Luna研究及主智能体复读](tx-04-21-i23-native.md)。

这些接口已接线，**不是 30／60 Hz、CPU、内存或带宽实测结果**。

## I24：结构、资源与关系切换

Structure／Resource 版本变化，或原生布局不能复用时，先用完整普通闭包构造可独立解释的 T，完成全部既有域预检，再撤旧快候选和借用、真实退休旧 owner、重建同一原生 Scene。稳定 Key 与本地实例／资源代次分开；先取得全部实例再绑跨域关系。没有 Spawn／Morph／Flip／Fire 等玩法 RPC。

这里采用完整安全重建作为结构切换策略，不把未解释的局部写入伪装成可复用。旧资源仍按实际 CPU／GPU／Lua 使用退尽条件退休。新结构完整提交前关闭整 Scene 展示，随后才接受依赖它的动态。I26 的同连接失败自动恢复和 I27／I28 的更广收敛／停止仍待继续。

## I25：真实提交与精确确认

新增严格能力 `native-range-commit-v1` 与 `world-dynamic-layout-v2`。AppliedAck 的每个 result 明列 version、attempt、instance／resource generation、实际原生域／纯值保留域和事件水位；顶层 versions／域并集／水位必须与 results 一致。接收、解码、ContextReady 与原生提交分别记录。

动态每 50 ms 有界聚合；Baseline／结构完成立即优先排确认，不能被新结构抢先。控制队列满时保留已提交不可变结果，成功入队才释放发送资格，不重做原生应用。可靠控制层负责丢确认重发，重复目标从有界提交缓存补同值结果；PlayerFast 只确认自己的具名子范围，不提升全世界完成度。

Host 保存每个精确已发送版本的依赖／域／水位事实，先核整组确认，再推进。未发送、错误世界／route、超出范围、伪造高水位、同版本不同提交事实及未确认旧版本倒退均拒绝并保留首因。Baseline 必须完整冷闭包已发送、真实域位和水位精确一致，才进入 Active。网络引用释放与 native 资源退休分别进行。历史事件抑制水位只在 Baseline 真实提交后安装；独立事件仍需实际消费，不能按最大已见 ID 确认。

## 来源修订与验证状态

普通分层来源补回完整身体公开后态，避免结构重建缺字段；`stream.world.v2` 的第二个字段保存十一域真实 capture scope／sequence／tick／owner／mutation。重组结构时不沿用冷 Baseline 的旧 capture scope，也不靠重贴 tick 假造同窗。原始 `source_scope`／canonical_state 及身体 History／连续性按实际来源保留；equipment 的静态布局与动态计时在纯值层重新闭合。

Release 的 `zhlIsaacCS` 全量干净编译／链接通过；没有运行测试或游戏。初始 MSBuild 再生成／文件跟踪调用持续无输出并被中止；改用任务本地 `CMAKE_SUPPRESS_REGENERATION=ON`、单进程 MSBuild、`TrackFileAccess=false` 后进入实际编译。此调整不修改仓库构建规范。编译暴露并修正了 capability迭代器语法、旧Decode重载冲突、纯值前置声明／RapidJSON allocator深复制，以及SDK C++17的Key比较。接口编辑与增量编译交错还出现过旧obj的未解析符号，最终以完整干净重编处理全部头文件依赖；这些失败没有记成测试结果。

静态复核另修正了四域局部模型被旧完整Baseline解析器拒绝的问题：显式 `RoomGridModelMode::StreamValues` 只允许room/grid/level/run各自完整、同窗的局部DTO，`publishable=false` 和真实missing_domains保持局部语义；完整Baseline入口仍要求十一域。Effect136搬起描述使用本次Grid来源，不能从旧A拷贝。待新原生步骤的Graphics／成员资格不阻塞此前已提交ACK的交付。Live能力明确广告Entity/Grid/Room/Player具名applier；纯模型能力仍不声称具备原生应用器。

I26—I40 和 I39 统一验收尚未执行。既有 18—20 的历史结果与本轮静态结论分开保留；没有四端实测、无失败自动恢复通过、无可玩性或实时性能通过声明。
