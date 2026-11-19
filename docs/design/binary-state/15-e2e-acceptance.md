# 15：E2E 适配与统一整体验收

**2026-11-20 玩家幽灵／复活修正，待复测**。用户报告任一玩家复活可使 Client 崩溃，Host 死亡后客机还会保留死前的血量并缺少幽灵。Client 23080 转储在原生 HUD 心量更新调用 `HasCollectible` 时访问空道具计数表（J273 RVA `0x369E7F`），同期日志记录普通 Body 拒绝 Player 主 ANM2 文件变更。块编码 Body 现可用自身的完整 E4／E5／E6 安装主精灵。单独 Body 失败保留 World 实例，真正退休前清除 HUD 绑定。Host 观察原生幽灵化与复活正常返回，在同一 `run/local/use/shape`、InitSeed、实体分类和预期幽灵位仍成立时允许该实例提供当前显示状态。复测覆盖 Host／Client 各自同房死亡、幽灵移动、复活和原生换房，核对幽灵外观、心量及仍存活玩家的画面推进。

**2026-11-19 Boss 房活板门换层崩溃修正，未续验**。用户实测 Host 从 Monstro 房经原生活板门进入第二层。Client 转储记录 `0xC0000005`、J273 RVA `0x343913`，调用链为 `HUD::PostUpdate` → `StatHUD` → 恶魔／天使房资格查询，访问了空 Boss 房配置的 `+0x38`。C++ Hook 原先以返回地址筛选原生调用，ZHL 中转使筛选不成立。修正后，客机在已安装 World 的 `HUD::PostUpdate` 作用域读取 Host 同步的资格值。转储前另有两条 `binary_live_object_replaced`，与最终异常的关系未确定。按用户指示仅构建，未运行换层或 HUD 实机测试。

**2026-11-19 Found HUD 与 Client 地图续验**。布局 86 在 World 的 Player P1 块增加六个原生属性输入，沿用布局 84 的块编码。两端的 StatHUD 仅登记本地控制组的身体。隔离 P2 固定种子 `S227 7MNF` 中，双方同时显示两名角色的生命栏，Host 属性栏只有伤害 4.25，Client1 属性栏只有伤害 5.75。Client1 按 Tab 展开和收起地图，Host 保持小地图。首轮客机 Tab 失败，因 ZHL 调用层使 C++ Hook 读不到原生调用返回地址，改为用已安装世界的 `Minimap::Update` 作用域后通过。另一次附加雅各／以扫和里拉撒路混合换形中，Client1 对成员 49 的布局 7 字段 0 报 `binary_native_special_write`，随后在原生 ESSM 清理时退出。该换形未通过布局 86 续验。普通双人异值复测未发生端点错误，客机正常退出并完成原生退休。没有运行 P4。

**2026-11-19 验收状态**。按用户确认，E01、E02、E07、E08、E10–E15 标记完成。E07 的缺资源和旧定义两项故障注入按用户决定跳过，历史记录中的实际覆盖范围保留。E04–E06、E09 经布局 85 的 P2 实机短窗续验通过。

