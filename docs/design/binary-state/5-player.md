# 5：EntityPlayer

**2026-11-09 当前版本39：Player装备与Plus展示依赖。** [完整消费者与安装复核](player-equipment-plus-synchronization-review.md)将ActiveItemDesc来源池／定时充能冷却、Weapon五项非展示输入及KnifePlus.hitboxSource留Host；保留主动槽六值、Weapon充能三值与类型、TemporaryEffect三值及Plus公共nullable覆盖。新增135号UmbilicalWhip真实主引用、136号TemporaryEffects外层disabled／列表，并补Familiar+D76绘制抑制字节。原已排除Plus项不恢复，其他UNKNOWN保持。源、生成表、正式Plus读取接口和第5／8／10—15步已修订，未构建或测试，运行时仍待接线。

状态：Player本体布局与运行接线仍未实施；版本39相关装备基础、必要动态主引用和效果外层已入生成链。上级：[总计划](README.md)。

**版本33 Familiar跨域展示**：[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)核定Incubus只需最终充能amount，不在Client创建其玩法Weapon；第5步Player Weapon目录不能被当成Familiar接管完成。Gello绳索另保留Player／反射／隐藏依赖及typed payload，与本步身体结构／武器owner分别接线。

## 目标

完成 `Entity_Player` 子类区（0x410—0x2E9F）的字节归属。划分逐身体快源块（body 序列，约 60 Hz）和普通块（world 序列，约 30 Hz），覆盖内嵌 Sprite、容器、装备、关系和查询类状态。

## 输入事实

### 对象大小与 SDK 声明

`EntityPlayer.zhl` 声明对象大小 0x2EA0，共 129 个字段。子类区约 11.5 KB，是所有类型中最大的。

### 已核位置（`world_layout.json` 常量）

| 内容 | 位置 |
| --- | --- |
| 内嵌 Sprite | 自 0x41C 起，步长 0x114，共 13 份（约 3.5 KB） |
| playerType | 0x13BC |
| index | 0x1610 |
| 主动道具描述 | 自 0x1574 起，步长 0x20，VarData 位于各描述内 +0x1C |
| 子身体存储 | 0x1D84 |
| 双生存储 | 0x1E4C |
| 备份存储 | 0x1E50 |
| 替换存储 | 0x267C |

### SDK 中的容器与关键字段

| 字段 | 位置 |
| --- | --- |
| `_costumeSpriteDescs`（vector） | 0x1220 |
| `_weapon[5]`（Weapon 指针） | 0x13D8 |
| `_cacheFlags` | 0x1568 |
| `_controllerIndex` | 0x160C |
| `_trinketsID[2]` | 0x16B0 |
| `_collectibleCounts`（vector<int>） | 0x16B8 |
| `_history` | 0x1DD8 |

### 公开成员现状

- **应用目录**：`player-application.json` 共 1265 个成员。其中 556 个是 I31 加入的只读查询（captured-readonly-query），523 个是 Host 权威操作。
- **Entity 域中的 Player 成员**：EntityPlayer 有 295 个 `ReplicatedState` 成员。
- **只读查询的消费者**：195 个成员路由到 `ClientPlayerDisplayView::Captured`，在 `mod/src` 中找不到调用方。

### 现有逐身体快源

Lua `CapturePlayerFast` 每个身体每次读取约 45.8 ms，第 73 轮 557 次读取全部失败，失败原因没有被记录（静态推断见 `capture-bottleneck` 记录）。逐身体证言、exact-pair 绑定和规范第 11 节的读取规则继续有效。

## 设计

### 5.1 字节归属

- **内嵌 Sprite**：13 份内嵌 Sprite 各按 ANM2 布局处理（第 2 步）。版本32起由各自的可空ANM2文件资源号表达有无资源，`loaded` 原字节不上线，由Client资源owner维护；加载等待／失败不能当成Null或提前ACK。实际Image与路径分开，is_playing／playback_speed及完整最终颜色保留，见[动画／资源复核](animation-resource-synchronization-review.md)。
- **容器**：`_collectibleCounts`、`_costumeSpriteDescs`、History 等按变长块处理。
  - `GetCollectibleNum` 等查询的结果，原则上应由库存、效果等原生状态在 Client 上重新得到。
  - 若某个查询依赖的状态无法全部同步（例如依赖 Host 权威计算），该查询归为“换算”，Host 读取后作为字段发送。
  - 这是本步的核心核对项，旧目录中 3605 行的参数查询矩阵借此退役。
- **Weapon五槽**：先按真实动态vtable／工厂核类，不发送地址；版本39只同步MaxFireDelay／Charge／Modifiers／WeaponType，基础布局13为0x44原生前缀、线上16B，不能按SDK0x40或此前缀复制整个动态对象。type15的UmbilicalWhip由135号0x48实际对象布局另保留+44主实体引用；五槽存在性／顺序和type10／11共用Bone vtable仍须明确。
- **身体关系**：子身体、双生、备份、替换存储归“引用”，转成成员序号；备份槽的语义沿用 tx-04-20 的四槽规则。
- **权威状态**：RNG、Host 专属的玩法状态归权威。

