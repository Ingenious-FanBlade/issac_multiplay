# tx-04-20 I04：Player 原生采用合同（静态研究）

日期：2026-11-04。目标：官方 REPENTOGON 1.1.3 / `v1.9.7.12.J273`。本记录只含公开资料静态确认、已有 09/13/17/19 事实复用和必要 IDA 静态观察；没有启动游戏、构建、测试、哈希校验或 patch。未知调用不因结构声明、成功的单次运行或同名 Getter 而升级为已核定。

I16—I20补充采用及主审见[装备／资源安装](tx-04-20-equipment-installation.md)：TemporaryEffects首字cache mask、History mutation-disabled／索引副作用、costume真实ANM owner、15类五槽Weapon工厂／全部EntityPtr／chargebar／窄Free。下文保留I04当时的未知；已补证项按该补充及机器索引解释，不能把未采用的玩法缓存或I21—I40误记完成。

I21—I25后续补证见[角色／持物安装与主审](tx-04-20-role-installation.md)、[R09—R11具体研究及纠错](player-i21-i25-native.md)：main／other／sub／hologram的实际leaf、16／17单向相反类型补体、35／40双向twin、29／38 backup及通用held父代画。原R09—R11中的UNKNOWN保留原时点，补证不扩大到I26—I40或完整I29绘制。

## 0. 证据口径与共同边界

