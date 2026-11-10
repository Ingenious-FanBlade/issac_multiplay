# 5：Player 的 P6、P7、P8 设计

**后续实施（2026-11-09）**：正式Player源与生成已迁入版本47，见[实现说明](implementation-5-player.md)。本文件保留设计时点和证据；构建不读取设计JSON。Plus未知与ABI模型限制保持，实际采集／安装及最终显示缓存仍待后续步骤，没有提前测试。

**2026-11-09 后续补充**：[统一字节账本](player-byte-ledger.md)及[旧成员逐行回映射](player-member-map.md)已完成设计登记；本文末节所列这两项待办是原时点状态，以新登记为准。新记录还区分原版通信域、待移除效果队列、移动箱保存体和Plus ABI推导；正式布局／运行接线及第8步最终缓存仍待实施，布局版本仍46。

日期：2026-11-09（Asia/Singapore）。目标：官方 REPENTOGON 1.1.3／Repentance+ J273，ImageBase=`0x400000`。接续[P0—P2](5-player-p0-p2.md)和[P3—P5](5-player-p3-p5.md)。本文完成后三块的职责、原生证据、草稿编码和安装合同，**没有生成 Player 布局或修改运行代码，整数布局版本仍46**。九块的设计不等于整个 `0x2EA0` Player／PlayerPlus 已逐字节闭合，也不等于正式采集／应用已经接线。

## 1. 判据、资料及证据边界

同步实际展示消费者所需的当前值、资源和结构；结果已经由最终缓存承接时，不再复制全部生产历史。原生 getter 存在、旧 writer 写过、旧回读要求相等，都不是独立的同步理由。Host 为发现身体或判断生命周期而读取某个指针，也不意味着它要成为线上关系。

先读现行规范、本地原版 `EntityPlayer`／`TemporaryEffect`／`TemporaryEffects`，REP `CostumeSpriteDesc`／`History`／`HistoryItem`，再读官方 ZHL、`LuaCostumeSpriteDesc.cpp`、`LuaHistory*.cpp`、`LuaEntityPlayer.cpp` 及既有 I16—I30 安装／逆向记录。随后只读核对 IDA 的 J273 指令、反编译、调用与字段使用；VA 减 `400000` 为 RVA。没有游戏启动、构建、测试、校验脚本、哈希或 IDB 修改。

本轮在此前32个 Entity／Player／HUD／Scene 展示根上加入 `HistoryHUD::Render 82FA10`。跟随五层直接调用，遇五个通用库存查询入口停止，得到33根、611个函数；另读库存查询递归、效果 leaf、costume 生产器、History／HUD 生产器及具体虚调用合同。位移命中逐项检查基址：例如 `785419`、`7FC540` 的 `+118` 是 Entity 颜色尾值，**不是 CostumeSpriteDesc.priority**。此范围不声称覆盖全部程序、间接函数或第三方 Mod；第三方 Mod 的 Lua 消费适配已迁至[后续方案](../mod-adaptation.md)，不再归第8步，也不为它恢复整个旧查询字典。

原生指令是静态事实；下面的裁剪、规范空值、显示投影与 C/S 结构记录是设计决定。特别区分“没有展示读取”与“读取发生在不得放行的生产器中”。

## 2. 三块结论

| 块／拟定物理号 | 真正同步的内容 | 留 Host／本地或交其他 owner 的内容 |
| --- | --- | --- |
| P6／16，world | 有序 costume 目录的配置身份、isFlying、hasOverlay；15项最终层映射的 costume 选择／layer ID／isBodyLayer；绘制拓扑 revision | costume priority、itemAnimPlay、肤色生成参数／hasSkinAlt／overwriteColor／itemStateOnly／playerType、map priority 留 Host；ANM2 表现仍在 P0，地址／capacity／self 本地 |
| P7／17，world 结构 | PlayerType、PlayerIndex、真实 twin／backup／held 边；独立激活资格、roster／room／render／update 资格和 C/S 控制组／身体角色／活动／consumer 语义 | native sub／replaced、Esau Jr 全局四槽及恢复快照留 Host；main／other／sub／hologram 等旧 getter 结果不另发；ControllerIndex／设备／原版 Online 不上线 |
| P8／18，world | disabled＋完整有序效果配置／count；仅 collectible704 保留 cooldown；实际有序残影；可选绘制序列的**当前可绘节点＋实际 ANM2** | 原始 History 全记录／mutation-disabled／16个索引、15个 form counter、效果 mask／evaluate、未选中的录制／回放节点和游标推进历史留 Host；HistoryHUD／库存栏最终图像列表及分数归第8步 |

P6 下界 `111+4C` B；P7 每身体21B，另有 C/S 控制组记录；P8 下界 `6+6E+4S+12A+171D` B，其中 C=costume 数、E=效果数、S=collectible704记录数、A=残影数、D=当前绘制记录是否存在（0／1）。171只计当前记录24B及 ANM2 的147B编码下界，实际图层等可变内容另计。均不含第9步记录头、资源／成员表，也不是实测带宽。仍使用0—24这25个物理块，不增加第33块。

## 3. P6：最终 costume 目录与层映射

### 3.1 消费者及裁剪理由

