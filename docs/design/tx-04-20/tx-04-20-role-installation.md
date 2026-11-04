# tx-04-20 I21—I25：持物与原版身体族应用

日期：2026-11-04，Asia/Singapore。同一 tx-04-20 继续完成 I21—I25 的代码、正式来源与功能目录。未构建、运行测试或启动游戏；统一运行验收仍由整个目标完成后的 I39 执行。本文件记录实现合同，不把静态工作记为运行通过。

## 实现

| 项 | 本轮交付 |
| --- | --- |
| I21 | 同 T 的 held Key／shape、ItemState、Suplex 0—5、aim countdown／target／land 最终量与 minecart 原有最终输入；父代画允许各已支持动态 Entity 和真实 Player，不再将20限制为Effect136。隐藏 held 由原生父分支临时置可见并调用自身 Render；可见 held 保留来源值，不强制再画。Body null-frame、13份内嵌ANM、costume及持物辅助资源保持到外层CPU队列退尽。实际投掷后的 Tear／Effect／删除按完整 T 的成员计划处理，不调用 Hold／Throw／Suplex／PickupGridEntity。 |
| I22 | `client_player_roles.inc` 逐个注册原版0—40及C01—C17、具名任务责任。普通／复活／随机已选形态共用I11—I20最终心量、资源、库存、服装和武器安装；不重发初始装备、不随机Eden、不套普通Isaac心量约束。C02—C05／C11—C17额外私有状态继续明确归I27，完整绘制归I29。possessor -1／variant2、其他Baby／内部变体和未声明Mod角色明确分类拒绝，不回退Isaac。 |
| I23 | 整个身体集合检查19／20相反类型、双向twin、roster资格、同参与者／控制组／连接代次／绑定代次；现有四组与64身体关系预算不缩成四个Player。每个身体保留自己的最终库存／心量／资源；原生main由实际索引决定，不由Jacob名字或ordinal推断。 |
| I24 | 普通16／17使用roster主身体到相反类型、负PlayerIndex非roster补体的单向sub；不强造反向边或额外参与者。35／40使用双向twin和真实roster；held关系单独应用，不强造Parent=holder。Source的活动、可输入、存在与可见仍分别保留。形态变化沿既有计划撤旧实例／资源并从完整T重建；逻辑Key保留，不在Client调用SwapForgottenForm。 |
| I25 | 29／38的flipped form与backup存储分别保留并核对，配对互指和Birthright实际twin关系分别检查；每身体两套装备／History／资源不合并。IsSubPlayer由PlayerIndex负值导出，IsHologram由原生exists／type／twin／双方索引导出；不新增hologram开关，不把hologram等同不可移动或库存备份。Flip只安装结果，不重放道具使用。 |

纯值解码与全身体关系预检在 `client_player_roles.cpp`，SDK操作在 `client_player_roles_native.cpp`。`ClientPlayers::Prepare` 从同一个 PlayerTarget／EntityTarget组装具名值并检查全图；拒绝缺副体、跨组配对、非互反twin／Flip边、错形态持物、两个holder共用同一held及递归持物环。引用缺失与显式Null不混用；缺来源不补零。

`ClientNativePlayersFinish` 改为全体先绑定storage／Weapon边，再全体从真实native对象、容器和公开getter回读。之前逐个身体边写边检查会在对方尚未链接时过早检查；本轮修正该顺序，不让某组先取得Scene提交资格。正常退休仍先撤全体裸借用和EntityPtr，再由唯一身体／装备owner清理，不调用递归玩法Remove或Flip回滚。更完整的故障／退休编排继续由I33收束。

成员删除、角色类型／shape改变继续选择现有显式整场景重建；其它组在完整T中的逻辑成员／独立值仍须重建并共同提交，不能被退出组的递归清理从T中删除。本轮不声明这些变化会原位保留其它组的原生实例；I28继续补完整变化及实例保留策略。

## 正式来源、投影及能力