- `PUBLIC`：本地原版 Wiki `F:/projects_isaac/IsaacDocs/docs`；官方 REPENTOGON 1.1.3 Wiki、wrapper、ZHL/SDK `F:/projects_isaac/REPENTOGON-1.1.3`。wrapper 说明 Getter 实际取值与拷贝/借用方式；ZHL 的成员布局本身只说明布局，不证明字段当前可读或资源已加载。
- `J273`：共享 IDA 数据库 `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，image base `0x400000`。下文的 VA 为 IDA 地址，RVA=`VA-0x400000`。没有校验任何映像摘要。
- `REUSED_09/13/17/19`：分别引用既有四参与者/多身体、正式 Player 来源资格、对象生命周期/展示隔离、Entity 应用与资源退休合同；不将历史 PASS 推成新角色或未测分支覆盖。
- **来源资格**：项目已接受的 `USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS` 只适用于经历自己正常 PlayerUpdate 的当前身体；不能转给 twin/sub/backup。公开 PlayerManager 来源枚举和关系 Getter 可以证明“此指针/关系当前可达”，不自动证明这个身体所有私有原生字段有当前合法值。关系、存在、roster、活动、可见、可输入、字段可读分别记。
- **写入与隔离**：本体最终值用正式公开结构/窄核定 writer 直接安装；不运行 `Player::Update`、`EvaluateItems`、`Add/RemoveCollectible`、伤害/拾取/投掷/`SwapForgottenForm`、武器 `Fire/Update` 或整条角色 Init 来“补值”。所有许可原生调用限制在主线程、唯一 Client 用途/生命周期 scope；Host/Off 原样透传，观测 wrapper 按原有规则 exactly-once。

### J273 共同地址事实

| 事实 | 证据与含义 |
| --- | --- |
| Player 布局 | REP `libzhl/functions/EntityPlayer.zhl:613—865`：大小 `0x2EA0`；`_bodySprite +0x41C`、`_bloodGushSprite +0xEE4`、`_heldSprite +0xFF8`、costume desc vector `+0x1220`、15 项 layer map `+0x122C`、5 个 Weapon 指针 `+0x13D8`、TemporaryEffects `+0x1500`、4 个 ActiveItemDesc `+0x1574`、History `+0x1DD8`、twin EntityPtr `+0x1E4C`、backup 裸指针 `+0x1E50`、PlayerHUD 裸指针 `+0x20A8`、unlisted-restore flag/state `+0x20BC/+0x20C0`、replaced 裸指针 `+0x267C`。已有 19 更正：`Player+0x44C` 是内嵌 BodySprite 的动画字段，不是 HeldSprite。
| PlayerManager 布局 | REP `libzhl/functions/PlayerManager.zhl:68—73`：公开声明的主 roster vector `+0x0`、`_esauJrState[4] +0x80`、结构大小 `0xC8`。注意这份 SDK 声明未描述 IDA `SpawnCoPlayer2(-1)` 读写的 `manager+0xD0/+0xD4/+0xD8` 独立 vector；不能拿 `0xC8` 大小封闭该特殊分支。
| Player ctor 与 inline ANM | IDA `Entity_Player::constructor VA 0x74DC70 / RVA 0x34DC70`；`0x74DD16` 对 `this+0xEE4`、`0x74DD25` 对 `this+0xFF8` 调 `ANM2 ctor 0x402B00 / RVA 0x2B00`。主审复读 ctor 指令确认从 `Player+0x41C` 起连续13项 `0x114` byte ANM2：单体 `+0x41C/+0x530/+0x644`、数组 `+0x758` 的7项、尾部 `+0xEE4/+0xFF8/+0x110C`。REP SDK命名 body/blood-gush/held 三项，其余沿 `world_layout` 中 `inline_n` 名称。构造证明对象存在，不证明已 Load 或绘制使用。
| Player Init | 官方 ZHL `EntityPlayer.zhl` vtable slot 1 声明 `void __thiscall Entity_Player::Init(type,variant,subtype,initSeed)`；IDA 唯一签名命中 `VA 0x74F6A0 / RVA 0x34F6A0`。不是空 setter；含多项角色、配置、资源和官方处理 callee。普通 Isaac 的既有有限前置见 tx-04-17，不外推。
| 构造双体 | IDA `PlayerManager::SpawnCoPlayer2 VA 0x990250 / RVA 0x590250`；`Entity_Player::InitTwin VA 0x7C8370 / RVA 0x3C8370`；`SetControllerId VA 0x79E1B0 / RVA 0x39E1B0`。均为 J273 x86；`SpawnCoPlayer2` 是 `__thiscall Entity_Player*(PlayerManager* this, PlayerType)`，`InitTwin` 是 `__thiscall Player*(PlayerType)`，controller setter 以 ZHL/既有 tx09 签名为准。
| Player 原生 Render | IDA Player vtable `0xB1BDB4` slot `+0x14` 指向 `VA 0x784A60 / RVA 0x384A60`；官方 ZHL 为 `void __thiscall Entity_Player::Render(Vector*)`。既有 17 研究到组合链 `0x784A60 → 0x783680 → RenderBody 0x786460 / RenderHead 0x786760 → ANM2::RenderLayer 0x405A50`。本次只补有限直接消费/临时写观察，不宣称完整所有 pass 闭包。
| Weapon 工厂/退休 | REP `Isaac.zhl` 声明 `CreateWeapon(WeaponType ECX, Entity* EDX)`、`DestoryWeapon(Weapon** ECX)`；IDA 工厂 `VA 0x9CADB0 / RVA 0x5CADB0`，退休 helper `VA 0x9CADE0 / RVA 0x5CADE0`。Factory 从全局类型工厂表创建动态类并调其 vtable Init；Destroy 对有效槽指针先调 vtable slot5 `ClearReferences`，再 slot0 `Free(1)`，最后才将槽写 Null。
| 普通运行时清理 | 复用 tx-04-09：`RemoveCoPlayer RVA 0x590560` 与角色清理 helper `RVA 0x351C10` 对 twin/sub/backup 有条件递归；按真实 PlayerIndex 升序且每次重读 roster 才避免重复移除。完整退出 `PlayerManager cleanup VA 0x98F7C0 / RVA 0x58F7C0` 需要有效 Room 与一致 roster，是正常场景 owner，不是半构造回滚器。

## R01—R16 逐项采用 / 否决合同

### R01：目录、读取资格、scope 与投影

- **决定：采用公开只读字段与参数的当前正式来源，否决“可达指针即全字段可读”。** PlayerManager roster、main/other、twin/sub、四个 Esau Jr 槽、replaced 仅组成有类型的来源图。当前普通身体有自己的正常 PlayerUpdate 见证才满足现行私有字段读取条件；备份/未活动对象不继承其锚点见证。Source owner/thread/scope 前后必须稳定；空/不适用、未访问、Null、读取失败分别保留。
- **证据：** `PlayerManager.md:64—75,117`；`LuaPlayerManager.cpp:242—270`（主 roster 与 0..3 备份索引）；`EntityPlayer.zhl:724—725,830—861`；tx-04-09 `four-participant-implementation.md:15—37`；tx-04-13 `tx-04-13-implementation.md:15,33`；tx-04-20 I03 由主线补正式公开 `application_player`。
- **ABI/前置/副作用/失败与清理：** 已采用入口均是公开只读查询（Manager Lua wrapper 在游戏 Lua/主线程内）；不新增未核定的 native 私有标量读取。R07 已有构造证据且同一合法 Player source scope 内复制的 13 项内嵌 ANM 对象是明确采用例外。costume desc 与 ANM getter 是 owner 内借用，必须当场复制纯值/受管资源句柄，换 instance/resource 时失效。reader 任一前后 root/use/shape 不一致则不发布该域值，不改世界；只释放本地临时 DTO，不调用游戏清理函数。
- **具体 UNKNOWN：** 所有非活动/备份身体的具体公开字段逐项来源/状态完整性仍按主线新增 manifest 判断；未列公开结构之外的私有 draw 标量目前无合法来源。不是将 `readiness=3` 或普通 PlayerUpdate 证言扩给 backup。
- **落点：** I01—I05；P01/P20；R03/R11/R13；正式拒绝应是字段/身体/source key 级。

### R02：Player 构造、Init、发布与列表 owner

- **决定：否决将 `SpawnCoPlayer2`/`InitTwin` 当作通用 Client clone API；普通 Isaac 已验前置只作为原窄场景历史复用。** 自有受控构造仍需按对象阶段显式编排；不得裸建 Player 或把自然自动生成结果与 T 中已有 Key 再创建一份。
- **证据：** REP `LuaPlayerManager.cpp:162—170`、`LuaEntityPlayer.cpp:179—201`；`EntityPlayer.md:1015—1018`（InitTwin 公义）及 `InitPostLevelInitStats` 文档；J273 `0x990250`：常规分支检查 roster 总数>=64 后返回 null；否则 `new(0x2EA0)`→`0x74DC70` ctor→先 push 主 roster→虚 Init（`Init(1,0,PlayerType,seed)`）正常返回后才 `EntityList::Add 0x413D00`；Manager RNG seed 被推进。`InitTwin 0x7C8370` 首调 SpawnCoPlayer2，立即于 `0x7C8394`解引用返回值，无 null guard；双向改 `+0x1E4C`、传播 controller、移动原身/新体位置，并调用 `0x980130`（既有 tx-17 定名 AssignPlayerHUDs，非纯关系操作）。
- **ABI/前置/副作用/失败与清理：** `SpawnCoPlayer2` 为 `__thiscall Entity_Player*(PlayerManager* this, PlayerType)`；主线程、有效 Manager/Game/Room/RNG、合法 PlayerType、roster capacity。Spawn 原生会分配/建构/入 roster、调用多态 Init、推进 RNG，之后登记 EntityList；Init 可能含角色/装备/资源/官方/玩法副作用，不允许泛用透传。失败前缀：满额在分配前返回 Null；分配/构造、roster push、Init 或 Room-list publish 各自是不同前缀；IDA 未证明中间异常/oom 自动回滚。InitTwin 的 Spawn 返回 Null 会在后续解引用崩溃；不能当可恢复 null API。正常释放只能由准确 PlayerManager/Room/关系 owner 按已发布阶段执行，不能对未 Init 对象调用 RemoveCoPlayer 或全局 cleanup。
- **特殊分支：** `PlayerType==-1` 进入 `manager+0xD0/+0xD4/+0xD8` 独立 vector 分支并调用 `Init(1,2,0,seed)`；它不走常规主 roster 末尾的 EntityList Add。公开 Manager ZHL 仅到 `0xC8`，该分支容器布局/owner/退出仍 UNKNOWN，不纳入普通 Player 构造合同。
- **落点：** I06—I09、I33；P01/P19；C01—C17 按族有独立构造决策。

### R03：身体关系、共享、备份与借用

- **决定：采用按边类型建图，否决“一个 Player vector 覆盖全部身体/关系”。** roster、Room EntityList、twin EntityPtr、sub、backup/replaced 裸指针、Esau Jr Manager 槽及 C/S participant/control group 分别建边；control relation 与 membership 不能互换，共享字段只按特定 owner 明示。
- **证据：** tx-04-09 `four-participant-implementation.md:15—37`（relation/node limit 64，four groups 不等于 body count，退出递归事实）；REP `EntityPlayer.zhl:830—861`（twin EntityPtr vs backup/replaced raw）；`PlayerManager.zhl:70—72`；tx-04-13 `tx-04-13-implementation.md:15`；tx-04-19 `entity-application-contracts.md:64,108—118,222—230`（Familiar Player/Weapon 裸依赖与跨写）。`sub_player +0x1D84` 是 tx-04-09 既有 J273 点位，但未被本次 ZHL 声明为字段。
- **ABI/前置/副作用/失败与清理：** 关系安装需目标对象已有有效 structure owner、Key/use/revision 已绑定；EntityPtr 写入与 raw pointer 不是同一种 setter/所有权。未解析非空边不能写 Null 伪装清理。遍历需环与重复有界。来源临时缺席只撤销该用途资格，不推断死亡/销毁。释放由产生边的 owner 负责逆向解绑；跨组重用拒绝。
- **具体 UNKNOWN：** `sub_player +0x1D84` 的当前官方声明/完整写者闭包、所有关系边的 native 反向引用维护及每型的跨身体共享字段仍需按实际采用 consumer 核定；不通过名字推导 owner。
- **落点：** I07—I10/I17；P01/P10/P15/P19；C06—C10。

### R04：公共状态、心量、资源与 stats

- **决定：采用当前同源公开最终值/direct final install；否决 Add/Remove/受击与重算链做同步。** 普通及特殊角色心量各按本角色结构/合法范围；Source 侧数值/缓存与显示动画分别归属。
- **证据：** 原版/REP `EntityPlayer.md` 的健康、资源及公开最终字段；REP `EntityPlayer.zhl:739—760,808—820` 的心量/钥匙/炸弹/硬币/最终 stats、骨/腐/破碎心及 six stat-up arrays；既有 tx-04-19 entity adapter 普通 Player 合同；tx-04-17 update/persistence/gameplay boundary。
- **ABI/前置/副作用/失败与清理：** 本条不采用 native 修改方法。AddHearts/TakeDamage/Revive/EvaluateItems 会引入规则、缓存、Lua/其他副作用；只允许 I11—I13 owner 对已验证目标做字段级最终量安装。写前完整目标与单位/位宽已解析，写后完整回读；部分写失败不提升 A，保留首个失败字段并停该 scope。清 stale 专用量由目标角色迁换/清零表驱动，不调用道具链。
- **具体 UNKNOWN：** 现有公共 schema 未覆盖的 private draw-only heart flashes、特殊角色 cache flags 和所有 writer 触发闭包目前未核定；不因 offset 已知而加入 source。
- **落点：** I11—I13；P02—P05；C01—C05/C12/C15/C16。

### R05：库存、饰品、主动/口袋槽

- **决定：采用完整有限容器/槽值；否决循环获得物品、使用物品或只复制 Get 默认参数。** `GetWeapon`槽集[0..4]与 ActiveItemDesc 槽[4]是不同对象域；ActiveItemDesc 的 4 个当前描述可按官方有限枚举读取。库存类型表/参数矩阵由 I03 正式 reader 限定。
- **证据：** REP `EntityPlayer.md:272—275,931—945`；`LuaEntityPlayer.cpp:477—489`（ActiveItemDesc getter 由 `LuaCheckActiveSlot` 验证并返回 native descriptor pointer）；`LuaWeapon.cpp:43—57`（slot 0..4，只读指针，空为nil）；`EntityPlayer.zhl:748,772,777—781`（weapon/active desc/容器布局）。`PlayerTypes_ActiveItemDesc.md`、`PocketItem`/`QueueItemData` Wiki；tx-04-19 已有只读查询/完整范围原则。
- **ABI/前置/副作用/失败与清理：** Getter 返回值/借用只在当前 Player use 内有效；写入若采用则只写清楚的最终槽存储并配对库存/派生 costume/wisp/weapon 的完整目标，不调用 `AddCollectible/RemoveCollectible/SetPocketActiveItem/ControlActiveItem` 生成副作用。槽范围固定；“空槽”须是显式 Null/有效性值，不作为未访问。失败在容器换代阶段停止发布并留住实际拥有者；客户端不析构游戏 vector header/跨 CRT allocator。
- **具体 UNKNOWN：** 所有角色专有 VarData 字段语义、ActiveItemDesc 动态配置 allocator 和原生槽更新会影响的每个 weapon/costume/familiar 关联仍要跟实际 I14/I15 writer 逐项补；C13 crafting bag/queue 现有 getter不等价于原生恢复链。
- **落点：** I14—I15/I20/I26；P06—P08；C11/C13/C14。

### R06：TemporaryEffects、变身与 History

- **决定：采用各自完整有序/计数结构或只读视图；否决用 HasEffect 布尔、库存或 History 代替另一域，也否决 History 重放授予物品。**
- **证据：** 本地 `TemporaryEffects.md`、REP `History.md`/`TemporaryEffects` wrapper；REP `EntityPlayer.zhl:761`（TemporaryEffects +1500）、`:798—800`（Player forms array +17CC）、`:827`（History +1DD8）；tx-04-13 `tx-04-13-implementation.md`（历史单独域和来源资格）。
- **ABI/前置/副作用/失败与清理：** getter 只在当前有效 Player source 中采值，保留空列表与未访问区别、顺序与duration/source。Add/Clear effect 和 transformation API 视为玩法/资源维护候选而不直接采用；直接最终容器安装前须确定容器分配器、borrowers 和缓存清理。部分 list replace 失败时旧资源/目标不可假装相等，保持 retirement owner 并撤销绘制资格；按原 owner 清容器/资源，不用 Update 完成衰退。
- **具体 UNKNOWN：** History 是否被当前原生 Player/HUD draw 直接要求、全部 TemporaryEffects 子项的共享和角色化时效、清理时覆盖的具体 Costume/Weapon/wisp 仍需对所选 consumer 闭合。
- **落点：** I16—I17；P09—P10；C 全角色专用状态。

### R07：Player 资源、costume、动态重载与借用

- **决定：采用真实嵌入 Sprite 和当前 CostumeSpriteDesc/costume map 资源，不复制 ANM2 的原始内存或只传一个主 Sprite；否决“同文件名就是同对象”。父线已将完整13项 inline ANM 纳入唯一 `world_layout` 并补正式只读来源，本研究确认构造前缀与资源身份。**
- **证据：** REP `EntityPlayer.zhl:728—735`；IDA Player ctor `0x74DD16/0x74DD25`构造 blood/held ANM2，主审复读完整项为 `+0x41C` 起13项、stride `0x114`（未命名成员按正式布局使用 `inline_n`）；`LuaEntityPlayer.cpp:3407—3411`的 GetBodySprite 返回内嵌地址；`LuaCostumeSpriteDesc.cpp:5—21`迭代 desc vector 并把 vector 元素地址放进 Lua userdata，`:23—35`返回 desc 所持 Sprite / ItemConfig pointer；tx-04-19 resource生命周期合同。
- **ABI/前置/副作用/失败与清理：** ANM2 所属 Player/Costume desc owner 先构造，renderer/resource context 与资源代次有效；复制层/track/pose/selector/display 为纯值，Image/native references 用既有共享 owner服务。costume vector 增删/角色换装/Reload 会使 element userdata 指针失效；关闭 draw admission→退尽实际借用/CPU队列→重装/重新解析→成功后再开放。ctor 调 `ANM2 ctor 0x402B00`只初始化对象，非 Load；未加载必须按 Null，不调用 reload/update补出。
- **具体 UNKNOWN：** 13项 ANM 全量成员分别被哪些角色/pass消费、所有头/手/overlay/costume来源和texture/GPU队列最终完成仍未闭合。`headAnim +0x1320`、blink timers等没有本轮定位合法当前 writer witness，暂不采集；不得因现存inline ANM统一来源而扩大私有标量资格。
- **落点：** I18—I19/I29；P11—P12；C01—C17。

### R08：Weapon 类型/owner/依赖/生命周期

- **决定：采用 GetWeapon 的 5 个具名槽和公开 Weapon 最终值；拒绝 fake pointer、通用 base layout 覆盖全部动态子类、Fire/Shoot/Update取缓存。Weapon类型/slot/主Entity分开建模，Null main entity 可以合法。**
- **证据：** REP `EntityPlayer.md:931—945`（0..4、nil可合法）；REP `Weapon.md`（owner/main entity/charge/delay/direction/modifiers）；`Weapon.zhl:28—82` base `0x40` 字节布局及虚表；`LuaWeapon.cpp:10—22,43—57` create/get wrapper；`Isaac.zhl:CreateWeapon/DestoryWeapon`。IDA factory `0x9CADB0` / retire `0x9CADE0`（见共同地址事实）。
- **ABI/前置/副作用/失败与清理：** Factory `__fastcall (WeaponType ECX, Entity* EDX)`索引全局动态factory表；空工厂项返回Null；非空后调用工厂，再虚 Init(type,owner)并返回。wrapper `Isaac.CreateWeapon`只是返回游戏对象，不自动写 Player weapon slot；`SetWeapon`直接换槽并记wrapper owner，非生命周期证明。创建需唯一动态类型工厂、正确 owner、Player slot唯一owner及相关 Entity/资源已准备。Destroy 是 `Weapon**` 槽 owner操作：先 virtual `ClearReferences`→virtual `Free(1)`→正常返回后清 Null；失败/异常在Free或Null前缀不明时不能再次 Destroy。Weapon部分构造/Init异常不能泛用释放。
- **具体 UNKNOWN：** W01—W15 全动态派生私有布局、每型 factory Init/自动 Entity/渲染专属缓存及正常退休副写尚未逐项闭合；base `0x40`不是子类完整大小。当前 profile 对未覆盖具体 type 应写前失败，不回退 Tears。
- **落点：** I10/I20/I29/I33；P13/P15；W01—W15；C03—C05/C16。

### R09：held Entity、Suplex、投掷与矿车

- **决定：采用同 T 的真实 held Entity/Player 关系及展示输入，拒绝调用 TryHold/Throw/Suplex 或重新生成投掷物。136 事实只在已有窄路径内复用，不外推所有持物。**
- **证据：** REP `EntityPlayer.zhl` `_heldEntity +0x140C`、`_suplexState +0x1F08`等；tx-04-19 acceptance 的 136 举持、Player代画与关系；tx-04-19 `entity-i20-i25-native.md:78—90`（Familiar Player借用/跨Render临时恢复，minecart/offset）；主计划 R09 对 19 `Effect136` 的窄事实。
- **ABI/前置/副作用/失败与清理：** Player Render callsite 仅在同一 scope 下使用有效 held Key/形态和已加载ANM；隐藏 held 的代画是父 draw 行为，不全局 Visible=true。Suplex 0..5状态只消费 Source最终状态。minecart 偏移与reverse relation 临时安装要保持到原CPU队列实际消费，再恢复全部原值。丢失/替换目标立即撤销借用，目标退休由自身owner处理，Player不重复Delete。
- **具体 UNKNOWN：** 除19已见Effect136、held visible/body null-frame、Suplex值外，各 held 动态类型/parent链与角色资源读取的通用闭包未完成；Type965乘员及所有间接caller不等于完整普通Player draw。
- **落点：** I21/I24/I29；P14/P15；C08。

### R10：Jacob/Esau、Forgotten/Soul

- **决定：采用“多身体一个控制组”的现有 Authority 语义；拒绝在 Client 调 InitTwin/SwapForgottenForm 模拟角色操作。** Jacob/Esau 两实体独立状态/库存；普通Forgotten 16/17切换使用同两实例的已观察语义；Tainted Forgotten 35/40 需独立 skeletal Player/Soul+held 关系合同。
- **证据：** tx-04-09 `four-participant-implementation.md:15—37`实际四组/八身体；其中 J273 `InitTwin RVA0x3C8370`及 `SwapForgottenForm RVA0x3C2470`；REP `EntityPlayer.md:1015—1018,1625—1639`、Lua wrapper `LuaEntityPlayer.cpp:179—201,1632—1637`；IDA `0x7C8370`副作用见 R02；tx-19 held/retirement合同。
- **ABI/前置/副作用/失败与清理：** InitTwin/SwapForgottenForm 为主线程游戏行为，会改变Manager roster、控制关系/位置/角色状态并可能触发效果；只接受 Source 最终 body state/关系/资源。构造体关系闭环先构造唯一 Key，再按对象kind后置链接；退出采用09真实索引升序/每次重读，不能先删递归子再删根。Scene 只有一个原子提交：任一组/关系写入失败即撤销整个候选 Scene 的展示资格；保留其他组已复制的纯值供诊断/重试，但不得把半应用组提交成 A，也不合成 participant。
- **具体 UNKNOWN：** Client 对普通Forgotten是原位修改还是同Key替换要按I24现状/资源owner核定；Tainted 35/40 的非roster身体完整只读资格和全资源没有由09 Authority运行事实证明。
- **落点：** I23—I24/I21/I28；P01/P03/P14/P16；C06—C08。

### R11：Tainted Lazarus/Flip/Birthright 与 Esau Jr

- **决定：采用四槽为全局原生run状态、与participant/player index分离；Client 只应用切换结果，不写/清理原生backup指针，也不把backup认作活动身体。**
- **证据：** REP `PlayerManager.zhl:70—72`四个槽位 `+0x80`; `LuaPlayerManager.cpp:258—270`合法索引0..3；REP `EntityPlayer.zhl:830—861`backup/raw replaced 关系与 private unlisted restore state；tx-04-09 `four-participant-implementation.md`的703槽1基映射和退出不清 Esau Jr 全局槽；tx-04-13 Esau槽按source单独记录。
- **ABI/前置/副作用/失败与清理：** `GetEsauJrState(index)`只读取槽；不能假设槽全空/拥有者可析构。已知 helper RVA `0x3A3A40`从活动道具实际槽值703找一基 backup slot，和Player/ordinal不等价；`ReplacePlayer RVA0x595E10`用 `+0x267C`保存互换关联，正常返回时清临时双方替换链接；未成功不转移逻辑归属。Flips/hologram动作由Source产生。Client自有镜像 owner 单独退休并先解绑，不碰Host PlayerManager槽。
- **具体 UNKNOWN：** backup未更新过身体全部私有字段何时有效、Tainted Lazarus Flip/Birthright 是否互换实例/容器的每个异常前缀、Esau Jr全局四槽的原生 run teardown 释放owner尚未核定；不能以“没有找到释放”宣称永久保留已解决。
- **落点：** I25—I26/I28/I33；P01/P07/P19；C09—C10。

### R12：全角色专用状态族

- **决定：采用PlayerType 0—40 的 C01—C17 分族，并为-1内部possessor、Baby/非Player关联对象另分类；否决把所有角色type仅写一个数字后宣称完成。** 状态容器可由官方公开API/既定有限字段读取；未公开私有结果如无法证明writer+consumer则具名缺失。
- **证据：** 原版本地 `enums/PlayerType.md`；`tx-04-20-player-application.md:C01—C17`；REP `EntityPlayer.zhl`的专用布局：BagOfCrafting `+0x1F20/+0x1F28`、Poop mana/queue `+0x1F30/+0x1F34`、Eve sumptorium `+0x1F58`、wisp map `+0x1F5C`、玩家专用魂/血/角色状态见结构段；Getters在官方 EntityPlayer wrapper 注册列表。结构偏移不单独授权读私有值。
- **ABI/前置/副作用/失败与清理：** 按正式角色配置/version/source manifest选择族；有限角色数组按实际类型门控，unknown、sentinel、internal值写前拒绝。Eden随机选择/tainted形态变化/复活/effects仅装Source最终值，不调用重随机、Use、update或普通角色Init兜底。转换时一起换库存/属性/资源/Weapon/wisp/clot等适用owner；任何必需未知成员阻断该body，不能回退普通Isaac。
- **具体 UNKNOWN：** 每族的全量结构/数据writer/Render读集仍未从公开API或已完成19事实逐一形成闭包；不能由上述少量布局字段推断相应全部角色皆可写/可画。目标“全类型基本形态”必须逐族审阅。
- **落点：** I22/I27/I29；P16/P20；C01—C05/C11—C17。

### R13：Player全pass绘制、缓存与外层caller

- **决定：采用已研究的普通 Player组合 render与安全 caller/队列合同；拒绝孤立slot调用、整条Update补动画、或把所有角色画成Sprite。**
- **证据：** J273 Player Render `VA0x784A60/RVA0x384A60`，vtable slot+0x14；IDA direct reads/writes包括继承 Entity render cache `+0x1A4/+0x1A8`，从 Player held pointer `+0x140C`做短时组合绘制；tx-04-17 `tx-04-17-native-maintenance-rendering.md:143—175,203—218`；tx-04-19 `entity-i20-i25-native.md:86—90`和 `entity-application-contracts.md:204—215`。Player完整 pass callgraph尚非本轮闭包。
- **ABI/前置/副作用/失败与清理：** 主线程、同一 committed Player/resource use、renderer pass/context/FBO/depth和原生CPU消费者已准备；`Render(Player*,Vector*)`本身会更新 render coords/cache并借用 held/附加资源，不能视为只读。临时变换/Visible/Color/ANM selector 按被写字段完整保存恢复，覆盖嵌套子Render并延至raw队列消费；正常返回恢复，异常/重入不明则停显、保留资源owner和隔离。
- **具体 UNKNOWN：** 所有C01—C17×W01—W15×pass的字段/ANM层/caller closure、特殊闪烁/ghost/flight和CPU/GPU最后使用点未完；静态调用证据不代表实际像素正确，统一E2E才判画面。
- **落点：** I29/I34；P11—P16/P19；C全族/W全型/E20。

### R14：输入关系、HUD与本地索引

- **决定：采用当前本地 PlayerIndex/ControllerIndex 作为本进程显示/原生槽索引，不作参与者身份；不启用原版Online IsLocalPlayer/PlayerHUD输入消费来获得C/S控制。** 安全必需HUD结构可能与完整HUD27分开处理。
- **证据：** REP `EntityPlayer.zhl:772—774,858` (`controller +0x160C`, `player index +0x1610`, PlayerHUD `+0x20A8`）；tx-04-09稳定participant vs原生index事实；tx-04-17 `tx-04-17-native-maintenance-rendering.md:151—160`（PlayerHUD Update会读输入/Drop并Lua、History/Stat刷新非纯只读）；`EntityPlayer.md`/`PlayerHUD.md`公开接口。
- **ABI/前置/副作用/失败与清理：** InitTwin里 SetControllerId `0x79E1B0`会按控制组传播，`0x980130`是 `AssignPlayerHUDs`，要求有效Manager/roster/HUD slots。display slot须独立映射，缺席时不得以服务器ordinal写入当前controller。禁止调用PlayerHUD Update/通用 input manager 解决同步，否则动作与Lua副作用进入。撤销/换实例时使Hud association失效并经其owner重配。
- **具体 UNKNOWN：** 全角色HUD最小安全指针/具体槽位分配对非活动/备份身体的完整需求不全；27负责完整布局与动态表现，20只建立draw/生命周期安全必要对象并且用正式只读Player值。
- **落点：** I30—I31；P17—P18；E17/E19/E24。

### R15：换形、删除、部分失败与在途退休

- **决定：采用逐Owner/逐前缀唯一清理；拒绝将正常 RemoveCoPlayer/全局 cleanup 作为任意部分构造回滚器。**
- **证据：** J273 `RemoveCoPlayer VA0x990560/RVA0x590560`; tx-04-09 `four-participant-implementation.md`递归 twin/sub/backup 事实及真实崩溃修复；tx-04-17 `tx-04-17-failure-retirement.md:34—90`阶段合同；其清理 `PlayerManager VA0x98F7C0/RVA0x58F7C0`要求 Room/roster有效，正常Exit先Room重建再cleanup；tx-04-19 `entity-application-contracts.md:222—246`类关系/CPU资源归属。
- **ABI/前置/副作用/失败与清理：** 阶段至少 raw / ctor-started / constructed / Manager-rostered / Init-entered-returned / Room-published / linked / resources / weapon / draw in-flight。进入可能写前登记尝试阶段；按正常 owner 拆出关系、双向引用、HUD、weapon、costume/ANM、userdata借用，等待真实在途退尽再退休。RemoveCoPlayer递归行为依实际 rosters而变，退出时升序重读Roster；对非roster backup使用本身owner，不能因从roster缺席而Free。未知 allocator/SEH/context前缀保持owner和隔离，不重复Remove/Free、不强交Off。
- **具体 UNKNOWN：** `Init`每个角色在部分派生/官方Hook异常时原生已清/未清哪些字段未穷尽；所有backup/Weapon/costume normal teardown边界和任意SEH恢复并未静态保证。每个实现采用路径需逐前缀填 owner，不将未知永久泄漏当正常成功。
- **落点：** I28/I33/I34；P19；E21/E23/E25。

### R16：玩法、RNG、Lua、持久化整链

- **决定：只采用无玩法重演的最终态值写和具体列白名单必要结构维护；所有角色 Init/Setter/Draw/Retire 周围的其它原生/官方路径默认不放行。**
- **证据：** J273 `SpawnCoPlayer2 0x990250`推进Manager RNG并动态Init；`InitTwin 0x7C8370`重绑控制关系/位置/HUD；`EntityPlayer.Init 0x74F6A0`是大量callee路径；`Player Render 0x784A60`有图形临时写；tx-04-17 `workflow.md`与`persistence-exit-isolation.md`全窗口保护；tx-04-19正式分类用途分派。
- **ABI/前置/副作用/失败与清理：** 每条实际原生调用须在唯一 Hook registry 用途 scope 记录 Host/Off原链一次、Client明确 CallOriginal/Skip；游戏主线程。Ctor、Init、cache recalc、active/weapon/effect actions、角色交换、PlayerHUD Update、渲染资源lazy reload及normal Exit/PGD writer分开审。写前登记可能执行，原链异常/早退不等于无变化；发生不明副作用后保持保护、不回滚玩法数据、不宣称Client完整。退休窗口持续至最后资源/持久writer安全返回。
- **具体 UNKNOWN：** 0x74F6A0所有角色Init嵌套Lua/ItemPool/随机/存档writer闭包、各角色setter嵌套官方回调和非主路径writer尚未逐一逆向；本记录因此否决整链采用，不宣称已知全部风险/已完成隔离。
- **落点：** I32—I33/I37；P20；E24/E25。

## 汇总：可立即实施的边界与未闭合项

1. 可直接走正式源：公开 Player/Weapon/TemporaryEffects/History/ActiveItemDesc/costume 参数与指定 API；父线已补完整 `application_player` 当前公开字段及有限参数查询。私有字段只在当前合法 source/witness 与 consumer明确要求时增加。
2. 可确认采用的数据布局：13项内嵌 ANM、costume vector/layer map、Weapon slots、ActiveItemDesc、TemporaryEffects、History、关系字段；布局不等于当前值/构造已完成/资源已加载。
3. 明确拒绝的通用调用：SpawnCoPlayer2 / InitTwin clone、Game/Player full Init、Add/Remove/Use item/effects、SwapForgottenForm、Weapon Fire/Update、Player/HUD Update和任意 PlayerManager 清理器作为半构造回滚。
4. `UNKNOWN`仍为本体的实际角色/Weapon完整reader-writer链、非活动/备份private字段读取资格、各类容器allocator、所有Pass/pixel闭包、完整Player Init/Setter/retirement副作用与失败展开；I04不把这些未知改写成“尚待用户决定”。
5. 本研究未运行测试；按现行规范，整体验收只在 tx-04-20 全目标完成后的 I39 统一执行。