| J273 VA／原生来源 | 静态事实与决定 |
| --- | --- |
| `7864A4—786595`，Player `+122C+16×i`、costume `+114/+11D` | 层1—3按最终map选择costume；CanFly且 costume.isFlying=false 时跳过；层1按配置类别／ID识别 null75—78 或 collectible625 并读取 Sprite 比例。配置身份和 isFlying 保留，不能只发资源文件名 |
| `783A32—783C19` | 29／38借画遍历15项map，`map+0C isBodyLayer`选择身体／头部比例与动画；非身体层另按 costume `+11E hasOverlay`选择 `_Overlay`。isBodyLayer 与 hasOverlay 保留；主／备各自目录不能合并 |
| `786302—7863C3`、`7867A0`、`7868DB`、`78693B` | Body／Head／Top／Glow 等从map的index／layer ID选择实际 ANM2 图层；没有重新按priority排序。最终map保留，priority不传 |
| `7FCD6F—7FCF68` → `757A10` | Room Light按15项map引用的costume配置作身份比较；IsItemCostumeVisible也只按最终map取配置。即便P0的Sprite完全相同，config身份仍不可删除；Null配置指针与type0的NullItem配置不同 |
| `7560C0`，例如 `756637/75670E/75672A/7568BF/756901/756E91` | priority、defaultSkinColor、skinColor、hasSkinAlt、overwriteColor、playerType用于生成最终层选择、肤色／贴图和资源。Client已安装最终map／ANM2，不运行此生产器；这些输入留Host |
| `77BD40` 的 `780CE7/780DD9/781019/781031` | itemAnimPlay／hasOverlay参与动画更新。hasOverlay另有上述Render直接消费者，保留；itemAnimPlay仅作生产状态，不因相邻而一起发 |
| `757B00` 的元素交换、`7D3F40/7D4640` 的复制及 `7D3870` 的析构 | 原生对完整0x130元素作正常ANM复制／交换／释放，不证明所有元数据都要同步。Client仍需要正常构造与析构，不能memcpy整个元素 |

`itemStateOnly`是costume生产／移除条件；它不等价于P0的当前ItemState。`defaultSkinColor`等也不是ANM2已产生的最终颜色。被裁字段不再参与变化比较、漂移检查或“结构相同”判断；否则虽然线上删除了它们，运行时仍会因其变化不断换装。

### 3.2 原生字节归属单行表

Player 中的两个容器区：

| `1220—122B` std::vector&lt;CostumeSpriteDesc&gt;【容器】有序元素；头／capacity／allocator本地，投影到P6／P0 | `122C—131B` PlayerCostumeMap[15]【嵌套】每项10h，见下表 |
|---|---|

每个 `CostumeSpriteDesc` 原生大小 `130h`：

| `000—113` ANM2【嵌套】按第2步逐字节归属，表现归P0；原有14B未知不变 | `114—117` ItemConfig_Item*【资源】可空(type,id)配置身份 | `118—11B` i32 priority【权威】最终map生产输入 | `11C` bool itemAnimPlay【权威】动画推进状态 | `11D` bool isFlying【状态】Render飞行层选择 | `11E` bool hasOverlay【状态】备份借画选择overlay | `11F` bool hasSkinAlt【权威】皮肤生成输入 | `120—123` i32 defaultSkinColor【权威】 | `124—127` i32 skinColor【权威】 | `128` bool overwriteColor【权威】 | `129` bool itemStateOnly【权威】 | `12A—12B` bytes[2]【未知】无填充证明 | `12C—12F` i32 playerType【权威】costume生成时的角色缓存，不代替身体PlayerType |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

元素独立未知2B，连同内嵌ANM2未知共16B；不把未传的已知生产字段改叫UNKNOWN。每个 `PlayerCostumeMap` 原生大小 `10h`：

| `0—3` i32 index【状态投影】非负为原有costume下标；负值只消费其符号 | `4—7` i32 layerID【状态投影】仅index非负时采用 | `8—B` i32 priority【权威】生成／覆盖竞争输入 | `C` bool isBodyLayer【状态】即使index负，备份路径仍读取此标志 | `D—F` bytes[3]【未知】不按对齐猜填充 |
|---|---|---|---|---|

### 3.3 草稿二进制布局

| `+00` u32 render_topology_revision | `+04` u16 costume_count=C | `+06` CostumeView[C]：原有顺序，每项4B | `+06+4C` LayerChoice[15]：每项7B |
|---|---|---|---|

| `+0` u16 config：真实配置资源号／FFFF=原生空指针 | `+2` u8 is_flying | `+3` u8 has_overlay |
|---|---|---|

| `+0` u16 costume：原生index≥0时为原下标，原生index&lt;0时为FFFF | `+2` i32 layer_id：有costume时为实际值，否则规范0 | `+6` u8 is_body_layer：两种情况都保留 |
|---|---|---|

`C≤16384`，且受真实字节／内存预算约束。map非空下标必须小于C；有实际图层时按该资源核定layer。Host明确无ANM资源不等于map必须为空：`405A50`在未加载时不画，`406B60`的越界路径会返回本地fallback LayerState；不能把旧安装器的“所有map必须有图层”当作原生不变量。无资源时保留真实非负map下标／layer／配置语义，按已核无绘制／fallback路径承接，不读不存在的图层数组；Client缺少一个Host已选择的非Null资源则仍须等待，不能走此例外。负index在已核消费范围只作符号分支，统一安装成-1；此时layerID不读取Host残留，安装0，但仍读取／安装isBodyLayer。没有依据把该标志也清零。不得把map index当成资源号，也不能按config ID排序或去重costume。

元数据直接读取 `+114` 的4B资源借用及 `+11D..11E` 的2B，不能吞入中间priority／itemAnimPlay。map读取index后条件读取layer，再独立读取body标志，不复制整个16B。config解析必须保持type0／1／2／3／4与ID身份，实际支持范围由资源合同决定，不把空指针当作NullItem0。

