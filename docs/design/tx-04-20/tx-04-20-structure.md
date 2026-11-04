# tx-04-20 I05—I10：投影、身体计划与结构 owner

日期：2026-11-04，Asia/Singapore。仍为同一 tx-04-20。本轮接入 I05—I10 的计划、结构与依赖接口；没有构建、测试、启动游戏或修改 IDB。I11—I40 继续按原清单推进，统一 E2E 仍在 I39；本记录不是运行验收结果。

## 交付与执行边界

| 项 | 实现 | 边界 |
| --- | --- | --- |
| I05 | 撤销 Player、Weapon、TemporaryEffects、History、PlayerManager 的旧按参查询省略；P1—P6逐项合同写入生成目录；有限槽和当前配置 ID 的四种计数组合沿正式 reader 完整保留 | 未调度、失败、Unavailable 不变成完整值；不新增 Client 玩法查询。主／Body／内嵌／costume 不因文件相同建立别名 |
| I06 | `replica/player_plan.*`：从最后提交 A 到完整 T 的身体／库存／主动／口袋／效果／costume／Weapon／History 子 owner 变化；记录成本、结构／成员／状态／资源／关系步骤；比较时在原组件解析 graph_ref | 重复 T 无原生写；旧 revision 和同 revision 内容冲突拒绝；remove／换实例／Room归属变化明确选择 Scene 重建。半应用值不成为下一轮 A |
| I07 | `client_players.*`／`client_players_native.*`：稳定 Key→实例／owner epoch／shape／role；用途区分 Structure、State、Relations、Draw；重新核对根、线程、vptr、seed、Room Index和本地实例 | 不使用 participant ordinal、ControllerIndex、地址或 seed 分配身份；退出／部分失败撤销借用；只有 Scene 可以提交 |
| I08 | 游戏 allocator→未构造 raw 预清零→完整 Player ctor→基 Entity Init 的结构前缀；单独安装 roster、分配本地 PlayerIndex、Room Index和 EntityList 发布；逐前缀退休／Free | 命名为 base-entity-initialized／role-uninitialized，不能视为角色已经初始化。完整 Player Init、SpawnCoPlayer2、InitTwin、Join不采用；不会自动生成 T 之外的身体／武器／Familiar |
| I09 | 正式同窗口集合计数及四个具名 storage 引用；分开全部身体、roster顺序、Room／render／update、控制组和四备份槽；Entity19用 owner见证替代强制roster成员资格和相等人数判据 | 非roster不意味着失效；无活动输入身体不等于全体退休。备份只在 C/S owner 纯值表中关联，不写原生 PlayerManager 的四个 Esau 槽；读取资格不因存在而扩大 |
| I10 | 具名 EntityPtr、twin、sub／backup／replaced raw、held 借用及同 T 的 Weapon owner／mainEntity 依赖；全身体结构先取得，再准备状态／资源，Entity／Grid实例全部存在后安装后置边 | Weapon具体动态实例与私有 mainEntity setter 由 I20 的子owner执行，I10提供完整路径／Key／类型及后置解析接口；不猜所有派生类的+8布局。136 lifted descriptor继续共用18／19同一T |

**本轮不提供 I11—I29 的完整最终状态、资源、角色与武器 adapter。** `ConfigurePlayerAdapter` 在计划前接收具名产品 adapter；缺少 prepare／install／relations／逐前缀 retire 时，`ClientPlayers::Prepare` 在世界写前给出 `players_state_resource_role_adapters_pending_I11_I29`。不能用空实现、全零或旧单 Isaac adapter 冒充这些后续任务；`native_apply`／`native_source_complete`／完整 Entity 提交仍为 false。I08 的结构实现可由后续完整 adapter 使用，但本轮没有声称多身体已经可以在游戏中完整应用或绘制。

## 来源和版本

正式合同为 `player-source-v2`／`player-target-v2`；能力协商同时声明 required／supported v2。旧单 Isaac 代表 profile 的识别分支保留；声明20扩展却缺v2集合字段的来源不能回退为该旧profile。

`PlayerApplicationMembership`只在原有采集窗口计数main／persistent owner和render／update成员，保留当前valid；不以可输入、存在或Visible推测集合。集合在采集结束再次比较。备份／非活动身体不能借用其他身体的PlayerUpdate资格。

