# 第 4 步：24 项内部实体关系的展示依赖

> 版本37后续复核：[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)只保留optimized／unoptimized最终点列和真实展示标量；十九项Laser输入、六项Homing标量、indices中间点缓存与Bezier／系数留Host，撤销两个Laser计时锚点。J273 Homing+34实际为8B双f32点对vector，本文旧i32／4B步长记录错误；原记录保留，不按旧类型继续传输。L2／L4／L7／L8预留，当前以版本37唯一源及生成表为准；未构建或测试，旧运行执行器仍待后续退役。

> 版本33后续复核：[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)排除Familiar11个参数范围；80补入最终充能amount，240补入typed绳索Sprite／节点布局。下文“尚待建模／补字段”是版本28／29时点；现在仍待第10／13步具名采集、展示及owner接线，不代表运行时完成。

日期：2026-11-09（Asia/Singapore）。布局版本 **29**。范围为上一轮 Wiki／SDK 对照中没有公开对应项的 24 个 EntityPtr 槽：Familiar 1、Knife 4、Tear 6、Pickup 1、Laser 12。只做资料阅读、J273 静态反编译、布局编辑和普通产品构建；没有运行测试、游戏、注入或部署。

## 结论与布局处理

**23 项留在 Host；Familiar `+DAC` 仅 variant240 保留同步。** 后者已证明的最低展示需求是关系是否存在，而非解引用目标读取其姿态。当前沿用真实成员引用编码和 SetReference owner，避免引入伪非空指针；不把非240变体中的同槽玩法关系一并发送。

为此新增 `EntityFamiliar_UmbilicalBaby`，布局编号 **130**，选择值为 Variant 240；只覆盖 `+DAC` 的归属、F2 块及 `NativeFamiliar.Related` 回映射。基础 Familiar 及 48／231 臂的同槽归权威。原布局号 120—129 和块号不变；130 不代表 UmbilicalBabyData 完整支持，具体绳索 payload 仍有明确缺口。

| 类型 | 偏移 | 布局字段 | 当前职责 |
|---|---|---|---|
| Familiar | `+DAC` | `familiar_related` | **仅240同步引用**；其他变体权威 |
| Knife | `+4F4` | `knife_linked_a` | 权威 |
| Knife | `+4F8` | `knife_linked_b` | 权威 |
| Knife | `+4FC` | `knife_child_a` | 权威 |
| Knife | `+500` | `knife_child_b` | 权威 |
| Tear | `+488` | `tear_child_a` | 权威 |
| Tear | `+48C` | `tear_peer` | 权威 |
| Tear | `+490` | `tear_child_b` | 权威 |
| Tear | `+494` | `tear_link_a` | 权威 |
| Tear | `+498` | `tear_link_b` | 权威 |
| Tear | `+49C` | `tear_child_c` | 权威 |
| Pickup | `+58C` | `pickup_related` | 权威 |
| Laser | `+4EC` | `laser_linked` | 权威 |
| Laser | `+4F8` | `laser_child_a` | 权威 |
| Laser | `+4FC` | `laser_child_b` | 权威 |
| Laser | `+504` | `laser_split_a` | 权威 |
| Laser | `+508` | `laser_split_b` | 权威 |
| Laser | `+50C` | `laser_split_c` | 权威 |
| Laser | `+510` | `laser_split_d` | 权威 |
| Laser | `+52C` | `laser_related` | 权威 |
| Laser | `+530` | `laser_terminal_a` | 权威 |
| Laser | `+534` | `laser_terminal_b` | 权威 |
| Laser | `+538` | `laser_terminal_c` | 权威 |
| Laser | `+53C` | `laser_peer` | 权威 |

23 项的原生范围、EntityPtr 类型、反向登记事实和已核等级保留；删除 `wire`、专用读取资格、状态块条目，并将对应 Native* 回映射改为 authority。Knife K3 变为空预留块；Tear T3 只留 StickTarget，Pickup P2 留公开 Ghost／两个 MegaChest，Laser L3 只留 BounceLaser。公共 Parent／Child／Spawner 等、KnifePlus hitboxSource 及这些公开关系不在本次24项之内，不借此批量删除。

按各取一只 Knife／Tear／Pickup／Laser 计算，去掉23个u16引用投影，共减少46B固定状态；非240 Familiar 另少2B，240仍保留原2B引用。这是布局算术，不是性能实测。

## 静态核对方法和边界

目标 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；以下地址为 VA。不修改 IDB、不计算哈希。使用原生入口、原始指令和真实目标来源判断，不能凭函数名、调用者类型或同偏移命中认定对象类型。