### 3.4 拓扑版本及安装

采用一个 **C/S会话内统一的Player绘制拓扑u32**，在全部Player的P6及P0重复携带相同值，初值非0、不回绕复用。它是窄结构依赖，不绑定整个world数值版本；普通位置／帧／颜色／充能或P3计数变化不升级它。以下变化共同触发新ordinary结构目标：

- costume有序保留投影、map选择／body标志、ANM2文件或图层结构改变；纯动画帧变化不触发。
- P7角色、PlayerIndex、twin／backup／held的目标use／shape、原生激活／列表资格及会改变绘制借用的结构改变。
- P1 CanFly改变；它与最终飞行层目录及P0共同提交。
- P3／P4／P5的选择改变实际绘制资源结构时，沿此前合同提交新拓扑；普通库存计数、充能或VarData变化本身不是无条件触发原因。

全体相关P6／P7、资源和同拓扑P0在ordinary事务一起提交，body只有与**已提交全体拓扑相等**才能安装；旧／未来拓扑body丢弃，不先按旧目录写入。共享revision也覆盖“备份目录变了但主身体目录未变”的跨身体借画依赖，不能只比较本身体vector地址／size。控制组仅binding代次变化而绘制结构未变时，走控制结构更新，不无故重建外观。

Host在PlayerPost仍按P0合同复制有界目录与资源结构，并检查当前P7的轻量原生元组（角色／索引／激活／实际引用目标use）及C/S拓扑见证。依赖身体的借用结构变化同样撤销旧快候选；稳定窗口复用已核拓扑，mutation通知缺失时必须以当前元组／依赖目录检查发现变化，不能只信旧缓存。检测到差异后等待ordinary发布；不在fast枚举库存、主动槽、Weapon或执行getter来补P7。

Client为候选costume逐项构造ANM，安装最终资源／P0投影和本节保留元数据，再一次交换真实有序vector和15项map。被裁元数据只保留本地正常构造／初始化的中性值，不用AddCostume／ShuffleCostumes／UpdateCostumes生成。旧vector只能在借用撤销、外层CPU队列退尽后逐ANM析构并用匹配allocator释放；构造／交换／析构异常前缀保留并停用相关owner，不盲重试。空C必须清旧目录和map选择；未就绪资源不等于Null，也不提前ACK。

## 4. P7：展示结构、原生资格与控制语义

### 4.1 关系逐项核定

| 来源／消费者 | 决定与依据 |
| --- | --- |
| `PlayerType +13BC` | 保留。Player Render、健康／主动HUD、充能条和角色分支直接读取；类型改变沿已核Player shape／结构替换，不按C/S ordinal猜角色 |
| `PlayerIndex +1610` | 保留完整i32。`74D9B0`取符号决定IsSubPlayer；`413140`、`7D1C90`和`758BA9`按双方索引选择main／hologram／表现。不是网络Key、ControllerIndex或参与者号 |
| `twin +1E4C` EntityPtr | 保留真实Nullable引用。`413160`返回它，`413140`按索引选择main；`758850`／`8378C0`／HUD直接消费。普通身体的GetMainTwin=self不应写成原生twin自环 |
| `backup +1E50` raw Player* | 保留。`7836F9—783DBE`真实借画另一身体；`769D6B`／`76A196`还按资格递归库存查询。与twin不是同一关系，也不按角色用途描述把其他真实backup清Null |
| `held +140C` raw Entity* | 保留。`784F6F—785733`多分支直接消费，特别是隐藏held父代画；目标仍是唯一真实Entity／Player成员，不要求Parent=holder，不新造副本 |
| `sub +1D84` raw Player* | **Host发现／模拟关系，不作为P7线上native边。** `6D86D0`是直读leaf，但没有直接code caller；本轮展示闭包中的实际读取在`9BAC90`分数重算，另有PlayerHUD Update `832C10`等生产路径。Client使用最终分数／HUD与C/S body_role／group，不运行这些生产器，故不恢复sub raw storage或单独GetSubPlayer结果 |
| `replaced +267C` raw Player* | **Host替换／恢复关系，不上线。** `995E10`维护两端，`7C2160`、`7A5660`等用于玩法恢复／生成条件，`926BA0`为保存链；本轮展示闭包没有直接消费。Client逻辑替换由Key/use/shape事务承担，不把旧实例重新装入此字段 |
| PlayerManager的Esau Jr四槽 `Game+1BB28..1BB37` | **Host未来Use／恢复存储，不发四个native槽，也不为槽内对象自动创建Client Player。** `7A3A40`给703 VarData写1-based槽；实际主动显示按VarData／配置绘制，`7896D0`最大充能没有703槽解析分支，本轮展示闭包也没有这四槽读取。P4仍保留VarData原值，独立对象以后真正进入展示成员才按自身生命周期接入 |
| `+20BC`可用位、`+20C0..267B` GameStatePlayer | Host恢复快照，不上线、不安装；“可恢复”不等于“正在被画” |

**这是对P3—P5文档P4中“703依赖P7备份槽”的进一步裁剪**，不是改变703 VarData的0／1—4事实。旧I26镜像四槽用于旧完整观测／回读，不再构成正式二进制下行要求。Host仍可为发现C/S身体、维护输入拓扑而读取sub／replaced／四槽；这与网络是否发送、Client是否写原生storage分开。

父Entity的Parent／Child等已核展示关系继续按自己的布局处理，不能把裁掉native sub扩大成删除Parent。17普通补体、35／40、19／20与29／38仍保留各自当前身份、原生实际展示成员及C/S控制组；不把本节裁剪理解为最多四个EntityPlayer或只支持一个形态。

