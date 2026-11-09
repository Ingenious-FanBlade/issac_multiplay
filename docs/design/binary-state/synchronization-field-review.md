# 第 3／4 步：原生字段同步职责复核

> 版本37后续复核：[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)只保留optimized／unoptimized最终点列和真实展示标量；十九项Laser输入、六项Homing标量、indices中间点缓存与Bezier／系数留Host，撤销两个Laser计时锚点。J273 Homing+34实际为8B双f32点对vector，本文旧i32／4B步长记录错误；原记录保留，不按旧类型继续传输。L2／L4／L7／L8预留，当前以版本37唯一源及生成表为准；未构建或测试，旧运行执行器仍待后续退役。

> 版本36后续复核：[Projectile运动与展示参数复核](projectile-synchronization-review.md)将Projectile追踪／曲线／加速／下落／摆动及Scale／ProjectileFlags共八项留Host，保留Render直接消费的Height；公共EntityFlags、最终位置／偏移／尺寸和Sprite保持。私有+440及+460／+464的展示来源仍UNKNOWN，未构建或测试。下文旧传输与原值比较约定保留原时点，现行以版本36源及生成表为准，运行时仍待后续步骤接线。


> 版本35后续复核：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait和未来循环列表，以及Slot八项参数留Host。Price／DropDelay、最终主／辅助Sprite与真实引用保留；Slot奖品+460／+470精确对应当前动画指针／is_playing。下文旧传输／原值比较约定保留原时点，现行以版本35唯一源及生成表为准；未构建或测试。


> 版本34后续复核：[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)将Knife10／Bomb11／Tear21个攻击、运动与计时参数范围留Host；撤销三个计时锚点，保留真实绘制输入、最终Sprite和Bomb pending清除。下文旧的参数传输／计时见证要求保留原时点，现行传输以版本34唯一源及生成表为准；未构建或测试，运行接线仍待后续步骤。


> 版本33后续复核：[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)排除Familiar11个参数范围；80补入最终充能amount，240补入typed绳索Sprite／节点布局。下文“尚待建模／补字段”是版本28／29时点；现在仍待第10／13步具名采集、展示及owner接线，不代表运行时完成。

> 当前版本31：[生命周期再复核](entity-lifecycle-synchronization-review.md)移出公共valid，保留Player独立激活结构资格；exists／dead继续同步。下文版本30保留valid的传输判断由本次修订取代。

> 版本30：[Entity公共同步必要性复核](entity-common-synchronization-review.md)撤销33个公共范围（含20个倒计时、毒／火相位、暂停／半帧历史位置和部分伤害／物理输入）的传输；valid／exists／dead及真实展示参数保留。下文Entity候选的“未闭合”是版本28时点，已由该复核取代。

> 版本29补充：版本29的[24项内部关系逐槽复核](internal-entity-relations-review.md)将23项留Host，Familiar+DAC仅variant240布局130保留；并证实Laser未优化采样点用于Room Light。下文保留版本28的17项调整和当时研究范围，内部关系的旧传输约定以29修订为准。

日期：2026-11-09（Asia/Singapore）。本节初次复核布局版本 **28**（当前版本31见上方补充）。本次按用户要求调整已明确不需要同步的字段，并继续静态核对展示依赖；没有运行测试、游戏、注入或部署。不改变已有历史结论及失败记录，版本 27 的 K4／命中集合传输设计由本文修订。

## 判断规则

原生字节归属完整，不等于把全部原生状态发给 Client。字段类型／用途已核、公开 getter 存在、旧应用目录曾同步、或者不能从其他值推导，都不能单独证明同步必要性。正式状态应服务于 Client 的结构构造、原生 Render／Shadow／PostRender、展示维护及 HUD；Host 仍是唯一玩法模拟器。

- **权威**：Host 的伤害、命中去重、掉落、分裂、未来运动切换等玩法输入／历史。保留其原生范围和证据，不采集或安装；最终成员、位置、效果及姿态照常同步。
- **本地**：原生地址、函数地址、虚表、容器头、共享控制块、反向引用树及本地缓存。已有本地归属保持，不为了减少线上字段而把玩法值误称本地。
- **状态／换算／引用／资源／容器**：实际展示或结构消费者所需内容，沿用具名转换及 owner；不能浅拷贝地址或重演玩法。
- **未闭合候选**：继续原归属并明确待查依赖；没有直接位移命中不是全程序无消费者的证明。UNKNOWN 不借此次调整改成填充或权威。