1. 复读相关主 Render：Familiar `61E360`、Knife `93ECD0`、Tear `675AC0`、Pickup `6E35D0`、Laser `9514B0`；同时纳入公共 Shadow `6AD450`、PostRender `6AC660`，Familiar Shadow `61E320`，Laser Shadow／Post `955290/956F80`。
2. 补入当前 Client 使用的 Player／NPC／Effect 等展示入口，以及各类 vtable slot13 的 GetRenderZ：公共 `6AC3F0` 等；按 SDK 签名定位并复读 Room::RenderEntityLight **`7FC110`**。读取 `client_presentation_native.cpp`，确认 Light 是正式展示消费者，不能只检查 Entity 的三个虚 pass。
3. 在整个 `.text` 中对这批22种不同位移检查直接内存／LEA候选及 ADD／SUB形成地址，共涉及220个候选函数；与上述展示入口的静态直接调用可达集合交叉，纳入 Light／排序后为572函数。此集合不受三层深度限制，但不伪称包含所有间接虚调用或任意外部 Lua 回调。
4. 交叉候选只有 Familiar `61E360`、Bomb `6A0F90`、Player helper `7B67A0` 和 Room Light `7FC110`。逐目标复读后，只有 Familiar `+DAC` 是本批关系读取；其余是不同对象／嵌套成员，详见下表。补查该展示集合中跨目标槽的宽位移内存访问，未新增此批关系消费者。
5. 对照各类 Init、Update／helper、ClearReferences 和 Remove 的关系用途及反向登记；这些正面证据支持 Host 生成／更新／清理职责。没有把“原生清理会 Remove 子体”当成必须向 Client 安装关系的理由。

这是当前原版／官方 REPENTOGON、Client 不运行玩法 Update 的采用范围结论，不是任意第三方回调、任意指针算术或所有虚分派的形式化证明。旧记录中“独立关系必须全保留”属于版本28及更早设计，原静态关系事实保留，传输决定由本文修订。

## 逐类消费者与采用依据

### Familiar：真实的绳索分段输入

`61E360` 的 variant240（case `0xF0`）在 **`61F551`** 读取 `[edi+DAC]`，随后 `61F557 neg`、`61F559 sbb`、`61F55B and eax,2`，在 **`61F560`** 调 `4308D0`。因此实际传入的是：空关系→0，非空→2；不是调用地址 `61F562`，也不是解引用关系目标。

`4308D0` 对 `Familiar+ECC` 做 UmbilicalBabyData RTTI 转换，遍历 payload 中的绳索节点，用第三参数选择完整区间或后半区间，再调用 `419D30/419F40` 提交绘制。关系目标的坐标／颜色不通过 `+DAC` 在此读取，但关系存在性会改变画面，故不能一律归权威或用最终实体 Position 替代。

`60B800` 在240分支对该目标调用 Remove，再撤反向登记。这是额外的生命周期事实，不是本次保留同步的理由。Client保留真实引用时仍须先撤边，再独立退休成员，不能重演Host级联删除。

**新增明确缺口**：此前基础臂把 ExtraData 统称“玩法私有”，只做48／231 typed payload；本次证实240的 UmbilicalBabyData 有绘制消费者。布局130只恢复关系职责，不虚构绳索节点布局已核或已采集／重建。第4步需补payload类型／节点／资源归属，第10／13步补同版本采集和展示 owner；当前不能声明240完整。若以后改发单独的present值，必须由具名展示参数／描述执行器消费，不得把1或其他假地址写入EntityPtr。

### Knife：四个槽留在 Host

- `93ECD0` 绘制使用公共 Parent 链：`93F650` 沿 Parent 找根刀、`93F5E0` 找 Player，均不读取这四槽。Shadow／Post／Light／排序及所用 helper 未发现这四槽的展示依赖。
- `93AAF0` Update 读取 `+4F4/+4F8` 组合位置／关联，管理 `+4FC/+500` 的子体最终状态；例如对 `+500` 目标设置位置、角度、长度和可见性。`941660/9418D0` 等消费者属于玩法／物品／攻击路径，不作为 Client 展示维护调用。
- `93A1E0` 撤四槽登记并对 `+4FC/+500` 目标 Remove；开头还会回查 Tear Parent 的 `+49C`。保留这些原生关系和跨类型事实，Client不因此安装四条玩法所有权边。

### Tear：六个槽留在 Host

`675AC0` 消费效果位、DeadEye／辅助 ANM2、Spawner 与公共最终姿态；三个其他展示消费者与 helper 不依赖这六槽。`663F90` Update 则通过 `+488/+48C/+490` 创建／维护独立效果或关联对象；`+494/+498/+49C` 被传入关系 helper（例如 `408100/6B17C0`）继续驱动玩法对象／攻击。不能用一次字符串搜索零命中忽略这类取址别名。

`663570` ClearReferences 对 `+488/+490/+49C` 可调用 Remove，处理 `+494/+498` 的两端 Tear 关系，再撤各槽登记。`677810` 亦有 ADD 形成 `+494/+498` 地址的候选；不把关系正面用途误作“填充”。相关子体作为普通 Host 成员分别同步最终资源／姿态；Client不运行这些生成和控制路径。