### 4.2 Render内的生产器不能成为扩大发送理由

有界调用得到 `HUD::Render 97C300 → 966680 → 9BAC90`。后者沿sub／twin／backup汇总心量等，维护整个分数对象，并调用随机相关helper；`9679F0`重算 `Game+25C5C` 后，`9679FA`取 `Game+25C90` 的最终i32分数显示。

因此不能把“在Render下可达”一概视作可放行的纯展示。第8步必须从Host既有生产完成后的合法缓存时点复制**最终分数及实际需要的其他分项显示缓存**，在Client该用途分派中跳过重算、读取已安装结果。这里已核具体总分源`+25C90`，其他分数界面完整缓存仍由第8步逐项完成；不为了补一个数字发送整份sub图、永恒／骨心掩码、完整随机／ScoreSheet生产输入，也不在Host为采集额外调用重算器。若缓存时点不足，报具名来源缺口，不能假装ordinary任意时点等于某次Render重算结果。

### 4.3 原生激活资格必须独立保留

继承Entity `valid +170`不是普通热状态。`783707`却以当前身体valid决定29／38是否沿backup递归；`769D4C`也读取它。它不等于Exists、dead、roster、输入active或room成员。历史冷Client的type38反例 `exists=true / valid=false / 非roster / 无room-render-update成员`仍有效，不改写为新设计已验收。

P7保留独立的 `activation_qualified` 见证，与三个原生成员资格和roster位置同一结构事务采集／安装：

- room_member ⇒ activation_qualified；render_member或update_member ⇒ room_member。**不加入反向等价。** 非room且qualified=true仍是可表示的独立状态。
- qualified只指导本方独占、已正常构造／初始化的Player承载对象的角色激活。它不证明Init完成、分配成功、池占用或可接收输入。
- room身体经真实本地列表发布取得valid；非room身体由Player结构owner在不属于工厂可复用池、无在途借用的条件下设置对应本地激活状态。既有`client_players_native.cpp`的非room资格安装是窄先例。不得用这个字段写普通Entity的valid，或把尚未完成构造的指针标成可用。
- 资格／room owner变化沿现有安全结构替换；普通render／update资格变化由列表owner撤销／恢复借用。原生update列表资格只表达目标结构，C/S Client调度仍屏蔽玩法Update，不因入表而开始模拟。

双向twin／backup本身不是非法循环；只对**实际会递归调用的绘制／查询路径**检查终止条件。若当前Room分支、双方资格和backup组合会形成借画递归环，拒绝该目标或停止相关绘制并报具体原因，不能擅自清一端valid修图。held递归亦按真实调用条件与当前目标use检查；预算上界内不无限遍历。没有证明的角色配对约束不冒充原生不变量，例如不能要求Birthright的twin恒等于backup。

### 4.4 原生字节归属单行表

| 继承`170` bool valid【本地／结构】本地owner资格，P7携独立语义见证；E2不发原值 | `13BC—13BF` i32 PlayerType【状态／形态】 | `1408—140B` Entity_Effect* markedTarget【权威】攻击瞄准，目标实体自己的展示单独同步 | `140C—140F` Entity* held【引用】真实父代画借用 | `1454—1457` Entity_Laser* megaBlastLaser【权威】攻击控制借用，独立Laser由其owner展示 | `160C—160F` i32 ControllerIndex【本地】Client中性-1，不传设备键 | `1610—1613` i32 PlayerIndex【状态／结构】完整符号及相对顺序 | `1D84—1D87` Player* sub【权威】发现／模拟／显示缓存生产 | `1E4C—1E4F` EntityPtr twin【引用】节点注册本地 | `1E50—1E53` Player* backup【引用】独立于twin | `20BC` bool restore_usable【权威】 | `20BD—20BF` bytes[3]【未知】不并入恢复区 | `20C0—267B` GameStatePlayer【权威嵌套】恢复快照，不因整段不上线而宣称内部逐字节已核 | `267C—267F` Player* replaced【权威】替换恢复，不是Client实例关联 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

列表资格、roster位置和C/S组身份无Player内单一offset；从已知owner的有界列表／注册表取值，不把它们伪造为原生连续memcpy字段。

### 4.5 每身体草稿线布局：21B

| `+00` i32 player_type | `+04` i32 player_index | `+08` u16 twin：成员号／FFFF | `+0A` u16 backup：成员号／FFFF | `+0C` u16 held：成员号／FFFF | `+0E` u8 native_membership：bit0=activation_qualified，bit1=room，bit2=render，bit3=update，余位0 | `+0F` u16 roster_rank：目标有序roster位置／FFFF=不在roster | `+11` u16 control_group：C/S组成员号／FFFF=明确无分配 | `+13` u8 body_role：Primary=0／Twin=1／SubPlayer=2／Backup=3／Replaced=4／EsauJr=5／Inactive=6 | `+14` u8 control_flags：bit0=active，bit1=can_consume，余位0 |
|---|---|---|---|---|---|---|---|---|---|

native_membership是四个语义bit的投影，不是原生位域。控制角色枚举沿`topology.h`，它描述Host C/S如何管理身体，不要求恢复所有同名native指针。can_consume ⇒ active；无组时role=Inactive、control_flags=0。未知分配与明确无组不同，未知时报Gap。roster必须有已知组、rank唯一且覆盖完整稠密顺序；roster身体PlayerIndex仍遵守已核本地roster索引合同，非roster可有独立负值／其他实际索引，不能一律填-1。