公开成员回映射是职责表。改归权威的公开 getter 保留行，改为 `disposition=authority`，不再要求 Client 原生值与 Host 命中／伤害历史相等；E2E 只读观测仍可按原职责保留。任意第三方玩法／绘制 Lua 回调读取任意私有字段，不在当前支持声明内，不据此恢复全对象复制。

## 已调整的 17 个范围

以下全部归 **权威**；原生大小、偏移、类型、未知字节和字段证据等级不变。特别是 `share_parent_hit_list` 的完整业务语义仍为“推断”，不因同步职责明确升级为“已核”。

| 类型／范围 | 字段 | 本次处理与理由 |
|---|---|---|
| Knife `+504—+51B`，六个 f32 | 父刀距离、相对角度、局部偏移 X／Y、分裂触发距离、碰撞方向角 | 撤销 K4 的 24B 传输及有限值校验；Host Update／分裂／HandleCollision 消费，Client 接收最终姿态 |
| Knife `+4D8`，12B vector 头及其元素 | `knife_hit_indices` | 不再复制 Host 命中历史，Client 自有 vector owner 保持本地安全状态 |
| Knife `+4E7`，u8 | `share_parent_hit_list` | 只控制命中 getter 的父刀历史选择，不控制已核绘制父链；随命中历史一起排除 |
| Bomb `+9C8`，12B vector 头及其元素 | `bomb_hit_list` | 同上，爆炸／碰撞裁决只在 Host |
| Tear `+444`，12B vector 头及其元素 | `tear_hit_indices` | 同上，不把 Host 命中索引当 Client 身份 |
| Laser `+520`，12B vector 头及其元素 | `laser_hit_indices` | 同上，不执行 Client 命中去重 |
| Projectile `+41C`，f32 | `projectile_damage` | 独立弹幕伤害裁决输入；不能因为它不同于公共 CollisionDamage 就要求发送 |
| Projectile `+450/+458/+45C`，u64/f32/i32 | `change_flags`、`change_velocity`、`change_timeout` | 未来玩法切换参数；切换发生后按最终效果／运动块同步，不在 Client 再触发切换 |
| Laser `+4F4/+51C`，两个 f32 | `black_hp_drop_chance`、`damage_multiplier` | 掉落概率与伤害倍率只供 Host 裁决；不触发 Client 伤害或掉落 |

同步删除这些字段的 `wire`、专用 `read_requires` 和线上 validator，并从各状态块移除。Knife K2／K4、Bomb 命中块、Tear 命中块、Laser L9 保留原块号为 `reserved=true` 的空块，后续块号不前移。不把空块声明理解为要持续发送零长度负载；第 9／11 步按实际状态块和变化规则处理。线上最小负载对各取一只 Knife／Bomb／Tear／Laser／Projectile 合计减少 61B，另省去四个命中集合的变长元素；这是布局算术，不是实测性能结果。

## 静态消费者依据与边界

