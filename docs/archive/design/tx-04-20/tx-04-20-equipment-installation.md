# tx-04-20 I16—I20：效果、History、身体资源、costume 与 Weapon

日期：2026-11-04，Asia/Singapore。沿用同一 tx-04-20。本轮接入 I16—I20 的正式来源、不可变值、原生安装及逐前缀子 owner；没有构建、运行测试或启动游戏。整个 tx 的统一 E2E 仍在 I39，不把静态实现记成运行通过。

## 实现

| 项 | 具体交付 |
| --- | --- |
| I16 | 完整有序 TemporaryEffect 项的本地配置身份、count、cooldown；容器的 CacheFlag mask、evaluate／disabled 状态及本地 Player owner；15 个真实变身计数。直接安装最终容器，不调用 Add／RemoveEffect、EvaluateItems 或变身触发链。 |
| I17 | 在 Player 组件图内解析完整 HistoryItem 顺序及时间、物品／饰品、stage、stage type、room、pool；安装真实 History vector 与已核 mutation-disabled 字节。`ClientPlayers::History` 仅在 Scene 提交后提供对应身体的不可变视图，供后续 HUD 接线；候选、部分写入和退休不暴露历史。 |
| I18 | 主 Sprite 加 13 个真实内嵌 ANM，各自安装完整主／overlay、分数 phase、layer／null selector、贴图、颜色、缩放、flip、shader 及播放标记；headAnim保留独立字节串，通过标准string赋值安装。头／手是实际 ANM／layer 的使用方式，不虚构独立对象，不按同文件合并资源。 |
| I19 | 完整有序 CostumeSpriteDesc、当前配置、priority、skin／flying／overlay／item-animation 等元数据及 15 项 layer map。结构不变时更新真实姿态；换配置／换结构时先构造本机 ANM 候选，再交换实际 Player 容器，逐项销毁旧 ANM；空目标也清除旧项。 |
| I20 | 五个显式 Weapon 槽与 1—15 的动态类型合同；最终 charge、delay、方向、modifiers、num fired 及两份真实 chargebar 结果；owner、main entity、item-animation 与派生 EntityPtr 全部按同 T 的稳定 Key 解析。类型替换、空槽、正常退休及异常前缀分别处理，不调用 Fire／Update。 |

`client_player_equipment_model.cpp` 与 SDK 原生实现分开。协议没有地址、虚表、allocator、vector 头或动态函数；所有偏移及动态类表由产品常量决定。两个组件的图分别建索引一次，数字相同的 node 不跨组件引用。源缺项、非法位宽、表洞、重复槽、错代引用及同 T 缺失依赖在写前拒绝。

每身体的 backing storage 与公开共享 Getter 语义分开。控制组、twin、数值相等都不证明共享库存／效果／History；已有公开查询结果继续保留在规范组件中，不用另一身体的值补缺。身体 Key／use 改变走原计划的替换／退休规则，不能误继承历史。

## 必要逆向及主审

按本次用户要求，使用一个 GPT-6 Luna max 子智能体只读研究，主智能体独立复读关键官方源码、IDA 反编译与指令。顺序为本地原版／REPENTOGON Wiki、官方 ZHL／Lua wrapper／生成 thunk、最后 J273 未公开点位。没有修改 IDB、运行进程、安装 Hook 或执行探测。