两个Player引用须解析成当前Player成员，held按实际受支持动态Entity／Player类型解析；所有引用都绑定同一member_epoch下的Key/use/shape，不发地址。Null须显式编码，未解析引用不可改成Null。完整图需要的目标先在Host身份服务预约Pending，再由发布端分配成员号，不能因采集时未有号而丢关系。并非每个Host存储对象都因此成为展示成员：裁掉的sub／replaced／四槽不产生这类引用闭包。

### 4.6 C/S控制组记录，独立于原生Player内存

participant沿已有身份空间9，group沿10。成员表分别登记其纯C/S身份和注册布局；实际layout整数号待生成，不借用Player／Entity布局，不分配原生游戏对象。participant只有身份／连接上下文，无需为它造一份Player状态。每组world记录：

| `+00` u16 participant：非Null成员号，空间9 | `+02` u8 ordinal：0—3，session内分配 | `+03` u64 control_session：非0 | `+0B` u64 binding_revision：非0 | `+13` u16 anchor_count=N | `+15` u16[N] active_anchors：保留Host注册表中身体锚点顺序 |
|---|---|---|---|---|---|

每组长度 `21+2N` B。全部成员由P7.control_group反建，不再重复一份membership数组；active_anchors的明确顺序独立保留，必须唯一、同组、active，不能从roster或body_role猜顺序。最多四参与者、每组64身体、全局256身体沿现有预算，不缩成4身体；ordinal／participant／group不能跨代冲突。未分组的非roster存储身体可保留自己的实体身份／展示状态，不能借一组的输入资格。

本记录只发布已提交的控制视图。Client每参与者仍只发一条canonical意图，由Host scheduler统一分派，body列表不变成多条输入流；roster排序、换装或角色shape变化不自行制造新连接代次。真正binding变化撤销待发边沿并按已有中性规则恢复。ControllerIndex、device pointer、平台handle、原版Online本地性均不进入该记录。

### 4.7 安装与退休次序

先预检完整身体／组／roster／引用图与资源；创建本方承载对象、完成状态和资源候选；全体目标存在后统一绑定twin／backup／held，再安装完整roster及列表／激活角色，并统一发布。不能边写一个身体边调用递归getter检查另一个尚未绑定的身体。IsSubPlayer、GetMainTwin、GetOtherTwin、IsHologram可从已核原生输入派生，不再传独立布尔或结果引用；**GetSubPlayer不是IsSubPlayer**，其native边已裁，不向旧全量查询回读承诺相等。

EntityPtr通过正常本地注册／解绑；raw backup／held进入唯一out-link服务，目标替换或退休前撤销所有借用。P7不拥有对端对象，不沿twin／backup级联Free。旧sub／replaced正常保持本地初始化Null，四槽不安装；原生Free可能检查这些本地存储时也不应看到从Host嫁接的恢复图。只有本方正常初始化前缀才能按既有窄退休链清理，不调用InitTwin／Flip／SwapForgottenForm／TryHold／Throw／Use703／ReplacePlayer／Kill／Remove补行为。

## 5. P8：效果、绘制历史投影与原始History的去向

### 5.1 TemporaryEffects：完整身份／count，按实际用途裁cooldown

版本39／46的通用布局14仍是10B三值，136仍是disabled＋有序列表；本设计**不伪称它们已经变成下面的Player专用投影**。

| J273 VA | 本轮复读结果／决定 |
| --- | --- |
| `90A490/90A7A0/90A520/90A8C0/90A900` | 先查外层+11 disabled，再按配置类别与ID找第一个匹配项；collectible类别1／3／4、trinket2、NullEffect0分开。部分查询只看存在，count查询返回+4；不能按ID跨类别合并，也不能对重复项求和替换原顺序 |
| `835CDF → 90A900`、`7FCB91` | 主动HUD／Room Light有真实count消费者；Player通用库存查询还读118等效果count。保留全部当前有界有序配置／count，不从库存、costume或已绘图像反推 |
| `758924`找704，`758952/758957` | 角色27进度条直接读效果+8，并除以配置+78 cooldown；该cooldown确有展示用途 |
| `836E14—836E4F` | 主动HUD的704分支同样经90A7A0，`836E35`读效果+8，再除以配置+78。因此不能只在PlayerType27时才发送704 cooldown |
| 其余效果+8 | 本轮实际显示路径及通用查询只读身份／count，没有第二种已核cooldown显示消费者。作为Host到期／更新输入留Host，不因通用TemporaryEffect有此字段而全部发送 |

保留完整列表不是复制效果更新系统：只传当前查询共同使用的值，**所有组的到期管理、mask／evaluate、额外触发／costume／familiar生成都不传或执行**。disabled=true时仍保留当前完整投影，保持独立内容语义和后续重新启用的可靠基础，不把禁用列表当作空列表；禁用不授予额外读取资格。

有重复704记录时全部保持顺序与各自cooldown，原生仍取第一个匹配项；不替它去重。配置空指针会被这些原生leaf直接解引用，不能当作合法空效果安装：Host当前条目为空或配置身份未解决时报具名Gap，decoder拒绝Null配置；这与允许空costume配置不同。count及704 cooldown保持i32，不凭“计数／计时器”统一禁止负值或随意clamp。704配置的cooldown除数必须满足实际展示分支的非零要求，否则不能放行该分支。

### 5.2 TemporaryEffects原生单行归属与投影

Player `+1500` 外层大小18h：