目标为 J273 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`。地址均为 VA。只读反编译／指令及源码检查，没有修改 IDB。

| 路径 | 本次实际核对 |
|---|---|
| Knife Render `93ECD0` | 主体／拖影读取公共姿态、`+410` 效果位、`+43C` 旋转偏移、`+444/+448` 距离／速度和 `+464` 状态；不是尾部 `+504—+518`。Render 调 `939A80` 选 Variant 类别，`93F5E0` 找 Player，`93F650` 沿 Parent 找根刀；后者不读取 `+4E7`，必须与命中 getter `93A2F0` 区分 |
| Knife 尾部 producer／consumer | 用途由[版本 27 证据](step4-first-four-evidence.md)中的 Init、两个分裂 helper、Update 和 `93F680` HandleCollision 确认；`+518` 是碰撞方向角，不再采用被撤回的绘制角解释 |
| Bomb Render `6A0F90` | 只在 `+462` 开关下绘制最终五份 costume；`+463` 可触发 `6A2420` 懒重建，Client 仍须清除它。该路径不需要命中 vector；不通过爆炸或重建资源恢复画面 |
| Tear Render `675AC0` | 消费效果位、DeadEye 强度、辅助 Sprite 和生成者相关展示；不消费 `+444` 命中历史 |
| Projectile Render `659460` | 读取高度、公共标志、私有展示参数 `+440/+460/+464` 与最终 Sprite，不读取 Damage 或 Change 三字段；公共 Shadow `6AD450`、PostRender `6AC660` 亦不执行延迟变化／伤害。私有展示参数仍有 UNKNOWN，不据此声称类型完整 |
| Laser Render／Shadow／PostRender `9514B0/955290/956F80` | 几何路径实际读取 optimized samples `+480/+484`；另有角度、效果位、采样模式、长度等输入。PostRender 还读取 `+4D0 MaxDistance`、`+516 GridHit`，不能把它们与掉落概率／伤害倍率一起删去。检查了 helper `945D60`（Variant 分类）、`6B32F0`（Spawner 链）、`958880/9588D0`（局部点容器维护）及图像 batch `9E21D0→9E3220`，不把图像维度虚调用误作 Laser 伤害裁决 |
| 公共展示 | `6AC4E0`、`6AD450`、`6AC660` 使用最终位置、Sprite、阴影和调试几何；不存在以被删命中历史重演碰撞的必要性。WaterClip `6B2F70` 仍有 HP、价格、分类／房间等输入，因此不批量删除公共玩法外观相关值 |
| Lua／REP 外层 | `8480D0/8486E0/849210/847510` 是 Lua render 回调桥；`847DF0/848CF0` 经 `859A10/859E30` 进入相同回调机制。官方 `CustomCallbacks.cpp` 的 Render wrapper 负责回调和 super，不据此产生命中历史的原版展示依赖；官方命中 getter／Contains／Add／Remove 的 Lua 绑定仍是玩法／观测 API |

这些结论适用于当前唯一 Authority、Client 不运行玩法 Update 的设计，不是对任意第三方代码或任意地址别名的全程序形式化证明。旧运行中的 Lua／JSON 路径本轮不提前退役，第 13／14 步须按新职责替换旧字段／集合执行器及验收比较。

## 候选的继续核对结果

### 已发现展示依赖，保留同步

- **Familiar FireCooldown `+D4C`、State `+D88`**：Render `61E360` 调用 `62C0C0`；该 helper 在变体 61／108／230／87／125／62 分支直接读取 FireCooldown 计算充能百分比，并在 variant87 等分支读 State。不能把它们按纯射击／AI 计数删除；FireCooldown 活动时钟缺口仍保留，不猜统一倒计时。
- **Familiar IsFollower `+D74`**：Render／Shadow 经 `60C190/60C1F0` 用它决定 Player 鞭形态关联和可见性；其邻接 `+D76` 仍未知，不能擅自补 bool。
- **Familiar Coins `+D7C`、State `+D88`、SizeMulti**：`647310` 在 variant231 用 SubType 和 Coins 取得配置缩放，以 State 控制附属 Sprite 淡出；“持有硬币”不能只凭名称归玩法。
- **Laser GridHit、MaxDistance、模式和最终采样点**：上述 PostRender／Render 有真实消费者，保持当前归属。公开 endpoint 与内嵌 endpoint 不因名称相近合并。
- **动画逐层／空层选择器、资源覆盖、HP／价格／WaterClip 输入、两份绘制 RNG**：既有正面展示证据继续有效，不能按“有最终 Sprite”全部替代。两份绘制 RNG 继续第 8 步 D0 的同窗借用合同。

### 其他候选与后续复核（版本30更新公共条目）

| 候选 | 已核范围与剩余依赖 |
|---|---|
| HomingLaser indices／Bezier 及采样标量 | 三个Laser绘制pass直接几何消费指向optimized；版本29另在Room Light `7FC110` 的Type7分支确认unoptimized有真实展示消费者，其必要性已闭合。其余采样中间量的完整维护／扩展依赖仍待核，当前保留五子块和四vector合同，不连带删除 |
| Entity 毒／火伤害相位 `+2E0/+2E4` | 版本30已按[公共复核](entity-common-synchronization-review.md)排除；原生伤害节奏归Host，不再要求条件相位锚点 |
| 暂停保存位置 `+314/+318`、PauseTimer、插值历史位置 `+344/+348` | 版本30已排除：原生位置／半帧模拟只在Host；Client接收最终位置，网络插值历史如需要则从本地已应用快照建立 |
| Familiar 轨道参数、Hearts／Keys／RoomClearCount、HeadFrameDelay／其他方向参数 | 本次正面核定了部分反例，不能由主 Render 无直接位移就批量归权威；剩余逐项调用关系和 Player 复合绘制继续核对 |
| 其他伤害值、碰撞／质量／摩擦、状态机参数 | 版本30已排除公共毒火／接触伤害、Mass／Friction、当前碰撞结果／无敌裁决及击退方向；保留碰撞类别、Size／SizeMulti及SpeedMultiplier的实际展示用途。其余子类仍须逐项核对 |
| valid／exists／removedByFactory、loaded、动画事件位 | 版本30将removedByFactory归本地，valid／exists／dead因实际绘制消费者保留，并明确禁止写Host标志接管本地池；具体owner／绘制借用writer归第5／13步。loaded和动画事件位继续各自资源／事件职责，不据此重演Spawn／伤害／音效 |
| Delirium DL1 六字段 | 本文版本28时尚未进入布局源，旧计划“先全发、第15步以后再判断”已撤销。版本41已按实际展示依赖归Host，块18空预留；版本42又将BossType／Variant归Host，DL0也空预留；ControllerId改表现三态、OverlayEffect只派生Scale，见[NPC复核](npc-synchronization-review.md)。不提前发AI输入或运行测试 |

### 新发现的跨域展示缺口

Familiar `+DB0 Weapon*` 的指针和玩法 owner 继续归权威，但不能再用“Render 不直接读取”宣称其全部内容对展示无用。`61E360→62C0C0` 的 variant80 分支读取 `+DB0`，调用 Weapon vtable `+1C` 获取充能比例。当前 Client 初始化将 `_weapon=nullptr`，该分支会跳过充能条；仅保留其他 Sprite 状态不能证明已经等价。

第 4／5／10／13 步需补充具名的**最终充能表现结果**及本地充能条应用路径，或给出已安装最终姿态完全覆盖该 helper 的证据；不得直接调用未知是否有副作用的 getter 充当只读采集，也不得为显示充能条重建 Host 玩法 Weapon。本次只记录并修正源语义，不虚构已有可用投影或提前接通运行时路径。

## 后续接线与验收范围

- 第 10 步只按生成计划复制状态；排除的容器连元素都不遍历，不因观测比较再偷偷附带。
- 第 12／13 步不解析或安装被删字段，容器和原生对象仍由 Client 的构造／退休 owner 管理；不把“权威”解释为可以留下悬空地址或省略正常析构。
- 第 13／14 步退役旧路径时同步移除旧命中／伤害比较，不为让旧验收通过重新发送这些字段；仍须保留必要引用撤边、Bomb pending 清除与资源安全行为。
- 第 15 步 E09／E25 按新版回映射比较实际同步结果；检查分裂、伤害、掉落／变更后最终成员与画面一致、Client 无玩法重演。删除对 K4／四命中历史及新增权威字段的 Host↔Client 原值相等要求。E13／E17 不再要求拒绝根本不上线的六个 Knife f32；其余实际线上浮点／引用／容器校验照常。E27 的六段 UNKNOWN 保持原范围。Familiar 充能展示须纳入原有角色／跟班回归，未接线不能标通过。

本次没有新增测试或验收编号。已通过同一普通 Release／Win32／v143 产品构建重新生成布局表，`ISAACCS_BUILD_E2E_TOOLS=OFF`；`isaac_cs_state` 与完整 `zhlIsaacCS` 编译／链接成功。没有运行单元测试、E2E、校验脚本、游戏或部署，未单独保留 artifact。纯编译不等于功能验收。