### Pickup：`+58C` 留在 Host

`6E35D0` 的关系绘制读取是 **`+594/+598` MegaChest**，不是 `+58C`。本次保留公开 Ghost／MegaChest 关系，不随私有槽一起删除。

全映像 `+58C` 候选中的 Pickup ctor `6DD550` 清零，Init `6DD7C0` 在 `6DD89B` 取槽地址并撤旧登记；派生清理 `6EA860` 处理该槽。`7A5660/7A7D30/9A4F00` 属于玩家保存状态，CRT／栈候选不是此Pickup关系。现有证据不为 `+58C` 赋予未经证明的具体玩法名称；其存储／清理事实已核，当前展示和安装消费者不需要它，因此不进入网络图。

### Laser：十二个槽留在 Host

`9514B0/955290/956F80` 和 Light 的几何消费者使用最终采样／姿态／模式等，不读取这十二槽。`94C940` 的两个子体、四路分裂、终端管理会创建或改写独立 Laser／Effect 的位置、角度、显示／计时；`950B90` 还通过 `+53C` 传播更新／状态，`954310` 存在 `+52C` 条件读取。这些是Host更新图，不是Client绘制图。

`956C60` 按双槽／四槽循环以及具名终端关系撤登记，部分分支会 Remove／Clear 子体；`956E90` 级联 Remove 多组目标。关系的存在不意味着 Client 要按它们再次删除已按成员表独立存活的实体。保留公开 BounceLaser `+4E8`，其余十二槽撤销网络身份解析与安装。

## 跨类型误命中与光照反例

| 候选 | 复读结论 |
|---|---|
| Bomb Render `6A0F90` 的 `+494/+500/+504/+508/+50C` | 来自 `Bomb+464` 的 costume ANM2 成员／变换，不能按 Tear／Knife／Laser 同偏移认定为EntityPtr |
| `7B67A0` 的 `+530` | 路径为 `Player Render 784A60→83AC80→834930→7B67A0`；从Player的库存对象取道具计数，属于库存数组项，不是Laser TerminalA |
| Room Light `7FC110` 的 `+48C/+490` | 明确在 **Type7 Laser** 分支，以end−begin按8B步长遍历 **HomingLaser未优化采样点**；不是Tear Peer／ChildB。`7FDCC9` 读490，`7FDCCF` 减48C；后续 `7FDCF6/7FDDA2/7FDE9F` 继续读48C |

**未优化采样点现在有正面展示证据，必须保留。** 版本28对 HomingLaser 的未决文字据此细分：unoptimized 的必要性已闭合；indices／Bezier／其余采样标量的完整展示／维护依赖仍待核，不连带删除。当前四容器布局不变，只补证据及语义。

## 第 10／13／14 步接线要求

- Host不为23条权威边采集或解析身份；它们不能使成员表因展示不需要的目标而报缺口。相关实体自身的出生／移除仍按完整成员来源同步，不能因不发边而漏掉子体。
- 基础 Familiar 及非240臂不传DAC；240按布局130恢复同版本真实引用。目标先构造，边后安装。最低需求虽是present，当前不能用任意哨兵地址替代目标。
- 新Client用途中的23槽保持本地安全空关系；复用／退出旧实例如已有绑定，先经SetReference撤反向登记，再独立退休。不允许直接memcpy零、保留悬空关系或调用原生级联Remove伪造生命周期。
- 旧 `client_entity_links.inc`、旧 Lua／JSON Source 和比较器本轮保持运行版本；第13／14步接新布局时按新职责退役23条安装／相等比较。旧表中的登记事实仍用于必要的本地清理，不能连清理保护一起删掉。
- 240绳索payload和variant80充能结果均是明确展示缺口，分别补足；不得把保留一条关系或 `_weapon=nullptr` 当作完整画面实现。

## 第 15 步验收修订

E09／E25按新版回映射比较最终成员、位置、资源、分裂／终端画面及Light；不再要求23条Host内部边的原值相等。E29仍覆盖实际保留引用的替换／撤边，并检查本地空槽、独立子体的退休和不发生级联误删。Familiar240补齐payload后覆盖关系空↔非空的绳索区间变化；目前不得记通过。E13／E17只对实际线上引用检查未知身份；不构造已撤销字段负载来要求其拒绝。E27的未知区范围和原始历史失败不变。

本次不新增验收编号，不提前运行任何测试。普通Release／Win32／v143产品构建已重新生成布局表，`ISAACCS_BUILD_E2E_TOOLS=OFF`；`isaac_cs_state`与完整`zhlIsaacCS`编译／链接成功。没有运行单元测试、E2E、校验脚本、游戏、注入或部署，未单独保留artifact。纯编译不代表展示或生命周期已验收。