| `00—03` u32 cache_flags【权威】待Evaluate mask，不是vptr | `04—0F` vector&lt;TemporaryEffect&gt;【容器】头本地，投影有序值 | `10` bool should_evaluate【权威】 | `11` bool disabled【状态】 | `12—13` bytes[2]【未知】 | `14—17` Player*【本地】承载Player回指 |
|---|---|---|---|---|---|

每条原生大小10h：

| `00—03` ItemConfig_Item*【资源】非空真实(type,id) | `04—07` i32 count【状态】 | `08—0B` i32 cooldown【条件状态】仅type1／3／4且id704；其他配置为【权威】 | `0C—0F` bytes[4]【未知】已知插入清零不证明填充 |
|---|---|---|---|

元素草稿投影：

| `+0` u16 config：非Null | `+2` i32 count | `+6` i32 cooldown：**仅资源身份为collectible704时存在** |
|---|---|---|

普通条目6B，704条目10B。长度由同resource_epoch已解析的配置身份决定，不由报文自报native偏移或任意flag决定；type0／id704不加尾部。实际view布局号随Player布局生成登记，不能用旧布局14的10B reader错位读取6B。外层disabled与列表在P8同一块提交，不再使用136两个body子块的调度标签；136的原归属证据继续作为历史通用基础。

Client构造正常本地vector，安装真实配置借用、count及704 cooldown；其他cooldown保持本地中性0，不再要求与Host相等。外层Player回指设本地承载对象，mask／evaluate保持本地中性且不进入任何更新器。新元素末字只按已核原生插入的本地清零行为初始化，不复制Host未知尾字，也不称该尾字已理解。空集合真实清旧项。禁止Add／RemoveEffect、ClearEffects、TriggerEffectRemoved、EvaluateItems及任何效果倒计时／到期维护。

### 5.3 原始History与16个索引不上P8

| 来源／消费者 | 结论 |
| --- | --- |
| 原始History `+1DD8`，元素1Ch | 时间、是否饰品、ID、stage／stage type／room／pool是获得／玩法／保存历史；不再按旧I17整表下行 |
| `HistoryHUD::Render 82FA10` | 读两个实际HistoryHUD_Player槽的owner和已生成0x28元素列表；绘制用元素+0的**visual_id**、+4的Image strong，以及全局表现时钟。没有读嵌套HistoryItem的时间／层／房／pool |
| `82FF40`／`8304B0` | 从原始History生产／去重／重排最终图像列表，并管理资源与两个HUD槽；还会处理29／38 backup选择。不在Client调用Recompute／GetPlayerIdx／RecomputeAll补列表 |
| `PlayerHUD::RenderInventory 839370` | 直接读PlayerHUD `+6A4/+6A8`的12B图像条目，不读Player的原始History或16个索引；仍依赖P3查询619决定容量等显示分支 |
| `PlayerHUD::Update 832C10`、`History::Add/Remove 71BFD0/71C160` | 用原始History和`+1DEC`的16索引生成／维护缓存，触发HUD／玩法；不是安装入口，不能为沿用旧writer而保留生产历史 |

第8步必须安装**真实最终缓存**：HistoryHUD当前两个原生槽的身体身份／次序、完整有序visual_id／实际Image资源／必要展示开关及布局；Tainted Isaac等的真实PlayerHUD库存图像列表、owner和布局。不能只传collectible ID后按当前库存排序，更不能用Host普通History等长列表代替这些已去重／变换／选择的记录。两个原生HistoryHUD槽不等于只支持两参与者；四参与者与多身体的实际HUD选择、布局和所需多个承载root仍由第8步完成，不在这里无声裁人。

`82FA10`对visual_id=633、有`88000`组合位和负ID分别选shader／彩色／负ID图像路径，因此要保留**实际完整i32 visual_id**，不能截成普通collectible ID。非Null的实际Image按资源owner同步；Null与负ID的绘制分支分别保持。嵌套HistoryItem在该Render不读取，不为它重发原始History。每图的strong计数／地址由本地资源owner维护。

原生归属单行表（不产生Player状态）：

| History `00—0B` vector&lt;HistoryItem&gt;【权威容器】完整获得历史留Host，头仍为本地表示 | `0C` bool mutation_disabled【权威】Add／Remove先检查，不是展示开关 | `0D—0F` bytes[3]【未知】 | `10—13` Player* owner【本地】`74E5E1`在Player+1DE8写自身；不是旧文档未采用的远端状态 |
|---|---|---|---|

| HistoryItem `00—03` i32 time【权威】 | `04` bool is_trinket【权威】 | `05—07` bytes[3]【未知】 | `08—0B` i32 item_id【权威】 | `0C—0F` i32 stage【权威】 | `10—13` i32 stage_type【权威】 | `14—17` i32 room_type【权威】 | `18—1B` i32 pool_type【权威】 |
|---|---|---|---|---|---|---|---|

| Player `1DEC—1E2B` i32 inventory_history_indices[16]【权威】生产图像列表／掉落索引，不发 | Player `17CC—1807` i32 player_forms[15]【权威】变身生产计数，不发 |
|---|---|

`71C410/71C490`通过History后的`+14`更新16索引，不能因这些函数越出History大小就把后续64B误算成History自身。form leaf `7A1CC0`检查`+2010`、CoopGhost与counter≥3；本轮展示闭包没有该leaf或对应计数直接读取，外观／属性已由最终ANM、costume及HUD缓存承接。无需再发15个counter或15个HasPlayerForm布尔；不调用IncrementPlayerFormCounter触发变身。