原生关系来源只在 `KnownCurrent` 且 readiness>=2 的现有合法窗口读取；否则显式Unavailable。`sub_storage`只表示+1D84实际槽，不能偷换成公开GetSubPlayer。twin／backup／replaced分别保留，不将main/other逻辑getter当成两块独立存储。原始槽偏移与两个Game借用槽集中登记在`binding/world_layout.json`。

图编号只在自身组件内解析。装备以完整Entity组件为图根，History保留独立Player组件；比较不把相同graph_ref编号当作相同内容，也不因节点改号构造新原生owner。History子owner当前保守采用完整Player组件失效，成本进一步收敛归I35。

## 必要逆向与主审

按用户要求，使用一个 GPT-6 Luna、max 子智能体只读研究，主智能体独立复读源码与 IDA。先查本地原版 `EntityPlayer.md`、REPENTOGON `PlayerManager.md`／`EntityPlayer.md`／`Weapon.md`，再查官方ZHL及Lua wrapper／EntityPlus／EntityManager／ESSM；下述具体未暴露分支才查J273。

| 静态位置 | 结论及主审修正 |
| --- | --- |
| ctor VA0x74DC70／RVA0x34DC70，0x74DCC8—0x74DD34 | 13个ANM及容器正常构造；不是整个Player字段初始化。PlayerType、Weapon[5]、held、marked、mega laser、sub_storage、backup等没有ctor store。只对尚未构造的自有raw预清零，之后调用真实ctor；不覆盖存活对象或补造Source值。ctor中途异常仍未知 |
| Entity Init VA0x6A4A20／RVA0x2A4A20；Player虚表VA0xB1BDB4的+0x64与+0x60 | 经过官方Plus链；+0x64→0x6A4F30加载公共Entity配置，+0x60→0x751B10复制颜色状态。主审确认它不是纯清零，也不是Player派生Init；PlayerType另装源最终值，本地controller保持-1，PlayerIndex在roster阶段分配 |
| Player Init VA0x74F6A0；SpawnCoPlayer2 VA0x990250；InitTwin VA0x7C8370 | 完整角色Init、manager RNG、双身体／控制／HUD副作用仍否决。普通Spawn的PlayerIndex取roster长度，Entity Index在Init后另由Room分配；两个索引不是参与者身份 |
| Player Free VA0x74E970→析构0x74F260；0x74F2C1—0x74F2E9 | +1D84非空且子身体不在EL时会递归ClearReferences／Free，必须先拆掉本方完整关系图，再各owner恰一次释放，不能先删子再让父递归删。twin是EntityPtr，其他raw槽不能套同一登记结构 |
| 析构0x74F302—0x74F316 | Game+1BB70／+1BB74等于当前对象时会被清空。自有退休先拒绝这些global owner仍借用的情况，不让析构暗中夺走其他owner |
| Entity Remove VA0x6A8000／helper0x84CA50 | BaseInit后Exists=true，Free会进入Remove并可能发67回调，不能当作无事件free或先改Exists绕过。主审补读`client_native_bridge.cpp::GuardedClientCallback`，要求当前VM的callback／持久保护有效，Client的一般ModCallback被跳过，官方EntityManager／EntityPlus／ESSM结构链保留 |

`GetMainTwin`／`GetOtherTwin`公开结果具有角色／Birthright条件，不直接写回twin槽；GetFlippedForm读取backup raw，ReplacePlayer的replaced raw是另一种临时关系。Weapon.GetMainEntity为动态虚函数，base0x40不是全武器布局；不能将其与Player五个槽或Weapon owner+4合并。Familiar.Player为裸借用，由19关系adapter安装最终关联，不调用CheckFamiliar或Update。

## 清理与提交

执行顺序为全身体结构→最终量／资源adapter→roster／Room发布→非Player实例／Grid→Entity边→Player／装备后置边→真实后态检查→Scene唯一提交。四种用途不会由一个“有对象”布尔代替；非render身体不进入绘制枚举。

退休先使Player票据失效，非Player owner撤边／退休，随后Player全图拆引用、撤roster、摘Room各列表并失效类型缓存、检查sub／twin／backup／replaced／held／Weapon／HUD及Game借用，再恰一次Free。ctor／Init／Free进入而未正常返回、未知allocator／官方异常、callback保护丢失或未退尽CPU队列时保留实际owner和首因；不重复释放或假报菜单安全。未进入任何原生前缀的候选可直接取消。

剩余完整最终值／资源／角色／武器／draw与HUD、逐前缀全族清理、预算进一步优化及统一E2E，分别按I11—I40执行。没有新增单元测试、分项试跑或最终artifact。