新增 `application_player_role` v1，仍属同次普通采集及同一KnownCurrent use。`player-source-v2`增加必须的 `role_state=player-role-state-v1`；该扩展独立版本化，旧来源缺标签或扩展将明确拒绝。既有final-state／equipment-state版本不变。原生补充只在已有readiness>=2下复制PlayerIndex、ItemState、Suplex具名值及公开sub／hologram结果，不向非活动身体转赠别人的PlayerUpdate资格。

Entity公开fields与新补充逐值核对；player域额外保留flipped_form，sub／main／other／flipped与真实sub／twin／backup／replaced storage分开。PlayerIndex不是网络身份；本地roster按目标顺序建立，非roster仍安装其来源索引，使原生sub／hologram判断与来源一致。

发送投影保留新增紧凑来源及上述公开查询，并记录具体消费者；没有按角色名、相同资源路径或相等库存推断共享。`player-application.json`由生成器登记41类型、I21—I25安装器、逐签名处置与I39未运行状态。产品CMake接入两个新模块及角色注册表依赖，SDK单元沿既有C++17边界，不增加Hook owner或测试夹具依赖。

缺完整I26／I27／I29 adapter时仍在世界写前返回 `players_remaining_role_draw_adapters_pending_I26_I29`。`native_apply`和完整`native_source_complete`继续false；本轮不提供空adapter使完整Player提前显示、提交或发送Applied ACK。

## 必要逆向与主审

按用户指定，一个GPT-6 Luna max子智能体先查本地原版／REPENTOGON Wiki，再查官方Lua wrapper／ZHL，最后只读核定J273缺口。研究正文见[研究及纠错](player-i21-i25-native.md)。主智能体独立复读以下实际采用路径：

| 点位 | 主审采用结论 |
| --- | --- |
| `0x74D9B0 / RVA0x34D9B0` | `mov eax,[ecx+1610h]; shr eax,31; ret`。IsSubPlayer是索引符号，不是sub关系的反向证明。 |
| `0x6D86D0 / RVA0x2D86D0` | GetSubPlayer直接读+1D84；不按角色名或roster补边。 |
| `0x413140 / RVA0x13140`、`0x413160 / RVA0x13160` | main为有效非-1且低于自身索引的twin，否则self；other直接读+1E4C。不能把普通16／17强解释成twin。 |
| `0x7D1C90 / RVA0x3D1C90` | IsHologram要求exists、type29或38、非self twin及有效较低twin索引；不读Birthright物品或独立flag。主审否决用backup非空模拟hologram。 |
| REP `LuaEntityPlayer.cpp::Lua_PlayerGetBackupPlayer` | GetFlippedForm实际直接返回+1E50；Wiki“仅用于Lazarus”是用途描述，不能将其他角色已存在的真实backup强改Null。 |
| `0x7C2470 / RVA0x3C2470`，`0x7C26DB—0x7C26ED` | 普通Forgotten交换两实例的PlayerType及大量状态／武器owner，伴随玩法和效果，不能当无副作用setter。主线保留逻辑Key并使用完整重建方案。 |
| `0x784A60 / RVA0x384A60`，`0x78558D—0x785733` | held.Visible=false才父代画；临时Visible=true并经vtable+14 Render，随后归还可见性、Sprite scale(+E4/+E8)、rotation(+EC)、alpha(+FC)。主审纠正子智能体把这些字段称为PositionOffset(+34C..+354)的错误；Render位置参数另由双方+33C/+340计算。原生不在此写Parent。 |

子智能体初读普通补体时曾报告“同型创建”，随后复读三元分支纠正为相反16／17；本次实现采用纠正结论，主审复读实际Swap类型交换。静态事实不外推全部玩法异常、任意第三方角色或完整draw输入已闭合。

## 交接

I26继续Esau Jr四槽，I27继续其它原版专用状态，I28—I38继续变化／完整绘制／输入／HUD视图／隔离／退休／Scene／预算／编排，I39统一验收。后续验收需要实际覆盖E03—E07、E18、E21—E25及18／19回归；本轮没有分项测试、游戏启动或tx artifact。