Client不安装HostHistory／禁用位／索引，也不向本地永久进度写获得史；其本地History维持正常构造的空容器和本地owner，16索引为初始化空值。旧`ClientPlayers::History`完整远端视图与Captured form／History字典按第14步退出正式消费；第三方界面Mod额外需要的历史视图已迁至[后续方案](../mod-adaptation.md)，届时单独声明schema和用途，不借此恢复整个原生History下行。合法Continue、Flip／恢复仍在Host执行，Client接收恢复后当前显示结果。

### 5.4 残影是真正的显示历史，要保留

Player `+1E38..1E4B`是实际deque控制结构，本地维护；外部 `AfterImageFrame` 原生0Ch全部有展示用途：

| `+0` i32 duration【状态】`785486—7854AD`直接转浮点／限制比例后乘alpha | `+4` f32 x【状态】 | `+8` f32 y【状态】`7854DF`起以该位置递归画当前身体 |
|---|---|---|

`78544C—78555C`遍历**全部当前元素，按原顺序**画每份残影。它不同于“未来回放队列”：不能因为名称有history／duration就整块删除，也不能只留最后一个位置或用Client旧P0样本自行生成。P8复制 `u16 count=A + A×12B`，保持duration的完整i32和有限位置；不在Client递减duration／裁到期，不从game_frame猜寿命。Host已有的队列内容是最终源。Client重建正常deque，空集合清旧项，native队列头／map容量／分块地址不上线。

### 5.5 可选绘制序列只传当前可绘节点

Player `+1C44`复合结构不是资源指针。`82E1C0`分配28h sentinel并构造`+0C` ANM2；`786048—786066`先查count及Game+25AB0，再调用`82E340`。后者只读取**当前cursor节点**，不遍历其他节点、不推进cursor：cursor==head或first_animation<0时不画；有效时按位置和两对animation／frame画当前Sprite。

因此旧I27“发送完整有序节点＋cursor下标”的合同裁为**当前显示投影**。未来／过往未选节点、head／count／cursor原值不传；第8步仍安装真实Game+25AB0的已核展示条件，不能在Client修改该条件强制开启。Host只在合法字段资格下按已核list owner解析cursor；验证其属于有界有效链可复用稳定owner／cursor见证，变化时重新有界遍历确认，不为每次绘制重复全图扫描。

源cursor=head或first_animation<0时编码D=0，不读取无效节点的position／frame／Sprite负载。D=1时保留当前位置、first_animation／frame；second_animation<0时规范为-1且second_frame=0，不读取其残留frame。首动画／第二动画的有效索引和帧须对应实际本地资源；缺资源不伪装D=0。第二动画存在但资源不能解释时拒绝目标，不画第一层后部分ACK。

原生单行归属：

| Player `1C44—1C47` list head【本地】 | `1C48—1C4B` u32 count【本地派生】Client投影列表为0或1 | `1C4C—1C4F` cursor*【本地派生】head或唯一节点 | `1C50—1D63` ANM2【嵌套】仅D=1同步当前Sprite表现；逐字节归属复用第2步，14B未知不变 | `1D64—1D67` 4B【未知】本轮不根据初始化零值命名推进语义 |
|---|---|---|---|---|

外部节点28h：

| `00—03` next*【本地】 | `04—07` prev*【本地】 | `08—0F` Vector position【条件状态】仅当前可绘节点 | `10—17` 8B【未知】原数据+8/+C未核语义，不读取／同步 | `18—1B` i32 animation【条件状态投影】当前节点首动画；负值折成D=0 | `1C—1F` i32 frame【条件状态】 | `20—23` i32 second_animation【条件状态投影】负值规范-1 | `24—27` i32 second_frame【条件状态】第二动画非负时采用 |
|---|---|---|---|---|---|---|---|

未知字节不因为已证明Render／析构不读就改成padding；本地候选node的这两个word按投影owner中性初始化为0，明确禁止Update／Save／Replay消费它们。`82E740`析构只沿next释放节点／head，资源由正常ANM owner释放。Client建立正常0／1元素list及真实cursor，D=0清旧列表并归head；D=1构造唯一节点及Sprite后再交换。**不重建完整原始序列，不推进到“下一个节点”。** NativeRender的SetAnimation／SetFrame及临时ANM更改按现有整帧强借用保存，CPU队列退尽后恢复。

这份额外Sprite随P8当前节点走world，**不偷偷并入P0的固定13份**，也不占用19—24的costume目录。节点、Sprite和普通目标原子提交；P0身体较新时，该辅助序列仍展示最近一次已提交P8采样，与其他world块同调度。以后若需要独立更高频率，须修改明确的块／布局合同并核算预算，不能把同一个Sprite同时交两个owner写。

### 5.6 P8草稿整体布局

| `+00` u8 effects_disabled | `+01` u16 effect_count=E | `+03` EffectView[E]：每条6B或10B，见5.2 | 随后 u16 afterimage_count=A | 随后 AfterImageFrame[A]：每条12B | 随后 u8 has_draw_record=D | 若D=1：CurrentDrawRecord，见下表；D=0到此结束 |
|---|---|---|---|---|---|---|

| `+00` f32 x | `+04` f32 y | `+08` i32 animation：≥0 | `+0C` i32 frame | `+10` i32 second_animation：-1或有效索引 | `+14` i32 second_frame：second=-1时必须0 | `+18` ANM2完整状态值：按ANM2三类投影顺序紧排，仅一份，不带原生地址 |
|---|---|---|---|---|---|---|