**2026-11-17 最终一致修订，布局 84 续验中**。[设计第 11 节](eventual-consistency-revision.md#11-第-15-步验收调整)为当前口径。E02 的成员目录改为随基线帧携带，E15 的失败判定改为单根隔离、连续失败阈值恢复，ACK 在接收并保留快照后推进。下方早期安装成功 ACK、普通 joint 和整批写入失败条目保留原时点记录。

### 2026-11-18 布局 85 的 E01–E15 续验

实机采用同机隔离 Host／Client1、种子 `S227 7MNF` 和布局 85 Win32 Release E2E 包。源树默认布局仍为 84。首轮布局 85 开局卡在输入法，重新启动后完成原生开局。后续按用户要求将功能观察窗口缩短到 10 秒，性能及压力复测窗口不超过 30 秒。此前的完整长窗口按实测时间保留。

- **E01** 同版本双端完成协商。真实 85 Host 与 84 Client 首轮在菜单拒绝，报告 `unsupported_capability`。修复后复测报告 `binary_layout_version_mismatch`，Client 应用计数为 0，Host 未开局。
- **E02、E14** Client 从菜单经真实网络获得基线并安装原生世界。定向丢失首份资源目录（transfer 280，kind 23，lane 3）后仍安装 World 1，进入 Displaying。覆盖冷准备及开局后数秒的客机累计双向记账为 428,174 B。冷准备的逐段计费、时限及其他准备／取消用例仍待核对。
- **E03** 四身体、8 个移动敌怪及至少 16 个攻击实体的 180.58 秒窗口内，Host ordinary 29.98 Hz，Client World／Body 发布 25.86／51.36 Hz，World 安装 3,794／4,669 帧超过 4 ms。客机栈采样定位到服装目录逐字段重复按名字查找，改为直接比较生成表指向的值。8 Gaper 加 64 移动泪弹的后续窗口里，进一步将平铺纯写段的安装作用域检查收敛到该段入口，原生回调仍撤销检查结果。28.67 秒复测的 Client World／Body 发布为 29.93／57.52 Hz，Body 安装均值 0.68 ms，World 安装均值从同负载先前的 13.14 ms 降到 7.37 ms，仍有 857／858 帧超过 4 ms。Host ordinary 采集均值 5.26 ms。E03 未通过。
- **E04、E08** 两组普通身体的真实物理移动向相反方向作用于各自身体，Client 报告移动、射击非中性值，Host 的普通与 odd 原生模拟路径均出现。炸弹、主动、卡牌、Drop 各在有效输入窗口观察到一次处理，分别采自两次运行。失焦时输入门限为中性。相反射击释放回退、暂停恢复和各身体的完整消费对照仍待覆盖。
- **E05、E06** 激光及炸弹破格夹具运行时 Client 继续提交。Host 房间由 84 变到 97 时，Client 场景代次由 2 变到 4，并恢复在新房推进。两端实际画面均见相同激光和房间布局。Host 真实拾币使共享余额 10→11，夹具随后使资源增加、归零和金币 0→1。末次截图双端 HUD 的金币／炸弹／钥匙均为 01／00／00。该窗口同时发生换房，最后变化后静止的收敛时限与其他实体逐项对照仍待核对。
- **E07** costume、Weapon、Jacob／Esau 形态、Lazarus 备用和 Forgotten 换形后，Client 都继续在原生场景应用。混合角色接在这些动作之后时 Host 留有旧备用身体，ordinary 停在 5,095，Client World 停在 618 后重复发出 Base 恢复请求。用户实际看到画面不一致。保留源侧失败及失败现场，补控制组资格诊断，混合用例改为独立新局。缺资源与旧定义矩阵仍待验。
- **E09** Host Lua 公开观察返回一次 Complete，Client 同期继续显示。当前同版本 Host 二进制、Lua 和 Client 活对象三方逐字段对照仍待执行。
- **E10、E11** 代理分别实际命中动态 World 单包丢失、重复、延迟 80 ms、接收语义 ACK 丢失，以及三份七片基线的首、中、尾片丢失和单条 ReceiveAck 丢失。各次后双流继续确认。N1 120 秒实际丢包 168、重复 341、额外乱序延迟 685，无代理队列溢出。逐片重传和严格时限的完整对照待补。
- **E12** 两秒下行中断实际丢 264 包，72 个上行包继续转发，Client 发出恢复请求后继续显示。8 Gaper、64 移动泪弹和四身体的 30.001 秒下行限速中，代理实际转发 1,881,711 B，最长排队约 1.47 秒，无队列溢出。其后 60 秒恢复观察结束，Host ordinary 从 1,553 推进到 4,248，Client World 从 1,450 推进到 3,318，无新增本地恢复或断线。三客机公平性待测。
- **E13** 独立命中旧路由 World、旧路由 ACK、错误连接 World，当前场景继续应用。具名的坏段数注入实际命中一次及三次，均继续显示。单坏帧和连续坏帧的准确恢复归因、剩余 epoch、transfer 和冲突字段继续保留待测。
- **E15** 首轮三次故障误分散在 Player 的 Body／World 记录，未触发连续阈值。修复注入限定到同一泪弹的 World 记录。复测一次对象写失败后两流继续推进，无基线；同一泪弹三次失败触发一份完整基线，Client 持续显示。Host 单根泪弹采集故障实际命中一次，其余帧继续应用。完整引用晚到和其他故障边界仍待核对。

**2026-11-19 短窗复测**。独立新局的混合组有 2／1 个活动身体，Host 与 Client 的 World 确认和原生发布继续推进到 314，双端截图的角色、攻击、敌怪、房间和共享 HUD 相符。串行换形后的旧备用身体场景仍保留为失败。E08 采到左射→右射→释放右键回退到左射；Host 暂停态 1→0，暂停时 Client 输入资格为 0、发送序号保持 4,653。E13 单坏帧命中一次，联合基线增量为 0；连续三次坏帧命中后增加一份联合基线。两段继续应用且无端点错误，Host 的短窗追踪无丢失，未直接记录具名 Resync 请求。现有配置仅有两个可启动实例，四人扩展未运行。

**2026-11-19 E07 连续换形修复与续验**。布局 85 同房依次执行换装、换武器、雅各／以扫、里拉撒路 Flip、遗骸换灵魂、混合组。修复前复现 `unowned_shared_controller`，Host ordinary 停在 1,051。原生名单保留旧里拉撒路身体，它单向指向已经和以扫双向配对的雅各。Host 现在按该原生关系保留旧身体的名单和显示状态，并将它标记为非活动成员。首次复测还发现 Flip 后新锚点资格未就绪时，Client 因 `client_plan_anchor_player` 退出。Host 现在跳过控制组锚点未就绪的候选帧。

两处修复后的同序复测通过。混合阶段 10.32 秒内 Host ordinary 219→529，Client World 实际发布计数 200→508，场景保持 Displaying，无陈旧或端点错误。双端原生名单按槽位均为 19／29／29／20，雅各与以扫的双向边以及旧身体回指雅各的单向边一致。两组活动身体数为 2／1，旧身体无输入资格。E07 的缺资源和旧定义仍待实际注入。

**2026-11-19 E04 短窗续验通过**。布局 85 隔离双端实际发送相反的移动和射击输入，分别覆盖普通 1／1、双雅各 2／2、混合 2／1 活动身体。双雅各组中 Host 的两具身体均向右约 148 距离单位，Client 的两具身体均向左约 73 距离单位。原生泪弹的 spawner 覆盖各应射击身体，两端停步位置、身体关系和资源读值一致。首轮夹具对已经是雅各的身体重复调用 `ChangePlayerType`，原生额外生成一个以扫并触发 `unowned_shared_controller`。修正夹具后同序复跑，混合段 Client World 621→882，无陈旧或端点错误。

**2026-11-19 E05 短窗续验通过**。两名角色各发一枚原生泪弹，双端同一 Fatty 的生命值均为 6→2.5→−1，死亡血效进入双方绘制列表，随后敌人离场。同一激光实例在双端出现、绘制和移除。炸弹破坏格子 69 后，双端岩石状态均由 1→2。首轮 Client 仍将爆炸后的炸弹留在原生绘制列表。诊断轮捕获 Client 摘掉原实例后，Host 又从暂存名单下发了该炸弹，Client 用了另一个池槽。正式采集改为保留原生暂存名单的复核，停止把其中的条目单独发布成世界成员。修复后两端炸弹最后一次出现的样本时刻相同，窗口末活动非 Player 实体均为 0。过门夹具改选可通行的普通房门，实际按键使两端均从房间 84 进入 85，Client 场景代次 2→4 后继续显示。

**2026-11-19 E06 短窗续验通过**。布局 85 双端原生名单均观察到同一组金币、炸弹、钥匙各一件的出现和移除。两人共享余额依次为 0／2／0 → 1／3／1 → 4／4／3 → 0／0／0 → 1／0／0。具名生成的拾取物 212126 在双方绘制列表出现，Host 显式删除后双方都移除该对象。归零时双方活动非 Player 实体均为零。最后一次余额变化后，距操作前取样约 87 ms 时已读取到双端相同终值，后续约 8 秒保持静止，Client World 1,046→1,291。

**2026-11-19 E09 短窗续验通过**。布局 85 隔离 P2 双端观察约 10 秒。两台实例通过授权的 `cs stream_test <token> e09 on` 采样，按会话、场景、源修订、游戏帧和成员身份对齐已发布记录。比较覆盖两名 Player 的实际移动和射击、共享资源、Fatty、泪弹、拾取物和岩石。5,226 条 Client 样本均匹配 Host 来源。World 33 个具名字段比较 29,712 次，Body 8 个具名字段比较 2,912 次。Host 公开 Lua 读值另完成 11,398 次同帧或同一原生 PlayerPost 时点的对照。Client 原生绘制实际使用了可对照的 95 个 World 和 183 个 Body 版本。无追踪丢失或未解释差异。标记为沿用旧值的 5 处可见字段差异均在后续 15 个游戏帧内收敛。

修复前，同一 World 安装在写入拾取物的 `exists=false` 后，将该池槽误判为失效并在写动画字段时拒绝该对象。World 现在可继续更新仍由当前名册持有、有效位及完整形态均相符的池槽。复测的拾取物种子 212124 先读到 `exists=true`，在第 216 帧读到 Host、Client DTO 和 Client 原生槽的 `exists=false`，第 219 帧双方名册均已移除该成员。Client 保持 Displaying，无对象拒绝。实际覆盖范围为上述 P2 夹具的具名标量及绘制版本。

**2026-11-19 E39 实机续验**。布局 85 隔离 P2 初次装入成功。首轮 Host 的普通门、淡入淡出、像素化和矿车进入不同转场阶段，客机收到了换房结果，但没有渲染对应的中间阶段。Host 的普通二进制采集要求本次 Manager 有 StandardRoom 玩法节拍，原生转场分支没有该节拍。现从正常返回的 Manager 尾部采集具名转场 World，沿用最近已提交的玩法 tick，场景和显示字段随同一 World 发布。

首次复测开始下发了原生转场字段，但客机在首个换房的 Prepare 阶段因 `binary_prepared_identity` 退场。同一格 83 的滑动计数 1→7 期间，Host 的房间身份从 3 连续增长到 9。源码原将每次 `RoomTransition::Update` 包为 World 生命周期边界，现由实际换房和新房入口负责撤销旧身份，保留原生转场调用的输入作用域。普通房的复测使用 Host `debug 10` 快速清怪，Boss 段恢复原生战斗条件。

第二次复测，格 83 滑动期间房间身份稳定为 2。客机在新房接收阶段仍因 `client_plan_roster_gap` 退场。转场中还没有新的 PlayerPost，Host 把同一原生名册中已随房间撤销输入见证的 Player 当成了未就绪对象。修复候选只在转场 World 内读取当前名册中已正常更新过、实例和形态一致且 InitSeed 相符的 Player。遇到仍无法读取的名册成员时暂缓整份转场 World，等待下一份完整 World。

后续复测还捕获了 `client_plan_player_membership`，同一 Player 恢复字段读取时未写回其已初始化的显示资格。补齐资格后，84→83 的首次换房持续 Displaying，未再断线。一次 520 ms 的本机下行中断实验至少丢弃 26 个包，客机在旧房保持 World 20 并继续绘制（78→93 次），恢复后继续六次换房并达到 World 117。该轮同时保留两个失败证据：滑入时旧画面曾被新房重抓，Boss 结束处有一个同源阶段样本差异。

直连双端通过 `stage 2c` 进入原版 Downpour II，再在格 84 以动画 18 实际往返维度 0→1→0。客机最终保持 Displaying，World 254，13 个同源绘制样本一致。此轮仍需区分各阶段实际使用的旧／新画面来源。

旧画面补验发现，Host 可能先以 `binary_host_source_retired` 退休旧来源，再通知客机准备新房。客机现等在途票据和借用退尽，在拆旧 World 前保存本地旧画面，并在新 World 装完后接回同图形上下文内的强引用、捕获新画面。Boss 结束帧的差异来自资源未就绪时 Game 根被整条沿用。资源沿用现在限于实体和 Grid，结构根等完整目标。布局 85 直连续验连续六次换房，覆盖四个方向、门滑动、淡入淡出、像素化、矿车和自然 Boss 开场。188 个同源实际绘制版本全匹配。首个换房同一游戏帧越过 mode 1，客户端仍先捕获格 84，再捕获格 83，滚动补偿为 −142。Boss 标记 44 的 93 个样本为 mode 2 且 HUD 隐藏，结束后恢复可见。

同房动画 11 的 19 个实际绘制样本一致，结尾仍在格 84。动画 16 已记录 Host 原生请求与完成，仍在格 84，阶段短于本轮绘制采样。Hourglass 从格 83 原生回溯到格 84，游戏帧 49→39，但原有 Restore 逻辑触发 `RunTeardown`，客机收到 `server_disconnected`。窗口调整实验在像素化 mode 2 期间将客机窗口从 782×511 调到 974×535，原生渲染宽度从 480 变为 600，输入泵之后的显示推进仍使用旧资格，客机报 `client_display_schedule`。本轮先保留这两个失败。

Hourglass 的具名原生换房作用域现保留本局会话，并照常撤销旧输入绑定。复测 Host 游戏帧 48→38，客机从 83 回到 84，随后接着进入 81 和绘制 Boss 演出。另修复 Host 发送前等资源表 ACK 时逐根沿用结构域的路径。复测同房、Hourglass、后续 Boss 共 148 个同源实际绘制版本一致，Boss HUD 由隐藏恢复可见，最终 World 188。

尺寸调整现在在输入泵之后复核。像素化 mode 2 中窗口从 974×535 调到 1166×559，客机请求完整恢复，按 720×326 重建两张本地图，继续完成 84→83→84，62 个绘制样本无差异，最终 World 215。另在像素化 mode 2 主动断开。客机进入 Off，原生对象数为 0，线程和场景都完成退休。使用同一隔离客机进程重连仍在运行的 Host，装入当前房间并推进 World 1→223。

**2026-11-19 E39 过门等待画面复测通过**。旧 World 退休后、新 World 提交前，原生 Manager 会短暂进入菜单绘制分支，触发 Lua 的 AwaitingWorld 提示。现在同局换房有有效旧房纹理时，Manager 在该分支绘制旧画面，继续走原生图形提交。首次连接仍走菜单等待。布局 85 隔离双端用真实移动输入连续经过 84→85→84，第二次在换房控制消息后对新 World 设置 700 ms 停发窗口，实际丢弃 21 个状态数据报。两次分别有 15、55 帧在菜单绘制分支提交旧房画面。等待期截图为旧房，HUD 随新场景恢复，未出现 AwaitingWorld 画面。5 个同源绘制样本无差异，客机最终在房间 84 持续 Displaying，World 222，无端点错误或陈旧，两台实例正常退出。此前迷宫诅咒生效时的 8 次实际过门都完成，未命中随机改目的地。按用户指示先搁置该场景，本次测试在过门闪屏通过后结束。

**2026-11-19 E39 普通门淡入淡出通过**。布局 85 隔离双端实测 9.78 秒，Host 实际按键走门 84→85→84，仍使用原版滑动。客机按 50／70 ms 的本地时钟逐帧淡出和淡入，首帧全黑分别在 65.1／64.5 ms，首帧完全显露新房分别在淡入开始后 85.1／85.7 ms，受显示帧间隔影响。客机窗口中心区域在等待阶段采样为全黑，换房前后都有正常房间像素。双端继续同步，客机处于 Displaying，World 248，无端点错误。双端正常退出。本轮验收覆盖实际普通门往返。

### 布局 84 首轮实机

- 隔离 P2，种子 `S227 7MNF`。双端完成首次普通世界安装。Host 切到两个雅各与以扫控制组，在房间第 174 帧一次生成 300 颗泪弹。夹具记录 300 颗、四身体、零特效。
- 起量后 Client1 失去控制口。崩溃转储记录 `isaac-ng.exe+0x3b67b3` 的读访问异常，目标地址 `0xAC0`。四个 Player 先报 `binary_native_special_parent`，随后 Body 报 `binary_native_special_write`。原生额外生命路径在空 Player 集合后崩溃。本轮无有效稳态性能读数。
- 修复候选在较早的文件／动画安装阶段递归接入当前字段所需父容器，并拒绝无可用 Player roster 的 World 候选。单对象失败仍独立隔离。另用收包时刻计算 ACK 提交耗时，和收包至原生发布分别计时。

**2026-11-15 指定用例续验**。用户恢复第15步，先执行 E04–E07、E09–E15。实机采用隔离 P2 双端，从布局79开始。

- E04 普通双人场景。两端分别实际移动和射击，用户确认表现正常。Host 为两个控制组各登记一个活动身体，Client1 发送水位及 Host 接纳、处理水位均到924。双体、混合组及 P4 待验。
- E05 普通门切房。首轮重现 `client_plan_transition_scope_or_completion`，Client 短暂 PreparingWorld 后退回标题。Host 改为提交原生 Start 返回后的已接受维度，并拆开 Client 失败原因。第二轮普通门复测中，Client 短暂 PreparingWorld 后显示新房间，场景状态为 Displaying，无 Client 错误。Host 控制代次从1增到2，并收到新房间的 World、Body 确认。战斗、炸弹／Grid、Laser／Knife 等子场景待验。
- E06 道具拾取。第二轮 Host 拾取 Big Fan（5.100.279）后，ordinary 停在2549，gaps 继续增长。Host 首因为 `ReferencePending(20)`，道具横幅生产类型18，Client 停在最后一次已安装的画面。横幅改用原生入口保存并在返回时核对的存活 Player `local/use`，复用现有转场事件的身份资格。第二轮两端已排空并正常退出。
- 第三轮切房时 Host 首因 `binary_confirmation_before_prepare`，Client 退回标题，拾取复测未到达。Host 修复已退休旧世界、传输路由尚未切换期间的迟到 `ContextReady`、`AppliedAck` 和 `Resync` 处理。第四轮切房后继续发布，Host 拾取 Piggy Bank（5.100.227）后，Client World 计划在 route 3 报 `client_plan_global_player_topology` 并退回标题。Host 已按身体分别生成修订号，Client 改为逐身体校验 P6 与 P0 修订号，结构变化仍走两流 joint。
- 第五轮 Host 两次拾取 The Sad Onion（5.100.1）。用户观察第一次 Client 画面短暂静止后跳到结果，无拾取过渡动画；第二次停在 AwaitingWorld 阶段。Host 采集持续推进，收到 World 2024、Body 4022、事件7的确认。Client 首个写前错误为 `binary_native_members_preflight`，route 2、ticket 4897，最终因 Host `binary_prepare_install_deadline` 断开并完成排空。已细分原生成员预检的拒绝条件并构建诊断包，第二次拾取的具体失败条件仍待实机捕获。E06 未通过。
- 第六轮两次拾取悲伤洋葱，第一下复现静止与跳过拾取动画，Client 首个写前错误为 `binary_native_member_entity_type_changed`。第一次之后确认继续推进，第二次恢复最终耗尽 Host 准备接收预算，报 `binary_prepare_receive_budget`。第七轮第一次拾取的 E2E 原始值为本地 `5/100/1`、目标 `5/100/0`、同 seed 3006591295，证明已有实体承载收到不同 SubType，根因与此前泛化的成员预检不同。Host 现按同一生存代次的已观察道具底座 SubType 变化提升形态版本，由现有结构事务应用空底座；原生 Morph／Init 已处理的形态重新取样。待复测同一道具重复拾取、动画表现和最终确认。
- 第八轮 Host31248／Client23164 连续两次拾取悲伤洋葱，Host 日志记录两次实际道具加入，事件确认从5到9，World确认到2744，Client已安装到2747并仍处于Displaying，双方无首因。用户确认第一次画面继续跟随，第二次有拾取动画且正常。该重复拾取子场景通过，E06 的消耗品、变回／归零／显式空与最终静止时限待验。
- 第八轮后续触发宝箱 `5/50/1→5/50/0`，本地和目标 seed 同为156019603。Client 写前拒绝，World 确认停在4808，Host 最终报告 `binary_prepare_install_deadline`。用户要求扩至全部 Entity。普通主线程采集现记录同一 use 实际 Type／Variant／SubType，任一字段变化即提升形态版本，初始化、Morph、Restore 和新 use 按现有生命周期重新取样。Player 的独立 Body 读取发现形态差异时等普通联合目标。身份表预算沿用原 136B 记录限制。
- 全 Entity 修复版首轮（Host31772／Client45720）经本机 N0 代理开局，Host 放炸弹且随后打开控制台后掉线。代理转发7655个数据报，记录零规则命中、零队列溢出、零丢弃。Host 最后 World／Body／事件确认455／934／8，首因 `binary_prepare_no_progress`，Client 无具名形态错误，收到服务端断开。次轮直连（Host45464／Client41296）单独开启控制台约10秒，暂停位为1，World 保持346，关闭后二者继续推进，无错误；关闭控制台后单独放炸弹，Host 处理一次炸弹边沿，事件确认到8、World 超过2506，Client 持续显示。单项未复现组合场景掉线，暂未归因。
- 同一直连轮复测悲伤洋葱与普通宝箱，用户确认均正常，World 确认到5024、事件确认到22，未再观察到原生形态失配。该样本覆盖道具与宝箱原位变化，全部 Entity 类别的运行覆盖仍待完成。
- 直连战斗场景曾在连续切房后卡住，用户观察到 Host 受伤。Host ordinary 停在6421，显示事件 `incomplete=1`，两个控制组仍存活且绑定。已使状态页优先报告显示事件锁存首因，修正独立 Body 枚举误用旧普通读取窗口。随后的直连轮 Host10664／Client47200 发现独立的同门往返阻塞，ordinary 停在882，显示事件 `incomplete=0`。首因是 Grid `246/2`，`ready=0 candidate=1 owner=0 birth_kind=19`、Door vtable RVA `0x71966C`、`initialized=1`。J273 `MakeDoor` 在 `0x7E4F92` 调派生初始化，`0x7E4F9D` 设置完成位，绕开公共 GridInit。已补具名门资格，沿用现有房间槽、索引和对象身份复核。已构建，等待同门往返复测。
- 修复后 Host45168／Client8668 复测 `1.2→1.457→1.2`，用户确认同门往返正常，回房后两流继续确认，双方无错误。战斗房含 Attack Fly（18）与 Fatty（208）。用户报告击杀时 Client 短暂停住、跳过死亡动画并直接移除怪物；窗口内显示事件正常转交，暂未定位是本体姿态还是独立特效丢失。E05 战斗动画仍失败。
- 用户确认击杀时 Host 主要显示爆裂和血花，已加入 E2E 同 Host 帧的血效采集／本机提交观测。观测轮 Host29016／Client17284 在进入战斗前快速往返卡住，首因 `ValueBudget(12)`，`BootstrapSfx` 转场播放队列满64项。循环声停止／音量／音高改为在现有和待登记循环状态中生效，播放队列只留真实播放请求。复测轮 Host46592／Client4224 快速进出两次后音效队列未残留、事件21全部转交，另在 Grid `205/1` 的 Decoration 路径停住，`ready=0 candidate=1 owner=0 birth_kind=19`、RVA `0x7196CC`、`initialized=1`。J273 `0x81AB28` 直接构造后于 `0x81AB6F` 调公共 Init，另核 `0x7E4C32→0x7E4C71` 路径。具名补齐 Decoration 当前读取，仍复核当前普通房间槽及索引。已构建，继续复测快速往返，击杀特效尚未采到对照数据。
- Host20728／Client38104 复现快速往返，路线 `1.2→1.51→1.2` 连续三次。Host ordinary 停在781，音效等待和显示事件 `incomplete` 均为0，Grid `696/1` 首因是已初始化的 Rock 候选（`ready=0 candidate=1 owner=0 birth_kind=19`、RVA `0x71969C`）。回房候选合并为同一读取判定：当前普通窗口、已记录构造的同一 use、已知 Grid RTTI、原生初始化完成位及当前槽索引匹配。此轮 Client 也独立记录了 route 5、6 的 `binary_transition_destination_mismatch`。完成事件已有实际房间代次，现以此区分历史完成与当前完成；当前目的地按描述符 GridIndex 核对，与 Host 来源一致。两项待快速往返重测。
- 同轮击杀时，Host 与 Client 都记录到帧656的血雾实体 `1000/2`，帧657两端均为一个血雾加四个 `1000/5` 血粒子。用户仍报告 Client 没有血雾。下一轮在 Game::Render 原生输入处对照血效的渲染列表成员及 Sprite 资源、动画、透明度。画面尚未通过。
- 包17第一次启动时，Host 已跳过启动器正在等待的选档菜单并进场，Client 收到了世界状态，但启动器等待确切菜单超时，正常关闭双端。用户指示直接重开，Host43652／Client6908 实机进场。快速同门多次往返后用户确认 Client 不再卡住。Client 维持 Displaying，World、Body 和事件确认继续增长，未出现本轮的转场目的地失配。该快速往返样本通过。
- 包17击杀复测，Host 原生绘制在帧451、527、941 分别观察到 `1000/2` 全尺寸血雾，加载、动画和可见位均已成立。这些实例没有出现在已提交的 Host 普通快照，Client 绘制列表中也没有对应 seed。帧909的缩小血雾则于双端绘制列表中出现，seed 351019981 及首帧姿态相同。用户确认仍有部分怪物缺失死亡血雾。Host 另有根读取资格 `MissingQualification(8)`，旧日志缺被拒身份。包18在相同当前普通窗口补记首次被拒的 Type／Variant／SubType、seed、readiness 和创建来源，可与缺失帧对照。血雾问题仍待定位和续验。
- 包18（Host23060／Client45728）在帧1103、1166 复现两个血雾实体 `1000/2/2` 被 Host 整帧采集拒绝。实例 seed 1496572505、3287299829 分别与同一帧 Host 绘制精确匹配，均为 Factory 取得、无外层 Spawn owner、基类 Init 已返回、已登记实体列表。随后普通发布恢复，Client 跳过了整个血雾存活区间。J273 存在 Factory→派生 Init→当前 Room EntityList 的内联生成链。已补同一 use 的 `Entity_Effect::Init` 正常返回及首次原生登记见证。后续正常帧照常采实体与 Sprite 值，由 Client 原生渲染，修复待实机重测。
- 包19（Host16340／Client10296）两次全尺寸血雾均进入 Host 普通快照和 Client 渲染列表，帧2654/2655、2671/2672 分别匹配 seed 1672328607、1828565027，Client 首次绘制姿态为1，随后按真实世界帧消失；同期七个飞虫爆裂实例也在双端匹配。Host 普通缺口计数在采样期间维持开局后378，无血雾资格错误。用户确认 Client 有死亡血雾，击杀后画面没有停留。E05 的击杀场景通过，后续补测炸弹破格和 Laser／Knife。
- 同轮 Host 用一枚炸弹破坏岩石，用户确认 Client 同步。Host 处理炸弹边沿一次，World／Body／事件确认继续到4333／8465／112，Client 持续显示且无错误。炸弹破格样本通过。
- 同轮通过控制台测试硫磺火换装，Client 间歇闪回 AwaitingWorld。Host 普通采集从9348继续增长，网络确认停在 World5610、Body10997。Host 发布原因 `host_resources:image_geometry_pending`。Client 每约2秒按 `Base(1)` 重试。资源表原先要求仅有路径的图像等待同路径的 Host 实例出现，直接授予道具时该图像可以一直未加载。新增 ImagePath 描述供配置、历史栏及图层路径使用，Client 走本地 Image 装载并核路径和本地尺寸，实际 Host Image 仍走严格尺寸合同。互通布局升级80，换装和激光待复测。
- 布局80首轮发现资源分派已登记 kind10，资源头仍报 `kind:scalar_enum`，导致开局无世界。已补资源头枚举及生成时的同源一致性校验。下一次启动复用旧代理导致新客户端端口被代理忽略，改用新代理实例后 Host49064／Client14608 正常进场。用户确认硫磺火激光能同步，发射后 Client 卡住并退出。Host 发布资源无错误，首因 `binary_prepare_no_progress resync=1 lane=2 ready=18/10 offered=25/10`，最后 World 确认876。代理共转发13653个数据报，无规则命中／丢弃／队列溢出，但默认带宽限制产生队首阻塞，发射后上下行最长实际排队约1715／1966ms。准备直连重测相同动作。该轮尚不能判定发射后失步是否在直连复现。
- 同一布局80包直连 Host42028／Client48484，控制台授予硫磺火并发射复测通过。控制台关闭后继续推进，用户确认无问题。采集和发布无资源错误，Client 持续 Displaying，Host World／Body／事件确认到1571／2957／12。E05 激光样本及 E07 本次 Weapon／costume 变化通过；代理积压条件、E07 其余换装／资源失败用例另待续验。
- 同一局按原生实体类型拾取一枚金币、一把钥匙和一枚炸弹。用户确认三项均正常，包括 Client 的拾取显示及 HUD；Host World确认到4999、事件从12到18，Client 无错误。E06 消耗品样本通过。
- E10／E11 续验（布局80，Host29984／Client28076）。代理修正上下行共用队首的阻塞，保留原限速。代理实际命中 World 动态单包丢失、单包重复、延迟80ms、Applied ACK 丢失各一次，无同包其他消息受影响。四轮 Host World 确认按352→390→430→469→509增长，Body按702→779→858→937→1015增长，Client 持续 Displaying，无首因。可靠基线首／中／尾分片、ReceiveAck、完整Resync及后续矩阵仍待续验。
- 同轮 E10 可靠基线首、尾片分别独立注入。首次过门丢 transfer16329 的第0片（共2片），返程丢 transfer23447 的第2片（共3片），两次共精确补发2片，超期0，Client 回到 Displaying 且继续推进。中间片、ReceiveAck 与连续故障场景待续验。
- 同轮 E11 两秒下行中断实际丢329个下行数据报，上行仍转发73个。Client 在 route4 发起 `Base(1)` 恢复，Host World／Body 确认从5242／10483追到5452／10901，双端继续正常推进。另注入30秒下行62.5 KB/s，静止样本实际下行951594B（约31.7 KB/s），Host tick 从21167到23341，Client 持续显示。本段覆盖短中断恢复及静止限速，不覆盖 E12 的实际过载。
- 同轮 E11 的静止 N1 样本持续120.001秒。代理接收42071个数据报，随机丢221个、重复440个、延迟乱序867个。Host World／Body 确认继续到12344／24684，Client 持续 Displaying，无新增恢复事件。
- 同轮 E10 同门往返实际丢 transfer99090 的中间片（第1片，共3片）。Client 恢复 Displaying，World 应用版本继续推进，Host 累计补发5片、超期0。该补发计数包含本轮其他注入。
- 同轮 E11 独立丢弃 Receive ACK（transfer107577），无同包其他消息受影响。复核 Client 持续 Displaying、应用到 World832，Host 确认 World832／Body1663，传输超期仍为0。
- 包22直连（Host22604／Client18068）E15 两个故障各命中一次。写前 `binary_e2e_world_before_install` 拒绝 route2／票据1177，Client 继续 Displaying，Host 确认到 World555／Body1106。原生写后 `binary_e2e_world_after_install` 使 route2／票据3625 失效，同期独立票据3626撤销，随后双端继续推进到 World1322／Body2640，无未匹配 ACK。此次轮询没有直接捕获 reason5、完整基线和联合回执，待在网络线程观测并命中联合目标复核。其余既定写前／写后故障边界待验。
- 包23直连（Host36020／Client29760）切房时命中 route2 联合票据8475（World2878／Body5751），写后失效。Client 已在新 route3 恢复 Displaying。随后在同一房间独立注入：World1846 写后失效，Host 在 route3 实际收到 reason5、streams=3，报告历史成功版本 World1844／Body3687。约79ms后 Host 实际接受一份 count=2 的联合确认 World1848／Body3691，无观测丢失。该同路由恢复样本证明失败目标未被这份回执确认。切房联合故障的网络回执因诊断环溢出未捕到；两路 base=0 的发送值待直接复核。
- 同一直连轮开启 Host Lua 观测，实际完成12次观测调用，实体域报告 Complete，Client 继续 Displaying。该样本仅覆盖 Host Lua 采集可用性，E09 同版本逐字段对照待验。
- 包24（Host12196／Client20380）E14 初始准备阶段在代理确认丢弃 ContextReady transfer43，无同包其他消息受损。Host 首因 `binary_prepare_no_progress resync=0 lane=0 ready=0/0 offered=2/1`，Client 安全退回标题，没有安装世界。原实现只发送一次 ContextReady。现保留当前目录对并按确认周期重发，直到观察到同路由的状态帧实际引用它；待复测同一丢包点。E12 高负载未开始。
- 包25（Host22032／Client15796）再次实际丢弃第一条 ContextReady transfer43，无同包其他消息受损。Client 成功安装 World1、进入 Displaying，World／Body 确认继续到220／440且无错误。E14 的初始准备单条 ContextReady 丢失复测通过，其余路由／依赖矩阵待验。
- 包25 同轮 E13 定向改动三份独立数据报的外层范围：旧 route 的 AppliedAck（transfer9399）、旧 route 的 World（10193）、错误 connection 的 World（10198）。代理均实际命中且无同包其他消息受损，Client 继续 Displaying，Host World／Body 确认到2159／4315、未匹配 ACK 为0。本段未逐包核对接收端拒绝计数，也未覆盖其余坏字段。
- 包25 同轮 E15 同房间写后故障命中 World2694。Host 在原 route2 实收 reason5、streams=3（历史版本2693／5384），实际发出 transfer15181，World／Body 的 base 均为0，目标2696／5387。约55ms后接受与该基线完全匹配的一份 count=2 联合确认。诊断无丢失，未确认失败目标。此样本通过同路由的完整重建及联合回执检查，其余 E15 具名故障点待验。
- 包25 同轮 E12 Host 夹具实际生成8个 NPC 和64个活跃攻击实体。该窗口内玩家生命耗尽，Host 已完成 tick 在18006停止推进，Client 仍显示。代理限速30.000秒实际下行207391B，上行685306B，所测最大下行排队890微秒，无队列溢出。未形成持续饱和负载。过载夹具现只在低血量时补充魂心，保留原版受击，待重测完整时窗。
- 同次全员失去活动身体后，Client 多次发起 `Base(1)` 恢复，最终在 Host `binary_prepare_no_progress resync=1 lane=2 ready=4/3 offered=4/3` 后退回菜单。该失败晚于限速恢复，未证实为链路饱和问题，保留单独记录。
- E09、E12–E15 完整用例尚未通过。

- 本轮收尾时 Client 为 Off，worker／binary／scene／menu 已就绪，native owner／written 均为0。Host 停止后为 Idle、peer Retired、device Removed，隔离双端已正常关闭，代理已退出。包26已构建，包含 E12 夹具补血修订。待下一次启动确认后复测持续负载。

- 包26（Host7940／Client32776）跑完30.000秒限速和60.010秒恢复观察，夹具保持8至9个敌人及64个攻击实体，两组仍各有一个活动身体，Host tick 在恢复窗口继续到5874。限速期间下行530496B，平均17.7 KB/s，双向无丢包或队列溢出，下行最长排队4.536ms。Client 画面卡住，在同一路由累计46次 `Base(1)` 恢复仍未稳定；Host 反复报告 `ValueBudget(12)`，见 layout9/field3、layout2/field2，ordinary 缺口持续增加。该轮触及采集预算，未打满0.5 Mbps 链路。
- Host 原生日志在负载夹具 frame423 前一条记录 `unlock steam achievement '80'`。成就事件与高负载在同一窗口出现，未独立分离，暂无因果结论。用户在 frame3614 按 F8 停止补充负载，随后活动身体耗尽，Host 报 `binary_prepare_no_progress resync=1 lane=2 ready=3/2 offered=3/2`，Client 安全回标题。按正常路径排空并退出双端和代理。采集写入器补充了耗尽子区、用量及请求量诊断，不调整容量；修复与针对性复测尚待完成。
- 包27直连（Host43800／Client39852）在 frame516 重建8个敌人、64个攻击实体，Host 原生日志未再记录成就解锁。再次卡住，具体首因为 `arena=cells roots=166 used=32768 limit=32768 requested=1`，Host 最终 `binary_prepare_install_deadline`，Client 完成排空。直连复现证明成就提示和代理均非必要条件。普通槽值条目与后台索引已改为65536，按现有64 MiB共享额度计费，值字节区及线上额度沿用既有值，待用同负载实机复测。
- 包28（Host30040／Client40868）以同样的8个敌人和64个攻击实体开跑，后续原生效果增加后，在第331个根记录处达到 `cells=65536`。Host tick 继续到3253，但 ordinary 停在640、缺口增到1315，Client 同路由累计26次 `Base(1)` 恢复。限速阶段结束后根据本次明确容量失败转入修复，未将该轮记为通过。普通槽和后台索引再扩到131072，待复测。
- 包28 收尾时 Client 已排空，Host 的设备、Player 与网络 peer 后续均已退休。Host 停止回调等候 peer 超时，逻辑状态保留 Retiring，日志 `retirement_error device=1 player=1 peer=0`，其后网络报告 `terminal=Stop error=0`。两隔离窗口正常关闭，代理队列已清空。
- 包29 采用131072条后，旧64 MiB总额的 Host 冷准备拒绝 `binary_entity_shared_capacity`，游戏未进入负载。已把共享采集值额度调为72 MiB，进程总池及每 Peer、每条消息预算均沿用既有值，准备继续实机验证。
- 包30（Host9196／Client15628）以131072条及72 MiB共享额度正常冷启动，重建8个敌人和64个攻击实体。普通快照由455推进到3153，缺口计数一直为375，未再报告条目容量失败。限速30.001秒实际下行400005B，代理无丢包或队列溢出；恢复约50秒时连接终止。Host 后台反复报告 `host_publish:capture_revoked_during_encoding`，随后见 `host_publish:member_index_exhausted` 和 `host_publish:body_requires_current_ordinary`，World／Body 确认最终停在339／636，Host tick 继续到6400。Client 因 `binary_prepare_no_progress resync=1 lane=2 ready=18/9 offered=18/9` 排空返回菜单。两进程和代理正常退出。扩容复测覆盖了采集墙，E12 仍未通过，持续高 churn 的编码撤销与 run 内成员号耗尽待查。
- 包32（Host22356／Client34832）使用相同负载和30秒限速。确认编码中撤销的候选原本就会丢弃私有账本。旧实现在编码完成时永久登记编号，之后即使候选未发送便被撤销也不回收。修复引入快照目录租约，释放未发送且未被 Peer 待发事件引用的编号。实机复用11884次，索引高水位977，未再达到16384上限。Host 后台仍反复报 `capture_revoked_during_encoding`，限速恢复约51秒后以 `binary_prepare_no_progress resync=1 lane=2 ready=22/8 offered=22/8` 结束，World 最终确认372，Client 完成排空。双端和代理正常退出，E12 仍未通过。
- 后续源码仅对已识别且已退休的 Tear、Projectile、Effect 保留上一帧已封存的纯值候选，当前原生读取仍会撤销，下一普通帧按真实成员表提交移除。包33已构建，用户要求先停止，尚未启动实机。恢复测试时先验证同负载下的 World 发布，再验证原生移除和限速恢复。已上网成员号的回收仍待独立验证。

**2026-11-15 上次实机停止记录。** 第二十二轮 Host38420／Client32676 切房后，用户反馈“PreparingWorld，然后迅速切到标题主界面”。Host新房间采集已继续：ordinary从1052增到1532、Body从3393增到4353，gaps维持375未增加；因此该窗口未再复现此前Player引用或Grid资格阻断。Client首因发生在World计划阶段、route=3：`client_plan_transition_scope_or_completion`，继而`client_receive_terminal_candidate`并安全退回标题。失败检查位于`mod/src/state/client_plan.cpp:535`，同时检查转场序号、完成位、请求／最终dimension范围、完成位与final_room关系；本轮未记录该事件各操作数，尚不能断言具体哪一项不符。静态续查线索：原生请求允许负dimension表示当前维度，而计划要求0—2；需核实际事件值后修正来源／消费语义，不能直接取消检查。

第二十二轮Client停止后Off、worker／binary／menu／scene退休就绪均1，native owner／written均0；Host正常cs stop后Idle、peer Retired、device Removed。保留当前修复源码、`.scratch/step15-incremental-failure22.json`、本轮启动日志及`build/isaac-dual/step15-incremental-run22`／`step15-incremental-package22`供续查。第15步仍未通过，跨房间尚未跑通；下文“继续”“待启动”均是此前各轮历史状态。本次按用户指示不再修复续测或启动下一轮。

**2026-11-15 用户同意接纳修订并继续**：已按[第13步10.4](13-client-incremental-apply.md#104-普通body单槽等待与前代解码基线2026-11-15用户采纳)补齐设计，实施Body单槽替换、一个周期等待、一个成功前代解码基线，以及独立锁定实际应用base。提交前额外核计划所比较的当前根；joint／恢复／关闭清空等待与相应历史，ACK仍只取真实成功。接受合同整数版本78→79，构建成功并已启动第十五轮。用户已确认启动配合；下文“待用户决定”是上一中断时的历史状态。

第十五轮 Host19740／Client33420：首个25秒静止窗口World24.55Hz、Body59.91Hz，Body旧基线拒绝0，1500批以前代基线接纳；等待替换1、过期1。随后45秒窗口覆盖实际客机输入，发送／接纳／处理均增加743；World27.85Hz、Body59.39Hz，Body旧基线拒绝25，2460批以前代基线接纳，替换1、过期2，资源／预算／依赖等待均0。Body主线程排队平均9.51ms、准备2.27ms、安装1.66ms、接收至发布22.07ms。用户反馈“基本无断续”，询问延迟提高原因；已说明保留原先被丢弃的包引入有界等待，成功样本选择也发生变化，该数字不是按键到显示总延迟。新`own_stream`只表示到达时本流忙，包可能进入等待位，不再可直接算作丢包。错误为空、无恢复。正在请求Host切房往返以覆盖联合／生命周期清空；频率完整目标、结构变化与失败场景尚未全部通过。

第十五轮切房未通过：用户报告Client停在PreparingWorld；实测Host仍Active、参与者代次1→2，Client场景代次2→3且旧binary已经排空，但新world_payload_unavailable。Host显示采集反复失败，reason=Incomplete(19)、layout237、旧通用Fail位置binary_display_capture.cpp:59；ordinary停在6597、Body停在26390，gaps持续增加，故障位已锁存，初始生产失败被后续Incomplete覆盖。不能归因于Body新等待位，亦不能跳过来源资格。已增加显示生产首因锁存、真实调用位置和生产类型诊断，构建第十六轮包；用户确认继续启动。第十五轮cs stop后Client Off、worker／binary／menu／scene就绪均1、native owner／written均0，随后正常关闭两端。

第十六轮 Host49004／Client4204 切房复现后捕获首因：Host ReferencePending(20)、layout453、binary_display_capture.cpp:301、production_kind=23(TransitionStart)、before_observe=0。Client另因stream_ack_route安全退出到标题；worker／binary／menu／scene均就绪1、native owner／written为0。静态核对发现转场事件用要求readiness≥2的通用字段读取引用入口解析Player身份；修正为原生请求入口保存既有存活、非candidate身份，返回时核同local/use及未退休，只传事件归属，不恢复Player字段读取资格。Host清空旧发送器但尚未激活新route时，迟到ReceiveAck原先进入无route发送器，现直接丢弃；新route的接纳及分片合法性检查保持。两项已构建为第十七轮包，用户确认启动，实际修复效果待续验。

第十七轮 Host36552／Client4900 已自动开局。切房提示后的首个45秒窗口尚未包含手动输入或场景代次变化，World实际发布29.67Hz、Body59.99Hz，Client错误为空；不能计为切房通过。`AwaitingWorld`是本实现正常显示时仍保留的会话状态，`scene_state=3`为Displaying，不能单凭前者认定卡住；Host采集gaps=376在该静止观察期间未继续增加，旧MissingQualification计数也不能视为正在发生的新故障。已请用户仅由Host进入相邻房间一次后松键，待实际操作反馈与新作用域安装结果。

第十七轮实际切房仍失败：用户反馈PreparingWorld。Host ordinary停在9934、Body停在39738，gaps增加到1384且继续增长；首因仍为ReferencePending(20)、layout453、binary_display_capture.cpp:305、production_kind=23。Client场景代次3、旧binary排空，native_reason=world_payload_unavailable，未见stream_ack_route。说明此前“仅是readiness误用”的推断不足，不能把第十七轮身份修正计为有效修复；跨房间路径已有接线但尚未跑通。第十八轮仅补充TransitionStart入口及返回身份存在／退休／candidate／readiness和local/use诊断，不放宽身份规则。已构建并打包，等待启动配合确认。第十七轮正常停止后Client Off、worker／binary／menu／scene就绪均1，native owner／written为0。

第十八轮 Host40084／Client34920：切房仍为PreparingWorld；Host首因细节`transition player=902bcb90 entry=305 exit=305 saved=0/0 now=1/1`（flags为十六进制，bit0存在、bit1退休、bit2 candidate、高位readiness）。入口／返回均为存活candidate且readiness=3，local/use=1/1；candidate意味着出生记录未确认，并不否定Collector::PlayerReturned已建立的当前Player更新资格。第十九轮修正只用于转场事件身份：candidate必须current_public_ready、readiness≥2且恰有一个有效session/generation控制锚点；入口保存local/use，返回仍要求同一身份且未退休。不改出生记录、通用字段读取资格或其他引用。已构建打包，运行效果待验。第十八轮正常停止后Client Off、worker／binary／menu／scene就绪均1，native owner／written为0。

第十九轮 Host33340／Client20160：切房仍停PreparingWorld，但Host失败已变为InvalidWindow(2)、layout0、capture.cpp:32；ordinary停在453、Body停在1810，未再记录原先TransitionStart的ReferencePending。不能仅据此认定全部转场事件已完成。已增加Current拒绝层次与身份的失败诊断（只在失败时格式化，每个读取段清空），第二十轮包构建完成，待用户启动配合确认。第十九轮正常停止后Client Off，四项退休就绪均1，native owner／written为0。

第二十轮 Host22740／Client17112：同样PreparingWorld；首因进一步定位`current=identity subject=2/52/1 shape=0 actual=0 ready=1 retired=0 ordinal=255 binding=0`，即新房间Grid资格被NativeCurrent拒绝。ordinary停329、Body停1314。第二十一轮仅扩充该失败分支的Grid candidate／创建owner／birth_kind／vtable RVA／initialized诊断，未放宽派生Grid读取；已构建打包。第二十轮正常停止后Client Off，四项退休就绪均1，native owner／written为0。

第二十一轮 Host9036／Client47616：仍PreparingWorld，Grid诊断为`grid=58/1 ready=1 candidate=1 owner=0 birth_kind=19 vtable=71960c init=1 read=1`。vtable对应Poop，19为GridCtor；物理构造与公共Init返回已观察，但房间直接构建未经过GridSpawn／Door，因此未确认出生。按[Grid公共证据](grid-common-evidence.md)的J273 0x705085派生InitSubclass调用、0x705090返回后置initialized=1，补齐NativeGridReady：只在ordinary合法窗口、同一已观察GridCtor use、无未完成创建owner、BaseInitReturned且initialized=1时接纳当前读取；实际RTTI／成员／index及末尾复核仍执行。不改变candidate或补造BirthConfirmed，原边界墙内联初始化分支保留。第二十二轮修复包已构建，待实机验证。第二十一轮正常停止后Client Off，四项退休就绪均1，native owner／written为0。

**2026-11-15 用户恢复同一步实机续验**：本轮先续测实际应用频率与移动连续性，并将新增 World／Body 双通道、增量准备与结构复用、共享承载、发布／反馈及正常排空纳入同一轮。用户已确认双端启动配合准备；位置插值继续暂缓。此前停止与失败记录保留，`display_world_not_current` 首因继续监测定位；本条仅登记恢复与范围，不表示任何场景已通过。

本轮首轮 Host PID 3872／Client PID 28240 在首帧准备时失败：`binary_atomic_staged_inputs`，首因 `staged_object_changed`，布局195／成员1；尚无发布或ACK，随后服务端断开，两端由启动器关闭。源码核对确认布局195是没有原生地址的逻辑参与者，合并扫描误把合法空地址当作失效。已修正为纯逻辑参与者／控制组仍检查暂存owner及生命周期，允许空原生地址；真实承载继续要求有效地址。修复已重建，后续实机结果另记；`after_resource_load` 是最后一次撤销检查资格的阶段，不能据此断言资源加载改坏了对象。

**2026-11-15 本次连续续验进度（尚未通过）**：

- 第二轮 Host34336／Client29940 首帧恢复成功；15秒静止窗口 World实际发布约29.84Hz、Body约30.04Hz，接收约30／60Hz。双流分别发布、接受和ACK，无资源／容量等待；用户双端移动反馈“两者仍断续，FPS稳定”。输入发送／接纳／处理均809。正常停止所有退休就绪位为1、原生owner及written为0。
- 第三轮 Host38084／Client14396 首帧成功后出现 `client_dispatch_admission` 并安全退休。原日志未包含具体分派门槛，不能认定取消竞态为已证实根因；现已补记分派root、owner、上下文与票据资格。第四轮有限诊断发现：专用文本写入器使用纯值字节，通用字符串暂存却每批重复构造；两流准备先于提交，也使已就绪任务多等一帧。
- 第五轮 Host18888／Client43540：专用文本跳过未使用的通用原生容器后，Body稳态容器构建为0、每批保留4个；World每批仍构建10个、保留19个。提交调度改为已就绪提交可先于另一路准备。不同静止窗口 World约15—25Hz、Body约30—41Hz，尚不稳定达标；名为 `moving5` 的采样实际发生在用户移动之前，不能算移动期间频率。用户之后移动仍断续，输入发送／接纳／处理均107。持续运行超过历史345秒失效时点，未复现 `display_world_not_current`，不据此排除历史问题。正常停止全部退休就绪、原生owner及written均0。
- 第六轮 Host13832／Client33076：仅在 Describe 只读窗口内复用父路径解析，并新增旧基线与重复目标计数。20秒静止窗口 World约18.56Hz、Body约41.66Hz；Body本流忙147次、旧基线拒绝220次，World旧基线拒绝230次；资源、容量、依赖等待均0。两路原生工作仍共享主线程8ms窗口，准备和安装成本、提交后ACK时序继续定位；未放宽基线、期限或预算规则。

- 第七轮 Host9364／Client12580 的细分计时：World／Body每批主线程构图约4.17／2.21ms，实际原生写入约1.49／1.06ms。第八轮 Host38388／Client41388 对每流128个普通批次有限汇总，主要成本为索引、逐值指令和排序，页权限预检并非主要热点。第九轮 Host44352／Client38984 改精确键索引与稳定分桶后，静止窗口约26.26／30.10Hz；未把耗时下降判为频率通过。
- 第十轮 Host17048／Client15688 在同一Manager窗口内有余量时再收一次邮箱；每票据每帧仍最多一个准备片，不等待网络、不新增候选槽。静止窗口约29.97／29.97Hz，Body本流忙588次、旧基线13次。第十一轮 Host17316／Client24444 对无handler普通字段直接分派，保留特殊例外；Body构图约1.34ms，静止发布仍约30.13Hz，World约23.49Hz。
- 第十二轮 Host43888／Client31112 合并同一数组字段的连续原样元素：每个元素先核区间，回滚及读回使用完整合并范围，单元素定位步长独立保留；重复投影仍按原元素边界逐字节核相等。首帧及热发布读回成功；20秒静止窗口World约27.95Hz、Body约30.00Hz，构图约2.30／1.33ms，安装约2.29／1.69ms，Body接收到发布平均约17.53ms。本流忙397次、旧基线拒绝203次，资源／容量／依赖等待均0。继续直接测量主线程排队和准备完成后的提交等待，以区分8ms共享窗口与一在途／零Body等待槽及ACK往返的限制。

- 第十三轮 Host35856／Client33168：首个20秒静止窗口World约28.92Hz、Body约57.60Hz，Body主线程排队3.44ms、提交排队0.08ms、端到端8.98ms；合计可超过60，不能推导存在每帧最多一个提交的硬限制。之后两个45秒窗口Body约30Hz；最后一个窗口Body主线程排队11.29ms、提交排队0.04ms、端到端16.78ms。说明达到57.6仅是短时样本，主线程处理相位与单在途／旧基线规则仍造成降频。已请求用户双端各约10秒移动，但这两个窗口客机发送数仍为0，不能把命名为 `moving13` 的文件当作已覆盖移动。
- 第十三轮保管失败现场已明确：`binary_install_hold` 的 `installed=5`、`staged=1`、`cancelled=0`、`withdrawn=0`。现已修改 `Collect`：完成发布交接后，立即再次尝试退休刚换出的历史批次，再接新票据；真实借用检查和5项上限保持。有限逐批诊断已撤除，正式聚合计时与失败现场保留。该退休修复已构建，尚未启动实机验证。

- 第十三轮用户随后完成双端移动，反馈“移动还是有点断续，fps稳定”。客机输入发送／接纳／处理均602，错误为空、无恢复；此前静止采样不能外推为这次移动期间的应用频率。正常停止后全部退休就绪位为1、原生owner及written为0。客机正常退出；房主会话已Idle、设备Removed，但最小化窗口无法经工具激活，核定隔离进程路径后结束旧房主进程。
- 第十四轮 Host32096／Client19788 已运行上述及时退休修复，并撤除有限逐批诊断。首个25秒静止窗口World约29.66Hz、Body约49.02Hz；Body收到1503包，本流忙273次、旧基线2次。后续45秒静止窗口World约28.35Hz、Body约43.09Hz；Body收到2702包、发布1941批，本流忙252次、旧基线509次。后一个窗口Body主线程排队约4.43ms、准备完成后提交排队1.10ms、准备2.29ms、安装1.62ms、接收至发布10.98ms（均按窗口总量／批次计算均值）；World对应约10.94／1.56／3.74／2.16／20.38ms。资源／预算／依赖／过期等待均0，未观察到保管容量错误、client_error或恢复。两窗合计发布均超过60Hz，但未稳定达到World30／Body60；不能将回收修复计为移动连续性修复。

第六至十二轮停止后均实测退休就绪位为1、原生owner及written为0，然后正常关闭两端。不同启动房间的字段数量略有不同，跨轮耗时不能当作完全相同工作量下的精确对照。第十三、十四轮临时目录 `build/isaac-dual/step15-incremental-run{13,14}/` 与 `step15-incremental-package{13,14}/`、`.scratch/step15-incremental-*{13,14}.json` 暂用于恢复连续诊断，不能当作验收完成产物。第三轮分派失败未复现，保留其必要现场材料供后续查证。

**下一项设计决定尚未实施**：第13步10.1仍限定普通Body零等待、每流一个在途，接纳和完成均要求wire base等于当前接受版本。当前观测证明本流忙／旧基线拒绝是剩余降频的重要来源，不证明所有断续都由此产生。拟议方向为普通Body增加一个可被更新目标替换的有界待处理位，并在现有总预算内保留最多一个前代Body解码基线；只重建完整目标，再相对于实际已提交状态生成应用差异，不能把旧基线delta直接写进新状态。主线程仍单写者，ACK仍仅在真实成功后产生；scope、成员／资源／共享结构、单调目标及取消／恢复检查保持，缺基线或不兼容就丢弃。等待期限、5份表上限及原预算需在详细设计中闭合，不承诺这一改动即可保证60Hz。该方向改变现行接纳合同，待用户决定是否进入设计与实施。

第十四轮正常停止后，Client为Off，worker／binary／menu／scene退休就绪均为1，原生owner及written均0，错误为空；Host为Idle、设备Removed。两端随后经窗口正常退出。本次中断用于确认接纳合同的调整方向，保留上述必要恢复材料。

以上属于同一第15步持续验收。移动连续性和30／60Hz目标未通过，完整双通道交错、结构变化、恢复及其余原版场景仍待覆盖。

**2026-11-15 较早的增量应用与双通道范围登记（历史状态）**：[新设计](13-client-incremental-apply.md)明确检查／读回复用、稳定容器和变化块、整体引用表发布／退休、两条独立应用通道及共享结构协调；本页15.7登记覆盖项。登记当时尚未实施、未构建或运行；其后实施和用户恢复实机的当前进度以上文为准。

**2026-11-15 本轮已停止，位置插值暂不实施**：按用户最新决定停止实机测试，刚开始的插值草稿、临时批次计时以及最后一项尚未实机验证的纯拷贝优化均已撤掉。收尾源码仅重新编译成功，没有再次启动测试。已验证的合作选择面板隔离／恢复、客机帧率与输入修复保留；移动断续尚未解决，第15步未通过。

收尾额外发现：最后一轮 Host PID 45840／Client PID 28316，Client 在有限热批计时结束约345秒后记录 `client_display_schedule`，状态首因为 `display_world_not_current`；该日志早于用户要求停止后的 `cs stop`，不是正常停止成功的证据。首个失效的显示字段／资源尚未定位，不归因于位置插值（实机包未包含插值）。停止后已核 Client `Off`、worker finished／binary drained／menu ready／scene ready均为1、native owner与written均为0，两端隔离游戏已关闭，x32dbg未附着本轮进程。保留 `build/isaac-dual/step15-phases/` 中的失败版本、控制记录和两端日志供后续定位；普通中间测试包与临时日志清理。后续先定位显示资格失效，再继续运动频率与剩余场景验收；未经用户恢复，不继续实机。

**2026-11-15 合作选择面板修复与连续续测（仍在第15步）**：此前称为“表情列表”的分支实际是原版合作角色选择面板。J273 `HUD::Render 0x97DC30/0x97DC3E` 按每槽两个非负索引进入，`0x97DD50` 对候选角色数取余；客机从菜单直接安装世界时候选表为空，因而除零。现在只在 C/S 客机的显示安装作用域保存四槽的八个索引并写 `-1`；按地址去重的快照不被热帧覆盖，正常退出和失败退休沿既有回滚恢复原值。Host、原版与 Isaac Sync 不经过这项写入。

- **本轮实机证据**：隔离 Client PID 45292、Game `0x573F8010`，x32dbg MCP 未设置断点，读取八项为 `[-1,-1,-1,-1,-1,-1,-1,-1]`；执行正常 `cs stop` 后，同一 Game 的八项恢复 `[0,0,0,0,0,0,0,0]`，`native_written=0`、原生 owner 为0。此前失败退休路径同样读到八项恢复0。两端实际房间可见，客机未显示原版合作选择面板；Host 的世界／身体确认曾均达到210。已补的图层顶点格式修复后，本轮普通续测未再出现既有堆损坏或该除零；不据此追认历史首次破坏指令已经捕获。
- **沿途修复**：相机退休改用官方 SDK `Camera::destructor()`，避免官方常驻 Hook 下多传栈参数导致返回地址错位；安装前处理菜单遗留的原生 resize 请求，避免首次 `Game::Render` 替换历史 surface 后使已发布资源身份失效；日志 PID 文件冲突改为独占新建后缀文件，保留旧日志；网络 receiver 关闭后继续 `Pump` 的 `Collect`，回收主线程归还的退休租约。
- **正常停止续验**：未附着调试器的 Host PID 5860／Client PID 42136，Host 世界／身体确认均到78；`cs stop` 后 Client 为 `Off`，worker finished、binary drained、scene ready、menu ready 均为1，native owner与written均为0。原先长期 `Retiring` 的实测首因是 worker未完成，资源及菜单已退休，历史失败保留。
- **仍未通过**：另一轮曾在世界／身体确认2后恢复并以 `binary_prepare_install_deadline` 断开；后续轮次可持续显示，但有候选取消／`binary_preparation_scope`，停止时 Host 还记录 `stream_ack_route`。客机失焦时地图和相机未推进；获得焦点后地图出现，心形 HUD 仍缺失。稳态性能、完整显示／输入／生命周期场景及 P4 仍须继续，不能把首帧或上述局部成功计为第15步完成。
- **后续 HUD／退出修复**：按 C/S 控制组 ordinal 绑定原生八个内联 PlayerHUD，保持 twin 选择语义，不调用会重写合作槽和查询 NetInputDevice 的 `AssignPlayerHUDs`。失焦只屏蔽输入，显示推进仍跟随 Host。PID 42972／30368 实机可见两组心形 HUD 和客机失焦时的地图；正常停止回到 `Off`，所有退休就绪位为1、native owner为0。Host 退休后丢弃晚到的状态／ReceiveAck，保留 Disconnect 的 OperationAck；本轮正常停止没有再记录 `stream_ack_route`。
- **用户报告的三个现象与诊断**：低且不稳的客机 FPS、无法移动、Host 移动在客机断续均已纳入本步。设备采样持续、`device_missing=0`，但恢复期间 `input_enabled=0`。阶段计时捕获到单边界350—500ms，准备约150ms、安装约210ms，以及 `Dependency` 恢复；窗口归属查询逐字段重复是热点之一。每个 Manager 边界完整核窗口、边界内重读句柄并复用归属结果后，安装降到约23ms。准备改为按实际两毫秒工作片推进，撤掉“一项资源／32项字段固定占一帧”的等待，未放宽协议截止时间。
- **输入续验与新修复**：首次开启真实客机输入后暴露 `live_input_ack_conflict`：Host 的累计 processed 序号不变时，中性帧仍推进 processed tick；Client 错把它当冲突。修正为接受同水位的新 tick、忽略旧／重复回执，仍拒绝新水位 tick 倒退。PID 49052／42812 已通过客机真实 D 键移动、两端对应角色位置对照；发送4253、接纳4252、Host处理4252，输入资格持续为1且双方无失败。此时客机仍约38—49 FPS，不能计性能通过；根快照重复读取与连续标量拷贝的页查询优化正在本步续验。
- **帧率恢复后的反例**：取消重复根快照读取，并只在连续纯拷贝片内复用页查询、跨原生调用清空后，PID 48024／42456 的热安装低于4ms，多个120边界窗口约1.983秒，仍有少量约20ms峰值。撤掉临时逐批日志后，用户在 PID 41564／47388 完成双端各约10秒持续移动，明确反馈“客机fps稳定，客机能移动，移动明显断续”。所以渲染60FPS与输入可用不能充当运动连续性通过。随后以 `cs stream client 0` 的实际发布计数测得 world约20次/s、body约8—10次/s；发送版本号不能代替安装次数。
- **运动连续性续查**：发送器原先让World／Body共用一个60Hz间隔，已改为各自协议节奏；单独这一修订加字段准备4ms工作片后，PID 45076／43644 的body仍约9次/s，保留该失败。PID 44056／48572 的少量批次时间点显示热安装通常同一Manager边界完成、下一边界消费后台确认，未见退休等待。接收侧原先在读取新UDP前以及每个World包之后立即调度等待World，易使后续就绪Body遇到busy被丢；现改为先消费完成反馈、处理本轮UDP，再调度等待候选。普通Body仍无等待槽，未放宽期限或带宽；修订正在本步实机验证，移动断续尚未计通过。
- **发送节奏与批次开销续验**：本机 `GetTickCount64` 实测以15／16ms跳变，原17／34ms截止只能得到Body约32、World约21包/s；改用既有高精度微秒时钟并保持独立发送相位后，PID 2700／45756 实收Body约60、World约30包/s，实际Body发布仍仅约10次/s。数字计划与原子安装索引改为按原Peer／进程预算记账的批次arena后，PID 30784／24716 的实际World／Body发布均约20次/s，渲染约60FPS；包数不再当作应用数。
- **接纳调度反例与修复**：旧批退休移到新批提交之后、同一Manager仅在有现成工作及时间额度时继续接纳后，PID 27104／43652 的World约30次/s而Body反降约6—8次/s。进一步在每个新状态包处理前取走真实完成反馈，避免socket等待期间的Success仍被当作busy；PID 48112／42620 仍出现Body约1—24次/s的相位波动。于是普通World成功后最多让出一个Body发送周期，仍只使用既有World等待槽；joint／恢复和100ms候选期限不变，不新增Body排队。配合主线程临时字段索引arena，PID 44488／48120 两流均约18—20次/s、渲染约60FPS，无首个失败原因，正常停止为Off且native owner=0。该结果仅消除了长时间饥饿，不能计移动流畅度通过。
- **当前剩余瓶颈**：PID 45840／28316 有限热批计时中，一类批次准备约0.7ms、Stage约3.4ms、安装约1.7ms，另一类约1.0／6.0／2.8ms；安装提交还需后台回执与下一批准备衔接。显示委托设计第5节明确记录客机跳过原生半帧移动且显示插值留待第13步决定；当前源码没有补上实体位置插值，所以渲染60FPS本身不保证位置连续。用户随后决定位置插值先不做，见本页停止记录；第15步继续保持未通过。

**2026-11-15 源码优先排查，未启动实机**：已确认二进制图层安装遗漏原生顶点格式配置，是本轮堆损坏的首要候选；尚未捕获历史失败中的首次破坏指令，不能把以下静态结论写成运行时根因已证实。本次只阅读当前源码、既有日志和 J273 静态反编译，未修改运行代码、构建或运行测试，x32dbg 会话未操作。

- **缺口所在**：`mod/src/game/client/scene/client_preparation_native.cpp` 的 `ClientPreparedResourceLoad` 对 ANM2 使用 `Load(path,false)`，对图片使用 `Manager::LoadImage(...,__ptr_g_VertexAttributeDescriptor_Position,false)`；前者不执行 `LoadGraphics`，后者的初始格式为 J273 `0xBA47B0`，三个属性合计 36 字节。`client_atomic_bindings_native.cpp` 的 `ReplaceSprite` 复制该未加载图形的 Sprite；`ClientBinaryApplySpecial` 的 `LayerImage` 分支随后直接安装 image/counter 并置 `_spriteSheetLoaded=true`，没有按 Sprite／Layer flags 配置 `SetVertexFormat`。同 pair 的提前返回和 `ClientBinaryReadSpecial` 也只检查引用／loaded 位，不检查格式。
- **原生必要步骤**：`ANM2::LoadGraphics`（VA `0x407DA0`）按 `ANM2+0x110 | LayerState+0x8C` 选择格式：`0x10` 分支为 `0xB88620`、104 字节；否则非 `0x200` 分支为 `0xB88760`、88 字节；`0x200` 分支不做该扩展设置。普通 Layer 绘制 `0x404570` 在 `0x404EFC` 调用 `0x67E7B0`，后者固定为四个顶点写入颜色扩展、纹理尺寸和显示参数，每顶点步长 88 字节。把尚为 36 字节格式的图像直接标成图层 ready，会让这个写入合同与实际顶点分配合同不一致；若图片已被其他正常原生路径配置过，缺口可能暂时不显现。
- **与现有失败的关系**：`0x67E7B0` 会把 padded width／height 作为 float 写入顶点；日志中的 `0x43800000` 正是 `256.0f`，因此与顶点写越界相符，但不能据此认定具体被绘制图片或首次写入者。`controls.png`／`light_gradient.png` 可以是相邻堆块中的受害对象，不必是出错的绘制源。后台线程随后报告 `0xC0000374` 也不足以证明后台线程首先破坏了内存。
- **另一个核对点**：官方 `REPENTOGON-1.1.3/repentogon/Patches/ImagePatches.cpp` 已有透明批次复用时检查 `batch->_vertexBuffer.elementSize == image._vertexFormatStride` 的补丁。它不补足上述缺失的图像格式；后续修复仍须核对格式变化前后已有 batch 的兼容性，不能只把所有图片统一改为 88 字节。原生 `0x9EFA70` 在 `0x9EFE35` 遍历 `0xBCE57C` 并调用 image vtable slot 28（批次重置），该列表不是待提交队列；清空它不能修复写越界。
- **后续定位／修复范围**：在全部 Sprite／Layer flags 和实际图像绑定安装完成、首次离屏捕获前，补足原生等价的图形就绪处理，并让热帧、同 pair 换 flags、换图及回退／共享图像路径采用一致语义。若需要实机锁定首因，在 Client 的 `0x404EFC`／`0x67E7B0` 核对实际图像 `+0x3A` 步长、所用 batch 的 `+0x2C` elementSize 和返回顶点缓冲；对受损图像首字设写断点追踪实际写入。以上均为 J273 静态 VA，附着时须按实际模块基址换算。本次未实施修复，第15步仍未通过。

**2026-11-15 布局 78 续测**。隔离 P2 能完成握手，Client 进入 `AwaitingWorld`，Host 开局并持续发布普通状态。首轮 Client 在 `BuildBackdrop` 前缀读到错误的房间指针。已校正 `0x821D00` 的调用方栈清理，并按原生调用补齐遮光绘制及离屏混合参数。后续普通 P2 仍可在 Client 首个 `Room::Render` 或第一次发布附近报告原生堆损坏。撤掉临时诊断后再次实机复现，Host 发布 224 个普通样本，Client PID 43276 以 `0xC0000374` 结束，Windows 应用日志记录 `ntdll.dll+0x1017BF`。未观察到 `client_world_applied=1` 或同批 `AppliedAck`，P4 及稳态性能待测。

两轮日志分别记录 `controls.png` 和 `light_gradient.png` 的同一原生地址在 `Room::Render` 前有效，返回后仍由 `0xBCE57C` 向量持有，但对象首字变成 `0x43800000`，随后主线程在 J273 `0x9EFE35` 提交时访问 `0x43800070`。另有一轮通过首帧离屏提交与五次音效重放，在进入发布后记录后台线程的 `0xC0000374`。当前证据确认共享渲染状态中的内存损坏及其最迟可见位置。首次写入与释放路径未抓到，线程竞态仍待验证。续查材料位于 `build/isaac-dual/step15-layout78-render-evidence/`。KAGE 的 `+0x1C` 向量在 offscreen 提交后也可保留合法图像，不能用清空它来绕过崩溃。

**2026-11-14 冷构建审阅修复已入源，尚未验证**：[第13步第12节](13-client-cold-build.md#12-审阅修复重建依赖与原生分配预算)补入以下统一覆盖：同房追加无关资源不重建且 FX 连续，必要输入与 resize 仍重建；各原生分配阶段额度拒绝不发布／不 ACK；失败前缀、Clear 后容量和缓存继续占账，真实释放才归还；realloc 失败／迁移／零大小销账；原版／Host 无作用域透传及 Hook 尾延迟。当前保持暂停；本轮未生成、未构建、未运行任何测试。以下“仅文档／待实施”和原始失败保留各自时点。

**2026-11-14 冷构建覆盖补充，仅文档**：[冷构建设计第9节](13-client-cold-build.md#9-第15步统一覆盖)并入本步现有首帧、资源、生命周期、漂移、显示推进及转场用例：从菜单直接加入、FX 适用性与本机就绪、真实 roster 下的背景构图、首帧转场捕获、主题／尺寸变化、部分失败／取消／重连及冷热成本。必须观察实际版本提交、`client_world_applied`、同批 `AppliedAck` 和双端稳态；接收或进入写入不能计为通过。保持当前暂停续测，原始失败记录不变，本次没有实施补齐或执行测试。

**2026-11-14 原生采集性能方案，仅文档**：[具体优化方案](10-native-capture-performance.md)第11节列出待实施工作，第12节给出失败方式、负载与计量要求，合并到本步既有性能／正确性覆盖，不另开验收编号。保持当前暂停续测；本次未实施优化、构建或启动测试。Host 十多帧及 Client 画面／退出问题仍未通过，下面原始记录保持。

**2026-11-14 用户暂停续测**：已移除每日挑战分数缓存。随后恢复需求裁剪误删的 ResourceEntryHeader／SurfaceRecipeHeader／SurfaceRecipeClear，当前布局77；修正原生 Entity/Grid revision=0 到非零线身份的映射，以及 Client 二进制确认沿用 JSON 对象正文导致的 `binary_state_envelope`。Host 已能发布二进制状态；Client 仍未显示游戏画面，第5、6轮均出现进程退出／控制连接10054，尚未确定崩溃根因。第5轮 REPENTOGON 日志中的最后异常栈对应 `BodyMachine::Fact → DecodeRoot → ClientStateReceiver::Pump`，该栈包含正常捕获的 scratch 扩容异常，不能直接判定为崩溃点。Host 静态字段路径验证已优化，实机仍存在显著采集成本，掉帧问题未验收通过。暂停时测试游戏进程已退出，未继续启动。续查材料：`build/isaac-dual/continue-e2e-{5,6}.log`、`launch-238f711cbd6348d49d92837feafc74e7`、`launch-072d45bf84644a8e9a85a8650839dbef`；临时诊断脚本位于 `.scratch`。保持当前第15步，用户恢复后再继续。

**2026-11-14 连续排查**：普通 P2 开局复现 Host `ordinary=0`、Client `AwaitingWorld/world_payload_unavailable`；采集在布局234字段0因每日挑战分数缓存没有生产证言失败。用户要求直接移除该缓存，布局76从需求清单移除 GameRun.score／RunScoreSheet，并移除 Host 采集缓存；不放行 Client 原生分数重算。原分数同步验收项不再执行，其余第15步继续，当前尚未通过。

**2026-11-12 显示委托修订**：按[修改方案](client-display-delegation.md)第 10 节修订验收内容。

**2026-11-12 RE-10 静态交接**：E36／E37 按[Manager 分类与音频调度第 5 节](client-display-delegation/RE-10/audio-and-schedule.md#5-第-15-步覆盖与未决)补入：音乐在相机被 paused 跳过时仍更新，SFX 当前乘数安装与本机 Options 的 dirty 刷新（ClearVolumeModifier，取代下文开题时“通用 SFX tick”措辞）、音乐种子效果 41—44／dizzy／红房读取需求、jingle／layer、输入泵复用、本机普通路径 A／B 保持及不运行共享 callback／截图／Online。FrameDelay=0 不能绕过调用前已存在的同 id 节流截止，覆盖本地显示 Play 后紧接回放的场景。只登记本步统一覆盖，未执行测试。

- **修订**：15.2 的 E09／E20，15.3 的 E27、E28、E30、E32、E33、E34。同日 RE-1—RE-3 完成后，E28 补入 Q8，E36 补入显示推进的调度。
- **删除**：E35。
- **新增**：E36—E40。E39 是原生切房过渡，由 Q5 追加；E40 是帧号消费者，由 RE-4 审阅后追加。
- **15.5**：退役域的条目已加标注。

验收要求双方看到同一个权威结果，不要求像素相同。第 15 步仍是唯一的统一整体验收。本次只修订文档，未改代码，未运行测试。

**2026-11-12 HUD代码迁移，版本70**：布局70和纯值模块已实施，尚未接通完整游戏链路，也未进入本步。继续统一覆盖合同第8节；本轮只有产品生成／纯编译，无任何测试。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD本地化验收口径，仅文档**：按[现行合同第8节](hud-local-presentation.md#8-第15步统一验收与本次完成范围)在本步统一覆盖：低血／护盾纯值派生、普通主动／饰品及中心缓存、八材料／六法术顺序、Drop实际消费驱动／更新相位／恢复、基础value格式与保留变化相位／概率、四参与者多身体独立排版。比较必要状态、资源、显示语义与时钟，不以已取消Host缓存／文字像素／屏幕坐标逐项相等为条件；特殊图像、心格、触发闪烁、有序列表、world资格、缺源不ACK及性能／隔离继续覆盖。此前公共绘制八项仍按原同步验收；本轮未运行任何测试、生成或构建。

**2026-11-11 版本66压缩交接**：唯一验收补[world/body独立zstd](9-world-body-zstd.md)双方／单方协商、None回退、丢前帧独立解压、坏压缩体／长度／字典／多帧／窗口拒绝，以及真实压缩率、CPU、合包后成本／峰值。原内容、动画／音频、增量及9Mbps指标不变；本次不提前测试。

**2026-11-11 第9步收尾交接，版本65**：[现行预算与静态未满足项](9-budget-capacity.md)已定稿，按用户允许90%带宽，模组持续9Mbps、总链路10Mbps；三端各375000B/s，首次准备1MiB／10秒不变。唯一验收覆盖实际分片／合包／压缩／重传与控制成本、非空完整目标、峰值与队列年龄、重复请求不续命、账本／事件／快照／池配额、writer部分失败停用及base=0重建。静态稠密负载已出现超额，不据下界例子声明性能通过；本次未运行任何测试。

**2026-11-11 当前验收口径调整**：[本地生成合同](local-media-presentation.md)要求核文字内容／样式／归属与本地生成、完整目标直接切房、本端hallucination生命周期和本地音频事件／资源。取消Host文字／反馈像素相同、前后surface传送和平滑切房的通过条件；未启用媒体payload应拒绝，全部通道确认不传原始媒体。真正必要来源缺口不放宽，其余性能／隔离保持。此处只修订未来第15步统一验收，不运行测试。

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。正文实现完成，版本60。本步唯一验收需覆盖所有有负载域、条件隐藏／704、固定槽／u8计数、坏bool／枚举／浮点／mask、带符号排序、Weapon choice、shared首尾、投影计数组／缺base、空主动规范和预算失败。BodyView不等于资源ready／原生提交；运行功能／性能结果仍未产生，本轮没有提前运行测试。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。第9步第一项已登记版本59公共结构；本步唯一整体验收须覆盖双分段原子提交、按Peer body／u32 GameFrame、旧表与序号回收、来源与目标分离、缓存／贡献身份、事件水位／明确Clear、恢复不回绕，以及三端总10Mbps／模组7.5Mbps和首次准备1MiB／10秒。32MiB保护不充当产品帧许可，三端共享编码仍按三份实际发送计费；当前没有提前运行测试。

**第8步版本58交接**：[正式结果与剩余Gap](implementation-8-presentation.md)已登记；本步仍是唯一统一整体验收。届时覆盖两配置挑战32／徽标／边框与独立查看、隐藏心格与官方星象房行、四组多身体详细／公共HUD、同帧多贡献／递归alpha、图像资源与各屏幕／转场及音频world意图隔离；本轮未提前运行任何测试。

**2026-11-10 显示批次及各端多角色HUD定稿覆盖**：[方案B及第9节覆盖](presentation-result-batches.md)纳入本步唯一整体验收：四参与者各端详细HUD跟随自身控制组、Jacob／Esau双身体与换形、其他组公共HUD不丢人；History顺序／实际图像／负ID／shader、Stat三值／星象房、Host个人关闭HUD及独立视口；多layer／pass／递归贡献、来源与逐Peer目标映射、Gap／资源Wait／失败不早ACK、旧批次退休及重复展示不重抽。原生两槽不作为系统容量，既有body窄依赖和三端预算保持。不新增逐组验收或测试编号，0—14步不提前执行；本次仅文档静态审阅，版本57不变。

状态：设计稿，未实施。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为15.5“各域统一验收交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

**2026-11-12 RE-7—RE-9静态交接**：[RE-7](client-display-delegation/RE-7/README.md)、[RE-8](client-display-delegation/RE-8/README.md)、[RE-9](client-display-delegation/RE-9/README.md)仅完成来源／ABI／风险与实施交接，没有执行验收。E37补同曲重播／Queue、同id新演出、fortune list／跨语言、循环多实例与world Stop／设备失效区分、转场显示／玩法声音来源；E38补天使XMM0 ABI、药丸识别往返、有效诅咒／混合setter隔离、Host最终多人价格、地图摘要／挑战32与程序道具资源；E39补Start请求与Maze最终目的地关联、三个换房点、目标迟到时上层冻结、Boss完成直接写入隔离、resize丢失旧surface内容、断线／连续转场和像素化RenderTarget正常退休。具体失败方式见各成果，不提前测试。

**2026-11-12 RE-1补遗与RE-7—RE-9审阅后（Q11／Q12／RE-10）**：按[修改方案](client-display-delegation.md)第 10 节修订 E36—E39，失败方式见修改方案第 12 节第 44—65 条。E36 补 Host 终结分支枚举选序列、paused 位经 `client_entity_draw_paused` 供给 IsPaused 的 53 个调用者、相机 MINECART（animation 19）例外、本地菜单合并、Pα 按已安装 alpha；E37 补 Manager 尾部音乐／SFX tick（RE-10）、回放 FrameDelay=0、转场声音全部经事件；E38 补 raw 诅咒同步后有效 getter 与直接读 raw 的消费者一致；E39 按 Q11 改为转场字段随 world 原子安装、旧 surface 在新房 world 安装前捕获、新 surface 在安装后捕获、新房晚到时停在旧房与 Host 当前进度、滚动补偿本地计算、Boss 结束后 HUD 可见性一致。上文 E39 的“目标迟到时上层冻结”“Boss完成直接写入隔离”由 Q11 取代：Client 不运行 FSM，不需要冻结或隔离。仅修订文档，未测试。

## 目标

本步做三件事：

- 把 E2E 工具改为验证二进制状态路径；
- 加入两类新的检查：未知区探测和 Lua 对照；
- 修订 tx-04-21 的 E01—E26，作为整个重构**唯一的**统一整体验收。

本步之前的 0—14 步不运行任何测试。本步开始后连续执行，遇到问题就地修复并续测，不另开编号。

**2026-11-10 范围调整**：本步先验收原版内容，包含原版全部必要类别／流程、四参与者／多身体及既定性能指标。第三方 Mod、StageAPI、自定义角色／资源／绘制及 Lua adapter 的适配用例已迁至[独立后续方案](../mod-adaptation.md)，不属于本次验收。原 E31 保留编号并标记迁出，不执行、不计通过；其余条目不重编号。继承旧 E01—E26、18—20 回归及补充矩阵时同样按此范围剔除第三方适配要求，保留全部原版要求；测试夹具本身不构成 Mod 支持声明。

## 输入事实

- **原验收清单**：[tx-04-21 E2E 方案第 4 节](../tx-04-21/tx-04-21-e2e-plan.md)，E01—E26 两人全套和四人扩展，以及第 3.1 节的指标。
- **原清单的进度**：tx-04-21 第 73 轮 E02 未通过，E03—E26 未执行（[进度](../tx-04-21/tx-04-21-e2e-progress.md)）。
- **E2E 构建**：E2E 工具只进入 `ISAACCS_BUILD_E2E_TOOLS` 构建（`mod/CMakeLists.txt:94-98,151-157`）。

## 设计

### 15.1 新增的 E2E 能力（只在 E2E 构建中）

| 能力 | 内容 |
| --- | --- |
| Lua 对照 | Host 侧用观测 Lua 读取公开成员，按回映射表（`state_members.inc`）与同一次采集的二进制状态逐成员比较；Client 侧在原生写入完成后读取同样的成员，与 Host 来源比较。“派生”成员只在 Client 侧比较。`Entity.GetLastParent`／`GetLastChild` 在首边为空时，J273 静态 getter 返回自身而 Wiki 写 nil，本项以实际结果为准并记录差异（第 3 步）。 |
| 未知区探测 | 对每个布局的未知区间，附加一份诊断复制计划，在覆盖的场景中逐采集记录是否变化。输出每个区间的结论：变化、未变化或未覆盖 |
| 帧统计 | 每个布局、每个块的字节数和变化频率，Baseline 大小按布局拆分，Host 与 Client 的分段耗时 |
| 故障注入 | 对状态帧、成员表、资源表注入以下错误：截断、错 record_bytes、错 block_mask、错 epoch、版本回退、非法值。另可临时屏蔽 D0，构造 record_count=0 的空帧（E33） |

处理规则：

- **未知区发生变化**：先按实际消费者、来源与生命周期补核归属；变化本身不能证明它应下行，也可能是Host生产／本地缓存／指针。确需修改布局时沿唯一源递增整数版本，在本步内修复并继续必要续测，不将原始失败或未知记录改成先前已经通过。
- **未覆盖的区间**：如实记录，不能宣称“完整”。
- **未知区未变化**：只说明本次覆盖窗口内未观察到变化，不消除UNKNOWN或补造字节语义，不单凭静止样本宣称类型完整。

### 15.2 E01—E26 修订

原条目中原版内容的场景和指标保留，判定依据改为新格式；第三方 Mod 适配部分按上述范围迁出。主要修订如下：

| 条目 | 修订 |
| --- | --- |
| E01 | 握手比较布局版本；版本不符时拒绝，并报具名原因 |
| E02 | 成员表＋资源表＋共同base=0目标；每Peer最终双向封装／路径／反馈／重传累计≤1MiB，8秒收齐／10秒就绪；不以未压缩保护上限充当产品额度 |
| E03 | 按现行9Mbps模组／10Mbps链路、world30Hz／body60Hz及主线程预算；另报按域采集耗时、最终包成本、实际变化块及密集负载未满足项，不以静态下界冒充通过 |
| E05、E06 | 增加“变了又变回”和“最后一次变化后静止”的收敛检查 |
| E09 | 按当前逐行回映射比较实际字段／派生及跨owner最终结果：Host二进制状态↔合法Lua对照↔Client原生；原目录改判Host／不适用的行保留理由，不要求原始数值相等，不把Getter当正式同步来源。**2026-11-12 起**，比较范围为需求清单中的字段，不逐字节比较 |
| E10、E11 | 改为验证：丢帧后由下一帧自愈；ACK 丢失后已确认版本不回退；Resync 回到 Baseline |
| E13 | 增加错误 member_epoch、resource_epoch，未登记的成员序号，以及布局编号不符 |
| E15 | 按13.3区分写前失败与可能部分写入：后者停用污染事务、不补旧ACK，reason5触发base=0完整重建；joint不得半确认，历史成功版本不再当本地差分源 |
| E17 | 增加 15.1 中的全部格式故障注入 |
| E25 | 改为 18—20 的应用回归，加上回映射覆盖报告 |
| E20（2026-11-12） | 双方看到同一个权威结果；画面由 Client 原生绘制，不要求像素相同 |
| 原补充矩阵中的旧内容（63／64／65 项分块、graph_ref、参数矩阵） | 改为原版容器上限边界、查询派生检查；Mod 图编码的共享与循环已迁出 |

### 15.3 新增条目

| 条目 | 内容 |
| --- | --- |
| E27 | **2026-11-12 修订**：差异驱动的未知区补查：只对第 15 步发现画面或规则差异的区域补查，不再要求所有未知区间都有结论（Q1）。<br>原条目：未知区探测：所有布局的未知区间都有结论；变化的区间已归类并重测。包括 J273 池步长或工厂分配大于 SDK 声明的尾部字节：Effect、Knife、Slot、Projectile（第 4 步），Door、TrapDoor、Fire、Poop（第 6 步） |
| E28 | **2026-11-12 修订**：漂移来自放行的显示推进、原生 Render 及维护 helper（第 13 步 13.4、13.8）。按 RE-5（原 RE-6 已并入）的写集检测并纠正，没有振荡，耗时计入安装预算。Q8 的 Game／Room 内联显示标量只由安装写入，不被本地二次推进；震屏结束后本地偏移归零（修改方案 4.6）。<br>原条目：漂移校正：Client 不运行原生 Update，漂移只来自仍在运行的 Render 和维护 helper（第 13 步 13.4），例如 base draw 写 Entity `+0x1A4/+0x1A8`。这些路径改写的字段被检测并纠正，没有振荡，耗时计入安装预算 |
| E29 | 成员序号回收：大量生成与移除后，序号在所有 Peer 确认之后才复用，旧帧不会写到新成员上 |
| E30 | **2026-11-12 修订**：资源表收窄为实体、Grid 和 costume 资源引用：同路径重载、新资源首次出现及退休正确，序号不复用；未启用原始图像／音频 payload 拒绝。文字改由事件队列（E37）验证。<br>原条目：本地资源表：同路径重载、实际贴图选择／声明变化、新资源首次出现及退休正确，序号不复用；文字／样式本地生成，未启用原始图像／音频payload拒绝，缺本地文件不自动转媒体传送 |
| E31（已迁出） | 原 Mod adapter 编码用例移至[后续方案](../mod-adaptation.md)；本轮不执行、不计通过，不作为原版完成条件 |
| E32 | **2026-11-12 修订**：只核对 Effect 各变体的可见状态，即需求清单中的字段，在 Host 与 Client 之间一致。<br>原条目：当前唯一源登记的全部Effect布局／变体，按实际RTTI与选择器核Host／Client一致：<br>- variant136只用最终主Sprite、State及Player持物／BodySprite，不发保存描述；<br>- 54／126仍按102，113按141，167按版本45的148，19／71／194按149；其余按当前正式臂，不把旧版本40的“11布局／167仍102”当现行范围；<br>- 108／166／191三个typed payload生命周期、Trail0—127及Ghost0—4有效前缀／多投影；<br>- 传播／Timer／生成／计数等只看最终展示与独立成员，FX2空预留，不执行Host请求／回调；未列变体仅按正式声明的基础臂处理，未知语义仍记录 |
| E33 | **2026-11-12 修订**：帧号方案B：静止场景中 Host 每次采集仍发帧，Client 的 `Game+0x25A70` 随帧头 game_frame 前进；构造 record_count=0 的空帧，确认它被接受、确认并推进帧号（不再借助屏蔽 D0）。<br>原条目：展示时钟与空帧（第 11 步 11.3、第 13 步 13.6）：静止场景中 Host 每次采集仍发出帧，Client 展示时钟随帧头 game_frame 前进。另在 E2E 构建中临时屏蔽 D0，构造 record_count=0 的空帧，确认它被接受、确认并推进时钟 |
| E34 | **2026-11-12 修订**：帧号方案B：锚点 `+0x328`、`+0x11F0` 直接安装 Host 原值，Client 的 FrameCount 和 `Room:GetFrameCount()` 与 Host 一致；Effect Timeout 等计数在安装点按 `Game+0x25A70` 还原；加测跳帧、停顿和安装失败时帧号不前进。<br>原条目：帧锚点（第 3、4、7 步）：静止和移动场景中，Client 的 Entity FrameCount（`+0x328`）、`Room:GetFrameCount()`（`+0x11F0`）、Effect Timeout 按展示时钟换算后与 Host 一致；仅在已核的线性计时区间要求锚点不变；Effect负值／停走／101双减／141刷新等按真实A变化重发并同帧还原，静止位置不保证锚点不变 |
| E35（2026-11-12 删除） | D0 与随机 alpha 退役，本条不再执行。<br>原条目：绘制上下文与随机结果（第8步8.2.1）：5B D0仅Manager相位／paused，与正常Host结果批次及真实world／presentation修订绑定；逐subject/use/shape/pass/重复调用的alpha／资格不按Client全局顺序分配，两个seed／shift／guard不下行。覆盖暂停／重复／不同成员顺序／可见分支／递归、缺源不填0、旧资源／world拒绝、Pickup临时Sprite与Familiar颜色栈在CPU消费后完整恢复 |
| E36（2026-11-12 新增） | 显示边界隔离：放行的显示推进和原生 Render 不产生玩法写入、玩法随机推进、保存或解锁；默认拒绝入口的计数为 0 或已逐项解释（第 13 步 13.8）；Q6 放行的回调分派不计入拒绝入口，只检查它们没有引起玩法写入或持久副作用。显示推进由 Manager 边界调度（同日 RE-1／RE-3 后追加）：30Hz 批次与相机阶段在 Client 上实际运行，本机 Manager 帧前进；暂停、演出等提前返回分支按分支表执行，不运行原生不会运行的显示更新。Q12（审阅后追加）：Client 按 Host 终结分支枚举选序列，覆盖 GameOver、Leaderboard、StageTransition、ItemOverlay＋暂停组合、Prompt、次级 fade、PauseScreen；Host 暂停时 IsPaused 的 53 个调用者（含 `Room::Render`）经 `client_entity_draw_paused` 读到同步 paused 位；相机按 paused 位与 MINECART 例外运行；本地菜单合并符合第 13 步；Pα 只在已安装 primary alpha >0 时调用 |
| E37（2026-11-12 新增） | 事件队列：顺序、去重、有界、溢出计数、过期丢弃，重连后不重放旧事件；事件引用的成员已移除或资源未就绪时安全跳过。表现状态：音乐、循环音效和全屏演出（ItemOverlay）在丢包、切房和重连后收敛到 Host。音效去重（Q7）：显示代码的音效（如 HUD 滑入 252）只在 Client 本地响一次，玩法音效只经事件响一次，不双响也不缺失。道具横幅和 fortune 在两端语言不同时按本机语言显示。审阅后追加：Manager 尾部音乐／SFX tick（RE-10）在 Client 运行，音乐淡入淡出、Crossfade、Queue 能走完；回放以 FrameDelay=0 播放，短间隔的同 id Host 事件不被本机节流；转场内 608／609、sound 1、sound 214 各经事件只响一次（Q11） |
| E38（2026-11-12 新增） | 注入值（修改方案 6.3）：恶魔房／天使房几率、药丸效果与识别、诅咒、贪婪波次、价格（含 Keeper）、故障道具和地图拾取图标与 Host 一致，变化后收敛；同一个值不会同时由字段和 getter 两处提供；getter hook 在 Client 会话外不生效。审阅后追加：诅咒按 raw `Level+0xC`／`Game+0x25AC8` 同步后，原生有效 getter 与直接读 raw 的消费者都与 Host 一致，含种子效果与挑战强制诅咒 |
| E39（2026-11-12 新增） | 原生切房过渡（Q5，修改方案 4.5）：各种门方向、动画与维度按 Host 参数回放，不重新选路、不消耗随机；目标未就绪时停在换房点，不显示半套房间；换房后背景、门与相机正确；连续快速切房、转场中断线与恢复不会卡死。Q11（审阅后修订）：转场字段随 world 帧原子安装，Client 不运行 Start／Update；旧 surface 在新房 world 安装前捕获、新 surface 在安装后捕获，mode1 与换房同帧发生时也不缺旧画面；新房 world 晚到时停在旧房与 Host 当前进度；滚动补偿按本地相机计算；Boss 开场 versus／肖像正常推进，结束后 HUD 可见性与 Host 一致；同房转场（animation 11／16）、Hourglass、MINECART 与活跃转场中 resize 均有明确结果 |
| E40（2026-11-12 新增） | 帧号消费者（[RE-4 第15步覆盖](client-display-delegation/RE-4/handling.md#5-第13步修改与第15步覆盖)，修改方案第 10 节）：HUD 周期刷新在 F 连续、29→31、跨多个周期和停在 30 时不漏、不重复重置，type 2 不被覆盖；Room 年龄 29→31、停在 30 和晚加入（年龄已超过 30）时历史画面捕获不崩溃，当前 SpawnSeed 非零且等于 Host 值（Q9）；同一 M 换 world 后平均位置不读旧目标；StatHUD 冷绑定后属性值正确、不出虚假变化提示；F 相位与 M 相位各按来源工作；F 回退或 world 重建后清除旧的去重与缓存资格；本机关闭插值时相机按原生节奏运行。另核 Projectile 每次 Render 后 PositionOffset 逐位不变（Q10） |

### 15.4 执行方式

各域场景、当前布局与比较口径统一见15.5；本节保留未来整体验收的执行安排。

- **P2 准备和启动（2026-11-14 用户调整）**：在仓库根目录执行 [`tools/isaac-dual/launch.ps1`](../../../tools/isaac-dual/launch.ps1) `-P2`。工具构建并部署二进制 E2E 包到同一台 Windows 物理机的 `F:\projects_isaac\Isaac-for-test\host\The Binding of Isaac Rebirth` 和 `F:\projects_isaac\Isaac-for-test\client1\The Binding of Isaac Rebirth`，启用两端 C/S，隔离第三方 Lua 和 Isaac Sync。两端从各自的 `Repentogon` 子目录启动。Steam 保持登录，测试子进程拦截 `SteamAPI_RestartAppIfNecessary`，各用独立 `isaac-dual-profile`，关闭云存档和失焦暂停，并保留 `SkipIntro=1`。本条取代旧 tx-04-21 的 P2 启动拓扑。
- **复用已有构建**：`pwsh -NoProfile -File .\tools\isaac-dual\launch.ps1 -P2 -SkipBuild` 直接复用现有双开启动器和 `out/repentogon/e2e` 的产物，并继续打包、部署、自动连接和开局。改过源码需要使用新构建时，运行常规的 `-P2`。`-SkipBuild` 与 `-BuildOnly` 互斥。
- **手动联机**：`pwsh -NoProfile -File .\tools\isaac-dual\launch.ps1 -LaunchOnly` 沿用 P2 打包和部署，两个窗口就绪后直接交接。到主菜单后，在 **Isaac CS → Session panel** 里先启动 Host（`27090`），再让 Client1 连接 `127.0.0.1:27090`，最后由 Host 选档开局。可同时使用 `-SkipBuild` 复用已有构建。
- **P2 自动连接和开局**：两端到菜单后经各自的控制口启动 C/S，Client1 通过 `127.0.0.1` 连接 Host。确认 Client1 在菜单且为 `AwaitingWorld`、Host 二进制采集器已就绪后，工具只向 Host 窗口发送原生菜单按键，选择第二存档、New Run 并开局。Client1 等待网络世界，工具观察 `client_world_applied=1` 后交接两端 PID 和窗口。二进制同帧对照、故障注入和后续用例接在此准备流程之后。
- **人数顺序**：先两人（P2）跑全套，再四人（P4）扩展，与原方案一致。
- **启动前沟通**：每次实机启动前，告知用户需要配合的操作和何时停止手动操作；需要手动配合时，先确认用户已准备好。
- **连续执行**：遇到失败先修复，再在本步内续测必要的条目，不因失败默认停下。
- **材料保留**：临时材料用完就清理；只在中途停下、需要续测时保留必要材料。
- **不放宽**：不放宽指标，不扩大池或预算上限来让测试通过。

### 15.5 各域统一验收交接清单

> **2026-11-12 修订**：下表中下列各行按[修改方案](client-display-delegation.md)第 8、10 节收窄，其余各行的比较范围改为需求清单中的字段。
>
> - “D0／Random／时钟”：D0 与随机 alpha 退役；时钟按帧号方案B（E33／E34）验收。
> - “配置／图像／shader资源”：只验资源引用。
> - “Music／SFX／环境声”：改由事件队列（E37）验收。
> - “Run／HUD／屏幕UI”：改为原生 HUD 读取已同步状态后与 Host 结果一致，不比较显示生产缓存。
> - “地图L2／Focus／Badge／Question”：只验描述，地图由原生 Minimap 绘制。

这是未来第15步统一整体验收的设计清单，**本次没有运行用例、故障注入、构建或游戏，也没有新增实机授权**。来源是[第9步完整编码交接](9-wire-format.md)、[第10—14步](10-host-capture.md)的实际来源／发布／解码／应用／退役合同及旧E01—E35范围；原编号保留，E31迁出且不计PASS。版本57布局入源、已有辅助函数／纯编译与运行覆盖分别报告，未注册结果先完成正式接线，不能以省略原版必要内容缩小验收。

| 域与来源／沿用条目 | 特殊约束与覆盖内容 | 身份／版本依赖 | 待落实项与记录口径 |
|---|---|---|---|
| 公共协议／收敛：[第9步](9-wire-format.md)、E01/02/05/06/10/11/13/15/17/29 | 版本不符、截断／错长度／mask／epoch、非法引用、变了又变回／最后变化后静止、丢帧ACK与Resync；不完整源不污染发布基线，部分应用不ACK新target | world/body分序列、成员use/shape／epoch和资源／专用依赖完整，成员离开后旧帧不得写到复用号 | 正式工具／故障注入及跨Peer场景待实施；本次未执行，旧E02失败与未执行项保留，不从样本包数推断应用成功 |
| 基础值／容器／资源空间：[第1—2步](implementation-0-2.md)、E09/13/17/25/29 | 固定槽无count、u8/u16显式计数、条件／choice／精确字节、共享投影同count与计数组、空↔非空／缩短／预算／非有限值；资源表间同u16不互换 | 真实生成布局与同目标引用／资源，已裁字段不造线上非法值要求，原生坏源按Gap看待 | 真实边界与失败候选清理／正常owner记录；不复制活STL／地址，不靠截断／默认值通过 |
| Entity公共／Sprite／Plus：[公共职责](entity-common-synchronization-review.md)、[动画资源](animation-resource-synchronization-review.md)、E09/25/28/29/30/34 | exists/dead与本地valid不同；完整selector／最终颜色／playing/speed，路径先变而Image未变、空资源／失败，nullable未设／false／零／非零切换与反射消费 | 当前use／shape、实际资源／动画及同展示时钟；事件／loaded／颜色生产队列不比较Host原值 | 真正保留输入／最终画面与正常退休待验；漂移比较不机械复用Host根偏移读Synthetic或本地scratch |
| Familiar：[结果／绳索](familiar-synchronization-review.md)、E09/13/17/25/28/29/34 | 80真实充能动画与资格、240 DAC分段／XYZ绳索四投影、蝗虫两组合计16384、IsaacsBody／D76表现，非空绳索0节点拒绝 | 主体／owner／资源／同目标几何及真实引用；时钟未知不伪装Frozen，节点未知不写 | typed reader／owner／adapter仍需运行覆盖，不能把生成表或空Weapon算充能通过 |
| Knife／Bomb／Tear：[三类复核](knife-bomb-tear-synchronization-review.md)、E09/13/17/25/28/29/34 | 分裂独立成员／拖影／飞行、五costume＋pending清除读回、三Sprite／DeadEye／Light；42个裁剪范围及旧锚点不比原值，不本地碰撞／重建资源 | 公共最终姿态、真实关系、辅助资源同目标，借用精确恢复 | 场景／非法实际字段／正常撤边与无玩法重演待验；历史未知不因减少线上字段消失 |
| Pickup／Slot：[职责复核](pickup-slot-synchronization-review.md)、E09/13/17/25/28/29/34 | Price与水面／价格Sprite、DropDelay独立隐藏、替代底座／MegaChest及三引用；Slot1—18奖品实际动画／playing。循环切换只比较当前后态，不比未来列表／开奖输入 | 计时与同world时钟，主／辅助资源与真实关系，随机alpha另列 | 空列表／缩短／清理／失败不ACK，确认无定价／购买／捐款／TryFlip／开奖副作用；本次未覆盖 |
| Laser／Projectile：[最终采样](laser-homing-synchronization-review.md)、[版本45](projectile-effect-residual-evidence.md)、E09/13/17/25/28/29 | 两点列不同count／空↔非空及Light unoptimized，真实sample_mode等；Height开关原始u8含非1值、冻结锚点和公共偏移，+350精确恢复，无采样／半帧／callback | 同目标完整几何／资源／flags／use与world帧，PR2不按最新Position伪造 | 当前真实字段／几何／候选替换及异常预算待验，旧版36“私有来源未闭合／只Height”不作当前结论 |
| NPC／Delirium／Text／Dummy：[NPC](npc-control-ui-evidence.md)、[Delirium](delirium-residual-evidence.md)、[Text](text-dummy-evidence.md)、E09/13/17/25/27/28/29 | NPC0/2与拒1、三deque／双Sprite／null Scale恢复／403HP；绕开受控UI与Spawner原版Online筛选。Delirium资源变不无故换代、companion独立；Text原始非UTF8／空／缩短／255界及146counter，Dummy历史cache | 真实RTTI／use与合法producer、Text独立scope不拼ordinary，不能将type964 Dummy套NPC | 实际writer／UI分支／历史显示源与池复用分别观察，无法取得真实场景记录未覆盖，不造假对象补通过 |
| Effect：[当前臂／计时](effect-next-five-evidence.md)、[版本45](projectile-effect-residual-evidence.md)、E09/13/17/25/27/28/29/32/34 | 当前全部正式臂，16/13B、Trail缺失与存在0/1/126/127／拒128、Ghost0..4多投影、typed owner错型／退休；Timeout负值／停走／101双减／141刷新／回绕精确同F还原，不本地Spawn／Timer／传播 | Parent、variant／RTTI、同holder／资源／world目标；静止位置不要求锚点不变 | 来源／多投影／CPU生命周期及真实UNKNOWN待验，已解决producer事实不重新算未决，未核变体仍不宣称语义完整 |
| Player快源／角色结构：[正式Player](implementation-5-player.md)、E09/13/15/17/25/28/29/34 | 四参与者／多身体、主备／held/twin／独立激活及roster/C/S组；P0＋三类Sprite独立块，P6内容变而地址/count不变、旧body丢弃与ordinary body-joint，空槽与跨身体借用 | participant/group空间9/10、真实use/shape、统一P0/P6拓扑和资源；P1—P8不混body，组数不等于身体数 | 全真实角色／身体族与slot容量／快源超64KiB退ordinary路径待验，不以少量静止身体代替支持范围 |
| Player库存／装备／效果／HUD输入：[中三块](5-player-p3-p5.md)、[后三块](5-player-p6-p8.md)、E09/13/17/25/27/28/29 | 固定4Pocket/8材料/6poop、dense／有效innate；四主动六值／空槽规范0，五Weapon槽分型mask／恒零类／10和11／Whip真引用与正常窄Free；P8 disabled与完整有序效果、只704加cooldown、残影／当前0/1节点 | 同资源身份／配置type-id、Player拓扑／组关系及同world，Plus模型不当已核偏移／API全量承诺 | 真实查询显示／装备／容器生命周期待验，不执行获取／使用／到期／HUD Recompute；不得比较已裁History／group/timer等原值 |
| Grid公共／17派生：[第6步](6-grid.md)、[旁表](grid-plus-water-source-evidence.md)、E09/13/17/25/27/28/29/34 | u32水面高位／nullable／Lock第七点及index别名，glitch纹理变换不靠Client地址；Fire≤5×16B与state分支／模板帧、Rock碎块条件、Pressure计时／Door目标Light／主色与extra，正常门别名退休 | 实例use、Room几何／颜色、目标描述／HasWater／Boss匹配／资源与nested_layout同world | 真实工厂／writer／draw恢复与静态描述→原生接线待验；禁止Init／Reward／Update／官方旁表写入补状态，UNKNOWN保留 |
| Room／背景／FX／rails：[版本57](implementation-7-room-level.md)、[显示资源](room-display-resource-design.md)、E09/13/17/25/27/28/29/30/34 | 十二房形／L形／448格最高位及256以上轨道／八门、特殊背景／冷纹理／water32B不二次乘量、独立phase／lighting／两波纹半帧、空ray／poop／vein图／重复展示、强制焦点与不同视口／Options | source-slot、真实几何／门与资源／world/presentation/pass完整，Host裁剪前缀不代表全可显示事实 | 正常来源／GPU／adapter／图形状态与CPU资源退休待验；不本地生成雨／装饰／矿车或执行82EE90／Camera.Update／Host函数表 |
| Level／Descriptor／保存探索：[两域](room-save-exploration-synchronization-review.md)、[版本57](implementation-7-room-level.md)、E09/13/17/25/27/28/29/30/34 | 三维／大房入口象限／负ListIndex／路由／Lil Portal、-19隔离、同count原位映射变化、双配置／HasWater／所有模式Boss／Greed／有效curse；回访／Myosotis／Moving Box／Flip／矿车／Maze／红房／Hourglass／Ascent／Continue的当前后态 | 活动源槽／描述成员／L3映射／双配置和world/use修订，不以ESSM ID续接；探索可回退 | 失效／重建／扫描预算与稳定保存时点待验；官方Greed Getter18204不与真实18374强行相等，不用错误Setter造样本 |
| 地图L2／Focus／Badge／Question：[地图设计](room-display-resource-design.md)、[两域核对](room-save-exploration-synchronization-review.md)、E09/13/17/25/28/29/30 | 可见未访问／完整L2的16类OR/max及9类动态帧、重复／顺序不改聚合；徽标333替三项／376追加／curse顺序／7上限与空列；challenge32两配置锚点／描述／安全169，Lost独立问号空／换图／失败 | L0—L3知识、地图／资源／world修订与显示资格一致，Host按键不控制访客展开／本地排版 | 纯地图／资源和计数／条件错误待验；Gap不折空、不写保存vector、不调用随机Refresh或分数重算，不能只比最后Sprite帧 |
| Run／HUD／屏幕UI：[Run](run-synchronization-review.md)、[Presentation](presentation-synchronization-review.md)、E09/13/17/25/27/28/29/30/34 | TimeCounter基准／七种子投影／ascent/daily、current颜色／两黑暗／最终分数／进度图标；内嵌／动态HUD、24心／4主动／2饰品／4Pocket、Stat三值／星象房、有序History负ID／图像、11消息／fortune／Prompt／过渡／死亡／暂停与屏幕pass | 当前Game／冻结结算／身体显示归组、资源及world／显示修订；本地视口／菜单提交／Options独立 | 未注册根／来源／owner与全部原版必要UI待验；不运行Calculate／UpdateHearts／原版Online选择／退出／解锁／排行榜请求补值 |
| D0／Random／时钟：[逐用途交接](room-random-handoff.md)、E09/13/17/25/28/29/33/34/35 | D0=5B，实际alpha／资格、暂停／重复／不同成员顺序／可见分支／递归／多pass，旧seed不上线；E33在E2E专门构造合法空帧推进时钟，Familiar栈／Pickup临时Sprite消费后精确恢复 | subject/use/shape/pass／调用身份、world/presentation和资源同批；不以仅GameFrame或本地序号映射，旧批次拒绝 | 正常生产结果／外层封装与时钟借用待验；未产生不填0，不能额外Render／抽样，E33夹具不进入产品 |
| 配置／图像／shader资源：[本地生成合同](local-media-presentation.md)、E02/09/13/17/25/28/29/30 | 同名／独立／原位描述变化、StageId35／Type16与Door依赖；实际Image／路径分离、本地程序合成／文字样式／pixel-UV映射、非法kind／媒体正文拒绝、乘加／依赖环／失败退休 | 不可变描述号／空间／epoch与真正必要本地资源ready；不等待取消的Host前后surface／历史图，不要求文字或feedback像素相同 | 有限本地生成／资源闭包与安全图形owner待验；确认不传原始图像／音频，全部有效声明／recipe／结构与重传计首次1MiB/10秒及持续7.5Mbps |
| Music／SFX／环境声：[音频职责](presentation-synchronization-review.md)、[sample来源](room-random-handoff.md)、E09/13/17/25/28/29/30 | 多sample／voice、16环境声／pan、crossfade双voice／六layer／jingle、持续声Baseline／中途加入／静音／停止clear与跨房，one-shot重复状态不重播；FrameDelay非启动延迟 | sample资源、voice代次／逻辑gain/pitch/pan／相位及场景生命周期，本端音量／静音独立 | 正式事件／持续状态／非随机播放leaf待验；不能只按soundID／currentId或仅one-shot断言声音完整，后端静音值不反除 |
| 全程隔离／性能／产品：[现行规范](../../refactor/project-conventions.md)、[总计划指标](README.md)、E02/03/09/17/25/27/28 | Host≥50FPS，采集／动态安装p95≤2ms、p99≤4ms，约30Hz ordinary／60Hz合格身体，512/128/128MiB三池与原带宽目标不放宽；产品无Lua/JSON热链／E2E夹具／第三方适配，四参与者统一输入与C/S隔离保持 | 同一实际发布路径／版本与真实游戏Client，诊断额外成本与正式性能分开计量；访客进度／Hook常驻／生命周期覆盖整个窗口 | 第10—14步全链与工具完成后未来统一验收；不测试原版Online隔离已永久移除项，不以期末dirty=0替代全过程副作用理解，UNKNOWN逐项保留 |

### 15.6 交接状态与连续验收口径

- **文字本地生成**：检查主／副文字、fortune有序行、样式／字体、空串／隐藏、多人owner及消息更新／冷连接。比较实际文字和已登记语义，不以Host文字缓存像素相同为条件；不能为生成缓存运行玩法HUD Update／Show或Lua。
- **直接切房**：普通门、传送、恢复等只要求合法完整目标就绪后直接提交、身份与输入生命周期正确，无半新世界、旧代对象或surface永久Wait；不要求滑动／混合动画，也不删独立Boss／Nightmare／结局语义内容。
- **本端hallucination**：在不同视口／分辨率／帧率、暂停重复显示、冷连接无历史、切房／恢复退休下检查有效性、参数、背景和本地有界合成；不要求Host历史采样／像素一致，不调用完整ShowHallucination或以原始图像兜底。以上覆盖复用本步既有相关用例，不新增提前验收或测试编号。

- **范围与结果分开**：上表均为待落实／待执行范围，没有本轮PASS。回映射用于确定字段／派生／Host／不适用及跨owner结果，不是把旧ReplicatedState原值全量恢复；相同原始数据被多个域引用时只按唯一owner采集，再检查各必要消费者。
- **版本与证据**：记录实际布局版本、场景、key/use/shape与world/body／成员／资源／显示修订，未注册结果／未知ABI／缺合法producer直接说明实际缺口。仅按整数版本兼容，不计算哈希，不设独立审计或预封流程。
- **未知与未覆盖**：发生变化、未观察到变化、未覆盖都保留真实标签；须补证才能改变字节语义。不因字段留Host、计数裁剪、单次静止或SDK声明就称类型完整，不将未执行项目或E31计通过。
- **执行与恢复**：未来按15.4先P2全套再P4扩展并覆盖必要多身体，失败修复后仍在本步骤续测受影响及未完成范围。每次启动分别提示配合与停止手动操作时机，需手动配合时确认准备好才启动；本次没有发起启动或授权询问。
- **材料与说明**：正常测试临时材料用后清理，默认不保留artifact；仅必要中断时留首因／进度／恢复所需材料并说明用途。最终报告实际覆盖／未覆盖、失败／修复／续测和仍存缺口，不用成功结果改写旧失败。

### 15.7 增量应用与双通道补充验收（2026-11-15，仅设计）

本节与[详细设计第12节](13-client-incremental-apply.md#12-第15步补充验收设计)共同规定后续覆盖，当前均未执行，不新增独立测试轮次。位置插值继续暂缓，实机仍停止；恢复后先定位已有 `display_world_not_current` 首因。

| 覆盖范围 | 观察重点 |
| --- | --- |
| 检查与读回复用 | 同一有效窗口内不重复全图检查／读回；原生回调、生命周期、结构／资源变化能正确撤销结果；未知写集不得假定安全 |
| 稳定承载、增量准备 | 只变化块1／3时复用块2；同结构角色更新复用路径与原生容器；归零、清空、删除、同count内容变化均正确应用 |
| 普通 World 未变字段 | 对照 [B3 回调写集与计数](13-client-apply-frequency.md#顺序-3-实施记录)。核稳定复用减少的写入与发布读回量、被跳过的字段发生本地漂移后的恢复，以及成员或承载变化、显示重建、事件与写后失败的完整路径 |
| B4／B5 持久模板与计划片段 | 对照 [复用计数](13-client-apply-frequency.md#顺序-4-实施记录) 和 Plan／Describe／Order／Seal 耗时。覆盖记录编号重排、同地址换实例、条件臂和依赖变更、容器换承载、具名回调触及、World／Body 交错发布和旧批退休后的漂移修复 |
| 整体引用表发布与退休 | 单次读操作固定一个完整版本；旧读者／计划／原生借用结束前不释放；无引用环、重复析构、提前销账或无限退休积压 |
| 独立通道与共享服装 | World慢准备／反馈不占用Body通道；两流交错完成不回退版本，不用旧配对Sprite覆盖最新值，漂移恢复读取最新发布表 |
| joint、取消与反馈 | 结构变更共同提交；交错／迟到回执按票据更新所属流，事件不重播；旧undo不能覆盖新任务，原生失败真实排空后才恢复 |
| 预算与实际性能 | 两流在途共用原内存额度、表快照最多5份、Body等待槽0；分别记录实际应用频率／间隔、主线程总阻塞、端到端延迟及退休占用，不以包数／FPS替代应用数 |
| 完整场景回归 | 保留原第15步内容、多参与者／多身体、输入、正常停止恢复和C/S隔离，不因优化缩减必要状态或掩盖既有失败 |

### 15.8 Host采集热路径补充验收（2026-11-15，仅设计）

对应[第10步性能方案第14节](10-native-capture-performance.md#14-source_native与实体body热路径实施方案2026-11-15)。初次登记时 G—K 尚未实施。现已接入源码，细分计量和克隆的具体口径见[实施记录](10-native-capture-performance.md#1413-k-与-g-的源码实施记录)。本步骤统一覆盖这批改动，现有包32／33的实机记录保留。

| 覆盖范围 | 观察重点 |
|---|---|
| 拓扑批次及输入归属 | P2／P4、多身体、同controller歧义、跨组重叠、Join／换根／死亡复活和设备变化；公共名单同批只读一次，更新前后不借旧见证，失败仍中性／撤销 |
| 区表与租约 | 读取跨页、缓存容量回退、原生getter／writer边界及锁不可用；保留对象范围、异常保护和后端保护，不跨Update持锁 |
| 绑定计划与组校验 | 组状态按递归实例隔离；同count内容／顺序变化、choice、Null、显式空、规范零、非法值和容量失败不漏检；特殊handler及网络解码仍受检 |
| Player联合来源 | World／Body消费同段结构，第二流及最终复核失败整候选回卷；移动射击、换装／动画／costume、奇数Body新结构仍依赖正确ordinary |
| Body批量克隆 | 源范围与目标偏移、共享路径／切片、引用目录相同／不同、Null／Pending、目标容量不足、源票据替换与停止排空；输出独立，真实提交与ACK不变 |
| 性能与发布 | 分场景报告ordinary／body-only主线程排他分位数和样本数、实际发布／应用频率、内存与带宽；覆盖短时实体高负载及现有编码撤销问题，不以机制计数或采集成功替代发布成功 |

详细失败方式、实现文件、统计口径和容量约束以性能方案第14.2—14.8节为准。生产计时关闭旧Recorder／逐字段诊断，显式正确性对照另列附加开销；不降低需求内容、频率或原主线程p95≤2ms／p99≤4ms目标。每次实机启动和必要人工配合仍按项目规范执行，默认不留测试artifact。

### 2026-11-16 性能压力续验（进行中）

用户明确授权直接启动并连续修复续测。本轮为第15步同机隔离P2实机，使用当前G—K源码，尚不代表跨设备／P4或完整性能验收通过。固定原生种子 `S227 7MNF`，夹具生成负载前检查 `AchievementUnlocksDisallowed()`；此前随机开局只检查资格，未启动压力负载。

- 包34（Host34764／Client35924）：8 NPC、64攻击实体持续180.752秒，Host完成tick约44.79Hz、ordinary约22.39Hz，Client World／Body实际发布各约0.459Hz。ordinary outer均值20.816ms，p95分桶上界32.767ms、p99上界65.535ms。采集成功仍伴随后台`capture_revoked_during_encoding`，未通过。
- 包35（Host15480／Client18592）保留ClearReferences前已观察的同use形态后，180.748秒Host约51.83Hz、ordinary约25.91Hz，Client两流仍各约0.459Hz；ordinary outer均值16.543ms，p95／p99分桶上界32.767ms。该修正单独未解决失速。
- 包36—38诊断确认主要撤销来自Sprite：复用地址上的旧use依赖，以及仍存活的Effect执行Reset，均曾全局撤销已封存帧。资源依赖增加拥有者地址（原结构填充内，不增加每项尺寸），主线程诊断记录精确local/use和操作。包39仅处理旧use与已请求移除实体，60.673秒Client约0.429Hz，仍未通过。包40进一步确认剩余高频Reset的owner是Published状态的type1000。
- 包41改为短时实体资源变化只撤当前未提交候选、移除旧依赖，下一采集重读；后台纯值和只追加资源目录保留。Host42272／Client47828在高负载中开始连续应用，但随后因`binary_prepare_wire_budget`退出。失败后余下采样没有有效Client，整段180秒不得用作稳定性能结果。NPC变形／资源变化仍可全局撤销，尚待进一步评估。
- 准备预算失败的代码原因：联合AppliedAck已经完整匹配安装目标，但最新Host来源继续变化，`Preparing()`保持true，导致多次成功准备累计进入同一个1MiB episode。包42改为每次被接受的联合AppliedAck结束该次episode，下次真实准备重新开始；不放宽预算。旧血效逐实体诊断改为随显式E2E trace开关开启，默认性能窗口不再执行该诊断。包42复测进行中。

计量使用实际累计发布次数差分；`client.applied_revision`可随恢复重置，不作为频率。分位数目前是log2微秒桶上界，不能把2.047ms桶当作已精确满足2ms。已观察的三池峰值均未超过512／128／128MiB，但稳定窗口、主线程目标、普通／多身体负载、限速恢复和长稳态尚未完成。

- 包42（Host48564／Client18248）：准备预算首因未再出现，随后约60—70秒成员号达到16384，后台发布转为`body_requires_current_ordinary`。连接保持但实际更新停滞，未通过。用户表示必要时可以提高编号预算；本轮先修复已发送编号永久占用。
- 包43改为目录租约：在途来源、每Peer已确认World／Body目录及事件队列均保留编号，事件实际ACK／过期／容量淘汰时释放自身租约。Host20284／Client44980完成180.672秒相同负载，编号高水位1406、复用69525次，末尾Client持续Displaying，无成员复用拒绝或准备预算首因。Host约49.96Hz，Client两流各约0.902Hz，ordinary outer均值17.390ms，p95／p99桶上界32.767ms，性能仍未通过。三池峰值未超限。编号上限暂未提高。
- 客机主线程准备发现池槽扫描对每个空槽再遍历全部暂存owner。包44将其改为每次同步调用构建固定大小保留位图，不跨帧或原生写入缓存；原子身份／生命周期保护保持。run47（Host40824／Client41076）180.830秒两流约4.20Hz，编号高水位1113。随后无并行构建的60秒窗口Host49.68Hz、Client两流4.567Hz，ordinary outer均值17.490ms，p95／p99桶上界32.767ms；仍未通过。
- 包45／run48（Host46336／Client8040）从干净种子局开始ordinary负载，180.794秒：Host51.94Hz、ordinary25.97Hz、Client World4.701Hz／Body4.718Hz；ordinary outer均值15.227ms，p95／p99桶上界32.767ms，Body outer均值1.209ms。编号高水位871、复用36124次，三池峰值未超限。原夹具普通负载携带饰品1，受击会不断掉硬币，实际世界还包含累积拾取物；不是严格只有8NPC/16攻击实体。场景仍为实际固定种子起始房间，未宣称已经进入旧计划的debug d.10。
- run48正式窗口结束后，短暂停读寄存器／栈作独立诊断，诊断窗口不计性能结果。已捕获的异常栈定位到HostStateCapture::Capture的来源撤销分支，游戏继续运行；热区包括字段格式解析、路径哈希查询和原生写段VirtualQuery。包46预展开不变字段格式，改进x86路径哈希低位分布，并在现有纯写片内缓存16个页区域；跨原生回调仍清空，身份和借用验证不放宽。run49复测进行中。
- 计量纠正：Client只对Install／Manager／端到端导出2ms、4ms阈值计数；其它阶段仅有n和sum_us，临时汇总中的缺失阈值不能解释为零超标。包43／44的部分180秒窗口与诊断构建有重叠，仅用于故障和改进方向；run47后续60秒、run48和run49完整窗口无并行构建。
- 包46／run49（Host18196／Client18980）180.771秒：Host57.60Hz、ordinary28.80Hz、Client World6.544Hz／Body6.600Hz；ordinary outer均值10.494ms、p95／p99桶上界16.383ms，仍未通过。编号高水位580、复用27172次，连接持续Displaying。包47移除纯描述元数据与缺席字段的空原生安装指令，完整plan及Preflight仍保留；包48补充夹具实际实体／拾取物／特效计数。
- 包48／run50（Host12664／Client25884）首次从干净局切换两组Jacob／Esau，实际日志确认四个身体及两组身份、8NPC／16攻击实体，但Host出现`type:scalar_enum`，Client停在PreparingWorld。本轮及时结束，不算四身体稳定窗口。包49／run51（Host15584／Client14624）补充诊断，精确复现`type:scalar_enum layout=431 field=0 value=0`。
- 首因是ResourceItemConfig431错误排除了合法NullItem和饰品类型。依据本地IsaacDocs的ItemType／ItemConfig.GetNullItem，以及官方SDK带边界检查的GetNullItem，版本80→81允许type0—4，Client对0／2分别查NullItem／Trinket目录；仅1／3／4允许负ID程序生成配置。身份、cooldown、图像和costume路径继续严格核对。包50待四身体复测，未把编码修复当成通过。
- 包50／run52（Host48528／Client47564）四身体成功安装并完成180.767秒：Host52.875Hz、ordinary26.437Hz、Client World5.560Hz／Body5.582Hz。ordinary outer均值12.631ms，p95／p99桶上界16.383／32.767ms；Body outer均值2.508ms，p95／p99桶上界4.095ms。编号高水位612、复用22962次，Client末尾Displaying且无安装错误，三池峰值未超限；性能仍未通过。含采样前后共214条有效两参与者负载记录均为四身体、8—9NPC、16攻击实体，实体42—239、拾取物0—142、特效12—92。正常cs stop后客机退休且native_written归零。
- 包50／run53（Host20784／Client21844）接入本机UDP代理，上行50KB/s、下行375KB/s，在正常基线阶段就以`binary_prepare_no_progress resync=0 lane=3 ready=0/0 offered=143/35`断开；还未执行下行限速。代理无丢包，上行排队峰值87685B。失败保留，不能作为限速恢复通过。
- 包51在完整独立Baseline验证后，按精确成员身份、字节和解码结构复用旧不可变存储；不从旧态补全缺席正文，不继承旧表或时间锚。run54（Host6384／Client12028）先将代理上行恢复375KB/s以隔离下行：基线60秒Host58.13Hz、Client World7.793／Body7.908Hz；0.5Mbps下行30秒Host55.52Hz、Client两流2.924Hz；恢复60秒Host52.19Hz、两流5.254Hz。全程Displaying、无断连、代理无丢包，队列峰值11821B，但性能指标仍未满足。
- run54消息统计确认完成历史ACK洪泛：恢复窗口结束时ReceiveAck共79739条，约16.4MB累计上行。Reassembler对保留的全部完成历史每RTO重复确认，即使未收到重复片。包52改为完成时立即ACK，然后仅在真实重复片通过一致性检查后再次ACK；发送方RTO、未完成位图、完成历史去重及AppliedAck完全保留。待50KB/s上行及主动丢ACK回归验证，不能仅凭修代码改判run53。
- 包52／run55（Host45376）在50KB/s上行下完成60秒正常、30秒0.5Mbps下行及60秒恢复。Host各段约60Hz，Client World约13.769／6.233／13.693Hz；代理无丢包，上行排队峰值1466B，恢复末尾ReceiveAck累计5775条，上行约2.99MB。后续连续丢20个纯ReceiveAck的故障注入实际在2.010秒内丢19个、重发18片，然后按2秒无真实进展合同断开；并非单次丢ACK恢复失败，不延长重复片续命。断连后参与者由2降1，该段不能算四身体稳态。
- 包52／run56（Host26800／Client41404）单个纯ReceiveAck丢失后重发5片、完成恢复；后续60.744秒Client持续Displaying、无首因，World／Body各8.248Hz，Host59.364Hz、ordinary29.682Hz。ordinary outer均值9.188ms、p95／p99桶上界16.383ms；Client安装均值25.58ms，501次均超过4ms，性能仍未通过。随后独立短时主线程采样显示页查询、路径索引构建和指令数组扩容热点，该诊断不计性能窗口。
- 包53将原子安装器路径索引改为完整键判等的有界连续开放寻址表，每路径只解析一次生成canonical格式，预留指令容量；连续纯拷贝片内保留页见证，原生回调／容器操作前后仍清空。内存继续计Values池，未削减字段校验或改预算，待相同四身体负载续验。
- 包53／run57（Host35172／Client38676）直连180.666秒：Host56.159Hz、ordinary28.079Hz、Client World8.247／Body8.253Hz；ordinary outer均值10.807ms、p95／p99桶上界16.383／32.767ms；Client安装均值23.768ms、1490次全部超过4ms。编号高水位518、复用27038次，三池未超限，无断连。画面确认期间有受击位移引发切房，因此保留为混合压力诊断，不算严格同房L0b稳定通过。随后独立Host采样的主要额外热点包括ReadMemory页查询（Collector、RefreshLifecycle、输入查询调用者），这些诊断不计正式性能。
- 包54在纯内存安装片内先对照真实当前值，相等时保留原值日志和最终读回、不重写；不同或不可读仍走完整页／范围／原值检查。夹具追加room_index／room_frame，同房压力时通过原生Close(true)关实际打开的门，防止将切房混入稳态；未停止NPC、碰撞、受击、攻击和拾取掉落。待续验，并需覆盖原生写后失败排空恢复。
- 包55包含上述DLL及更新后的夹具。run58（Host32788／Client33344）同房180.749秒：Host58.258Hz、ordinary29.129Hz、Client World8.271／Body8.304Hz；ordinary outer均值10.039ms、p95／p99桶上界16.383ms，Body outer均值2.263ms、p95／p99桶上界4.095ms；Client安装均值25.804ms，1495次均超过4ms，仍未通过。编号高水位347、复用24886次，三池未超限。其后`binary-joint-after-install`命中一次，60.696秒内Client两流各提交364次，末尾Displaying且错误为空、Host无首因；原生写后排空与恢复实际成功。含前后和诊断共310条负载记录均为room84、四身体、8—9NPC、16攻击实体，总实体39—251；不能把这310条全部当作180秒计时段。
- 包56根据独立Host调用栈采样优化两处重复工作：StableConsumerSnapshot只在一次无回调核对内复用8个已验证页区域，身份／阻断／关系值仍逐次读取，跨consumer不复用；CaptureValueWriter将生成布局的canonical投影关系预展开为父块到子块的只读索引，实际mask、choice及子块有效性仍在运行时检查，索引capacity计Values池，分配失败返回采集失败。待实机续验，未改判此前性能失败。

- 包56／run59直连180.777秒：Host57.507Hz、ordinary28.759Hz、Client两流各7.501Hz；ordinary outer均值10.210ms、p95／p99桶上界16.383ms，Body outer均值2.160ms、p95／p99桶上界4.095ms。Client安装均值28.467ms，1356次全部超过4ms；编号高水位379、复用16565次，三池未超限，末尾无首因且Displaying，仍未通过。负载前实机键盘分别移动Host与Client，authority记录controller0的x从320到425.94、controller1从311.74到210.21，彼此未被另一端输入带动。独立诊断采样仍见原子描述／路径索引、页查询及Plus读取热点，诊断不计性能窗口；正常stop后Client retired=1、native_written=0。
- 包57仅新增`ordinary-steady`／`dual-steady`夹具入口：使用回形针（本地Wiki枚举19）保留饰品装备，避免吞下的硬币在持续受击下无界积累金币；原ordinary／dual入口及此前失败事实完整保留。NPC／攻击下限、受击、碰撞、同房及持续时间要求不变，也不删除已有实体。DLL保持包56实现，先实测稳定负载与金币增长过载的区别；该调整不能把旧过载失败改判通过。

- 包57／run60（Host42292／Client31432）`dual-steady`直连180.714秒：Host59.962Hz、ordinary29.981Hz、Client World16.108／Body16.191Hz；ordinary outer均值5.399ms、p95／p99桶上界8.191ms，Body outer均值2.245ms、p95／p99桶上界4.095ms。Client安装均值10.441ms，2911次全部超过4ms；atomic_describe均值8.783ms、native_write均值6.611ms。编号高水位179、复用10996次，三池未超限，无断连。稳定负载与金币增长负载区别明确，但两者性能都未通过，不能据此省略过载恢复。正常stop后Client retired=1、native_written=0。
- 包58准备：已相等EntityPtr不重复SetReference，保留当前安装作用域、原引用日志与最终读回；动画selector完整校验后相等不重写，资源指针／计时／FX计数／具名位转换同样只在实际变化时记录和写入。新增原子描述六段聚合计时（输入、索引、字段、叶、排序、范围），没有逐字段日志。构建与续验进行中，需重新覆盖原生写后失败恢复。

- 包58／run61（Host37696／Client48716）`dual-steady`诊断60.656秒：Host59.862Hz、Client World17.459／Body17.574Hz，Client安装均值7.259ms、native_write3.837ms，性能仍不通过。atomic_describe8.381ms，其中叶指令3.606ms、排序2.050ms、索引0.977ms、字段0.779ms、输入0.478ms、范围0.450ms；这是60秒诊断，不替代180秒验收。其后联合原生写后故障实际命中1次，60.634秒恢复窗口两流各13.458Hz，Client仍Displaying、error为空、Host无首因；正常stop后retired=1、native_written=0。
- 包59仅将每条数字路径的canonical、late、display、根块和costume归属预展开，校验父路径必须在前；不缓存原生地址，也不改变实际写入／回读／资源资格。用于消除每个字段和指令重复遍历静态父链，待续验。

- 包59／run62（Host19252／Client39836）`dual-steady`诊断60.602秒：Host59.998Hz、Client World17.557／Body17.673Hz，Client安装均值7.456ms、atomic_describe7.806ms（叶3.067ms、排序1.932ms），仍未达到性能要求。其后独立Host主线程采样以原生页查询、ReadMemory及采集字段写入为主要项目热点；诊断不计正式性能窗口。正常stop后Client retired=1、native_written=0。
- 包60只为本工程Release静态库和DLL启用MSVC链接期优化，允许跨模块内联／常量传播，仍链接同一外部官方SDK及保留原有检查；全量构建完成，实机续验中。

- 包60／run63（Host22660／Client29980）`dual-steady`180.646秒：Host59.996Hz、ordinary29.998Hz、Client World18.312／Body18.450Hz；ordinary outer均值4.996ms、p95／p99桶上界8.191ms，Body outer均值2.138ms、p95／p99桶上界4.095ms。Client安装均值6.482ms、atomic_describe7.015ms、native_write3.449ms；编号高水位165、复用10245次，三池未超限，无断连，性能仍未通过。负载前两端输入分别移动角色；Host暂停期间Client连按移动未改变位置，恢复后正常显示。稳定场景中段135条记录均room84、四身体、8—9NPC、16攻击、38—69总实体且0拾取物，该记录数不代表全180秒。
- 包61准备修复Host发布端把所有可靠joint误当base=0的重复工作：已有两流真实成功base且配对来源不倒退时走既有可靠联合增量；新启动、恢复或来源早于任何成功base仍全量。所有joint继续完整携带控制组与Player配对投影，新成员由完整新记录闭包核定；其余块按真实来源变化选择。仍共用一个独占联合事务与一次双项ACK，未改线格式、预算、目录引用规则或失败恢复资格。显式E2E trace可记录实际发送的joint base／target，默认关闭；待实机续验。

- 包61／run64（Host7244／Client44388）`dual-steady`180.653秒：Host59.966Hz、ordinary29.980Hz、Client World16.734／Body16.800Hz、线速99460B/s；ordinary outer均值5.286ms、p95／p99桶上界8.191ms，Body outer均值2.176ms、p95／p99桶上界4.095ms。Client安装均值6.571ms、3023次全部超过4ms，atomic_describe7.306ms、native_write3.467ms。编号高水位189、复用13061次，未见池超限／断连，性能仍失败。实际trace先观察到World600→601／Body639→640的非零base联合增量；随后两次注入`binary-joint-after-install`均命中，第二次完整捕获World base0→5126／Body base0→5195及恢复基线事件，客机回到Displaying且error为空。第一次输出仅保留了队尾增量，未声称其捕获了base0。
- 包62／run65（Host21152／Client38624）将稳定分桶排序的整份Op副本改为整数目标位置与原位置换；60.738秒诊断窗口Host59.831Hz、Client World14.439／Body14.587Hz，ordinary outer6.458ms、Body1.921ms、Client安装10.353ms、atomic_describe9.578ms，仍失败。真实效果实体更多（窗口后观察95总实体、66效果、四身体、9NPC、16攻击、0拾取物），不能把跨轮差异全部归因于排序实现。窗口外20秒主线程采样显示ReadMemory、ClientBinaryPlus、WritableRange及完整键树索引仍热；采样为首个DLL栈候选，并非完整展开调用栈。退出前确认Client隔离已退休且native_written=0。
- 包63按上述热点修复：Plus检查比较MSVC原始RTTI类名，仍逐次核当前holder／实际动态类型；字段暂存及特殊字段描述的纯等值树索引改为预留连续索引，完整键判等、碰撞探测、半满上限与实际arena预算保留。未改变地址区间重叠检查的有序树。构建完成，实机续验中。

- 包63／run66（Host32528／Client34576）`dual-steady`180.726秒：Host59.920Hz、ordinary29.963Hz、Client World15.172／Body15.211Hz、线速109164B/s。ordinary outer均值7.343ms、p95／p99桶上界16.383ms，Body2.108ms、p95／p99桶上界4.095ms；Client安装11.253ms（2742次中2741次超过4ms）、atomic_describe10.103ms、native_write5.564ms。编号高水位263、复用33680次，三池有界、无首因或断连，仍未达标。紧接着`binary-joint-after-install`命中1次，60.651秒恢复段Client World15.367／Body15.350Hz且持续Displaying。正常stop后隔离退休为1、native_written=0。
- 包64继续针对原生页查询热点：描述栈内为Plus地址解析复用只读页区表，离开Describe无条件清空指针；原生写入、回读与晚绑定不借该见证。成员预检仅在复用对象的纯读分支共用区表，新建对象进入原生类型调用前清空；真实对象、holder、动态类型和成员身份仍逐次核查。未修改通用ReadMemory或省略页保护／窄SEH。实机续验待完成。

- 包64／run67（Host39688／Client42668）`ordinary-steady`180.639秒：Host59.943Hz、ordinary29.971Hz、Client World15.888／Body15.905Hz、111453B/s。ordinary outer7.471ms、p95／p99桶上界16.383ms；Body1.019ms、p95／p99桶上界2.047ms（该桶不能证明≤2ms）；Client安装10.756ms、2870次全部超过4ms，atomic_describe10.173ms、native_write5.765ms。编号高水位298、复用34812次，未超池预算／断连；两普通身体仍未通过。`binary-joint-after-install`命中后30.720秒Client约16.211／16.178Hz并保持Displaying；stop确认退休且native_written=0。
- 包65／65b进一步修复无条件联合提交：后台Body记录保留完整实际成员引用，真实Body ACK来源独立持有；仅当Body成员／目录号／身份／拓扑／绑定及它的旧引用在新成员表中逐项兼容时，World成员变化走原有单流路径。资源变化、身体增减／结构变更、丢失Body依赖、冷启动或恢复仍走联合事务。当前发送目录和两流结构真实安装后结束该episode，不等待未来采集表。实际容量计费与目录租约保留；加E2E单流发送trace及`dual-steady-changes`复用既有装备／普通身体／双身体动作。构建完成，待实机续验和故障／网络回归。
- 包65b／run68经375000B/s下行、50000B/s上行代理，切换四身体后准备超时；代理无丢包。包66／run69直连复现`binary_prepare_install_deadline`。包67／run70诊断为`binary_prepare_no_progress`；资源等待计数在停滞时不增长，启动阶段累计等待不能解释为资源死锁。
- 包67／run71协议跟踪捕获客机首因`client_member_index_still_borrowed`，先于主机准备超时。包68／run72清除跨成员／资源epoch、按既有RetainedBodyBase规则已不可使用的前代Body解码基线，仍在负载切换后复现相同借用冲突；10.717秒提前终止，无有效稳定性能样本。编号高水位168、复用683，不是16384容量耗尽。两流安装计划仍可能保留对侧旧目录，不能只凭Host当前两流ACK推断所有借用已释放。
- 包69撤回包65跨代表单流发送实验与其额外来源／引用存储，恢复包64的成员换表联合提交路径；保留跨代际无效解码基线释放及按需诊断。上述失败轮次均正常stop，客机retired=1、native_written=0，未提高编号或内存预算。待继续实机回归，性能仍未通过。
- 包69b／run73、run74仍在负载切换后出现准备超时。run74无丢失协议跟踪观察到联合目标World321(base318)／Body633(base631)随后停滞约两秒，之后才发送base0恢复；联合目标330之后再次停滞。普通帧晚到的真实ACK可能已推进联合base，而发送器awaiting_joint只等旧联合ACK，不重新生成可接受目标。包70补充检查：只在真实ACK已超越最近联合增量base时解除该等待，保留旧账本并生成新联合目标，期限与预算不延长。待实机验证。
- 包70／run75（Host41272／Client28392）四身体180.757秒无断连：Host59.959Hz、ordinary29.985Hz、Client World17.676／Body18.544Hz、114172B/s。ordinary outer5.852ms（p95／p99桶上界8.191／16.383ms），Body1.989ms（p95／p99上界4.095ms）；Client World安装8.395ms、3195次中3188次>4ms，atomic_describe8.387ms、native_write4.550ms。编号高水位241、复用21993，三池未超限；仍未达到性能目标。其后独立20秒调用栈采样不计性能。装备变化45.691秒持续Displaying；随后切回普通身体触发`binary_native_init_postcondition`，再以准备超时断连。正常stop后retired=1、native_written=0。
- 包71对纯晚绑定解析窗口复用父路径和页见证，使用Describe已保留容量，退出即清空，不跨原生回调／写入／回读；同时只在Init后置失败时记录实际类型、种子、valid、vptr及配置匹配。构建和实机定位继续进行，未改判上述失败。
- 包71／run76四身体30.637秒诊断：Host59.927Hz，Client19.029／19.714Hz，World安装均值6.063ms、583次中580次>4ms；不替代180秒验收。装备变化保持Displaying，但切回普通身体稳定复现Init后置失败。诊断为卡牌5/300/2，实际／目标type、variant、subtype和seed均相同，valid=0、vptr匹配，仅config不符。字段源entity.json已明确+330是实际选中配置，与按当前三元组查询的配置不同；包72移除Init结束时过早的config相等要求，仍由既有priority4的Config操作在Init后安装并在发布前逐指针读回。继续实机回归。
- 包72／run77（Host34256／Client15076）经本机UDP代理完成四身体→装备变化→普通身体（含掉落卡牌）→四身体，未复现Init配置误拒绝。正常下行375000B/s、上行50000B/s的60.714秒窗口Client18.496／18.628Hz；下行62500B/s持续30秒的30.739秒窗口为8.296／8.263Hz；恢复60.627秒为18.688／19.232Hz。全程无断连，代理无丢包／重复下行分片，峰值队列5521／1466B，结束队列为空。实际注入binary-joint-after-install命中1次，随后45.731秒Client17.406／17.494Hz且Displaying、error为空。以上验证压力切换、带宽降级和该故障恢复，不代表性能阈值通过。
- run77随后连续丢弃ReceiveAck：实际丢弃19个、下行重复分片18次，Host按现行2秒无真实进展合同以binary_prepare_no_progress停止Peer，Client正常退休。该边界测试不计为可恢复丢包成功，也不放宽期限；单次确认丢失另行复测。包73将同样的纯读窗口复用用于发布最终核对及repair=false的漂移检查，各修复setter之前清空，保持主线程身份核验与最终读回。待续验。
- 包73／run78（Host13776／Client28784）四身体180.707秒：Host59.942Hz、ordinary29.971Hz、Client World18.522／Body18.643Hz、120937B/s。ordinary outer6.196ms（p95／p99桶上界8.191／16.383ms），Body2.089ms（p95／p99上界4.095ms）；Client安装5.606ms、3347次中3336次超过4ms，atomic_describe8.403ms、native_write3.187ms。编号高水位208、复用24939次，三池未超限、无首因或断连。随后单个ReceiveAck实际丢失，代理记录一次重复下行分片；30.726秒续测保持Displaying。binary-joint-after-install命中1次，35.766秒续测恢复且无错误。两端独立20秒栈采样在正式计时外执行。正常stop后retired=1、native_written=0。
- 包74／run79（Host36936／Client8236）将不可变字段归属按canonical路径和object缓存，并把回读条目的完整键改为稳定基数排序，内存仍按实际容量计费。四身体180.609秒：Host59.975Hz、ordinary29.987Hz、Client World19.246／Body19.329Hz、127747B/s；ordinary outer6.414ms（p95／p99桶上界8.191／16.383ms），Body2.022ms（p95／p99上界4.095ms）；Client安装5.980ms、3476次中3470次超过4ms，atomic_describe7.027ms、atomic_order1.219ms、native_write3.479ms。编号高水位214、复用24071，三池未超限、无首因或断连。后续装备与普通身体切换保持Displaying，写入后故障续验进行中。性能仍未通过，不将描述阶段变快解释为全部目标达标。
- run79的binary-joint-after-install实际命中1次，35.678秒续测恢复且无错误，正常stop后retired=1、native_written=0。包75继续去除已核格式的重复复制与不消费普通标量的特殊字段查询；晚绑定实际地址改为稳定基数排序，仍完整核重叠／一致投影，临时整数空间在Describe预留。run80普通双身体复测进行中。
- 包75／run80（Host27372／Client43444）普通双身体180.684秒：Host59.967Hz、ordinary29.986Hz、Client World22.963／Body23.832Hz、129971B/s；ordinary outer4.769ms（p95／p99桶上界8.191ms），Body1.036ms（p95／p99上界2.047ms）；Client安装3.702ms、4149次全部超过2ms且682次超过4ms，atomic_describe5.277ms、seal1.117ms、native_write2.054ms。编号高水位185、复用10813，三池未超限、无首因或断连。binary-joint-after-install命中1次，30.651秒续测恢复且无错误。独立20秒和15秒主线程采样在计时窗口之外，仍见InstallField的Plus逐实体页查询及字段索引／分配热点。正常stop后retired=1、native_written=0。包76准备将Plus只读检查集中，并给Host单次RTTI分类增加独立纯读页窗口；未把构建或这些改动当成已通过。
- 包76／run81（Host27956／Client25432）四身体180.703秒：Host59.955Hz、ordinary29.977Hz、Client World19.070／Body19.103Hz、129489B/s；ordinary outer7.090ms（p95／p99桶上界16.383ms），Body2.177ms（p95／p99上界4.095ms）；Client安装5.176ms、3446次全部超过2ms且3311次超过4ms，atomic_describe7.664ms、seal1.761ms、native_write2.643ms。编号高水位249、复用31988，三池未超限、无首因或断连。装备20.718秒、切普通身体25.738秒均持续Displaying；恢复四身体后binary-joint-after-install命中1次，35.622秒续测恢复。stop-supply后10.621秒Client World21.748／Body43.120Hz，无断连；该短段只验证停止补充负载后的更新恢复，不充当长稳态或清空场景通过。正常stop后retired=1、native_written=0。性能仍未通过；包77进一步合并连续纯字节写入，保留逐字段撤销记录，并扩充夹具的zero-empty／last-static后续动作，待实机续验。

- 包77／run82（Host24404／Client35204）四身体180.670秒：Host59.982Hz、ordinary29.988Hz、Client World19.638／Body19.721Hz、127598B/s；ordinary outer6.226ms（p95／p99桶上界8.191／16.383ms），Body1.997ms（p95／p99上界4.095ms）；Client安装4.470ms、3548次全部超过2ms且3025次超过4ms，atomic_describe6.916ms、seal1.454ms、native_write2.324ms。编号高水位216、复用26804，三池未超限、无错误。装备20.676秒、普通身体25.616秒、恢复四身体后的写入后故障命中1次及30.578秒续测均保持Displaying。zero-empty清空实际非玩家实体并将钱钥匙炸弹归零后60.702秒，Client World29.966／Body59.998Hz，两路安装平均1.005／1.105ms，1819／3642次全部不超过2ms；last-static增加1金币后30.715秒，画面实际显示1金币，World30.018／Body60.003Hz。验证了低负载恢复和最终静态更新，高负载性能仍未通过。已发送正常stop并关闭双端；本轮未留存退休回读，不能据此补写retired结果。包78仅细分Seal四段计时，继续定位高负载瓶颈。

- 包78／run83为40秒诊断窗口，World安装平均4.468ms，其中Seal1.503ms；Seal子项为绑定准备1.380ms、附件0.023ms、根快照0.079ms、租约0.021ms。独立20秒主线程采样仍见字段索引及特殊索引扩容初始化，未与性能计时混用。正常stop后retired=1、native_written=0。包79将特殊索引按实际特殊／显示字段数量预留，保留完整键比较及所有字段验证；不改变成员编号或内存总额，继续实机对比。

- 包79／run84四身体60.637秒短时对比：Host60.029Hz、ordinary30.015Hz、Client World19.988／Body20.483Hz、118875B/s；World安装3.391ms、1212次全部超过2ms且95次超过4ms；Seal0.774ms，其中绑定准备0.678ms，native_write2.011ms、atomic_describe5.869ms。比包78诊断窗口的安装／绑定准备均值下降，但负载动态变化，不能将短时均值改善记作性能通过。写入后故障命中1次，30.628秒恢复且无首因；编号高水位165、复用8711，三池未超限。正常stop后retired=1、native_written=0。包80继续消除相邻地址区间重复树查找，区间重叠及投影一致性检查不变，待续验。

- 包80／run85四身体60.731秒：Host59.953Hz、ordinary29.985Hz、Client World20.303／Body21.241Hz、119100B/s；World安装3.215ms、1233次中1231次超过2ms且54次超过4ms；atomic_describe5.334ms，其中叶处理2.028ms，Seal0.747ms、native_write1.885ms。写入后故障命中1次并恢复，正常stop后retired=1、native_written=0。双端各20秒采样在计时窗口以外：Host仍有页查询、拓扑核对及采集值写入成本；Client字段暂存的无效槽查询与过量索引空间仍显著。包81只为实际StageFields预留槽，并用保守路径角色并集跳过非消费者查询，完整对象／路径归属及实际数据验证不变。性能仍未通过。

- 包81／run86（Host26652／Client10000）四身体180.726秒：Host59.958Hz、ordinary29.979Hz、Client World20.030／Body20.451Hz、131772B/s；ordinary outer6.242ms（p95／p99桶上界8.191／16.383ms），Body2.121ms（p95／p99上界4.095ms）。World安装3.968ms，3620次中3619次超过2ms、1774次超过4ms；atomic_describe6.400ms、Seal0.931ms、native_write2.325ms。编号高水位236、复用24153，三池未超限、无断连或首因。装备、普通双身体及卡牌掉落后保持Displaying，恢复四身体后写入后故障命中1次并恢复。zero-empty后60.650秒World29.942／Body60.000Hz，安装1.246／0.961ms，World1816次中12次超过2ms、没有超过4ms，Body3639次全部不超过2ms；最终1金币静态值在客机画面确认。正常stop后retired=1、native_written=0。高负载仍未通过，不把低负载恢复或短窗口改善扩展为全部性能达标。
- 包82追加静态生成字段分类表和LayerImage消费者清单，按实际容量计费；Host根核对在单次纯读调用内共享页见证，Game／Room／索引／维度每次仍读实际值，定制读取器保留原逐次copy回退。改动源于run85独立采样，没有放宽采集／安装指标或编号上限。
- 包82／run87普通双身体180.668秒：Host60.011Hz、ordinary30.000Hz、Client World27.592／Body29.402Hz、151859B/s；ordinary outer4.478ms（p95／p99桶上界8.191ms），Body1.020ms（p95／p99桶上界2.047ms，不能据此宣称不超过2ms）。World安装2.678ms，4985次中4942次超过2ms、51次超过4ms；atomic_describe4.461ms、Seal0.577ms、native_write1.627ms。编号高水位191、复用11163，三池未超限。实际暂停菜单、客机暂停画面及恢复已观察；写入后故障命中1次，随后30.784秒保持Displaying且无首因，World26.475／Body29.561Hz。正常stop后retired=1、native_written=0。高负载性能仍未通过。
- 包83减少字段描述中的完整输入复制，改为借用同一准备／安装租约内地址稳定的暂存槽；回读排序跳过全批一致的字节，仍保留完整键和稳定顺序。Host计时增加超过2000／4000微秒的精确计数，以消除直方图桶边界歧义；尚待实机续验。
- 包83／run88（Host31880／Client17568）四身体60.713秒：Host59.872Hz、ordinary29.928Hz、Client World22.153／Body23.735Hz、127686B/s。ordinary outer4.881ms，1819次全部超过4ms；Body2.114ms，1819次中980次超过2ms、2次超过4ms。World安装3.176ms，1345次中1342次超过2ms、89次超过4ms；atomic_inputs0.101ms、atomic_describe4.650ms、Seal0.646ms、native_write1.910ms。编号高水位147、复用3506，无新错误，性能仍未通过。装备变化后写入后故障命中1次并恢复，随后30.699秒健康；普通双身体／卡牌掉落及重新四身体均保持显示。zero-empty后30.602秒World29.998／Body59.898Hz，安装0.915／0.874ms，World918次全部不超过2ms、Body1833次中1次超过2ms且没有超过4ms；最终1金币在客机画面确认。上述为空场景短窗，不代替高负载180秒验收。
- run88计时窗口外分别采样双端20秒，栈地址仅作候选而非可靠展开。客机纯读父路径表每次回读全量初始化仍有成本；包84改为仅清空实际访问项，所有地址仍在单个纯读窗口结束时清零，工作区仍按容量计费，待续验。
- run88正常stop后客机retired=1、native_written=0。包84／run89四身体60.677秒：Host59.974Hz、ordinary29.979Hz、Client World23.024／Body25.298Hz、126969B/s；ordinary outer5.003ms，1820次全部超过4ms；Body2.147ms，1821次中1344次超过2ms、1次超过4ms。World安装3.050ms，1397次中1393次超过2ms、43次超过4ms；atomic_describe4.405ms、Seal0.637ms、native_write1.851ms。编号高水位150、复用3444，三池有界、无新错误，性能仍未通过。装备变化后写入后故障命中1次并恢复，30.615秒窗口持续Displaying；普通身体／卡牌、重建四身体、zero-empty与最终1金币均在客机确认，正常stop后retired=1、native_written=0。
- 包85把两个主线程描述器重复建立的完整规范路径索引移入后台不可变安装计划；完整父链、canonical字段、元素和Choice仍参与归属。字段路径解析在所有路径齐备后分片完成，数字结果随计划计费；原生定位、生命周期、字段校验与写入仍在主线程。尚待实机续验。
- 包85／run90（Host38312／Client27076）四身体180.704秒：Host59.971Hz、ordinary29.988Hz、Client World23.242／Body25.019Hz、129270B/s；ordinary outer4.776ms，5419次全部超过4ms；Body2.062ms，5419次中2308次超过2ms、2次超过4ms。World安装3.051ms，4200次中4174次超过2ms、135次超过4ms；atomic_describe4.151ms、atomic_index0.596ms、Seal0.634ms、native_write1.867ms。编号高水位165、复用10281，三池有界、无首因或断连，性能仍未通过。装备变化后写入后故障命中1次并恢复，随后30.720秒保持Displaying；普通身体／卡牌、重建四身体与zero-empty继续正常。
- 包86继续把完整(object,规范路径)字段归属索引移入后台，槽只存字段下标并比较完整键，保持首个投影归属；分片生成和有界查询保留。静态检查补齐scratch扩容重试时规范路径索引的回卷，并消除常量0的收窄警告。原生安装增加初始化、字段owner准备、晚绑定定位和图形收尾四段聚合计时，完整安装总量不变。待续验。
- run90最终1金币已在客机确认，正常stop后retired=1、native_written=0。包86／run91（Host48344／Client20652）四身体60.521秒：Host60.095Hz、ordinary30.039Hz、Client World20.522／Body21.034Hz、124006B/s；ordinary outer5.204ms，1818次全部超过4ms；Body2.013ms，1818次中648次超过2ms、没有超过4ms。World安装3.535ms，1242次中1241次超过2ms、199次超过4ms；atomic_index0.132ms、atomic_describe4.405ms、Seal0.728ms、native_write2.192ms，其中初始化0.066ms、owner准备0.0005ms、晚绑定定位0.513ms、图形收尾0.141ms。编号高水位201、复用4956，三池有界，无新错误，仍未达标。装备变化后写入后故障命中1次并恢复，随后30.565秒保持Displaying。
- run91切回普通双身体后的60.703秒仍包含前序装备与掉落，不当作新开局L0a：Host59.898Hz、ordinary29.966Hz、Client两流均19.867Hz，ordinary outer7.790ms、Body1.055ms；Body1819次全部不超过2ms，World安装4.652ms、1206次中1203次超过4ms。动态负载不同，不将这一窗直接归因于身体数或索引改动。计时外诊断改用StackWalk64：首轮首帧处理错误只取得PC，修正后房主638／客机702样本取得多层栈；最长单次暂停约61／82ms，诊断不计入性能结果，随后双端仍健康。客机FieldAt及ResolveWrites、Host Store／SetSlice为实际热点。
- 包87将字段索引完整键和结果放在同一槽，避免查询跳转原始记录／路径数组；晚绑定排序省略整批恒定的地址字节，保留完整地址冲突检查。Host已绑定父路径的标量切片直接登记最终cell，保留事务、容量、宽度和根arena归属检查；不改变值或采集频率。待续验。
- run91恢复四身体、zero-empty与最终1金币继续在客机确认；正常stop后retired=1、native_written=0。两端展开采样结束后没有断连或新首因。

- 包87／run92（Host4316／Client44200）普通双身体180.856秒：Host59.970Hz、ordinary29.991Hz、Client World27.845／Body29.465Hz、155618B/s。ordinary outer4.309ms，5423次全部超过2ms、4089次超过4ms；Body0.936ms，5424次全部不超过2ms。World安装2.659ms，5036次中4993次超过2ms、68次超过4ms；atomic_describe3.375ms、Seal0.573ms、native_write1.620ms。Host entity_values2.529ms、scene_values0.642ms。编号高水位187、复用11143，三池有界、无新增首因，性能仍未通过。写入后故障命中1次并恢复，随后30.781秒World27.062／Body29.271Hz、持续Displaying；正常stop后retired=1、native_written=0。
- 包88将客机稳定排序的逐项交换改为置换环单次移动，保持完整排序键、桶内原顺序和操作身份；Host当前窗口直接借用同一证言时避免自身逐成员比较，其他证言仍完整比较，线程／读段／根身份检查保留。分析中一度误判资格登记条件，复核后撤回该判断和修改，没有将其计为修复。待四身体实机续验。

- 包88／run93（Host21428／Client37692）四身体180.878秒：Host59.985Hz、ordinary29.993Hz、Client World22.728／Body24.337Hz、132188B/s。ordinary outer4.691ms，5425次全部超过4ms；Body1.871ms，5425次中746次超过2ms、没有超过4ms。World安装3.065ms，4111次中4091次超过2ms、264次超过4ms；atomic_describe3.893ms、atomic_order0.762ms、Seal0.650ms、native_write1.860ms。编号高水位228、复用12467，三池有界、无新增首因，性能仍未通过。装备变化后写入后故障命中1次并恢复，30.761秒World19.570／Body19.538Hz；随后普通身体／卡牌、重新四身体、zero-empty与最终1金币均在客机画面确认。正常stop后retired=1、native_written=0。
- 包89把标量及其父字段的完整归属查询合入后台原有128项分片，主线程消费同一不可变计划的数字下标，原生资格检查不变；小端平台的1／2／4／8字节标量改用合法未对齐memcpy读取，其他宽度／字节序保留逐字节路径，所有原有数值validator仍执行。待实机续验。

- 包89／run94（Host46796／Client45536）普通双身体180.718秒：Host59.989Hz、ordinary29.997Hz、Client World20.159／Body20.391Hz、137777B/s。此轮中途实际负载158实体／132特效，明显高于部分前轮，不能直接归因于代码回归或改进。ordinary outer6.943ms，5420次中5317次超过4ms；Body0.987ms，5420次中2次超过2ms、没有超过4ms。World安装4.778ms，3643次中3642次超过2ms、3366次超过4ms；atomic_describe5.084ms、atomic_leaves2.340ms、Seal1.058ms、native_write2.930ms。编号高水位312、复用38301，三池有界、无新增首因，仍未达标。写入后故障命中1次并恢复，30.694秒两流均19.906Hz；Host暂停菜单及恢复后的客机推进已观察，正常stop后retired=1、native_written=0。
- 包90为冷计划已证明的定长、无条件、非共享、无计数组且无handler的Copy叶提供直接采集分支。保留原双次访问计数、动态资格、Copy窗口与arena范围、数值validator、selector位表示、块长度与根尾复核；其余字段沿原完整分派。待实机续验。

- 包90／run95（Host33868／Client43660）四身体180.913秒：Host59.940Hz、ordinary29.976Hz、Client World20.739／Body21.568Hz、136222B/s。ordinary outer5.440ms，5422次中5105次超过4ms；Body1.811ms，5423次中689次超过2ms、没有超过4ms。World安装4.020ms，3752次中3738次超过2ms、2125次超过4ms；atomic_describe4.089ms、atomic_leaves2.013ms、Seal0.875ms、native_write2.445ms。Host entity_values3.065ms、scene_values0.566ms。编号高水位247、复用28083，三池有界、无新增首因，仍未达标。装备变化后写入后故障命中1次并恢复；普通身体／卡牌、重建四身体、zero-empty及最终1金币均在客机确认，正常stop后retired=1、native_written=0。
- run95计时外两端各10秒StackWalk64采样取得Host663／Client740样本，最长单次暂停约19.4／25.3ms；诊断不计入上述性能。Host采集调用链中的页查询32样本，整体还有生命周期／输入查询；read_fallbacks=0，不能据此声称Host页表容量耗尽。客机WritableRange在InstallNow路径19样本，FinishFields的Plus页查询11样本。包91将客机纯读／纯写页表从16项环缓存改为64项有序定长表，表满继续查询；原回调／写入边界的清空、实际地址范围、页保护和SEH均保留。待实机续验。
- 包91／run96（Host23176／Client21528）四身体60.768秒诊断采样：Host59.966Hz、ordinary29.983Hz、Client World22.775／Body24.042Hz、132178B/s；本轮不足180秒，不作为持续负载通过。ordinary outer4.320ms，1822次全部超过2ms、1633次超过4ms；Body1.914ms，1822次中296次超过2ms、没有超过4ms。World安装3.234ms，1384次全部超过2ms、74次超过4ms；atomic_describe3.385ms、Seal0.661ms、native_write2.023ms。编号高水位181、复用3753，三池有界、无新增首因，仍未达标。装备变化后写入后故障命中1次，30.772秒续测World22.163／Body22.781Hz；普通身体／卡牌、重建四身体、zero-empty及最终1金币均在客机确认。正常stop后retired=1、native_written=0。
- 包92继续同一验收：IEEE754平台以指数位判断浮点有限性，拒绝NaN和正负无穷，非IEEE平台保留原转换；其余数值validator不变。客机页缓存失效改为撤销有效前缀和最近页，旧槽位不可达，下次访问仍须重新查询；避免每次回调擦写整张64项表，失效边界不变。待实机续验。
- 包92／run97（Host35180／Client15492）四身体180.812秒：Host59.941Hz、ordinary29.970Hz、Client World24.429／Body26.304Hz、134570B/s。ordinary outer4.088ms，5422次全部超过2ms、2613次超过4ms；Body1.744ms，5422次中543次超过2ms、没有超过4ms。World安装3.019ms，4417次中4399次超过2ms、114次超过4ms；atomic_describe3.260ms、atomic_leaves1.557ms、Seal0.629ms、native_write1.857ms。编号高水位149、复用10800，三池有界、无新增首因，仍未达标。装备变化后的写入后故障命中1次，30.856秒续测World23.950／Body24.760Hz；普通角色／卡牌、重建双角色、zero-empty及最终1金币均在客机确认；正常stop后retired=1、native_written=0。
- 包93将逐叶维护的地址树改为描述结束时的稳定地址排序：对未合并的完整元素区间核同址一致性和部分重叠，重复投影保留最早项，然后按原顺序压缩、合并同字段连续元素并分配撤销日志。262144条最终操作预算、Values池计费、原生写入前预检和晚绑定后二次区间核验保留。仅E2E构建新增binary-write-overlap／binary-write-conflict候选计划故障，准备检查拒绝和恢复；产品构建不包含注入分支。待实机续验。
- 包93／run98（Host36228／Client29988）先后注入部分地址重叠和同址目标冲突，各命中1次；原生日志分别记录serial736／1321的binary_atomic_projection_write_conflict拒绝，随后恢复Displaying。之后四身体180.690秒：Host59.987Hz、ordinary29.991Hz、Client World23.720／Body24.938Hz、139538B/s。ordinary outer4.172ms，5421次全部超过2ms、3174次超过4ms；Body1.798ms，5421次中658次超过2ms、1次超过4ms。World安装3.292ms，4286次中4266次超过2ms、281次超过4ms；atomic_describe3.168ms、atomic_leaves1.440ms、Seal0.659ms、native_write2.080ms。编号高水位180、复用11878，三池有界、最终无活动错误，仍未达标。原生日志也出现准备票据被联合候选撤回的cancelled／description_revoked及恢复记录，不能将最终状态无错误解释为完全没有候选拒绝。
- run98装备变化后写入后故障命中1次，30.801秒续测World20.649／Body21.071Hz；普通角色／卡牌、重建四身体、zero-empty及最终1金币均在客机确认；正常stop后retired=1、native_written=0。计时外两端各10秒StackWalk64取得Host696／Client747样本，最长单次暂停16.425／16.859ms。Host采集链页查询27样本，Client叶处理／Add各20样本、CanonicalField12样本；完整计时窗口有151058009次读取、1202084303B、998091次页查询、0次区表回退。索引修改后阶段均值变化不能脱离实际负载差异解释为同工作量加速。
- 包94继续优化纯标量校验的静态分派：无逐值约束的整数保留宽度与调用者结构校验，浮点仍逐项拒绝NaN／Inf；范围、枚举、掩码及特殊值规则保持。只有EntitySource的纯内存reader声明不改变根／线程／证言且在复制前后检查外部writer；CaptureWindow.Copy保留入口、地址／范围和reader保护，省略该reader已经覆盖的末尾重复Check，其他reader默认维持双侧检查。最终来源复核和提交检查不变。待实机续验。
- 包94／run99（Host39516／Client26456）普通双身体180.814秒：Host59.973Hz、ordinary29.987Hz、Client World20.751／Body20.950Hz、139317B/s。ordinary outer5.929ms，5423次全部超过2ms、5032次超过4ms；仅Body0.863ms，5423次均未超过2ms。World安装4.154ms，3752次中3738次超过2ms、2674次超过4ms；atomic_describe4.006ms、atomic_leaves1.864ms、Seal0.973ms、native_write2.478ms。编号高水位303、复用34981，三池有界，仍未达标。随后负载记录实际为152实体／126效果／8NPC／16攻击，并非只有两名玩家和24个目标；本轮177326084次读取、1339827261B、992507次页查询、0回退。写入前故障命中1次，日志serial5888记录binary_e2e_world_before_install；30.756秒续测World19.053／Body19.086Hz，最终Displaying且活动错误为空；正常stop后retired=1、native_written=0。
- 包95补充执行器原生连续段：现有生成CopyRange因变长字段之后wire_offset为动态而无法合并部分合法字段。执行器按同一已选块的实际遍历顺序，只合并严格原生地址相邻、无条件、无handler／计数组／共享owner且根稳定资格相同的定长Copy叶；不跨UNKNOWN、填充或容器尾部。每个字段仍独立登记cell、消费selector和validator；私有段表在冷准备预留并按Workspace容量计费，普通Dispatch与线上布局不变。待实机续验。
- 包95／run100（Host24748／Client43308）普通双身体180.821秒：Host59.977Hz、ordinary29.991Hz、Client World21.928／Body22.315Hz、141526B/s。ordinary outer5.363ms，5423次全部超过2ms、4378次超过4ms；仅Body0.867ms，5424次均未超过2ms。World安装3.730ms，3965次中3950次超过2ms、1784次超过4ms；atomic_describe3.553ms、Seal0.861ms、native_write2.222ms。编号高水位291、复用30398，三池有界，仍未达标。本轮166354848次读取、1259127155B、726682次页查询、0回退；与run99负载不同，不能将所有差值归于连续段优化。写入后故障命中1次并恢复，30.811秒续测两流均19.669Hz，最终Displaying且活动错误为空；正常stop后retired=1、native_written=0。
- 包96将客机操作表保留为稳定物理槽位，排序只改变32位逻辑索引；安装、附件索引、读回、晚绑定冲突检查和反序撤销均消费同一逻辑顺序，两数组仍由Peer计费。Host私有Read不再先复制完整subject再与自身比较，外部Current入口仍比较完整身份；读段序号、线程、实际来源资格和复制前后外部writer检查保持。F32有限性直接位读取另限定小端平台，其他平台走原通用转换。待实机续验。
- 包96／run101（Host45124／Client8520）两项地址冲突各命中1次，serial1741／2236均记录projection_write_conflict拒绝并恢复。四身体180.850秒：Host59.945Hz、ordinary29.975Hz、Client World19.834／Body20.127Hz、127088B/s。ordinary outer4.701ms，5421次全部超过4ms；仅Body1.616ms，5422次中375次超过2ms、没有超过4ms。World安装3.943ms，3587次全部超过2ms、1047次超过4ms；atomic_describe3.685ms、atomic_order0.700ms、Seal0.864ms、native_write2.386ms。编号高水位213、复用28801，三池有界，仍未达标。装备变化后写入后故障命中1次，30.757秒续测World19.930／Body20.125Hz；普通角色与卡牌已在客机确认。
- 索引排序实验未显示可靠总体收益，而引入了每次操作访问的间接寻址；包97撤回该项，恢复包93的物理指令稳定排序，保留批量地址冲突检查及Host读取优化。不把不同负载的均值直接认定为该实验造成的回归。包97还将采集热状态与容器来源游标分为两个同深度／字段切片：每次Walk仍重置全部标量、selector和完成位；容器游标仅在本实例首次Open时恢复默认值，所有count／presence／identity消费仍发生在Open之后，子实例不能借用兄弟实例来源；冷容量与热状态一起计入Workspace。待实机续验。
- run101后续重建四身体、zero-empty与最终1金币均在客机确认。最后静态空场景另采30.803秒诊断：Client World29.998／Body59.995Hz；World安装0.995ms（924次）、Body安装0.901ms（1848次），两者均无超过2ms的样本。Host ordinary2.594ms，922次全部超过2ms但未超过4ms；仅Body1.658ms，922次中35次超过2ms、没有超过4ms。由此可区分空场景能达到双流频率与高负载仍失败；该短窗不能替代L0压力验收。正常stop后retired=1、native_written=0。
- 包97／run102（Host23276／Client9060）四身体180.818秒：Host59.978Hz、ordinary29.992Hz、Client World22.968／Body23.731Hz，130126B/s。ordinary outer3.940ms，5424次全部超过2ms、1282次超过4ms；仅Body1.718ms，5423次中494次超过2ms、无超过4ms。World安装3.680ms，4153次中4152次超过2ms、615次超过4ms；atomic_describe3.360ms、native_write2.384ms。编号高水位157、复用10876，三池有界，缺口504保持不变，仍未通过性能要求。装备变化后joint-after-install命中1次，30.803秒续测World23.764／Body24.705Hz，普通角色与卡牌、四身体重建、zero-empty及最终1金币均在客机确认。正常stop后retired=1、native_written=0，两端正常退出。
- run102计时之外使用真实StackWalk64诊断，Host1065次／Client1148次，最长暂停18.314／20.741ms。Client最上层项目帧主要为Add42、Leaf34、Describe21、Kind16、LatePath11等；这些采样不计入性能窗口。包98将候选Op原位构造，压缩时仅在出现空洞后移动，保留原顺序、重复地址拒绝、原始／压缩容量及回退；不可变字段的特殊分类和晚绑定资格共用冷表，仅对生成数组的真实元素引用采用数字定位，其他定义与缓存分配失败仍走完整分类。待实机续验。
- 包98／run103（Host28172／Client26548）地址重叠／冲突故障各命中1次，serial36／228均以projection_write_conflict拒绝并恢复。四身体180.784秒：Host59.978Hz、ordinary29.992Hz、Client World23.393／Body25.456Hz、134443B/s。ordinary outer3.984ms，5423次全部超过2ms、1756次超过4ms；仅Body1.714ms，5423次中466次超过2ms、无超过4ms。World安装3.406ms，4229次中4221次超过2ms、380次超过4ms；atomic_describe3.170ms、atomic_leaves1.422ms、atomic_order0.636ms、native_write2.152ms、native_resolve0.563ms。编号高水位185、复用11282，缺口506保持不变，三池有界，性能仍未通过。计时外1148次Client调用栈中Add20、Leaf32、WritableRange18等，最长暂停23.167ms，不混入压力计时。装备变化后joint-after-install命中1次，30.886秒续测World24.315／Body26.161Hz。普通角色与卡牌、四身体重建、zero-empty及最终1金币均在客机确认；正常stop后retired=1、native_written=0，两端正常退出。
- 包99复用IndexWrites已有地址顺序执行原生页／附件预检，然后按原插入顺序压缩和执行排序；不改变任何实际检查范围，不跨写入复用页见证，晚绑定后的地址解析和冲突检查保留。转换值在所有叶收集完成后才绑定稳定缓冲，压缩仅合并已逐段预检的连续纯字节。AtomicRanges单独计量，从AtomicLeaves扣除同段时间，主线程总时间不改；增加完整描述批次的原始／最终／晚绑定／委托操作数聚合，容量计费和262144条上限不变。待实机续验。
- 包99／run104（Host49028／Client15260）普通双角色，地址重叠／冲突各命中1次，serial42／197正确拒绝。180.742秒：Host59.969Hz、ordinary29.988Hz、Client World28.875／Body29.722Hz，159901B/s。ordinary outer3.819ms，5419次全部超过2ms、1324次超过4ms；仅Body0.998ms，5419次全部不超过2ms。World安装2.766ms，5219次中5195次超过2ms、60次超过4ms；atomic_describe2.482ms、atomic_ranges0.353ms、native_write1.692ms、native_resolve0.493ms，仍未通过普通采集和World安装要求。编号高水位221、复用11872，缺口506不变，三池有界。完整描述窗口累计raw/native/late/delegated操作120825642／102841526／27092066／11799957，晚绑定约占最终操作26.3%。窗口后夹具记录56实体，其中8NPC、16攻击、30自然效果；不得将不同效果数量的均值差直接归因于优化。world-after-install故障命中1次，30.822秒续测World29.200／Body30.433Hz；正常stop后retired=1、native_written=0，两端正常退出。
- 包100保留当前候选静态地址顺序：压缩阶段记录原始→紧排映射，物理执行排序后用完整order映射实际下标；晚绑定阶段只排序实际新地址，再按地址／执行序号归并全部区间，沿用原逐项范围、交叠和一致目标检查。静态表使用既有Peer allocator计费，不跨候选保留地址，不改变Init、写入或undo顺序。需要覆盖压缩／合并后的下标映射、两显示阶段隔离、同地址排序和后Init失败回退；新增binary-late-overlap／binary-late-conflict仅改候选晚绑定地址，预期resolved_projection_conflict拒绝并恢复。待实机续验。
- 拟继续减少Host连续纯标量段的登记开销：只使用同块、相同根稳定资格、无条件且无handler／共享／计数组的严格连续Copy字段。实施前明确失败方式：不能跨未知字节或handler；模板错位不得覆盖其他字段；递归／兄弟实例路径不得混用；字节／条目／访问预算不足整根回卷；单个NaN／Inf或其他数值约束仍拒绝整根；失败后不得留下可发布cell或复用资格；公共冷容量必须按实际模板capacity计费。仅在本第15步中继续实机覆盖，不增加隔离单测。
- 包100／run105（Host3032／Client9072）新增late-overlap／late-conflict各命中1次，serial34／331在Init之后以resolved_projection_conflict使候选失效并恢复显示。四身体180.781秒：Host59.962Hz、ordinary29.981Hz、Client World19.991／Body20.074Hz、131242B/s。ordinary outer4.774ms，5422次全部超过2ms、5420次超过4ms；仅Body1.760ms，5422次中574次超过2ms、无超过4ms。World安装3.718ms，3614次全部超过2ms、561次超过4ms；atomic_describe3.534ms、atomic_ranges0.450ms、native_write2.212ms、native_resolve0.522ms。编号高水位205、复用27979，三池有界、缺口505不变，性能仍不达标。装备变化后joint-after-install命中1次，30.806秒续测World20.061／Body20.223Hz。普通角色与卡牌、四身体重建、zero-empty和最终1金币均已在客机确认；正常stop后retired=1、native_written=0，两端正常退出。
- 包101实现上述Host纯标量批登记：冷准备为同块连续合格字段建立cell模板，在同一读取段一次保留arena、复制及登记路径／条目；所有字段保持独立cell、数值validator和selector值，双次访问额度、字节／块／根预算、资格和最终证言复核不变。模板只由私有执行器入口消费并按实际Workspace容量记账，不扩展任何原生读取范围或跨handler借用。失败撤销整根；cell容量不足仍定位首个超出字段。构建成功，待实机续验。
- 包101／run106（Host3952／Client17224）四身体180.819秒：Host59.966Hz、ordinary29.980Hz、Client World20.424／Body20.612Hz、136964B/s。ordinary outer4.869ms，5423次全部超过2ms、4917次超过4ms；仅Body1.617ms，5423次中362次超过2ms、无超过4ms。World安装3.875ms，3693次中3679次超过2ms、1205次超过4ms；atomic_describe3.699ms、native_write2.294ms、native_resolve0.558ms。编号高水位242、复用29580，三池有界、缺口506不变，性能仍未达标。计时外将StackWalk64深度提高到48并保留25ms软限，1220次采样，最长暂停25.889ms；包含BinaryEntities::Prepare的280个样本中，最内层项目帧QueryNativeReadRegion66、Copy20、Track14、Walk13、WriteBoundScalarBatch10等，确认批登记实际执行；不把这些暂停混入性能计时。其后夹具94实体，含9NPC、17攻击、64自然效果。装备变化后joint-after-install命中1次，30.793秒续测两流均19.907Hz。普通角色与卡牌、四身体重建、zero-empty及最终1金币均已在客机确认；正常stop后retired=1、native_written=0，两端正常退出。
- 包101／run107（Host15148／Client48720）64攻击实体过载180.852秒：Host59.264Hz、ordinary29.632Hz、Client两流13.038Hz、135881B/s；ordinary outer10.570ms，5360次全部超过4ms；仅Body0.789ms，5361次全部不超过2ms；World安装8.577ms，2358次全部超过4ms。编号高水位553、复用72912，随后持续过载及限速窗口高水位651、复用161042，未触及16384预算。窗口外观察309实体，含9NPC、64攻击、234自然效果。追加60秒基线／30秒62500B/s下行限速／60秒恢复：Client World约12.565／4.881／12.434Hz、Body约12.565／4.848／12.418Hz；三段均Displaying、无最终错误，缺口506不变。代理无丢包或溢出，峰值下行队列8168B，恢复完成队列为空。两端三池峰值均在512／128／128MiB内。压力下继续发布和恢复正常，不据此宣称L0性能通过。停止补给、zero-empty及最终1金币均在客机确认；正常stop后retired=1、native_written=0，两端正常退出。
- 包101／run108（Host37936／Client17900）只作标准双角色调用栈诊断，不作为180秒性能验收。客机30秒、2192次48层采样，最长暂停25.771ms；主线程安装栈中真实页查询主要来自Plus检查和可写区检查。采样结束后正常stop，retired=1、native_written=0，两端正常退出。
- 包102增加read_page_queries／read_page_full／write_page_queries／write_page_full固定聚合计数，分别统计显式原子Describe／Seal／InstallNow／PublishNow调用段，含失败前缀；不含未进入这些段的渲染、后台或单独漂移修复。未修改64项页缓存容量和有效期。run109（Host47584／Client8088）标准双角色约30秒诊断：World发布28.084Hz、Body29.416Hz，World安装2.764ms、865次中862次超过2ms、15次超过4ms。World计入读页查询58173、写页查询62121，两类表满回退均0；Body同样无表满回退，因此否定“扩大64项表即可消除此轮查询开销”的猜测。未称为180秒验收通过。正常stop后retired=1、native_written=0，两端正常退出。
- 包103将EntityReference预检的可读页见证放入当前ClientAtomicReadRegion；只减少连续检查段内重复VirtualQuery，真实holder/vptr仍每次读取。与原写页见证在所有回调、EntityPtr setter和附件边界同时Clear，修复路径仍每项独立作用域；不跨帧借用，不提高页表或编号预算。准备实机续验。
- 包103／run110（Host37376／Client32328）标准双角色180.825秒：Host59.964Hz、ordinary29.985Hz、Client World29.006／Body29.913Hz、161127B/s。ordinary outer3.574ms，5425次全部超过2ms、560次超过4ms；仅Body0.808ms，5424次全部不超过2ms。World安装2.631ms，5245次中5217次超过2ms、47次超过4ms；atomic_describe2.465ms、native_write1.562ms、native_resolve0.341ms。World读页查询311706、写页392659，两类表满回退均0。编号高水位184、复用11251，两端三池有界、缺口506不变。不能把不同自然效果数量下的跨轮均值差归因为本次修改，L0仍未通过。world-after-install命中1次；随后30.766秒恢复World28.961／Body30.553Hz，无新错误。正常stop后retired=1、native_written=0，两端正常退出。
- 包104实现Prepare内显式纯读页作用域，详见10-native-capture-performance.md第14.14节。每个独立Body仍拥有自己的read_serial、PlayerPost窗口、来源值与证言；仅页可读区域在getter之后、Prepare返回之前复用。段间外部资源水位变化、撤销／异常均清空；实体／资源／生产回调在段间也撤销。页表和编号容量不变，增加page_spans／page_continuations聚合计数，准备四身体及装备／角色变化实机续验。
- 包104／run111（Host40900／Client19852）四身体180.804秒：Host59.993Hz、ordinary29.999Hz、Client World25.121／Body27.024Hz、140470B/s。ordinary outer3.702ms，5424次全部超过2ms、665次超过4ms；仅Body1.466ms，5423次中161次超过2ms（2.97%）、无超过4ms，仅Body这一子项满足p95≤2ms／p99≤4ms。World安装3.214ms，4542次中4515次超过2ms、229次超过4ms；atomic_describe2.970ms、native_write2.033ms。Host读页查询814864、页作用域10848次、保留非空区域表继续独立读段16272次，页表回退0。编号高水位171、复用9786，三池有界、缺口505不变；ordinary和Client仍不达标，不外推整体验收通过。计时外Host采样1200次、最长暂停25.900ms，包含Prepare的157个样本中QueryNativeReadRegion25、Store8、Copy7、ScalarBatch7；不把采样暂停或不同自然效果数量解释成性能保证。装备变化后joint-after-install命中1次，30.836秒恢复World24.549／Body25.652Hz；普通角色与卡牌、四身体重建、zero-empty与最终1金币已在客机确认。正常stop后retired=1、native_written=0，两端正常退出。
- 包105／run112（Host31764／Client23384）四身体180.867秒：Host59.928Hz、ordinary29.967Hz、Client World19.904／Body20.087Hz、131555B/s。ordinary outer4.702ms，5421次全部超过2ms、5387次超过4ms；仅Body1.461ms，5421次中190次超过2ms、无超过4ms，仅Body子项满足分位要求。World安装4.481ms，3600次全部超过2ms、2883次超过4ms；describe3.791ms、write2.807ms。计时结束时编号高水位208／复用29165、三池有界、缺口504不变；现场自然效果73个，不能用跨轮均值判定优化因果。static-overlap及late-conflict各命中1次并恢复。**随后诊断阶段失败**：Client报client_member_index_still_borrowed，Host随后binary_prepare_no_progress；失败时编号高水位305／复用50294，远未达到16384预算，不属于编号耗尽。此轮不算稳定通过，未完成装备切换。同期Client采样2105次、最长暂停25.961ms，断线后空闲占大部，不能用作健康热路径证据；目前未证明采样导致故障。正常stop后Client retired=1／native_written=0，两端正常退出。继续定位实际目录借用和Host租约释放时机。
- 包106修复候选：收Members之前执行现有Collect，覆盖完成回执刚释放工作槽但轮尾尚未回收纯后台退休对象的时序；真实仍被主线程／Render持有的对象仍保留。借用冲突补充精确index、旧／新epoch、当前epoch、引用数及身份诊断，拒绝规则与编号预算不变。待同一第15步实机续测。
- 包106／run113（Host17564／Client28436）延长四身体压力480.642秒：Host60.003Hz、ordinary30.002Hz、Client World20.092／Body20.165Hz、137518B/s。ordinary outer5.091ms，14419次全部超过2ms、14108次超过4ms；仅Body1.368ms，14418次中115次超过2ms、无超过4ms。World安装4.181ms，9657次中9653次超过2ms、5994次超过4ms，性能仍未通过。计时结束编号高水位239／复用91400、三池有界、缺口505不变，无借用冲突或断线；静态重叠、晚绑定冲突均实际命中并恢复。计时后CPU事件采样受本机策略限制未启动（确认无残留会话）；改用短展开采样2378次，单次预算2ms但实际最长暂停15.156ms，全程保持Displaying，不能把该采样成本混入计时。栈样本显示SortReadback50次及其64位移位辅助20次，成员预检ReadMemory40／Query34，特殊分类Facts20。装备变化、普通角色／卡牌、四身体重建、zero-empty均已确认；联合安装后故障命中1次，随后30秒窗口World／Body均19.920Hz且无错误。清空后编号高水位298／复用142872，最终1金币已在客机确认；stop后retired=1、native_written=0。
- 包107：按run113采样优化Readback稳定基数排序，逐键字节读取替代x86的64位变长移位，保留原完整七字节键；成员预检仅在真正调用新玩家ToPlayer前后撤销页见证，新非玩家纯读分支保留本批页缓存；特殊字段整批借用不可变分类表，外部定义和分配失败仍走原分类。另修正包106新增失败诊断中use的64位格式，原拒绝行为不变。待实机续验，不据静态分析宣称性能通过。
- 包107／run114（Host37276／Client5204）四身体180.821秒：Host59.965Hz、ordinary29.980Hz、Client World26.258／Body27.746Hz、150105B/s。ordinary outer3.539ms，5422次全部超过2ms、357次超过4ms；仅Body1.365ms，5423次中50次超过2ms、无超过4ms。World安装2.907ms，4748次中4697次超过2ms、85次超过4ms，仍不达标。atomic_order0.567ms、seal_bindings0.446ms；按raw_operations归一的order成本由run113的25.025降至19.672微秒／千项（约21%），本轮自然效果仅约16个，不能把总耗时改善全归因于代码。编号高水位153／复用11979、三池有界、缺口504不变，静态冲突／晚绑定重叠命中并恢复，无目录借用错误。计时外Host采样3597次、最长暂停9.474ms；Prepare链中Query65、Walk26、Copy23、ScalarBatch20、Qualify13，作为后续优化依据，不混入性能计时。其后武器变化、普通角色与卡牌、四身体重建和zero-empty已在客机确认；binary-joint-after-install命中1次并恢复。清空后编号高水位287／复用68158，最终1金币已确认，stop后retired=1／native_written=0。
- 包108按上述采样把Host同资格纯标量分组扩展到多精确读段，保留所有字段内容、验证与窗口，每个原生空隙分段读取；预算不足沿原逐字段失败顺序。设计及失败方式见性能文档14.15，待本轮实机续测。
- 包108／run115（Host4968／Client4716）四身体180.843秒：Host59.975Hz、ordinary29.987Hz、Client World26.725／Body27.980Hz、145488B/s。ordinary outer3.543ms，5424次全部超过2ms、364次超过4ms；仅Body1.366ms，5424次中42次超过2ms、无超过4ms。World安装2.828ms，4833次中4832次超过2ms、92次超过4ms，仍不达标。编号高水位148／复用10887、三池有界、缺口506不变，无目录借用或其他终止错误。bound_writes约1.5968亿，接近run114的1.6015亿；字段执行总wall15.909秒对16.365秒，归一差异约2.5%，ordinary总耗时基本不变，不能据此宣称多读段有明确收益。武器变化及binary-world-after-install已命中恢复，其后普通角色／卡牌、四身体重建和zero-empty已在客机确认，清空后编号高水位222／复用29468，末尾1金币已确认，stop后retired=1／native_written=0。
- 包109撤回包108的多读段结构，恢复严格原生连续段；仅保留相同qualification_slot核定后的根稳定资格合并和预算充足时批计数，紧预算仍沿原顺序失败。客机readback在完整键可单调映射、桶数≤操作数两倍且已有整数工作区capacity足够时做一次稳定计数排序；不扩容工作区，范围不适用退回完整字节排序，原key和同key操作顺序保持。待实机续验。
- 包109／run116启动失败（Host6024／Client24032）：客机第一次cs status响应前发生原生访问异常，原生Manager::Update RVA 0x52E54B（读取this+0x21450所得索引乘0x4C，再访问this+0x20C78）。C/S尚未建立，未进入存档或压力计时，新增采集／排序路径尚未执行；Host随后由启动器正常关闭，不能误记为两端同因崩溃。转储保留在隔离客机profile的crash_dumps/isaacv1.9.7.12.J273-20261116-101655-24032-38368.dmp；目前原因未确定，不将其归因于本轮优化或宣称已修复。继续用同一包复现。
- run116后续定位：历史tx-04-21记录已出现相同RVA 0x52E54B，静态对应Cutscene队列索引；官方CutsceneSkip.cpp在SkipIntro=1时跳过原生Show，而原Show承担队列初始化。当前isaac-dual的prepare_profile仍强制SkipIntro=1，修为仅隔离副本SkipIntro=0，保留原生初始化并由正常播放／真实按键结束开场。未改原版或REPENTOGON二进制，不能补造run116实际写入历史。run117同包在修订后的启动器尚未构建前正常进入世界，说明原失败并非每次启动必现；新启动配置待后续实机验证。
- 包109／run117（Host12296／Client7960）四身体180.735秒：Host59.977Hz、ordinary29.989Hz、Client World20.455／Body20.616Hz、133602B/s。ordinary outer4.804ms，5421次全部超过2ms、5073次超过4ms；仅Body1.561ms，5421次中222次超过2ms、无超过4ms。World安装3.574ms，3697次全部超过2ms、434次超过4ms；atomic_order0.693ms，native_publish0.488ms，仍不达标。计时结束编号高水位223／复用28686、三池有界、缺口504不变，无终止错误。联合安装后故障命中1次并恢复，装备变化、普通角色／卡牌、四身体重建、zero-empty及最终1金币已在客机确认；清空后编号高水位298／复用60806。正常stop后retired=1、native_written=0。
- 包110将最终纯字节读回合并的借用限制与安装阶段统一为同record／根块，并保持实际来源／目标同时连续、128项作用域上限、逐字段失败定位；masked／引用／header仍单独检查，不跨影子分配或晚绑定路径。启动器SkipIntro=0已构建，待同轮实机续验。
- 包110／run118（Host48464／Client44324）使用SkipIntro=0，两端实际播放开场后按Enter正常进入会话与世界。四身体180.849秒：Host59.962Hz、ordinary29.981Hz、Client World25.430／Body27.006Hz、145354B/s。ordinary outer3.596ms，5423次全部超过2ms、524次超过4ms；仅Body1.417ms，5424次中113次超过2ms、无超过4ms。World安装3.250ms，4599次中4596次超过2ms、281次超过4ms，native_publish0.384ms，仍不达标。编号高水位153／复用12795、三池有界、缺口506不变。原始操作1.3038亿，World写页查询507581、读页查询325119，缓存表满均0。计时外两次短栈采样均2393次，最长暂停9.466／9.875ms，WritableRange主调用链来自InstallNow，实际PC未集中在其循环体，提示真实VirtualQuery成本，不能视作缺少缓存容量。静态写冲突、安装后world故障及晚绑定冲突各命中1次并恢复；装备、普通角色／卡牌、四身体重建、zero-empty及最终1金币已在客机确认；清空后编号高水位161／复用22935，stop后retired=1、native_written=0。
- 包111保留纯容器借用的页见证与分类表批次借用，边界及失败方式见性能文档14.16；构建成功，待实机续验。
- 包111／run119（Host40728／Client47396）再次正常播放原生开场并经Enter进入世界。四身体180.797秒：Host59.951Hz、ordinary29.978Hz、Client World25.692／Body27.882Hz、146086B/s。ordinary outer3.676ms，5420次全部超过2ms、630次超过4ms；仅Body1.434ms，5421次中113次超过2ms、无超过4ms。World安装2.857ms，4645次中4621次超过2ms、103次超过4ms；native_write1.767ms、native_publish0.368ms。原始操作1.3406亿，写页查询267304，约57.55次／安装，对run118约110.37次；read_page_queries281749，两类表满均0。编号高水位181／复用10930、三池有界、缺口505不变。联合安装后、装备变化后的world安装后及晚绑定重叠故障均实际命中并恢复；总体性能仍未达标。普通角色／卡牌、四身体重建、zero-empty和最终1金币均在客机确认；清空后编号高水位254／复用29090，stop后retired=1、native_written=0。
- 包112只为Covers未接管的通用字段增加ValueOrder入口，默认绑定器仍回退Order；NativeBindings保留原Room／Level前缀顺序，其余None／FXCount沿原4序，委托字段仍完整特殊分类。避免每个普通叶重复查特殊分类，不改变优先级／执行次序。已构建，待实机续验。
- 包112／run120（Host13108／Client40144）第三次正常经原生开场进入世界。计时外Host采样3197次、最长暂停9.524ms；Prepare链首项目帧Query61、Copy23、Walk21、Track16、Open14，整进程其它输入Hook中的查询不能全部计作Prepare开销。四身体180.745秒：Host59.963Hz、ordinary29.981Hz、Client World19.929／Body19.995Hz、139369B/s。ordinary outer4.988ms，5420次全部超过2ms、5408次超过4ms；仅Body1.428ms，5420次中128次超过2ms、无超过4ms。World安装3.856ms，3602次全部超过2ms、930次超过4ms，仍不达标。原始操作1.3038亿、每安装约36195项，较run119约28862项更多，不能直接按均值断言优化回退；atomic_order约18.662微秒／千原始项，run119约19.393。编号高水位221／复用40663，三池有界，缺口506不变。装备变化后联合安装后故障及静态写重叠各命中1次并恢复。普通角色／卡牌、四身体重建、zero-empty及最终1金币均在客机确认，清空后编号高水位428／复用100828，stop后retired=1、native_written=0。
- 包113将纯Bytes／Masked32的比较、取原值及写入放在同一个SEH叶中，形状只核一次；相等时保留原不查询可写页的行为，非相等仍先核完整可写范围，原值完整后才写。返回CapturedFailure时立即登记done及written再沿原顺序撤销；取原值失败不得登记写入。未跨回调或原作用域，也未省最终读回。已构建，待真实游戏续验。
- 包113／run121（Host42280／Client12704）四身体180.806秒：Host59.976Hz、ordinary29.988Hz、Client World26.858／Body28.445Hz、150642B/s。ordinary outer3.699ms，5421次全部超过2ms、628次超过4ms；仅Body1.483ms，5421次中140次超过2ms、无超过4ms。World安装2.726ms，4856次中4822次超过2ms、64次超过4ms，仍不达标；native_write1.678ms、native_publish0.362ms、atomic_order0.493ms。原始操作1.3964亿，写页查询264893、读页查询360241，表满均0。编号高水位168／复用10562、三池有界、缺口507不变。联合安装后及装备变化后的World安装后故障各命中1次并恢复；没有动态构造真实页异常，不据此声称CapturedFailure的SEH异常分支已实测。晚绑定冲突实际命中并恢复，普通角色／卡牌、四身体重建、zero-empty及最终1金币已在客机确认；清空后编号高水位194／复用17928，stop后retired=1、native_written=0。
- 包114保留所有自有附件成功安装前后的页见证，依据是已核成功路径只新增Peer注册表节点、保管新旧header及改写自有Deque代理；无原生回调、析构或旧承载释放。失败立即退出批次，EntityPtr与委托回调仍清空。实现边界与失败方式见性能文档14.16；实机结果如下。
- 包114／run122（Host26372／Client3672）四身体180.775秒：Host59.975Hz、ordinary29.988Hz、Client World26.729／Body29.180Hz、144985B/s。ordinary outer3.450ms，5420次全部超过2ms、464次超过4ms；仅Body1.309ms，80次超过2ms、无超过4ms。World安装2.500ms，4832次中4553次超过2ms、41次超过4ms：本窗口p99≤4ms成立，但p95≤2ms仍失败，不能判整体通过。native_write1.476ms、native_publish0.346ms、atomic_order0.477ms；写页查询146765（约30.37次／安装）、读页291538、表满均0。编号高水位188／复用9417，三池有界、缺口506不变。联合安装后、装备切换后World安装后及晚绑定冲突各命中1次并恢复；普通角色／卡牌、四身体重建、zero-empty、最终1金币均在客机确认。结束编号高水位299／复用46704，stop后retired=1、native_written=0，代理无丢包、队列清空。
- 包115根据run122计时外采样，改为在稠密键适用时直接计数、按原操作顺序构建回读索引；完整64位键、桶内稳定顺序、窗口撤销检查及扩展键回退保留。不改变原生写入／回读内容。已构建，待实机续验。
- 包115／run123（Host23788／Client47664）四身体180.892秒：Host59.947Hz、ordinary29.979Hz、Client World20.134／Body20.642Hz、134442B/s。ordinary outer4.585ms，5423次全部超过2ms、5357次超过4ms；仅Body1.480ms，205次超过2ms、无超过4ms。World安装3.286ms，3642次全部超过2ms、250次超过4ms；native_write1.986ms、native_publish0.451ms、atomic_order0.609ms、seal_bindings0.564ms。原始操作1.2275亿（约33705／安装，相比run122约28109更高），写页115712／读页259746、表满0，三池有界、缺口506稳定，编号高水位225／复用28520。每千原始操作的排序耗时约18.06us（run122约16.96us），本次不能证明包115提速，整体仍不通过。计时外Host40秒采样3195次、最大暂停8.44ms；客机30秒采样2389次、最大暂停10.11ms，DescribeFields重新成为显著热点。联合安装后、装备切换后World安装后和写入冲突故障各命中1次并恢复；普通角色／卡牌、四身体重建、zero-empty和最终1金币已在客机确认；最终编号高水位344／复用124285，stop后retired=1、native_written=0，代理无丢包、队列排空。
- 包105把叶转换及EntityReference ABI的具名handler分类移入不可变生成定义索引，按实际容量计入Values；数字值、掩码、可选状态和引用目标仍逐批校验，未知handler仍拒绝。分配失败退回完整分类，不缓存原生地址或窗口。已在run112执行实机续验；性能及后续生命周期失败如上记录。










## 改动范围

- E2E 工具：对照、探测、统计、故障注入。
- E2E 方案文档：在本目录下另起 `15-e2e-plan.md`，保存修订后的完整条目与指标。
- 进度与恢复：只在未来验收必要中断时保留续测所需记录，并说明位置／用途；正常连续完成不为本步骤强制另建进度artifact。本次不创建测试执行材料。

## 失败方式

- **对照工具接入同步路径**：Lua 对照被正式同步路径调用，或者影响主线程的计时结果。
- **探测成本未扣除**：未知区探测的诊断复制计入正式性能指标，使结果失真。探测和计时要分开运行，或者分别报告。
- **对照对错帧**：Host 采集和 Lua 对照不在同一窗口、同一次采集，误判不一致。
- **只看平均值**：只报告平均 FPS 或平均耗时，掩盖 p95、p99 或最大值超标。
- **混用旧材料**：tx-04-21 的暂停材料与新格式的结果混用。
- **锚点字段对照口径错误**：Client 侧对照在展示借用之外读取 FrameCount、`Room:GetFrameCount()` 或 Timeout，用的是 Client 自己的帧计数，误判不一致（第 13 步 13.6）。
- **空帧无法出现**：D0 几乎每帧变化，正常运行中很少出现空帧；不专门构造就宣称空帧路径已验证。

## 完成内容

本次仅完成文档整理与静态审阅；以下为本步骤后续实施／验收需要交付的内容，不表示已经实现、编译或通过测试。

- **未来验收完成条件**：当前原版范围的E01—E30、E32—E35须按两人全套和四人扩展完成，实际失败／首因／修复与必要续验如实说明；E31迁出不计PASS。原版必要项不得按Mod适配迁出，本次没有任何新增通过结论。
- **字节归属**：全部布局的UNKNOWN保留语义／证据与实际覆盖结论；未观察到变化不等于语义已知。类型字节归属完整与功能／场景验收分别说明，不以静止或一次成功推定全部原版行为完整。
- **回映射**：覆盖报告中，每个 `ReplicatedState` 成员都已比较，或已说明为什么不比较。
- **性能**：总计划第 1 节的性能指标全部达标。

## 未决问题

- E2E 对照与正式采集在同一窗口内的计时隔离方式。
- `out/tx-04-21-p2-20261106-01` 恢复材料的处理（第 0 步，用户决定）。

## 历史修订提示与原分域补充

以下保留原页首与原正文中的修订补充，按各自时点阅读；旧版本号、未生成／待注册和原位置措辞不覆盖上文现行清单。版本47的Player和版本57的Room／Level已经入源，完整消息／结果批次及运行owner仍按各项实际状态说明；历史失败、UNKNOWN及尚未观察的事实不改判。

**原页首提示**

**2026-11-10 第7步必要显示／资源待验收范围（仅本步执行）**：按[设计第8节](room-display-resource-design.md#8-后续实施验收与剩余未知)覆盖双配置同名／独立／原位变化、Home StageId35和Type16分别组合／Door Light、十二房形与特殊背景、冷连接错过纹理生产、文件路径先变而实际Image未变、程序图同名／pixel与UV／padding／RGBA8精度／资源失败与退休。覆盖最终water／mode／flow／方向、不同本地图形选项／视口、lighting425／460／NPC407／特殊−9／Home、两波纹半帧／重复显示；地图全部可显示事实而非Host窗口前缀、两边框／徽标、挑战32锚点／描述／安全169边界及本地独立查看。blob／recipe计入首次准备1MiB／10秒和三端持续7.5Mbps；不隐性改预算、降质、调用随机／生成／分数重算或写访客进度。本轮只有静态设计审阅，未执行测试。

Lost诅咒还须覆盖独立question_image的默认／空／实际内容变化与失败不ACK，不能仅比较两个图标Sprite或硬编码文件。Minimap+614..61B已核为ImageStrongPair，直属未知23→15B不等于其余UNKNOWN已解决。

**2026-11-10 RoomSave／Exploration验收范围补充（仅设计）**：[197行目录／同步决定](room-save-exploration-synchronization-review.md)要求覆盖地图可见而未访问／四种揭图与诅咒显示／各端独立展开；L2动态图标／楼梯／Portal及重复／顺序不改聚合；MapBadgeList的333覆盖三项、376追加、诅咒顺序与7项上限／空列／本地排版；当前保存体陈旧、回访／Myosotis／Moving Box／Flip／矿车、Maze交换／红房／维度／Ascent／Hourglass／合法Continue失效重建。超预算／源失效／资源缺失不得当空或ACK；不运行Client保存恢复／随机Refresh／原版Online输入或写访客持久数据。API类型／Grid Clear／ESSM marker差异按实际语义审阅，不以旧签名相等代替结果。所有测试仅在本步骤统一执行，本轮无测试。

**2026-11-10 RoomDescriptor验收修订（仅本步执行）**：[专门核对](room-descriptor-synchronization-review.md)补普通TrapDoor Boss匹配／route、HasWater与waterAmount独立的Backdrop32/58/59坑Light／重绘、献祭／pitch-black最终lighting、Error→ModelingClay图像与颜色、D1仅1B bool且完整效果ID不发／数值Getter不伪造。商店两表及价格封顶只比较最终Pickup/Price/Sprite，Client不补货／改价／花费解锁。L2覆盖16种mask、9类动态帧／固定0不发送、心／硬币／钥匙／药丸／69箱子／Card/Rune／Slot default与14／Portal／楼梯、重复和顺序变化而聚合不变不重发；拒绝条件长度／帧界／错world、Gap伪装缺席、源扫描超预算或误写78h保存头。描述直属未知24→0不消除保存74/type5视图70/Grid3B及外部未知。本轮只登记第15步范围，未执行测试。

**2026-11-10 Level验收口径（仅本步执行）**：按[静态核对与失败方式](level-synchronization-review.md)覆盖三维／大房／L型缺口／入口象限、负ListIndex/Genesis/-100/-101与-20寿命、-19不读写、同count原位映射／图标变化、双配置／门Light、visited布尔及五位flags无关位不重发、Greed波次／图标、有效curse强制/阻止、challenge44最终背景／楼层名、challenge32已选地图位置与保存实体图标／Grid楼梯。Myosotis、portal／Death Certificate仅Host执行创建／返回，Client无生成／过房／存档副作用；根22B及保存等外部未知保留，描述原24B已核为Host商店表不下发。官方Greed Getter读18204错误映射项，真实计数18374，Lua对照不得要求两者相等或调用错误Setter造样本；不比较未发送的seed／全flags／保存历史。本轮未生成Level生产布局、未执行上述用例，不新增提前测试或验收编号。

**2026-11-10 Room待统一验收范围（续核修订2）**：按[Room职责](room-synchronization-review.md)及[剩余字节补证](room-residual-evidence.md)覆盖两张448项表边界／占格计时不当path、最高格／256以上轨道／八门别名与特殊目标、所有房形／特殊背景／冷纹理、水量／水色插值／颜色空间／流水／冷却熔岩，以及反向水面／不同FPS／重复显示。覆盖16环境voice的中途加入／停止／静音／跨房／pan，死亡名单标记与实际奖励、十组输出后态、空FX／空ray组／poop／收缩vein、强制焦点／各端视口、暂停／换房。确认不传或执行Host函数地址、格子延迟队列、死亡历史、EntityList碰撞／查询／原版Online环，无Client雨滴／补货／装饰／Minecart重复生成或Camera／地狱背景Update移动实体。水色不二次乘量，ray相位不累计，程序图像不按同名复用；UNKNOWN和缺来源不伪装空后ACK。仅本步统一执行，本轮未提前测试。

**2026-11-10 Random待统一验收范围（交接修订2，仅本步执行）**：按[15项交接](room-random-handoff.md)覆盖零额外随机／回调／Host Render、5B D0与逐用途alpha／资格、首次／暂停／重复显示／不同成员顺序／不同可见分支／多pass／递归、Familiar颜色栈平衡及Pickup临时Sprite／CPU消费后恢复。缺源、旧world／资源修订和额外Client可见调用不得填0或复用旧alpha；两seed／shift／guard与BD0F40不下行。覆盖挑战32锚点／对应描述／裁剪窗口／标记、169边界安全拒绝或具名支持、Error 1B谓词变化、药丸／Loot／池隔离、实际sample／多voice／静音／停止／跨房／持续baseline；不比较未发送流的原值。本轮没有执行上述测试。

**第6步影子描述代码（仅本步执行验证）**：按[已实现接口与失败方式](grid-description-implementation.md)覆盖17类／27种type的完整记录生成、错key/use/shape/layout、缺块／未定义块／预算、合法旧变化版本、Statue三空块缺席和224无Seed。检查描述／投影复制共享自有字节及租约、借用撤销后内容不变、不同角色真实nested_layout和ANM2条件投影仍受第12步递归校验；完整u32水面、Door负索引／两个布尔、Fire16B粒子与Rock有序重复偏移保持。旧JSON legacy描述不得隐式进入正式安装。第10／12／13步运行接线完成后再验证实际游戏行为及AppliedAck，本轮仅纯编译未执行测试。

**2026-11-10 Presentation待验收范围**：按[具体清单](presentation-synchronization-review.md#8-后续正式布局与唯一验收)覆盖四参与者/多身体/动态HUD、24心与特殊健康/HolyMantle/低血、StatHUD0—8行/两列、11消息槽/fortune/程序图、History顺序/负ID/shader、各端独立地图展开/视口/挑战32/黑框、屏幕/转场/死亡/暂停只读结果，以及crossfade/jingle/各端静音/loop中途加入与停止/one-shot去重。确认不运行HUD/Map/Overlay/概率/分数生产器，不触及Online或访客进度；未知来源/图像失败不伪装为空后ACK。本次未提前执行测试。

**版本55 Grid回映射消费口径（仅本步执行）**：按[正式119行](layouts/06-grid/grid-catalog-members.md)选择实际布局及处置；Lua对照以目录路径＋行号保留第2/8、61/63、105/110行的属性／方法区别，不按ID去重。field／derived只比较对应展示投影，不恢复74行Host原值相等；ReducedSpawnRate第92行不适用，不探未知字节冒充该属性。SpawnSeed的198／224、GetSprite的实际nested_layout、Rock条件主姿态／layer0及无符号GetBigRockFrame须分别覆盖；Render／RenderTop行不授权重放原生副作用。正式reader／writer、七族水面、资源／pass来源和借用恢复仍须实际接通并在本步验收，生成／纯编译不算测试通过。本轮未执行测试。

**版本54 GridEntityPlus（仅本步执行）**：按[来源及失败方式](grid-plus-water-source-evidence.md)覆盖七族、Null／覆盖0／高位完整u32、Reset恢复各自默认；特别覆盖Lock设置override无效、同index门／装饰／Lock的不同最终值，以及同index其它对象Factory／Init导致共享重置、Init内重新Set。Remove／换房不假设官方自动清，复用／停用／退休撤本地借用；缺模块／错ABI／越界／非0或1 engaged／读失败／错use不ACK。七个C/S原链各一次，Lock须有新增覆盖点，out+4保持零；验证仅最终值与画面，不比较optional／填充或制造其线上负例。旧JSON的<=255限制实际迁移前不得记高位通过。本轮没有执行测试。

**版本53 PressurePlate／Door（仅本步执行）**：按[失败方式](grid-pressure-door-evidence.md)覆盖普通／Greed板、真实TimerPlate有无资源与非variant2已加载、计时0／负值／分钟边界／Host帧闪烁与震屏；门alpha零／负／正及幅值不影响入口、墙色与extra独立颜色／ExtraVisible、target type27／15及负特殊索引／目标Flags40000、Light bit0x20／维度／layer3/5／独立glitch、方向最终双旋转、门别名／跨房／释放与资源等待不ACK。previous／动画名／其余flags／Seed变化而最终投影不变不应单独重发；不恢复被裁原值相等或为其制造线上负载。拒绝非法布尔／截断／非有限／错use与未知target依赖；根31／32B UNKNOWN包含Door27F，不能以阴性结果判填充。正式owner未接线不记通过，本轮没有执行这些测试。

**版本51 八类Grid覆盖（仅在本步执行）**：按[完整失败方式及静态边界](grid-eight-subclasses-evidence.md)覆盖八类真实RTTI／Factory／重建、Decoration隐藏variant1000、五类地板色变化与draw恢复、Wall在16／非16房间、TrapDoor计数不发但最终开闭同步；Fire双附属Sprite、主模板不同残留帧／独立overlay／is_playing插值、state>=4空投影及重新可见、有序0／5粒子与删除、24B原生／16B线元素、跨房／退休／Free所有权及三Sprite各自Dataminer选择。拒绝count>5／容量或长度不符、非有限位置、maxLife=0或>29、age<0或>=maxLife、错版本／use／资源；状态<4的负数仍按原生有符号判断处理。只比较保留的实际展示状态，不恢复计时／样式／速度／必覆写颜色／旧模板帧相等要求，不为已删除字段制造线上负例。E27保留七类根17B、Fire45B UNKNOWN；draw／资源／容器owner尚未接线不能记通过。本轮没有运行这些测试。

**版本50 Grid验收修订（仅在本步执行）**：按[复核](grid-common-synchronization-review.md)比较最终水面值与实际画面，不比较Host CollisionClass、默认动画名、图层待加载路径或旧来源元数据。原Grid／E09／E25／E28／E29覆盖七原生水面族、nullable未设／有值0／高位u32与同index门/装饰，Client+3C本地中性初始化、wrapper原链各一次、无碰撞玩法重演；覆盖路径先改但仍画旧Image、显式动画／selector与资源失败不ACK；覆盖Dataminer两端不同ANM2地址、0—3四种最终纹理选择、pass开关／暂停／use换代／角色切换及draw精确恢复，不能按Client地址生成后记通过。E13／E17拒绝glitch=4/255、旧版本／截断／坏资源／index／角色绑定，合法u32水面值不沿用旧<=255限制。E27根17B与各外部图层4B未知保持。两种合成字段实际来源／writer／绘制adapter缺席时保持未通过，不恢复已删除负载；本轮未运行任何测试。 冠军着色验证前四实际GPU参数，拒绝16B截断／非有限值；后28B不上线不比较，普通颜色全部11值继续覆盖。

**版本48 Grid公共验收口径（仅在本步执行）**：按[公共归属与未知区](grid-common-evidence.md)，原Grid场景及E09／E25只比较保留状态与最终画面，不再要求Host SpawnCount／VarData／Initialized／VariableSeed／SpawnFrame或对象_rng在Client原值相等。E27保留对象+1D—1F与ANM2递归14B未知，共17B；不得将未观察变化说成填充。E13／E17在真实子类布局接入后覆盖index447合法／448及65535非法、身份／门别名冲突、来源失效与实际资源／长度错误；不能用抽象公共前缀199代测派生类。E28／E29核Client无Init／PostInit／Update／奖励／玩法RNG重演，本地owner安全退休及临时颜色／Image／visible精确恢复；Light验证SpawnSeed＋同world Host帧，覆盖Pause／帧跳变，不能以本地时钟代替。Plus水面覆盖及Room颜色／压力板文字等实际依赖须闭合才可记相应用例通过。本轮未执行测试，不新增验收编号。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步P6—P8设计覆盖（2026-11-09，仅本步执行）**：[完整失败方式](5-player-p6-p8.md)要求覆盖飞行／overlay／Room Light配置身份、15 map及negative map的body标记、明确无ANM与资源等待、原位重排／跨backup／held use变化和统一拓扑body拒绝；四参与者／64身体组与角色主备／sub／Flip／703变换后实际成员，独立valid不由room／Exists推导，不把Host sub／replaced／四槽嫁接Client。效果须覆盖重复first-match／disabled／704角色环与非27主动HUD冷却，其他cooldown变化不重发；HistoryHUD负ID／633／88000与真实库存图像列表、原始History变化而最终缓存不变、Render内分数重算隔离及最终分数来源时点。残影按全部元素／顺序／duration／位置比较；可选序列只装当前节点与实际ANM，D=0／second=-1空负载、未选节点不重发、0／1本地list及CPU借用／异常退休均覆盖。P3—P5旧段的“703备份依赖”以本次裁剪取代：仍核VarData，明确不要求四槽下行。非法截断／条件长度／引用／bool／预算／早ACK及原三端性能保持；本轮没有构建或执行测试，版本仍46，第15步是唯一统一验收。

**第5步P3—P5设计覆盖（2026-11-09，仅本步执行）**：[字段与失败方式](5-player-p3-p5.md)要求覆盖完整dense库存／熔炼黄金位／wisp／附加计数、blocked∩innate、legacy／多group相同有效计数、timer推进而投影不变及到期有效计数变化；检查本地投影容器合法且无计时／回调。四Pocket含后三槽、空type与ActiveSlot+1、queued饰品golden、crafting八位置／poop六位置与实际HUD最终结果必须覆盖，不能用counts代替显示列表。P4覆盖四槽、occupied三段、空槽旧负载不参与、partial真实浮点／VarData／703备份依赖。P5覆盖1—15真实factory大小、10／11同类不同type、五恒零类不读／不比较充能、十类mask位影响／无关位不重发、first-positive与first-nonempty顺序、Whip实际引用及独立退休；不由槽指针相同就制造双Free或用ClearReferences停音／删实体。非法截断／count／ID／mask／错view／非有限／资源未就绪、四参与者多身体／恢复与三端性能按现有整体验收覆盖。P3跨域计算查询最终显示来源的未决须实际闭合后才记该项覆盖，三组有界阴性输入不新增原值比较，不能通过省略画面宣称通过。本轮无Player新实现，未执行这些测试，版本仍46。

**第5步P0—P2设计覆盖（2026-11-09，仅本步执行）**：按[定稿与失败方式](5-player-p0-p2.md)覆盖原生13B8闪隐与BlinkLock区分、五种Render充能／真实Weapon与704效果依赖、29／38六比例／headAnim、主Sprite E6同body、costume同count原位重排／拓扑换代及资源未就绪。健康覆盖834930普通缓存及83B160特殊分支、永恒心overlay／骨心排列、BlackHearts掩码、负额外生命修正、Jar／Urn／魂血／poop资源及真实多身体backing；不比较不上线的EternalHearts／BoneHeartsMask原值或Player属性生产历史，仍比较最终属性栏五值缓存／心缓存。拒绝P1／P2混入body、P0截断／超长／非有限值、错误P6版本和不一致投影count；在四人／多身体／三端扇出下验收变化块大小、安装与总带宽，不能用全量下界声称性能通过。当前没有Player新布局或reader／writer，本轮未执行这些测试，布局版本仍46。

**版本46 Delirium覆盖（仅本步执行）**：按[原未决处理](delirium-residual-evidence.md)，E09／E25覆盖经Init及不经Init的变身、资源／几何变化、410 Child和其他实际companion独立成员，稳定公共shape下不得额外重建根NPC；E28／E29检查BE8没有Client写入／玩法消费／重复创建／级联Remove，正常引用撤边和独立成员退休继续覆盖。两个Host打包值及官方u32／u8查询不作跨端原值／漂移比较，也不为它们新增线上非法负载；17／18仍为空预留。E27的Delirium专有范围由11B收窄到BD4—BD7的4B，根590B，其他既有未知保持；BDD／BE4已经归入打包存储，不重复当未知。池残留只记录自然同槽前后不同use的实际原值；静态可达不算已观察，不通过强写或重复Init制造样本。无样本如实记录，阴性结果不证明padding。第4步本轮没有执行这些测试。

**版本45 Projectile／Effect残余覆盖（仅本步执行）**：按[逐项证据](projectile-effect-residual-evidence.md)，E09／E25／E28比较Projectile Height＋440零／非零开关＋独立冻结锚点，覆盖锚点不等于当前Position、默认锚点、冻结结束／Remove、非零PositionOffset、Shadow／水面及draw精确恢复；不为测试触发Client冻结随机或原生Update／Interpolate。E13／E17拒绝PR0／PR2截断及非法浮点，u8=2等非零原值应合法，勿误当bool。Effect覆盖148的167、149的19／71／194、原102的54／126及119已知／其他callback，不开放不适用公开API、不比较不上线的伤害元数据、内部flags或callback地址。E27观察Projectile本类434四字节／441三字节／44C四字节／474四字节及公共335B；Effect按新残余表保留167的434及438后8B、119按callback复用区、其他未核variant、三段41C／44C／4BC和其他空白，不用阴性结果伪造填充。此前五项既有覆盖保持，不新增独立验收编号；本轮未执行测试。

**版本44 Text／Dummy覆盖口径（仅在本步执行）**：按[字段决定](text-dummy-evidence.md)，E09／E25覆盖145基础臂、146的variant2、147真实Dummy RTTI，Text最终RGBA／字节与variant2缩放、Dummy正常可见标签与公共Sprite；不比较Text duration／颜色空间标签，也不为基础臂发counter。E13／E17拒绝count256／65535、长度不符、内嵌NUL／未终止、失效窗口、来源变化、旧版本／错臂及RGBA非有限值；非UTF8、空串和255B有效前缀合法。E28／E29覆盖writer缺席、slot复用／Init／退休撤销、长→短→空串、同world Manager／pause切换、字体未加载、无Update／Interpolate／到期Remove及draw恢复；跨视口与Dummy visible切换／Baseline进入不可见实例的历史屏幕cache依赖必须实际闭合，缺来源如实记Gap，不能按当前位置重画后记通过。E27包括Text52C—52F和两类公共335B未知。Dummy自然来源未核事实保持，不用Spawn964生成的NPC代测或宣称自然可达；合法测试池桥仍仅限E2E构建。第4步本轮未执行上述测试。

**原正文分域补充**

**版本36 Projectile口径**：按[Projectile运动与展示参数复核](projectile-synchronization-review.md)，八项Host运动／规则输入不上线、不做原值／漂移比较。E09／E25覆盖追踪、左右曲线、加速／减速、下落速度／加速度、摆动相位、Scale选图及ProjectileFlags组合产生的最终轨迹／大小／Sprite／独立效果；Height覆盖+440两分支与公共偏移非零、Shadow／水面／排序及特殊绘制路径。E28／E29确认没有本地Update／Interpolate／碰撞／+470回调，+350及Sprite借用精确恢复；Game帧展示借用仍供应原Render局部随机相位。E13／E17继续检查Height有限值及真实资源／引用，不再构造八项被撤销字段的非法负载；PR1预留不作为构造必收。E34不新增摆动／其他运动锚点；E27保留全部375B未知，私有+440及+460／+464来源仍未闭合，不能跳过场景后称画面完整。只安排第15步，未提前执行。

**版本35 Pickup／Slot口径**：按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)，20个原生范围留Host、19项原线上字段／容器撤销，不比较其原值或未来列表。E09／E25覆盖选择组拾取后的成员退出、触碰／充能、自动价格／补货重抽、盲目图层、循环切换后的最终成员与Sprite；保留Price／价格图形和水面反射、DropDelay正值隐藏／归零显示。Slot覆盖1—18变体的支付／捐款／触发／开奖后最终Sprite及6／15奖品Sprite的动画与播放条件。E28／E29确认无本地定价／循环Morph／支付／开奖／资源生产setter／持久副作用，检查循环本地安全空初始化、资源借用和正常退休。E34撤销Pickup Timeout／Wait，仅保留DropDelay与其他真正线上计时；Slot八个标量也不建立锚点。E13／E17不构造已撤销字段／循环数组的非法负载，继续检查真实资源／选择器／引用与DropDelay编码；预留块不作为构造必收负载，E27未知不变。只安排第15步，未提前执行。

**版本34 Knife／Bomb／Tear口径**：按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)，42个Host参数不做原值相等／漂移比较或线上字段拒绝用例。E09／E25覆盖Knife投掷／挥砍／旋转／路径／分裂及拖影，Bomb下落／火箭／爆炸／不同范围伤害Flags和Scale对应的最终效果，Tear等待／粘附到期、下落／追踪／父跟随及不同高度缩放旋转的最终位置／Sprite；保留Tear flags／DeadEye的Room Light、排序／水面与所有辅助Sprite。E28／E29确认未放行Update／Interpolate、爆炸／击退／LoadCostumes／ResetSpriteScale／高度重算，Bomb pending清除及owner退休保持。E34不再比较Bomb ExplosionCountdown、Tear WaitFrames／StickTimer锚点；其余真正线上计时保持原合同。E13／E17继续校验真实线上浮点／引用／资源／容器，E27未知范围不变。只安排第15步，不提前运行。

**版本33 Familiar口径**：按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)，11个Host参数不做跨端原值比较或线上拒绝负载。E09／E25检查轨道最终运动、Hearts／Keys／清房奖励后的最终画面和方向／射击动画；80覆盖原版Weapon充能族、modifier、0／1／中间／100、空Weapon、出生6帧／隐藏限制和四动画阶段；240覆盖payload空↔非空、DAC空↔非空、逻辑环顺序、全段／后半段、节点Z／反射及Sprite资源变化。E13／E17拒绝amount越界、非有限XYZ、非空payload节点0／超限、非法map／错类型与投影count不一致。E28／E29覆盖DB8借用恢复、候选失败／释放、真实引用撤边及无玩法重演；E27增加payload尾44B、嵌套ANM未知与每节点108B未知观察。布局与来源已补入，运行时handler和owner仍未接线，不能标通过。

**版本32动画／资源／颜色口径**：按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)，不比较Host事件位、两个loaded原始字节或ColorParams队列／锚点，也不为这些已排除字段构造线上拒绝用例。E09／E25覆盖主／叠加／逐层／空层独立姿态、播放／停止与不同速度的官方绘制插值，以及颜色覆盖优先级、淡出、正／负Lifespan与shared传播后的最终Entity／Sprite／图层／costume颜色；完整ColorMod的11分量继续对照。E28确认事件本地清理、无事件重放／本地颜色合成及官方插值临时改写恢复。E30覆盖Null↔非Null文件资源、Client等待／加载失败不得ACK、路径已变而仍画旧Image、随后实际Image替换及Null Image；本地准备结果不由Host布尔覆盖。E34不再覆盖ColorParams时钟，其他展示计时保留。新资源handler与未核Player／Grid／HUD owner未接通不能标通过。

**版本31生命周期口径**：按[再复核](entity-lifecycle-synchronization-review.md)，公共valid不做Host↔Client逐字节相等对照；E09／E25／E28／E29覆盖本地发布／池资格、角色29／38互惠backup复合绘制与递归终止、Player结构激活资格、非房间但exists=true的备用存储以及尚未退休的死亡展示。exists／dead继续按回映射比较；不能通过Kill／Remove或同步Host池字节取得期望值，也不能默删旧Player结构资格源。P7编码与writer未接通不能标通过。

**版本30公共字段口径（valid部分由版本31修订）**：按[Entity公共复核](entity-common-synchronization-review.md)，33个已排除范围不比较Host↔Client原值、不为不上线字段构造解码拒绝用例。E09／E25检查毒／火／冻结等最终flags、颜色／图标、HP／死亡、暂停及最终位置；覆盖valid参与的角色29／38替身复合绘制、exists／dead差异、碰撞调试几何和Effect172旋转／光照。E28检查本地工厂／池生命周期及完整绘制借用恢复，不能通过同步Host工厂字节修正本地owner。E34不再比较公共20个计时器和毒／火相位，版本32又移出ColorParams，继续覆盖子类及Player实际保留的展示计时。插值如实施只使用本地已应用快照历史，不放行原生Interpolate。未知字节与既有失败记录不变。

**版本29内部关系口径**：[24项逐槽复核](internal-entity-relations-review.md)中23条Host边不再比较原值，也不构造其未知引用负载作拒绝测试；E09／E25继续比较分裂／终端等独立成员最终画面。Familiar +DAC只在240布局130检查，payload补齐后覆盖空↔非空造成的绳索分段；当前不能标通过。E29检查真正保留引用和被排除槽的本地空状态、旧绑定正常撤边以及不得级联误删。Light必须覆盖Laser未优化采样点。下文“全部独立引用”均指当前实际线上引用，不能恢复旧23条玩法边。

**版本28回映射口径**：按[字段职责复核](synchronization-field-review.md)，17个已改权威范围不再要求Host↔Client原值相等；四类命中历史连元素也不比较。E09／E25仍覆盖分裂、掉落／伤害、弹幕变更后的最终成员与画面，并确认Client未重演玩法。Familiar的FireCooldown／State／Coins等实际展示依赖继续覆盖，variant80最终充能表现尚待接线，不能记为已通过。未闭合候选仍按现有线上归属比较，不提前裁剪。

**版本37 Laser口径**：按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)，E09／E25覆盖追踪、曲线、环形、旋转、收缩、父体跟随和接触派生造成的最终Angle／Length／点列／Sprite／独立成员；兼顾sample_mode_raw=-1／0／1、完整效果位、公开端点、MaxDistance／GridHit及+4F0展示分支，Room Light实际使用unoptimized。E13／E17拒绝真实上线点数越界、非有限几何／标量、来源变化、未知引用、分配失败及预算溢出；不构造撤销输入的非法负载。E29覆盖两组不同点数、空↔非空／增长／缩短、同目标版本完整安装、失败不部分替换、正常退休及本地indices／Bezier空owner；E28确认Client不做采样／Update／Interpolate／接触模拟，draw借用精确恢复。E34撤销两个Laser锚点／原值比较，其他实际展示时钟保持；E27保留421B未知。预留L2／L4／L7／L8不作为完整构造必收负载。本轮只修订验收安排，未执行。

**版本38 Effect验收口径**：按[Effect各臂与payload复核](effect-synchronization-review.md)E09／E25／E32比较最终画面、Scale在variant121的Light、172旋转、160连接几何、196跟随显示、136持物及落地后的真实成员、三种payload全部实际状态；不比较六项Host参数、TearFlags／136描述、MrMe四项Host私有状态、Ghost派生Offset／Scale或独立i32 count原值。Ghost覆盖0—4前缀、三投影count一致、同资源不同实例、缩短／归零／再增长与present切换；E13／E17拒绝源i32负数／>4收窄、线上计数不一致、错RTTI／present／资源／非有限值及预算溢出，不构造已撤销字段负载。E29核正常清理四槽ANM2、独立Trail节点及共享owner和失败不部分替换；E28核draw临时派生精确恢复，Client无Trail追加／衰减、loot选择、传播／Spawn、举起／投掷／落地或Update／Interpolate。E34仍比较真实展示Timeout的负值、停走、跳变与锚点重算；E27保留根对象和各ANM2的UNKNOWN，不把Ghost未发送槽中的未知消除。MrMe16B与Trail0x40已有静态正面证据，原未决事实见历史报告，不再把旧producer缺口当现行同步Gap。这里只安排第15步，未提前执行。

**版本39装备／Plus验收口径**：按[完整复核](player-equipment-plus-synchronization-review.md)E09／E25覆盖四主动槽／空槽的Charge／Battery／Sub／Partial／VarData及特殊HUD，15种Weapon及10／11同vtable、两个真实进度条、Whip主实体＋Familiar D74／D76展示切换、Berserk704环、Null109／112／trinket69效果及disabled且列表非空的查询差异。E29覆盖五槽类型替换／清空、Whip主引用正常撤边、效果列表清空／重建及窄Free，不级联删除独立成员；E13／E17拒绝真正在网的坏资源／引用／count／非有限值／nullable与预算溢出。E28检查无Fire／Update／HUD Update、效果到期／增减、重算／回调玩法补值、重抽／重采样／初始化音效，以及进度条／HUD临时姿态精确恢复。Plus覆盖未设→false／有效零→true／非零→再未设，Scene实际水面／反射须消费；其他已排除项不恢复。E34核最终效果cooldown与世界展示帧，不为已撤销冷却新增锚点；E27保留Weapon20B、TemporaryEffect4B、外层2B及各Plus未知。只比较保留字段与最终画面，不比较八个撤销范围；这些测试仅在第15步执行。
**版本40 Effect后续五项验收口径**：见[证据与落地](effect-next-five-evidence.md)。E09／E25／E32覆盖11布局、113正确落141、六个传播variant与Timer／生成／计数的最终展示，客户端不读取或重演Host请求／函数回调；原有三payload与未核variant的UNKNOWN保持。E32覆盖Trail present缺失、存在且0／1／126／127节点、增长越过原生128裁剪点后仅发布127，以及缩短／再创建；不为测试触及生产数据而新增持久副作用。E13／E17拒绝线上count=128／65535、长度不符、旧版本／错臂、非法浮点／资源／实际预算；原生损坏／中途采集的128或更大数量应报Gap，不能截断通过。E34分别覆盖负值、零、线性递减、101局部＋公共双减、141刷新、暂停及跳帧／回绕，比较同已应用world帧精确还原；仅线性区间断言锚点不重发，不把静止位置当计时条件。E28／E29核未知或错variant非空holder拒绝、前后变化报缺口、正常owner清理、无Spawn／传播／回调／Trail维护，失败不部分替换；E27继续保留167后16B及各臂剩余未知。本批没有运行上述测试，第15步仍为唯一统一整体验收。

**版本43 NPC现行合同**：受控UI覆盖F1C=-1→0及其他合法来源值→2、C/S本机/远端参与者相同展示语义、来源变化/窗口失效/读取失败报Gap、线上1/其他非法值拒绝、Client设备变化不改变已应用表现。用同一次整体验收确认reader不读Manager/NetplayManager、不调用IsIdxLocalPlayer或slot5；Client两个受控UI整段和Spawner远端淡出整段均不借原版Online筛选。保留403独立HP路径及所有原有可见/资源条件；Delirium资源变化不强制换代、OverlayEffect Scale恢复、五标量/完整点列/引用/UNKNOWN范围保持。未提前执行。见[来源闭合](npc-control-ui-evidence.md)。

**原正文其余修订提示**

其中Type964是NPC Render的静态数值分支；实机场景必须先有真实NPC RTTI及合法producer，不能把同数值的NativeDummy强套NPC布局。未取得来源时如实记未覆盖，不造V1或假对象满足用例。

**原15.4分域补充（旧时点说明）**

第4步原未决前四项的补充沿用既有条目：版本28起Knife K4六个f32只归Host权威，E09／E25比较分裂后的最终成员、位置／姿态，不再比较K4原值；`+0x518`是碰撞方向角，Client不得为验证其效果执行原生碰撞。E13／E17只对实际线上字段拒绝非法浮点，不再要求拒绝不上线的K4负载。E27保留Knife末四字节、Slot末四字节、Projectile末四字节和Effect`+0x41C/+0x44C/+0x4BC`的未知区观察，并复核六段空白的填充／保留字节假设；Effect传播／生成用途已静态核定，验收确认Client未重演这些Host玩法。Slot进度指针、Projectile回调指针不得按状态写入或执行；Effect不得在Client重演Spawn／传播。这里只安排第15步覆盖，未提前运行。

第 4 步 Tear／Pickup／Laser 的覆盖并入上述既有条目，不另开验收编号：E09／E25 比较三份 Tear 辅助 Sprite、两份 Pickup Sprite、全部当前线上独立引用、循环切换后的最终形态／Sprite、Laser原始有符号采样模式、真实展示标量和optimized／unoptimized两组最终点列；版本35起不比较Host未来循环列表；E13／E17 覆盖 count 越界、非有限几何、未知引用和分配失败；E29 覆盖关系替换、撤边及级联 Remove 不得误删独立存活对象；E34仅覆盖此组三类中仍上线的Pickup DropDelay条件计时，Pickup Timeout／Wait自版本35、Laser Timeout／RotationDelay自版本37起不比较；其他真实线上未知时钟仍须报Gap；版本34撤销Tear两个计时的锚点比较；E27 包括这三类的全部未知区。容器内容必须完整提交，不能只比较 count 或跳过空集合清除。

Slot／Projectile／Effect 同样归入既有条目：E09／E25覆盖Slot 1—18变体、奖励Sprite最终外观／动画／图层及真实播放条件，以及Projectile的Height／公共DepthOffset单份映射与最终轨迹／Sprite；版本36起不比较ProjectileFlags等八项Host输入；Slot八项Host参数自版本35起不比较，其原生16位等宽度事实仍保留；E32 覆盖 Effect 六臂、三种具体 owner 的存在→缺失→再创建和同目标版本跨块变化；E13／E17 拒绝非法 present、缺失值非零、尾投影缺少存在性、未知动态类型、非有限轨迹与超预算。E29 覆盖旧 owner 正常退休、失败候选不得部分替换；E34 比较 Effect Timeout 的负值、停走、跳变和到期帧重算，E27 包括三个根对象和外部 payload 的全部未知区。基础臂未核 VarData 不得按零或不透明内存伪装成完整状态。

包116稀疏数值叶索引已构建，原主线程消费谓词在后台Node遍历中原样登记；scratch扩容重试同步回卷。只减少无关标量扫描与重复Bindings二分查找，原生校验和回读不变，待第15步续验。

包116／run124（Host5616／Client23252）四身体180.689秒：Host60.015Hz、ordinary30.007Hz、Client World20.942／Body21.230Hz、140868B/s。ordinary outer4.937ms，5421次全部超过2ms、4763次超过4ms；仅Body1.476ms，152次超过2ms、无超过4ms。World安装3.346ms，3784次中3772次超过2ms、417次超过4ms，仍不达标；native_write2.108ms、native_publish0.467ms、atomic_order0.632ms、seal_bindings0.479ms。原始操作1.3328亿、约35222／安装，比run123约33705更高；seal_bindings均值仍从0.564ms降至0.479ms（按操作量归一约下降18.7%），说明过滤扫描成本减少，不能据此宣称整体性能通过。写页122451、读页259482、表满0；三池有界、缺口506稳定、编号高水位231／复用31443。联合安装后与装备变化后的World安装后故障均命中1次并恢复；普通角色／卡牌、四身体重建、zero-empty及最后1金币已在客机确认；晚绑定重叠故障命中1次并恢复。最终编号高水位291／复用83619，stop后retired=1、native_written=0，代理无丢包、队列清空。

包117已构建，Host首次未命中证言复用插入位置、引用句柄以显式小端8B拷贝编码；现开始双普通角色标准负载续验。

包117／run125（Host31580／Client9308）双普通角色180.778秒：Host59.980Hz、ordinary29.993Hz、Client World29.301／Body30.391Hz、163089B/s。ordinary outer3.429ms，5423次全部超过2ms、347次超过4ms；仅Body0.769ms，5423次无超过2ms或4ms。World安装2.169ms，5297次中4418次超过2ms、6次超过4ms：p99≤4ms成立、p95≤2ms仍失败。native_write1.339ms、native_publish0.303ms、atomic_order0.431ms、seal_bindings0.318ms；plan_slice24742片、均值1.672ms。原始操作1.2436亿，写页130878／读页320670、表满0。三池有界、缺口506稳定、编号高水位197／复用13833；World安装后故障在计时前命中1次并恢复。不能将普通角色结果当成四身体通过，亦未证明Body目标频率满足要求。晚绑定冲突命中1次并恢复；stop后retired=1、native_written=0，代理无丢包、队列清空。

包118后台三张路径／字段索引改为完整键散列表，已构建；其余有序小表不变，待四身体实机续验。

包118／run126（Host35608／Client42040）四身体180.902秒：Host59.955Hz、ordinary29.977Hz、Client World29.497／Body30.364Hz、159844B/s。ordinary outer均值3.536ms，5424次全部超过2ms、326次超过4ms；仅Body1.354ms，5425次中29次超过2ms、无超过4ms。World安装2.540ms，5336次全部超过2ms、16次超过4ms，p99≤4ms成立但p95≤2ms失败。native_write1.578ms、native_publish0.369ms、atomic_order0.488ms、seal_bindings0.360ms；plan_slice18481片、均值1.710ms。原始操作1.5618亿、约29268／安装；写页161682、读页369059、表满0。编号高水位140／复用11943，缺口506稳定，三池有界。自然FX数量与run124不同，不能将全部频率改善归因于包118。计时前联合安装后故障命中1次并恢复。

run126另用新增有界输入工具isaac-input，对隔离Client1连续70秒真实键盘移动／向上射击；窗口中间采样60.730秒，画面确认移动和射击。Host59.855Hz、ordinary29.936Hz、每秒身体记录239.486（四身体合计，不是应用Hz）；Client World15.281／Body16.269Hz。ordinary outer3.777ms，1820次全部超过2ms、464次超过4ms；World安装2.696ms，928次中898次超过2ms、13次超过4ms。编号高水位211、三池有界、无新增缺口／最终错误。由此不能把低Body应用频率解释为角色静止；这一动态输入诊断不替代180秒正式窗口，也不标通过。

持续输入工具沿用既有HeldKeys和正常C/S输入，不写Player位置／速度；启动前核隔离exe、实际PID映像、E2E令牌及显示状态，时长上限240秒。错误路径包括错误PID／映像／令牌、非显示状态、窗口不唯一及运行中失焦；均拒绝或退出，HeldKeys释放已按键，不自动夺回焦点。只用于本次整体验收，不进入模组DLL。

包119按对象分段建立CanonicalField小表：先在解码回卷结束后分片计数、再分配至少半空的小段，段内仍比较完整路径并保持第一字段优先；对象身份由独占段确定。需要续验空对象、World／Body合并、装备／成员变动、回卷与预算，以及安装故障恢复；不缓存原生地址、不放宽预算或原生检查。

run126计时后World安装后及晚绑定冲突均命中1次并恢复，装备／普通角色与卡牌／四身体重建已在客机确认。stop-supply之后等待构建时Host死亡；虽F8步骤6／7仍记录action，但此时不再是正常活跃世界，zero-empty／最后1金币不能记通过，下轮连续重测。stop后客机retired=1、native_written=0；代理无丢包，队列清空。此失败不修改前面的有效计时样本，也不以死亡后的冻结画面作为性能结果。

包119／run127（Host36280／Client42848）四身体180.812秒：Host59.952Hz、ordinary29.981Hz、Client World22.294／Body23.101Hz、146894B/s。ordinary outer5.060ms，5420次全部超过2ms、4579次超过4ms；仅Body1.656ms，5421次中477次超过2ms、无超过4ms。World安装3.295ms，4031次中4005次超过2ms、439次超过4ms；native_write2.045ms、native_publish0.443ms、atomic_order0.656ms、seal_bindings0.512ms。原始操作1.4406亿，约35738／安装；读页320937、写页132953、表满0。编号高水位247／复用31668，缺口444稳定、三池有界。操作量和自然负载较run126更高，不能证明分段索引提高了整体性能。计时外30秒客机采样2389点、暂停最大11.98ms（包含一次装备切换）：DescribeFields70、BuildReadback34、CanonicalField13，后者PC仍集中在槽数据读取，查找热点未消失。

run127联合安装后、装备World安装后、晚绑定重叠故障各命中1次并恢复；装备／普通角色与卡牌／四身体重建均在客机确认。随后连续stop-supply→zero-empty，在Host仍存活时确认4个Player、NPC／攻击／拾取／FX全0，客机资源归零、最后1金币已显示，补齐run126未完成收尾。最终编号高水位287／复用62333，正常stop进入退休；包120固定宽度复制已构建，待续验，整体性能仍失败。

### 2026-11-16 布局 82 应用频率验收

- 同机隔离 P2 使用固定种子 `S227 7MNF`。Host 夹具确认两组、共四身体，初始生成 8 NPC／16 攻击实体。首轮直连中 Host 的完成 tick 仍在推进，普通来源约 30.13Hz。20.678 秒故障窗口内 Client 仅完成 11 次 World、447 次 Body 安装，随后 Host 以 `binary_prepare_install_deadline` 断开。成员目录背压增加 242 次。止于断线的窗口只记录失败。
- 回查目录所有权发现，普通 World 发布后仍持有准备阶段借来的旧 Body，可能使 `member_floor` 卡在三份在途成员目录的最旧一代。普通 World 的计划现只弱引用对侧姿态，接收器在真实原生完成和事件回执后释放准备阶段的强引用。联合提交仍保留两流，等待或执行原生安装时仍持有所需目录。
- 直连 B 复测 180.733 秒未断线。Host 59.973 tick/s、29.984 ordinary/s。Client 实际 World 18.580Hz／Body 53.350Hz，平均应用间隔 53.82ms／18.74ms。Host 发 5358 个 World、9931 个 Body、0 个联合，Client 收到 5358 个 World、9927 个 Body。Client World 拒绝旧基线 1990 次。Host 成员目录背压 1593 次，目录回报 3344 次，未匹配 ACK 为 0。World 安装均值 2.457ms，3358 次中 2716 次超过 2ms、38 次超过 4ms，p95 与 p99 均未达标。Body 安装均值 0.721ms，9642 次中一次超过 2ms、无超过 4ms，两项达标。
- 直连 B 高负载窗口中，World 的 B5 复用 257670 条计划记录，B3 略过字节和 B4 相对叶模板命中均为 0，仍有 20 次执行顺序复用。Body 相对叶模板命中 16435653 次、顺序复用 9642 次。该四身体样本仍反复执行成员登记，不能把 Body 的模板收益用作 World 证据。
- 同轮装备阶段 `binary-joint-after-install`、`binary-world-before-install`、`binary-world-after-install`、静态写重叠和晚绑定冲突命中各一次，随后 World 确认均继续前进。联合写后故障之后又完整发送一次联合。切到普通角色后仍显示，切回四身体时 Host 记录 `binary_prepare_no_progress`，末次目录就绪为 7051/38、当时来源为 7111/42。其后的空场景和最后一次变化没有有效客机，停机后两端 Idle，客机排空为 1、原生写保留为 0。
- 续修让成员目录背压只延后新表交接，已交接目录的合法 World 仍可发送。Body 的纯值解码前代在闭包精确兼容时借当前已安装成员目录，旧原生承载的租约仍等实际退休。原生提交前封存本批晚绑定和附件的数字清单，减少写段中全量叶扫描。Host 无进度时记录下限、租约池和两流的确认代次。
- 直连 C 复测 180.746 秒未断线，夹具 205 个采样均为四身体和 16 个攻击实体，含 8–9 NPC、8–104 FX。Host 59.979 tick/s、29.992 ordinary/s。Client World 18.396Hz／Body 52.040Hz。World 安装均值 2.718ms，3325 次中 3065 次超过 2ms、93 次超过 4ms。Body 安装均值 0.730ms，9406 次中一次超过 2ms、无超过 4ms。Host 成员目录背压 1026 次，Client World 旧基线拒绝 2046 次。自然 FX 负载随轮次变化，B／C 计时差异无法独立归因于安装器改动。
- 直连 C 的普通角色阶段正常确认，切回四身体仍复现无进度。记录表明三个租约都占用，下限 6143，当前发出的目录为 6145。下一轮同时释放单流 Body 完成后只用于配对的旧 World 准备租约。若新拓扑堵在三份成员租约外，Host 可将最近已发且闭包仍兼容的 Body 来源用已确认的 World 目录再次发送，使客机通过真实安装归还旧成员号。仍要求实测场景切换和丢包。性能续修将相同旧来源的重发延后一周期，让更新来源或真实 ACK 先行。已知将执行实体或格子 Init 的 World 批次省去随后必定撤销的叶模板构建。
- 启动复测 D／E 的 Host 控制口已响应，60 秒内一直报菜单 -1，实际日志停在开场。D 的原代理端口同时被占用，E 换独立端口后仍在开场超时。按用户当轮要求，隔离 P2 启动器恢复 `SkipIntro=1`，两份测试 profile 由启动器分别设置。旧 `SkipIntro=0` 与历史 Cutscene 故障记录保留。
- 代理 F 使用 `SkipIntro=1`，两端均未调用开场按键，直接进入菜单并完成首个世界安装。180.792 秒稳态前后未注入故障，无代理丢包。Host 59.964 tick/s、29.979 ordinary/s。Client World 29.266Hz／Body 52.718Hz。Host 发 5300 个 World、9531 个 Body、0 个联合。World／Body 拒绝旧基线为 13／2。World 安装均值 1.644ms，5291 次中 356 次超过 2ms、4 次超过 4ms，p95 未达标，p99 达标。Body 安装均值 0.709ms，9531 次全部 ≤2ms。Body 的 9531 帧发送全部落地，缺的频率在 Host 发送之前。同轮空场景的 3 秒诊断中，Host 181 个 tick 发出 180 个 Body、90 个 World。两轮自然 FX 规模不同，频率与耗时变化须结合负载读数。
- F 计时后，将一条 ContextReady 延后 240ms、丢一条可靠接收 ACK、在 230ms 内丢两个成员表分片。最后一项触发一次 World 缺表到期、两个下行分片重传。三个场景都继续应用 World／Body，无联合重建、无未匹配 ACK。装备、普通角色、切回四身体、停止补充、zero-empty 和最后静止时两流均继续提交。Host 的人数采样确认身体 4→2→4，最终无 NPC／攻击／拾取／FX。停止后两端 Idle，客机排空为 1、原生写保留为 0。
- G 试验在普通来源封存后提前归还原始采集槽，让同依赖的 Body 在等待事件接管时推进。180.679 秒窗口内采集槽替换由 F 的 25317 次降至 21679 次，客机应用却只有 World 26.185Hz／Body 32.704Hz，Body 发送 5914 帧、应用 5909 帧。自然 FX 较多，仍不能据替换次数推断发送缺口已改善。撤回 Host 采集槽试验，保留客机 Seal 复用紧凑附件清单。G 的 World 安装 4731 次，3998 次超过 2ms、49 次超过 4ms。Body 安装 5909 次，4 次超过 2ms、1 次超过 4ms。稳态结束后客机排空为 1、原生写保留为 0。

### 100 泪弹与 100 敌怪追加场景

按用户要求，在标准四身体混合压力之后分别从固定种子的干净房间启动 `ISAAC_CS_P2_LOAD=tears-100` 和 `ISAAC_CS_P2_LOAD=enemies-100`，每轮仍为两参与者、四身体。泪弹场景参考本地 isaac-lan 的 `state_shared_view_encoding.lua`，使用 100 个真实蓝泪弹，关闭碰撞与下落以保持稳定数量。敌怪场景单独补足 100 个活跃的 Gaper，保留原版 AI 与碰撞，夹具不主动发射攻击。记录实际实体数、世界帧、两流应用次数、安装耗时阈值，以及提前断开的首因。热身、实体清空和最后变更另计。

- H 直连，100 泪弹 180.869 秒，175/175 个采样达到 100 泪弹、四身体、0 敌怪与 0 敌弹。另外有 100–200 个原生 FX、204–304 个总实体。Host 57.567 tick/s、28.789 ordinary/s。Client World 14.242Hz／Body 26.721Hz，未断线。World 安装均值 5.652ms，2576 次全部超过 4ms。Body 均值 1.050ms，4833 次中 201 次超过 4ms，p95≤2ms 成立、p99≤4ms 失败。World 旧基线拒绝 2101 次，Host 发送 200 个联合，且记录 803 次未匹配回执。客机排空完成，窗口关闭后仍有已排空客机进程存活，复核隔离 PID 和映像路径后结束进程才继续敌怪场景。
- I 直连，100 敌怪 180.806 秒，182/182 个采样达到 100 个活动且移动的敌怪和四身体。没有泪弹、敌弹、拾取或 FX。Host 59.937 tick/s、29.966 ordinary/s。Client World 18.489Hz／Body 28.992Hz，未断线。World 安装均值 3.244ms，3343 次全部超过 2ms，其中 205 次超过 4ms。Body 均值 0.740ms，5242 次中仅一次超过 2ms、无超过 4ms。Host 发 5238 个 World、5237 个 Body、0 个联合，无未匹配回执。World 旧基线拒绝 1900 次。完成清空和最后一次变化后客机确认号继续前进。两端退出，客机排空为 1、原生写保留为 0。
- 这两个追加场景完成了规定实体规模和持续时窗，均未达到 World 30Hz、Body 60Hz 与 World 安装分位数目标。第 15 步仍有频率、安装耗时、完整 P2 矩阵和 P4 扩展待验。

### 高负载发送与 isaac-lan 对照

- 本地 isaac-lan 的 `docs/synchronization.md` 与 `src/bridge/sync/replica.lua` 描述实时完整视图：prefix 引用已确认的不可变字节，还原完整目标后应用。主线程里相同的待处理视图可复用解码树，可靠低频依赖按修订匹配。常规 Host 完整更新是 30Hz，60Hz 设置用于输入，半帧应用和本地表现也不等同于本工程独立的 60Hz Body 提交。
- 本工程 World 增量要接在当前已安装影子上。100 敌怪轮 Host 发出 5238 个 World，客机完成 3343 次安装，并拒绝 1900 个旧基线候选。World 安装 p95 与 p99 也超标，100 泪弹轮主线程描述约 7.35ms、写段约 3.29ms。相比之下 isaac-lan 用具名字段的 Lua／原生批量应用。其同房每接收者视图 1.326ms 是含编码的平均采集耗时，不能换算为本工程一次事务的安装 p95。
- 100 敌怪轮 Body 原生安装均值 0.74ms，Host 发送 5237 帧，客机发布 5242 帧，差值包含采样边界在途帧。Body 的主要缺口在发送前，不能用四个身体的采集记录数替代独立帧数。采集后台封存的来源以前经过下一次主线程 `PublishPeer` 才被网络看见，慢 ordinary 和其后的 PlayerPost 易在同一个交接点合并。
- 续修保留严格 World 应用基线。等待真实 ACK 时仅合并到最新来源，既有两周期重试间隔也适用于同表的新来源。可靠目录和 Body 仍可推进。主线程授权来源键及撤销水位，网络每轮直接读取采集后台的纯值邮箱并核对来源，同一批供全部 Peer。ordinary 原始采集槽仍等现有的事件接管，避免重做已撤回的 G 试验。
- 100 敌怪、四身体直连复测 180.633 秒，Host 59.972 tick/s、29.989 ordinary/s。World 发送 3720、安装 3717，实际 20.578Hz，旧基线拒绝由 1900 降到 2。Body 发送 10670（59.070Hz）、安装 10283（56.927Hz），安装均值 0.801ms，3 次超过 2ms、无超过 4ms。World 端到端均值 42.809ms，主线程接票等待 11.408ms、准备后等待提交 6.430ms。安装均值 3.950ms，3717 次全部超过 2ms、1349 次超过 4ms。计时外 World 写后故障命中，随后完成一次联合恢复并继续显示。正常停机，客机已排空且无原生写保留。
- 试验用有界地址表一次扫描七份原生名单。相同 100 敌怪场景 180.749 秒，World 20.470Hz／Body 56.808Hz，World 安装均值 3.993ms，发布子阶段 0.469ms，上一轮为 0.454ms。该试验未观察到收益，撤回。仍需处理耗时更高的字段指令生成、原生写段及等待主线程窗口。

### 2026-11-16 布局 83 World 应用节奏续验

- Release／Win32 E2E 构建及打包成功，生成表为布局 83、`world_send_window=2`。首轮 MSBuild 在文件扫描后无进展，结束本轮构建进程后在实际 Windows 会话重建成功。以下均为隔离 P2 本机直连，固定种子 `S227 7MNF`，热身和收尾另计。
- 100 敌怪采样 180.708 秒，181／181 个样本均有 100 个活跃且移动的敌怪及四身体（两参与者），泪弹、敌弹、拾取及 FX 均为 0。Host 59.948 tick/s、29.971 ordinary/s，平均上行 109482 B/s。Host 与 Client 控制页按顺序采集，发送与安装差分的边界略有偏移。

| 流 | Host 发送 | Client 安装 | 平均安装间隔 | 收包至发布均值 | 原生安装均值 | 安装超过 2ms／4ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| World | 4695 / 25.981Hz | 4700 / 26.009Hz | 38.45ms | 52.330ms | 3.583ms | 4700／672 |
| Body | 10579 / 58.542Hz | 10331 / 57.170Hz | 17.49ms | 17.021ms | 0.790ms | 3／0 |

- World 窗口满跳过 539 拍，在途峰值 2，完整重试 74 次。Client 经前代完整解码 4344 次，精确前代缺失、旧基线拒绝、原始报文位替换和过期各 0 次。本窗口联合发送、未匹配 ACK 和成员目录背压均为 0，Host 采集缺口计数保持 503，Client 持续显示，两端无错误。事件确认水位从 82 进至 578。
- World 接票等待 5.606ms，准备到提交等待 6.976ms。将阶段累计耗时按 4700 次安装归一，Decode 约 2.593ms、Inspect 0.148ms、Plan 7.694ms、Describe 0.094ms。World 安装较布局 82 同负载 20.578Hz 增加 5.431Hz，收包至发布均值从 42.809ms 增至 52.330ms。两流仍低于 30／60Hz 目标，World 安装 p95／p99 未达标，Body 安装达标。
- 计时外，`binary-world-after-install` 命中一次，Host 联合发送由 4 至 5，两流继续确认，Client 恢复显示。停止补充、清场及最后一次金币变化后，空场景 30.586 秒，31 个样本均为四身体、0 敌怪／攻击／拾取／FX。World／Body 安装分别为 29.981／59.994Hz，收包至发布均值 16.751／15.612ms，917／1835 次安装均 ≤2ms。窗口跳过、重试、旧基线拒绝、原始报文过期及联合恢复在空场景均为 0。两端正常停止，最后观察到 Client 排空为 1、`native_written=0`。
- 100 泪弹直连 180.660 秒，173／173 个样本达到 100 泪弹及四身体，有 100–200 个原生 FX，敌怪、敌弹和拾取均为 0。Host 57.373 tick/s、28.689 ordinary/s。普通 World／Body 分别发送 3099／7993 帧（17.154／44.243Hz），含联合的 Client 安装数为 3178／7501（17.591／41.520Hz）。收包至发布均值 82.180／20.209ms，安装均值 5.784／0.999ms。World 3178 次均超过 4ms，Body 207 次超过 2ms、201 次超过 4ms。Host 窗口跳过 1431 拍、在途峰值 3、完整重试 35、目录就绪 598、成员背压 0、联合发送 200、未匹配 ACK 423。Client 前代解码 2897，旧基线拒绝及原始报文替换均为 0，原始报文过期 1 次。采样期间两端保持运行，Client 持续显示且无错误。
- 泪弹窗口的 World 原始操作约 61,234 条／成功安装，敌怪窗口约 38,958 条。后台切片累计按成功安装数折算，Plan 13.77ms、Decode 5.70ms，包含撤回候选的工作。主线程安装指令生成均值 8.04ms，其中叶操作生成 5.17ms；原生安装 5.78ms，其中 `native_write` 3.36ms。接票等待为 8.02ms，准备至提交等待 3.95ms。Body 的主要频率缺口已出现在 Host 发送前，Host 普通采集均值由敌怪窗口 5.51ms 增至 9.15ms，且本轮有 200 个联合发送。`binary_body_dependency_blocks` 为 0，现有计数无法将漏发逐拍归因于采集合并、联合排期或带宽门控。Client Body 接票等待 10.16ms、原生安装 1.00ms，记录等待位过期 371 次、替换 184 次及 Body 旧基线拒绝 94 次。
- 本机代理在 100 敌怪、四身体场景丢弃两个完整 World 目标（6 个分片）。Host 在途峰值由 2 增至 3，完整重试增加 1 次，World 确认由 336 进至 412。随后在 250ms 内丢弃 48 条 AppliedAck，Client 记录两次精确前代缺失及旧基线拒绝，Host 在途峰值增至 4、确认进至 478。两段均保持显示，未新增联合恢复。再对普通 World 下行注入 60ms 延迟，12.734 秒内 World／Body 安装为 15.314／55.521Hz，窗口满跳过 167 拍，World 收包至发布均值 37.895ms（不包含代理延迟）。撤掉延迟后 World 确认进至 753，Client 保持显示。该短窗观察到限频方向，未逐帧测量发送至 ACK 的 L。
- 83 Host 与 82 Client 实机混连，两端均报告 `unsupported_capability`，Host 的 World 确认和 Client 的安装标志均为 0。双端关闭后隔离 Client1 恢复 83 DLL。
- 本轮仍待覆盖可控的事件预算截断、相邻目标回退到解码基线值、成员号复用与租约耗尽，以及双在途下切房和关闭撤回的精确时序。直连、丢包和混连样本保留在验收记录中，主线程性能目标仍未通过。

### 2026-11-16 布局 83 悬空泪弹夹具复测

沿用布局 83 的 Release／Win32 E2E 二进制与 `world_send_window=2`，重新打包修正后的 `tears-100` 夹具。夹具在 Host 每次原生更新前维护现存泪弹的高度和速度。隔离 P2 本机直连，固定种子 `S227 7MNF`，两参与者、四身体。热身及收尾另计。修正前 180.660 秒，修正后 180.696 秒。修正后 180／180 个样本均为 100 个泪弹、0 FX、0 敌怪／敌弹／拾取，主客机无错误，World 和 Body 频率统计客机实际发布且包含联合发布。

| 指标 | 修正前 | 修正后 | 变化 |
| --- | ---: | ---: | ---: |
| Host 原生 FX 数量 | 100–200 | 0 | 180 次采样均为 0 |
| Host 普通采集均值 | 9.15 ms | 5.295 ms | −3.855 ms（约 −42%） |
| Client World 发布频率 | 17.591 Hz | 29.918 Hz | +12.327 Hz |
| Client World 收包至发布均值 | 82.180 ms | 45.133 ms | −37.047 ms |
| Client Body 发布频率 | 41.520 Hz | 59.608 Hz | +18.088 Hz |
| Client Body 收包至发布均值 | 20.209 ms | 15.349 ms | −4.860 ms |

- Host 59.990 tick/s、29.990 ordinary/s。普通 World／Body 分别发送 5406／10806 帧（29.918／59.802 Hz），Client 实际发布 5406／10771 次。World 窗口满跳过由 1431 降至 14，完整重试由 35 降至 0，联合发送由 200 降至 0。该对照反映夹具修正后的负载变化，协议与采集器沿用同一构建。
- Host 普通采集 5419 次均超过 4 ms，其中实体值采集均值 3.336 ms。World 原生安装均值 3.344 ms，5406 次均超过 2 ms，352 次超过 4 ms。Body 原生安装均值 0.702 ms，2 次超过 2 ms、1 次超过 4 ms。Host 普通采集和 World 安装仍未达到 p95≤2 ms／p99≤4 ms。
- 计时后停止补充、清场及最后金币变更，双端 `cs stop` 均成功，Client 排空为 1、`native_written=0`，两端窗口关闭。

### 2026-11-16 布局 83 任务二 M0 续验

- 沙箱内首轮 MSBuild 停在文件扫描。切到 Windows 会话重建后发现状态库缺少 E2E 编译定义，导致 M0 诊断符号链接失败。为状态库及调用方统一定义后，Release／Win32 E2E 构建和打包成功。
- 隔离 P2 本机直连，种子 `S227 7MNF`，两参与者、四身体。100 移动敌怪和 100 悬空泪弹分别采样 180.118／180.206 秒，179／179 和 180／180 个样本符合各自负载，均为 0 敌弹、拾取及 FX。计时内双端无错误，客机保持显示。两轮均通过 `cs stop` 排空，观察到 `client.retire_binary_drained=1`、`client.native_written=0`，游戏进程均正常退出。
- M0 读数见 [World 应用节奏 4.1.1 节](13-client-world-cadence.md#411-m0-实测)。两轮分别成功发布 4,217／5,370 次 World，World 频率 23.412／29.799 Hz。两轮的 D1 都为 0。World 安装超过 2 ms 分别为 4,217／5,370 次，超过 4 ms 分别为 1,465／1,494 次，性能目标仍待后续优化。

### 2026-11-16 布局 83 任务二第 2 步续验

- Release／Win32 E2E 构建和打包通过。沿用隔离 P2 直连及种子 `S227 7MNF`。100 移动敌怪轮 180.085 秒，179／179 个样本合格。100 悬空泪弹轮 180.076 秒，180／180 个样本合格。两轮均为两参与者、四身体，0 敌弹、拾取及 FX，计时内客机保持显示。
- 敌怪轮 World 27.715 Hz、Plan 6.265 ms、指令生成 4.230 ms，窗口满跳过 240 拍。泪弹轮 World 29.848 Hz、Plan 6.284 ms、指令生成 3.994 ms，窗口满跳过 29 拍。两轮旧基线拒绝、联合发送及未匹配 ACK 均为 0。对照与复用命中见 [World 应用节奏 4.1.2 节](13-client-world-cadence.md#412-第-2-步实测)。
- 两轮收尾时，F8 夹具因 Host 失焦拒绝发键。恢复隔离 Host 焦点后在原进程续做，稳定窗口读数保留。清场和最后金币变化后 World 持续发布，四身体保留。双端 `cs stop`，确认客机排空为 1、`native_written=0`，两轮隔离游戏进程均已退出。

### 2026-11-16 布局 83 任务二第 3 步续验

- Release／Win32 E2E 构建和打包通过。M1b 两轮各连续 180 秒，100 移动敌怪、100 悬空泪弹均为 180／180 个样本合格。实际写入约占提交操作的 3.0%／0.06%，据此加入 M4。两轮压力段受原生成员次序和已知回调资格约束，片段省略计数均为 0。
- 受控段用隔离 Host 的临时原生深度输入让真实成员次序通过 B3。初次漂移注入命中时未观察到修回，随后把写脏点移至滚动检查即将访问的省略区间。最终版在 12 个普通 World 批次累计省去 19,893 条操作，命中一次 Seal 回退、一次漂移修复和一次写后联合恢复。漂移从写脏到检查修回为 11 µs，两条失败路径均观察到普通发布恢复。原生深度输入在压力采样开始前复原。
- 最终版敌怪轮两次预热分别残留 2 个和 1 个特效，均未进入计时。两次均在清场后确认 `client.retire_binary_drained=1`、`client.native_written=0`，双端退出。第三次在预热中尝试清理，仍观测到 2 个特效。用户确认按该负载继续，保留 100 移动敌怪加 2 个特效的最终版读数。
- 最终版敌怪轮 60.116 秒、泪弹轮 60.104 秒，各 60／60 个样本合格。World 分别发布 1,722／1,797 次，均未达到安装分位目标。两轮计时内客机持续显示，清场后 `cs stop`，均观测到 `client.retire_binary_drained=1`、`client.native_written=0`，隔离进程均退出。[操作数、安装分位数和资格首因](13-client-world-cadence.md#413-第-3-步实测)

### 2026-11-17 布局 83 任务二第 4 步续验

- 先按 M8 复核 J273 原生登记、显示排序和 SFX 回放。只调整普通 World 的 render 列表当前性检查，证据与具体保守分支见 [M8](13-client-world-cadence.md#m8-回调写集补全需要逆向)。首轮构建暴露 native 翻译单元的 C++17 边界，改用同文件既有的指针和数量接口后，Release／Win32 E2E 构建和打包通过。
- 首版敌怪轮 90.097 秒，90／90 个样本合格，100 移动敌怪加 1 个 FX。2,563 批 World 中 1,834 批省略片段，累计省略 23,368,382 条操作。按用户要求，后续单轮采样 30 秒。首版悬空泪弹轮 30.108 秒、30／30 个样本合格，903 批全部省略片段，累计省略 19,789,245 条。两轮原生成员当前性拒绝均为 0，清场后客机完成排空，双端退出。
- 最终版增加同 Host 排序键及非正常显示分支的原顺序检查。100 移动敌怪加 2 个 FX、100 悬空泪弹两轮分别采样 30.103／30.117 秒，各 30／30 个样本合格。分别有 676／867 和 903／903 批省略片段，原生成员当前性拒绝均为 0，安装均值为 3.520／2.357 ms。泪弹轮没有安装超过 4 ms。两轮均未达 p95≤2 ms。清场后 `cs stop` 观察到 `client.retire_binary_drained=1`、`client.native_written=0`，隔离双端均退出。[第 4 步完整读数](13-client-world-cadence.md#414-第-4-步实测)

### 2026-11-17 布局 83 M5 敌怪续验

- 首轮 30.103 秒、30／30 个样本合格，100 移动敌怪加 2 个 FX。预留排序使准备与安装 709／734 同窗，World 降至 24.383 Hz，端到端均值增到 59.177 ms，窗口满跳过 149 拍。实际吞吐回退。
- 第二轮把排序预留限于准备的首窗，并让跨窗票据优先续做。30.136 秒、30／30 个样本合格，100 移动敌怪加 1 个 FX。同窗 762／811，World 26.911 Hz，端到端均值 50.626 ms，窗口满跳过 63 拍。两轮均在清场后成功排空并退出。随后移除主动延期排序的启发式，以真实工作截止续做，再按估算选择写入时机。
- 第三轮 30.153 秒、30／30 个样本合格，100 移动敌怪、0 FX。同窗 393／655，World 21.723 Hz，端到端均值 67.013 ms，窗口满跳过 206 拍。224 批已经完成准备，在安装估算下跨窗。清场后排空并退出。最终版将分片和安装估算限定在最近实测准备加安装能够放进 8 ms 的负载，避免重载被持续分片或延后。
- 最终版 30.132 秒、30／30 个样本合格，100 移动敌怪加 2 个 FX。同窗 106／871，World 28.906 Hz，Body 55.621 Hz。计时内原有调度处理全部 World 批次，窗口满跳过 21 拍。计时前 M5 处理了 15 批，清场后累计 91 批。[对照和阶段耗时](13-client-world-cadence.md#415-m5-敌怪复测)
- 最终轮 Host 49484 正常停止并退出。Client 49920 原生排空、菜单和场景就绪均为 1，`client.native_written=0`，但停在 `Retiring`、`client.retire_worker_finished=0`，后台诊断仍有 1 个在途、1 份退休值。正常关闭后进程仍驻留，经复核只终止该隔离进程。性能读数取停机前稳定窗口。没有定位到 Worker 保留的具体票据，正常退出未通过。

### 2026-11-17 恢复直接调度续验

- 按用户指示移除 M5 耗时估算及描述中途截止，恢复第 4 步的准备顺序和写段尝试时机。Release／Win32 E2E 构建和打包通过，仍为布局 83。
- 隔离 P2 本机直连，两参与者、四身体、100 移动敌怪加 1 个特效，30.023 秒内 30／30 个样本合格。World 发布 861 次（28.678 Hz），Body 55.257 Hz。World 同窗 141／861（16.38%），安装均值 3.558 ms，收包至发布均值 41.687 ms。Manager 超过 8 ms 共 1,013／1,802 个窗口，Host 窗口满跳过 22 拍。
- 清场后双端 `cs stop` 成功，Client 确认为 `Off`、`client.retire_worker_finished=1`、`client.retire_binary_drained=1`、`client.native_written=0`，隔离双端正常退出。

### 2026-11-17 300 静止泪弹对照

- 同机直连，两参与者各选择雅各与以扫，同房共四个身体。C/S 使用 J273 + REPENTOGON、布局 83、恢复直接调度的构建，种子为 `S227 7MNF`。isaac-lan 使用 v0.7.2 + J460，种子为 `YV03 9KQF`，两端关闭游戏 Mod 开关，只加载原生 isaac-lan。
- C/S 在房间第 22 帧一次刷出 300 颗静止泪弹。Host 30.057 秒推进 1,778 个权威 tick（59.15 Hz），全程保持 300 颗、0 个特效。房间第 330 帧仍有两参与者，第 360 帧只剩 Host。客机显示 `AwaitingWorld` 后返回标题，控制通道返回 `server_disconnected`。本轮未取得稳定的 World／Body 频率或断开的首因。随后客机完成排空，双端正常退出。
- isaac-lan 首次启动遇到 Workshop 自动更新，关闭其他 Mod 后重试又遇到原启动器的固定目录检查。修正隔离副本的测试路径后，双端联机，但最初的夹具未持续维持泪弹悬空，这一轮的性能读数无效。把悬空维护放到 isaac-lan 现有测试使用的 Host 更新回调后，双端各跑满 30.000 秒。每个采样点均有 300 颗静止泪弹、四个身体，另外各有 1 至 13 个房间实体。双端退出码均为 0。
- isaac-lan Host 采集 900 次，客机成功应用 World 900 次（30.00 Hz）。Host 渲染 59.97 FPS，客机渲染 59.93 FPS。内部完整采样桶覆盖约 28.2 秒，Host 采集均值 4.67 ms，客机应用均值 6.90 ms，Host World 线上流量约 31.3 KiB/s。World 流量与 C/S 掉线窗的全量网络字节口径不同，两套实现的游戏版本和房间种子也不同。

### 2026-11-17 300 静止泪弹恢复修复续验

- 原构建复现时，客机已安装的 World 停在版本 66。第 167 张原生票据在写前报 `binary_stage_entity_pool_capacity`。J273 原生 Tear 池共有 512 个槽，旧场景仍持有泪弹时无法同时冷建另一套 300 颗。Host 未收到恢复基线的安装确认，到 `binary_prepare_install_deadline` 后断开。
- 客机收到同一路由的完整双流基线时保留原票据，先撤显，沿原有生命周期归还借用并退休旧场景，再准备该票据。第一版释放旧槽后遇到 `binary_stage_existing_owner_missing`，因为计划仍借用同身份的旧成员。随后把完整双流基线的安装动作统一为冷建，成员历史仍用于状态校验。[场景退休与准入](../../../mod/src/game/client/scene/client_binary_scene.cpp) · [原生安装计划](../../../mod/src/state/client_plan.cpp)
- Release／Win32 E2E 直连，同种子、两参与者四身体、300 颗静止泪弹、0 个特效。30.078 秒内，Host 29／29 个负载样本合格，客机 33／33 个采样都处于显示状态，已发布 World 均含 300 颗泪弹且未变为陈旧。客机原生 World 发布 549 次（18.25 Hz），Body 发布 694 次（23.07 Hz），渲染调用 1,802 次（59.91 FPS）。
- 另在隔离本机线路丢弃 68 个 Body 数据包，其他流正常转发。Host 发送可靠联合恢复，客机重新显示 300 颗泪弹。恢复阶段有两次 `binary_preparation_budget` 写前拒绝，随后继续完成发布。链路恢复后的 30.077 秒内，客机 33／33 个显示样本合格，World 发布 532 次（17.69 Hz），Body 发布 688 次（22.87 Hz）。两轮均确认客机原生排空、Worker 结束，隔离双端正常退出。

### 2026-11-17 F3 F5 300 静止泪弹续验

- 同机直连 J273，布局 83，同种子 `S227 7MNF`。先稳定两个参与者的四个身体，在房间第 242 帧一次生成 300 颗蓝色泪弹，夹具按同一生成身份检查世界坐标、速度及悬空。
- 第一轮以更新后的高度精确等于 −20 判断静止，导致 Host 检查未通过。改用生成坐标、速度及负高度后检查通过。一次采样中，从已经退休的安装器借用错误字符串，造成控制口文本解码失败，编排器主动停机。通道现自行保管有界错误字符串，写前失败在安装器退休后仍有有效首因。
- 复现起量闪退等待页时，首次完整基线入场就提前撤掉旧画面。改为检查实际原生池空位后，第一张基线保留显示。已显示 300 颗后，第二张同路线完整基线仍可能触发退休。后续让完整基线通过已核的成员身份和原生实例借用健康的旧承载，仍执行全量写入，Init 只用于实际新构造的成员。只有新构造成员需要更多槽位、且退休能释放足够槽位时，才沿用冷基线退休路径。
- 同身份复用轮的两张基线进入时均保留画面，第二张在写前报 `binary_atomic_budget`，随后客机约两秒未更新 World 并发起本地恢复。客户端 Values 峰值接近每连接 128 MiB 上限，进程总池没有拒绝。完整基线改按可执行叶数预留指令，地址排序临时区计入 Workspace，发布后释放排序及纯值撤销区。保留漂移和退休所需的已发布数据。
- 最终 Release／Win32 E2E 构建在起量前后取得 179 个采样。两张完整基线均保留显示，客机从零颗过渡到 300 颗，未观察到撤显或陈旧，三类撤显计数均为零。
- 随后的 30.043 秒中，Host 30／30 个样本有 300 颗静止泪弹、四身体、零特效，Client 35／35 个样本均稳定显示。客机实际发布 World 680 次（22.63 Hz）、Body 683 次（22.73 Hz），渲染 1,793 次（59.68 FPS）。World 安装均值 6.994 ms、收包至发布 37.087 ms。同窗 586／680。F5 共享 233,920 条 World 记录，记录导入按每次 World 发布归一为 0.842 ms。F3 复用约 656 万条晚绑定配方及 441 万条等值委托，实际取得可比原值的晚绑定和委托两类 D1 均为 0。
- 采样后主动停机，客机 Worker 和原生场景均排空，隔离双端退出码均为零。清理临时部署包并恢复标准测试夹具。