| 位置 | 采用结论及纠正 |
| --- | --- |
| 原版 TemporaryEffects／TemporaryEffect Wiki；REP ZHL；AddEffect VA0x90A090 | 效果不是假库存。TemporaryEffects 首字是 OR 的 CacheFlag mask，不是虚表／padding；vector 从 +4 开始，+0x10／+0x11 为 evaluate／disabled，+0x14 为本机 Player 借用。直接安装来源 mask，不重算或消费它。 |
| Typed effect wrappers VA0x90A150／0x90A1B0／0x90A220 | 记录 stride 为 0x10，配置指针、count、cooldown 是不同值；类别由本地配置确定，不能把末字 +0xC 猜作类型标签。typed wrappers 将末字置零；该字没有公开语义，安装器不传递未解释的地址或把它伪称来源字段。 |
| History Add VA0x71BFD0、Remove VA0x71C160、索引维护0x71C410／0x71C490 | +0xC 字节禁止这两种变更。Add／Remove 还会维护 Player 外部的16个 inventory-history indices，并通知全局 HUD；因此只重建最终 vector／当前禁用位，不重放这些方法。+0x10 未采用，16个索引的角色消费仍归 I27。 |
| REP LuaHistory.cpp／LuaHistoryItem.cpp／GetPlayerFormCounter | History 是独立完整列表；相同 ID 可以重复，不按集合去重。15 个 form 槽为0..14，15是计数 sentinel。获得时间按来源保留，包括负值；构建记录时不读本地时间补值。 |
| REP CostumeSpriteDesc.zhl／LuaCostumeSpriteDesc.cpp／Lua_PlayerGetCostumeLayerMap | 描述大小0x130，ANM内嵌于+0；元数据、15个map项及0-based costumeIndex／layerID独立保留。容器和资源借用在换代前撤销；不调用 AddCostume／RemoveCostume 重新裁决外观。 |
| Costume元素copy0x7D3F40／0x7D4640、析构0x7D3870；ANM ctor0x402B00／copy0x402CD0／dtor0x403790 | 元素复制先真实ANM copy ctor，再元数据；旧容器逐ANM析构后free。主审复读copy／析构调用，采用逐项新构造候选和本机资源加载，不复制原ANM字节或绕过其共享引用计数。 |
| headAnim +0x1320；ctor0x74DC70；773990内calls0x778E03／0x778E21／0x778E3F／0x778E5D；reader0x783680 | 子智能体最初未找到业务writer而保留UNKNOWN。主审独立反编译和逐指令复读发现明确的HeadLeft／Right／Up／Down字符串赋值，纠正该未知。正式来源仅在当前合法普通窗口复制已构造字符串的字节，hex避免猜UTF-8；Client用std::string赋值维护SSO／heap并真实回读，不复制24字节对象表示，不扩大其他私有draw字段读取资格。 |
| Weapon factory VA0x9CADB0，Init0x9C5200，Destroy helper0x9CADE0 | 工厂为 `__fastcall(type ECX, owner EDX)`，按动态注册表创建后调用 virtual Init；Init先Reset再赋owner/type。fresh ctor的空引用使这条取得路径不生成攻击Entity。Destroy helper的Clear→Free→清槽不是无副作用的资源释放。 |
| Weapon MainEntity getters0x9C4E70／80／90／A0 | 分别返回空、+0x44、+0x48、+0x4C；逐动态类型匹配，不统一写+8。+8是独立的item-animation EntityPtr。Knife／Bone／Urn还存在额外已核引用槽，全部参与解绑。 |
| ClearReferences0x9C5050／0x9D4240／0x9D7E10／0x9D6A00／0x9D0990／0x9E0D50 | 会对关联 Entity 执行 Remove；Bone还调用Entity清引用，Urn还停止声音22／23。主审否决将通用Destroy helper用于Client正常替换／退休。 |
| 各类 Free，base析构0x9C4FC0，EntityPtr析构0x5787D0 | 动态析构先注销自身EntityPtr再走base和游戏free；base会修改Player/Familiar owner。采用先EntityPtr.SetReference(nullptr)、owner=null，再真实virtual Free(1)，正常返回后清槽。这样Entity19保持实体唯一owner，不发生额外Remove、音效或owner表现写入。 |
| 全部 GetMaxCharge slot6 | 主审复读15类的具体getter，均只读当前max_delay及modifiers，按真实动态getter回读；没有可通用写入的独立max-charge字段，不补造另一份缓存。 |
| chargebar slot7／8，VA0x9C5E80／0x9C4E60 | 当前Player／HUD消费仅依赖charge、max_delay、modifiers及真实虚函数结果；没有Weapon独立Render或内嵌ANM。正式补充明确`public-base-only;gameplay-caches-authority-only`，不强读、复制或覆写未被Client消费的Fire／Update缓存。source两份bar值保留，安装后用真实动态getter回读，不用构造默认值冒充来源。 |
| 注册初始化0x401E80／0x401E90、factory对象0xBAE154／0xBAE160、ctor0x9CB380 | 主审拒绝初步按类名将type11映射Deathmatch的猜测。实际type10与11均经Bone工厂创建0x60字节的Weapon_Bone；type11仍保留Notched Axe的枚举与实例type，使用相同动态布局／析构，不能拒绝合法type11或建立错误vtable。 |

逐类型表及具名引用采用记录见源码；未知 padding、未采用的玩法缓存和原研究的 UNKNOWN 保留，不能由 SDK 声明大小推断所有动态类的真实分配大小。尤其base ctor0x9C4EB0写到+0x40，公开SDK的Weapon大小0x40不是分配依据，必须使用真实动态工厂。

## 寿命与后续接口

`ClientPlayerEquipmentOwner` 随稳定身体实例持有 costume 候选／被替换存储及五个 Weapon owner。所有游戏操作在原有主线程 Apply／Retire 窗口，要求当前 renderer context、CPU 队列退尽、Lua／callback 保护及相同实例。构造／析构进入与正常返回分开记录，异常或外部改槽保留实际 owner，不能重复Free或将半套状态提升为A。

全身体结构取得之后，按 I11—I15 最终量、I16—I17 容器／History、I18—I20 资源与武器、后续角色 adapter 的次序准备；跨域实例全体存在后绑定Weapon／身体边，再从真实容器、ANM和动态getter回读。只有Scene提交后才开放最终视图。

I21—I40 的角色、完整draw、HUD布局、全族退休／恢复接线和统一验收继续按主清单推进。缺少完整角色／draw adapter时依然在世界写前拒绝，`native_apply`／`native_source_complete`仍为false；本轮不提供空adapter绕过这一实际缺口。没有新增测试或tx artifact。