E、A各≤16384，不能只按计数上限忽略world32MiB／自有缓冲／整体内存预算；每条效果长度由真实配置识别，ANM2可变内容由已注册布局严格消费。所有bool仅0／1；D=0不带旧负载；decoder恰好用完P8长度，不接受尾部垃圾或把原始History伪装成第二个列表。效果／残影／当前记录都由同一身体、use和ordinary窗口产生，不借其他身体的字段资格。

## 6. 第8—15步接线与变化合同

| 步骤 | 必须落实的合同 |
| --- | --- |
| 8 Run／Presentation | 安装实际HistoryHUD与PlayerHUD库存图像列表、身体选择／布局／资源、最终分数与分项显示缓存；分数重算在Render内同样要隔离。Player的两Image strong `+1FF8/+2000`、palette最终色与动态HUD `+20A8`的具体二进制owner继续本步完成，不能恢复Host controller键或完整HUD Update补值 |
| 9／12 格式与decode | P6／P7／P8为world物理16／17／18；body不得夹带。新增纯C/S group记录及participant身份，真正layout号待生成；成员／资源引用、21B P7、条件6／10B effect及D分支恰好消费。整数版本须在真正迁入唯一源并生成时提高，当前46不接受草稿 |
| 10 Host capture | 主线程直接复制保留字段到预分配自有缓冲，资源／引用交身份服务；无Lua／JSON／getter／分数重算／动画更新，无热路径分配。Host发现图仍可读sub／replaced等，但不把它们塞回状态块；History／完整回放不读作正式下行。被裁metadata／effect cooldown不参与memcmp |
| 11 publish | 拓扑变化统一ordinary事务及P6 revision；元数据原位改变不能仅比地址／size。P8每块比较规范投影：非704 cooldown、未选记录、History／索引／forms或costume生产参数变化不造成该块变化。残影duration和704 cooldown真的变化时仍会重发，不承诺静止身体完全无包 |
| 13 apply | 先建全体owner／资源／组，再绑定保留引用与列表资格，共同提交。普通热路径不写valid；P7结构owner独占激活安装。被裁字段不作Host原值漂移校正；P8构造效果vector、残影deque与0／1记录list，禁止重放Update／AddRemove／恢复动作 |
| 14 retire | 退役旧完整costume元数据比较、sub／replaced／四备份槽写入与getter全量一致要求、原始History／索引／form字典、非704 cooldown读回、全节点回放源。最终HistoryHUD／库存栏／分数新owner就绪后才能真正切掉旧消费，不能先删来源留下空HUD |
| 15 sole E2E | 全目标完成后统一验证下列实际场景及原性能预算；本轮没有提前执行 |

## 7. 失败方式与统一验收关注点

1. 把costume priority／肤色更新读集当作Render读集，重复传生产历史；或反过来丢config、isFlying、hasOverlay和map body标志，破坏Room Light／飞行／29与38借画。
2. negative map清掉isBodyLayer，主备比例换算出错；把图层ID当作costume下标或按config去重，body快帧写入错对象。
3. 主身体目录没变就接受新快源，实际上备份目录／held目标use已换；拓扑revision不同仍ACK。
4. 以Exists／roster／room／active推导valid；双向backup一律判非法，或为消除递归强改资格。验证非room／valid=false但Exists=true及合法非room独立资格。
5. 把被裁sub边与C/S SubPlayer角色混同，丢弃原本仍需展示／控制的补体；或用BodyRole反向伪造native关系。验证16／17、35／40、19／20、29／38、703变换后的实际成员及四参与者多身体。
6. ControllerIndex上线或用于Client本地性；形态变化错误推进binding；组成员各发一条输入流，重复消费边沿。
7. 声称所有Render都是纯展示，放行966680中的9BAC90，重新随机／重算分数；为了凑缓存恢复整份sub／心量生产存储。核对最终分数来源时点及零持久副作用。
8. 用counts／原始History直接重建HistoryHUD／库存栏，丢排序、选择、实际Image、633／负ID／黄金shader分支；或切掉原始History后没有安装最终缓存。
9. 效果跨类别合并，重复项改变first-match；把disabled等于空；704在非27角色的主动HUD漏cooldown；其他cooldown仍造成网络变化／被本地递减。
10. 将残影当作可删历史或由Client猜轨迹；只传duration不传位置。确认全部当前元素、原顺序和alpha读集。
11. 可选序列发送完整未来队列、在Client推进cursor，或D=0仍读取空／失效节点；第二动画负时发送旧frame。确认原生0／1节点投影及ANM资源／strong／CPU借用恢复。
12. 正常候选析构与异常前缀混用；对端被换use后raw held／backup仍借旧指针，或沿互惠关系重复Free。依既有单次整体验收连续修复／续测，不另增测试轮次。

## 8. 完成边界与保留工作

本轮P6—P8的必要性、裁剪、原生范围、草稿单行表、跨块拓扑、C/S控制记录、缓存交接与生命周期合同已完成；P0—P8九块均有设计。原始事实与旧I16—I30／版本39生成布局仍按当时事实保留，本设计不回写成旧测试已经覆盖。

仍需第5步后续完成：从继承0到Player `2EA0` 的统一字节账本／剩余角色与保存区嵌套归属、PlayerPlus其余字段、全部ReplicatedState逐成员回映射、唯一`state/player.json`及新投影布局生成。本文各分表只覆盖审查范围，不能相加冒充整个对象未知总数；ANM2、costume、map、效果、History、回放节点及Player间隙的UNKNOWN明确保留。第8步的最终缓存／Image／palette／动态HUD及正式第10／13／14步接线仍未完成，暂不能宣称全Player画面或同步可运行。没有改动运行代码、生成文件／布局版本，没有构建或运行任何测试。