**版本31公共部分与结构约束**：P0不携带Host暂停保存坐标、原生半帧历史位置或击退方向；网络展示插值如需要，由已应用world／body快照建立本地历史，不放行原生Interpolate。公共20个计时器和毒／火伤害相位不上线，Player自身`+13B8`伤害闪烁等展示计时保留。版本31将valid从公共E2移除，但角色29／38的RenderHead及递归查询仍需要正确活动／备用资格：P7必须接管room／render／update成员、身体角色和原生激活资格；现有`native_membership.valid`只证明room_member⇒valid，不能删去独立资格或一律设true。资格变化走结构事务，不覆盖本地分配／初始化完成事实；具体编码／writer尚待本步及第9／13步实现。见[生命周期再复核](entity-lifecycle-synchronization-review.md)。

### 5.2 状态块（初步划分，本步定稿）

| 块 | 序列 | 内容 |
| --- | --- | --- |
| P0 身体快源 | body | Entity E1／E4／E5 中的 Player 部分，加上头部与身体的动画和朝向等逐帧值 |
| P1 属性 | world | 伤害、射速、射程、移速、幸运、飞行等最终值，以及 cacheFlags |
| P2 生命与资源 | world | 各类心、钥匙、炸弹、硬币等 |
| P3 库存 | world | collectibleCounts、饰品、卡牌和药丸槽 |
| P4 主动道具 | world | 四个ActiveItemDesc六值，各24B；来源池／定时充能冷却留Host |
| P5 武器 | world | 五槽存在性／次序／动态类型、MaxFireDelay／Charge／Modifiers／WeaponType及15号真实主引用；不传其他玩法关系／输入 |
| P6 服装与 Sprite | world | costume 列表和 13 份内嵌 Sprite |
| P7 关系与身体结构 | world | 子身体、双生、备份、替换、控制组及room／render／update成员、活动／存储角色和原生激活资格；不在公共E2重复发送valid |
| P8 临时效果与 History | world | 布局136的disabled及完整有序TemporaryEffect列表（布局14三值10B），mask／evaluate留Host、Player回指本地；History按本步独立核对 |

body 序列只包含 P0。没有合格 PlayerPost 的身体，P0 改由普通窗口采集，沿用现有“ordinary body-joint”语义，见第 11 步。

## 改动范围

- 布局源 `state/player.json`。
- Client 端的 `ClientPlayerFastValues`、`ClientPlayerDescription` 以及状态、装备子结构改由影子状态生成（第 13 步）。
- 现有原生写入保留，包括 I11—I27 已核的最终量、资源、角色和备份路径。

## 失败方式

- **查询派生判断错误**：把依赖 Host 权威计算的查询当成可在 Client 派生，Client 读到不同结果。
- **未加载的 Sprite**：内嵌 Sprite 未加载时，仍按已加载布局复制字段。
- **快源依赖错误**：body 序列的 P0 依赖了旧的成员表版本，身体关系与实体身份错配。
- **按指针判断武器变化**：Weapon 指针在两次采集之间被替换，Host 只比较地址，没发现内容变化。
- **快源静默失败**：沿袭第 73 轮的问题，PlayerFast 失败时不保留原因。新路径必须记录具名首因。

## 完成内容

- **字节归属与表格**：Player 子类区的字节归属全部完成，带证据；P0—P8 块定稿，两张单行表由生成器产出。
- **回映射**：Player 的全部 `ReplicatedState` 成员有回映射，每个查询类成员都标明是派生、换算还是权威。

## 未决问题

- 556 个只读查询里，哪些必须换算发送、哪些可以在 Client 派生，需要逐项核对。
- 195 个路由到 `ClientPlayerDisplayView::Captured` 的成员没有调用方，需要确认它们是否仍需同步。

### 5.3 版本39装备展示与安装合同

[完整消费者与依据](player-equipment-plus-synchronization-review.md)覆盖Player、PlayerHUD及Familiar跨域：四主动槽Item／Charge／BatteryCharge／SubCharge／PartialCharge／VarData不能由HUD缓存替代；装备充能条由已核动态slot7→slot6及四个保留值重建，不重复发送MaxCharge／两份chargebar；全部已核slot8为常零，仍保持真实Disappear动画。基础类型的body序列标记在嵌入P4／P5／P8时由这三个world块承载，不把装备枚举加入P0快源。

UmbilicalWhip主实体先构造再用EntityPtr绑定，Familiar还需同版本Player关系、PlayerType32／619库存、IsFollower及新D76抑制字节；不能把这些条件当作AI而删除。TemporaryEffects的disabled与完整列表同目标版本安装，配置保留type/id及角色704环使用的config.cooldown，不能用最终costume或库存替代记录。所有效果计时只装已应用帧原值，不更新／到期。

HUD图像、bookImage、chargebar资源与flash／布局缓存由第8步具体owner准备，不运行完整PlayerHUD Update、Add/Use/Charge操作或EvaluateItems。官方1072／1073等回调的Mod有效结果由声明adapter承接，Client不执行Host玩法Lua补值。五个Weapon经已核factory／Init建立本地对象；只写保留字段及15号主引用，其他原生引用保持正常空owner。替换时先正常撤边、owner=null，再窄Free；不调用会Remove独立实体或停音的ClearReferences／DestroyWeapon。

公共Plus飞行／水面覆盖继续由原生owner赋optional，玩家专属Plus当前尚未入完整二进制表。角色、库存、资源、Room、效果、Weapon与主体需要同目标版本和合法生命周期；完整Player／HUD接线及全部未知仍按原步骤完成，不能以本次布局生成称为已可玩。
