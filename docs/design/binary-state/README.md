# 原生二进制状态同步重构：总计划

**2026-11-09 当前版本45：Projectile展示来源与Effect残余。** [J273静态证据与逐臂结论](projectile-effect-residual-evidence.md)闭合Projectile+440开关、+460/+464冻结绘制锚点的构造／Init／生产链，新增9B独立PR2，PR0保留4B Height、最小状态404B、根未知350B。Effect新增148号167臂与149号19/71/194内部flags臂，167的+430核为Host生成伤害快照；+434及后8B仍未知。119尾部确认一个callback的8B DamageFlags特例，通用臂仍保持未知。41C／44C／4BC没有语义或填充证明；此前五项不重做。普通Release／Win32产品纯编译与链接通过，未运行测试；正式接线仍按第10／13步。

**2026-11-09 版本44：Text／Dummy。** [字段职责与J273证据](text-dummy-evidence.md)及[实现说明](implementation-4-text-dummy.md)将Text145／variant2缩放146／Dummy147接入布局生成链。Text保留有效原始字节前缀与最终RGBA，counter仅146传输；duration与此固定字体不消费的颜色空间标签留Host，通用KColor20B保持。新增有界c-string读取／暂存和无尾部等大派生支持。Text339B／Dummy335B根未知、Dummy不可见标签历史缓存及自然来源缺口保持；普通Release／Win32产品纯编译与链接通过；第10／13步仍须接入实际窗口／owner，第15步前不运行测试。

**2026-11-09 版本43：NPC受控UI来源未决已闭合。** [静态证据与纠错](npc-control-ui-evidence.md)核定8E7550为原版NetplayManager::IsIdxLocalPlayer，撤销上轮“本地设备可用性”的误解。C/S只以NPC+F1C的-1哨兵生成0/2受控表现，不读取原版联机对象；新增ReadNpcControlledUi／DecodeNpcControlledUi，拒绝旧模式1。布局长度、根未知586／597B及其他未决保持，正式窗口／绘制接线仍按第10／13步；第15步之前不运行测试。

**2026-11-09 版本42：NPC同步职责修正。** [逐段复核](npc-synchronization-review.md)取消Delirium BossType／Variant的8B传输与无根据的shape换代；ControllerId本地设备键改为1B受控UI表现，禁止跨进程raw写入；OverlayEffect新增144号场景视图，8B Scale由当前主Sprite null-frame派生。五项原生绘制标量、真实引用、三deque与必要双Sprite保持。NPC／Delirium最小负载均710B，未知586／597B及Pathfinder SDK声明不变；实际控制UI来源／adapter仍待第10／13步，不运行测试。

**2026-11-09 版本41：Entity_NPC。** [实现说明](implementation-4-npc.md)与[静态证据](npc-evidence.md)将NPC142／Delirium143、双辅助Sprite、三组deque及44个唯一公开成员回映射接入生成链。保留真实绘制参数与8B变身shape，AI计数留Host；新增无分配deque读取接口。NPC本类223B未知、根586／597B未知与Pathfinder声明边界保留。普通Release纯编译／链接通过，没有运行测试；实际采集／安装仍待第10／13步。

**2026-11-09 版本40：Effect后续五项未决。** [静态证据与落地](effect-next-five-evidence.md)补入137—141号VarData具名臂，已核范围留Host／本地、其余UNKNOWN保留；Rotation／Scale移入FX0，Trail逻辑节点上限收紧为127。核到101可双减Timeout及141刷新常数，修正静止必不重发的验收表述；原生ExtraData仍为三种具体payload。正式reader／writer待第10／13步，测试只在第15步。

**2026-11-09 版本39：Player装备与Plus展示依赖。** [完整消费者与安装复核](player-equipment-plus-synchronization-review.md)将ActiveItemDesc来源池／定时充能冷却、Weapon五项非展示输入及KnifePlus.hitboxSource留Host；保留主动槽六值、Weapon充能三值与类型、TemporaryEffect三值及Plus公共nullable覆盖。新增135号UmbilicalWhip真实主引用、136号TemporaryEffects外层disabled／列表，并补Familiar+D76绘制抑制字节。原已排除Plus项不恢复，其他UNKNOWN保持。源、生成表、正式Plus读取接口和第5／8／10—15步已修订，未构建或测试，运行时仍待接线。

**2026-11-09 版本38（Effect合同继续适用）：Effect各臂与附属payload。** [Effect同步职责复核](effect-synchronization-review.md)撤销六项公共更新输入、TearFlags全128位与136描述八成员；State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale及三种表现payload保留。Ghost只发0—4有效Sprite前缀，Offset／Scale派生、i32 count由投影计数重建；MrMe原16B未知核为Host内部状态，Trail找到64B分配与完整复制。六臂编号不变，FX2均预留；公共Effect最小444→420B。源、生成表和第6／10／12／13／14／15步合同已修订，未构建或测试，运行时仍待接线。

日期：2026-11-08；更新：2026-11-09（Asia/Singapore）。状态：**第 0—3 步已落地，第 4 步的 Familiar／Knife／Bomb、Tear／Pickup／Laser 和 Slot／Projectile／Effect 布局与专用表现已接入生成链，普通 Release 纯编译／链接通过**。入口为[第 0—2 步实现](implementation-0-2.md)、[第 3 步实现](implementation-3.md)、[Familiar／Knife／Bomb 实现](implementation-4-familiar-knife-bomb.md)、[Tear／Pickup／Laser 实现](implementation-4-tear-pickup-laser.md)、[Slot／Projectile／Effect 实现](implementation-4-slot-projectile-effect.md)和[生成布局表](layouts/README.md)。当前布局版本45（Projectile／Effect残余见[新证据](projectile-effect-residual-evidence.md)；Text／Dummy见[字段证据](text-dummy-evidence.md)；NPC受控UI来源见[闭合与纠错](npc-control-ui-evidence.md)，旧版本事实保留，未运行测试）；[第4步原未决前四项](implementation-4-first-four.md)已完成工作量估算与DepthOffset复核，补入Knife尾部六个f32、更正Slot／Projectile本地指针和Effect生成参数归属，续研核定Knife六值的分裂／碰撞用途与Effect参数的Host归属，六段空白的填充假设及证据边界仍保留。SDK 声明、未知字节、子类计时分支和跨域未决照实保留，不表示类型完整或同步已可运行；第4步NPC／Delirium已迁入布局源，仍有未知与后续接线；Text／Dummy布局及字段职责已接入版本44（见[text-dummy-evidence.md](text-dummy-evidence.md)），未知与Dummy历史屏幕缓存依赖保持；第5—14步待实施。0—14 步没有测试，第 15 步是唯一统一整体验收。

**2026-11-09 版本37（历史范围）：Laser／Homing采样、运动与计时。** [Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)保留optimized／unoptimized两组最终点列与当前角度、长度、完整效果位、原始采样模式、MaxDistance／GridHit、公开EndPoint和+4F0展示分支输入；19项Laser更新输入、六项Homing标量、indices中间点缓存及Bezier／系数留Host。indices原生类型纠正为8B双f32点对，旧i32步长记录错误。L2／L4／L7／L8及Homing子块0／3／4预留，父投影仅1／2；撤销两个Laser计时锚点。最小Laser542→439B，未知421B不变。源、生成器和第10／12／13／14／15步合同已修订，未构建或测试；运行时仍待接线。

本目录保存这次重构的总计划和逐步设计。按用户 2026-11-08 的要求，步骤**直接用数字编号，从 0 开始**，不再使用 `tx-0x-0x`；文件名为 `<编号>-<主题>.md`。旧 tx 文档中的事实和失败记录保持原样；与本目录冲突的设计以本目录为准，具体范围见第 6 节。

**2026-11-09 版本28同步职责修订（历史范围）。** [字段同步职责复核](synchronization-field-review.md)把 Knife K4 六值／命中父链选择字节、Knife／Bomb／Tear／Laser 四组命中历史、Projectile 伤害／三项延迟变更参数、Laser 掉落概率／伤害倍率共 17 个范围改归 Host 权威，并同步撤销线上字段、校验及公开成员的状态回映射。旧空块号保留。Familiar 充能／状态／硬币等已有真实展示消费者，继续同步；variant80 经 Weapon 查询充能的展示缺口已记录，未虚构已实现。其余候选仅在依赖闭合后调整，UNKNOWN 保持。没有测试，唯一功能整体验收仍是第 15 步。

**2026-11-09 版本29：24项内部关系展示复核（历史范围）。** [逐槽结论及证据](internal-entity-relations-review.md)：Knife4／Tear6／Pickup1／Laser12共23项留Host；Familiar+DAC仅variant240的绳索分段有展示依赖，新增布局130保留真实引用，其余Familiar臂不发送。Knife K3显式预留，原有块号不前移。240的UmbilicalBabyData绳索payload仍待补齐，不能声称展示完整。Room Light已证实读取Laser未优化采样点，继续保留该容器。布局源、回映射和第13／14／15步同步修订；没有测试。

**2026-11-09 版本30：Entity公共同步职责复核（历史范围，valid由版本31纠正）。** [逐项结论与静态依据](entity-common-synchronization-review.md)：33个公共范围撤销传输，其中32个为Host玩法状态，removedByFactory为本地工厂生命周期。包含毒／火伤害相位、20个公共计时器（含PauseTimer）、暂停保存／半帧历史位置及部分伤害／物理输入；每个Entity最小负载减少223B。valid／exists／dead有实际绘制消费者，继续保留；碰撞类别、尺寸及Effect使用的速度倍率同样保留。布局源、生成表、回映射及第5／10／13／14／15步同步修订，不改变UNKNOWN或历史证据等级，没有测试。

**2026-11-09 版本31：valid／exists／dead再复核（历史范围）。** [结论和证据](entity-lifecycle-synchronization-review.md)：撤回“Render读取valid就必须公共同步”的判断。valid移出Entity E2，由Client本地构造／发布／退休owner维护；Player活动／备用身体的结构资格仍须由第5步P7接管，现有证据不能证明valid与room_member等价，也不能删除旧Player结构源的独立资格信息。exists／dead继续同步：存在性与激活／成员分离已有历史反例，死亡状态有实际展示消费者，未证明可由HP等价替代。公共最小状态进一步由411B降为410B，UNKNOWN不变；没有测试。

**2026-11-09 版本32：动画／资源／颜色职责复核（历史范围）。** [动画／资源／颜色职责复核](animation-resource-synchronization-review.md)撤销两组AnimationState事件位、两个loaded原始布尔及完整ColorParams队列的独立传输；资源有无并入file_name可空资源号，路径与实际Image仍分开。官方InterpolV2绘制读取is_playing／anim_frame／playback_speed，继续保留；最终颜色保留全部11个ColorMod分量。Entity E7及原事件／颜色块保留空块号，公共最小状态410B→391B。唯一布局源、生成表及第2／3／8／9／10／13／14／15步同步修订，资源执行器和未核owner仍待接线，没有构建或测试。

**2026-11-09 版本33：Familiar参数、Incubus充能与Gello绳索。** [Familiar参数／充能／绳索复核](familiar-synchronization-review.md)将11个轨道／计数／方向范围共48B归Host；FireCooldown／State／Coins等真实展示输入保留。新增variant80布局131的最终充能amount（不重建玩法Weapon），variant240布局130补入133号UmbilicalBabyData及132号XYZ节点，四投影同版本提交。基础Familiar最小661B→613B，80为617B，240缺失payload时623B。新payload仍有58B未知、每外部节点另108B未知；具名采集／展示／owner尚待第10／13步，不表示可玩或验收通过。本轮没有构建或测试。

**2026-11-09 版本34：Knife／Bomb／Tear攻击、运动与计时。** [Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)将42个范围归Host，撤销Charge／MaxDistance、爆炸计时／伤害／范围、BaseDamage／KnockbackMultiplier／WaitFrames等输入及三个计时锚点。Knife保留拖影距离／速度、旋转偏移、飞行状态和效果位，Bomb保留最终五份costume及绘制开关并清pending，Tear保留效果位／DeadEye强度／三份辅助Sprite。最小状态分别450→422B、1181→1127B、953→854B；UNKNOWN不变。唯一源、生成表与第10／12／13／14／15步已修订；运行时仍待接线，没有构建或测试。

**2026-11-09 版本35：Pickup／Slot选择、交互与奖励。** [Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup12／Slot8个原生范围留Host，撤销19项原线上字段／容器：选择组、触碰、自动价格、商店标识、机器状态／捐款／触发／奖品输入及未来循环列表等；Pickup Timeout／Wait两个锚点撤销。Price有WaterClip消费者、DropDelay直接控制Render，继续同步；Slot保留主／奖品Sprite，精确回映射其动画指针与is_playing。最小Pickup744→703B、Slot564→538B，另省循环列表元素；未知不变。源、生成表和第10／12／13／14／15步已联动，未构建或测试，运行时仍待接线。

**2026-11-09 版本36：Projectile（历史范围）运动与展示参数。** [Projectile运动与展示参数复核](projectile-synchronization-review.md)将追踪／曲线／加速／下落速度及加速度／摆动六项输入与Scale、ProjectileFlags共八项留Host，最小状态431→395B，PR1预留。Height因Render临时偏移有真实消费者保留；公共EntityFlags、最终位置／绘制偏移、尺寸和Sprite保持，DepthOffset仍只继承一次。私有+440及+460／+464的展示来源仍UNKNOWN，375B未知不变，不能称画面完整。源、生成表及第10／12／13／14／15步已修订，未构建或测试，运行时仍待接线。

## 1. 目标与用户决定

用户 2026-11-08 确定的方向：

1. **原生采集**：Host 在 C++ 侧直接读取游戏对象内存，正式同步路径不再调用 Lua getter，也不再构建 JSON 树。不采用 isaac-lan 那种“白名单字段＋兼容补丁”的适配方式。
2. **二进制状态**：每个类型的状态按固定布局连续排布：头部是类型标识，后面依次是父类字段和子类字段。字段在原生内存中不必连续，Host 构建状态时把采集到的字段连续排好，Client 按同一映射规则还原。
3. **合并读取与比较**：原生内存中相邻的已知字段合并成一次复制。Host 先把字段 memcpy 到自有缓冲区，再用 memcmp 判断变化；Client 按对应规则解码、校验并写入。
4. **完整性按字节归属**：每个类型从偏移 0 到对象大小的每个字节都必须有归属，见第 3 节。
5. **Baseline 统一为二进制**：Baseline 与增量使用同一格式，Baseline 就是“已确认版本为 0 的状态帧”。
6. **布局用单行表格画出**：每个类型都要画出状态二进制布局，每一格写清类型和语义，见第 5 节。
7. **新目录与数字编号**：即本目录。

性能和网络目标沿用 [tx-04-21 E2E 方案第 3.1 节](../tx-04-21/tx-04-21-e2e-plan.md)，本次不放宽：

- **Host 帧率**：不低于 50 FPS，即 20 ms 整帧。
- **Host 采集**：主线程每帧 p95 ≤2 ms、p99 ≤4 ms。
- **发布频率**：普通动态约 30 Hz，合格身体约 60 Hz。
- **带宽**：Host 三端总上行 10 Mbps，模组持续 7.5 Mbps。
- **首次准备**：不超过 1 MiB，从发起到 Client 画面就绪不超过 10 s。
- **Client 安装**：动态安装 p95 ≤2 ms、p99 ≤4 ms。

## 2. 为什么重构

第 73 轮 `p2-E02.cold-56` 的实际记录（[双人 E2E 进度](../tx-04-21/tx-04-21-e2e-progress.md)）：

| 项目 | 实测 | 目标 |
| --- | --- | --- |
| Host 主线程采集（stage12），596 样本 | 平均 184 ms，最大 460 ms | p95 ≤2 ms |
| 帧间隔（stage13），596 样本 | 平均 195 ms，最大 1.16 s | ≤20 ms |
| 普通窗口 Entity 域读取 | 平均 113.5 ms | — |
| 后台构建（stage11），259 窗 | 平均 231 ms | 每窗 ≤33 ms 才跟得上 30 Hz |
| Client 应用 | 90 秒内 `world_applied=0` | — |

结构性原因（静态核对）：

- **逐项调用 getter**：每个成员都经 Lua getter 读取，再组织成嵌套 Lua 表。
- **多次复制整树**：同一份数据至少完整生成三次：
  - 主线程 `CopyLuaSource` 复制成纯值（`mod/src/game/world_sources/lua_entity.cpp:662`）；
  - 后台 `PartitionSource` 分层（`mod/src/replica/stream_capture.cpp:234`）；
  - 身份投影（`stream_capture.cpp:276,285`）。
- **先构建后比较**：内容比较放在全部构建之后（`stream_capture.cpp:176`）。场景静止时版本虽然不变，读取和构建成本一分不少。
- **Client 按字符串处理**：Client 要从 JSON 树还原目标，再按字符串成员名逐项匹配写入（`mod/src/game/client_entity_common_native.cpp:43-50`）。

这些问题来自数据形态本身，逐项优化 Lua 读取无法达到上面的预算。

## 3. 完整性：字节归属

### 3.1 归属类别

每个原生类型从 0 到对象大小的每个字节，都必须恰好归入下列一类。类别名也是布局表格中使用的标记。

| 类别 | 含义 | 是否进入状态 | 例子 |
| --- | --- | --- | --- |
| 状态 | 有语义的值，直接复制 | 是 | `Entity._pos`、`EntityEffect.Timeout` |
| 换算 | 需要 Host 换算后才有跨进程意义 | 是，发送换算结果 | `EntityEffect.Timeout` 换算为到期帧 = 全局帧 + Timeout（第 4 步 4.1.5） |
| 引用 | 指向另一个实体／格子的指针 | 是，转成成员序号 | `Entity._parent` 等 `EntityPtr`（`+0x3BC—0x3C8`） |
| 资源 | 指向资源或配置的指针 | 是，转成稳定标识 | `ANM2._animData` 转成动画序号；房间配置指针转成配置标识 |
| 容器 | `std::vector`、`std::string` 等容器头 | 是，发送元素数和元素 | `Entity._colorParams`（`+0x1AC`） |
| 本地 | 只在本进程有意义：虚表、缓存指针、锁、Lua 引用、自有对象回指 | 否 | `Entity._luaBridgeRef`；NPC `+0xF40` 拥有的玩法对象指针 |
| 派生 | 可由已发送状态重新得到 | 否 | 只有确认 Client 写入路径或原生逻辑会重建时才可归入此类；未证明同步必要性时保留证据边界，不因无法派生就自动归状态 |
| 权威 | 只属于 Host 的玩法状态，Client 不应拥有 | 否 | RNG 种子与游标（沿用 Random 热路径 AuthorityOnly 先例，`mod/src/game/world_sources/observation.cpp:120-122`） |
| 填充 | 已证实没有语义的对齐字节 | 否 | 必须有证据，不能凭“看起来是填充”认定 |
| 未知 | 尚未确定 | 否，但必须列出 | 每段都要记录位置和长度 |

### 3.2 证据与“完整”的标准

- **证据来源**：每一段归属都要注明证据，可以是 SDK 声明（`libzhl/functions/*.zhl`）、J273 静态反编译、REPENTOGON 官方源码、Lua 绑定或 E2E 观察。
- **SDK 名称不等于公开语义**：SDK 的 `_depthOffset`（`+0x2B0`）并不是公开的 `DepthOffset`，后者实际在 `+0x358`（`mod/src/game/client_entity_fields.inc:5`）。只有 SDK 声明、尚未核对 J273 的条目，状态必须标为“SDK 声明”。
- **“完整”的定义**：一个类型的未知字节数为 0；或者每段未知字节都已在 E2E 未知区探测（第 15 步）中确认，在覆盖场景里不发生变化，并写明理由。未知字节既不复制，也不能宣称完整。
- **公开成员回映射**：旧目录中的公开成员改为核对表。每个 `ReplicatedState` 成员都要映射到某个布局字段、某组字段的派生、Host 权威或明确的不适用，用来检查字节归属是否有遗漏（第 1 步生成）。
- **同步必要性**：用途已核、公开 getter 存在、旧目录曾发送或不能由其他字段推导，均不单独构成同步理由。按 Client 的结构、原生绘制／展示维护／HUD 实际依赖决定传输；纯玩法输入／历史归权威，本地 owner／地址归本地。排除传输不删除字节归属和证据。具体调整、保留理由及未决见[字段职责复核](synchronization-field-review.md)。
- **Mod 域例外**：第三方 Mod 的自定义状态是 Lua 表，没有原生布局，不适用字节归属，处理方式见第 8 步。

## 4. 新数据流

```text
Host 主线程，在合法窗口内（ordinary／PlayerPost，沿用已核窗口规则）
  → 用已核来源枚举成员，按虚表确定布局编号
  → 按生成的复制计划：合并区间 memcpy、换算、引用记录身份、容器复制元素
  → 写入预分配的自有缓冲区，非阻塞交给后台
Host 后台构建线程
  → 身份映射为成员序号；按块 memcmp，变化的块记录变化版本
  → 按 Peer 已确认版本组装状态帧，确认版本相同的 Peer 共用一份编码
网络线程（沿用现有通道、分片、补片和 zstd）
Client
  → 有界解码与校验 → 更新影子状态 → 只写入变化块 → 原生安装成功后回 AppliedAck
```

增量规则：每个块记录“最后变化版本”，每个 Peer 记录“已确认版本”，状态帧携带两者之间所有变化过的块。丢包后下一帧仍会带上这些块，两端都不需要保存历史快照。“变了又变回”的块也不会漏，因为判断依据是变化版本，不是与旧快照逐字节比较。普通世界和逐身体是两条独立的版本序列，详见第 9、11 步。

**帧锚点规则**（2026-11-08）：随游戏帧递增或递减的计数，不发送计数本身，而是发送不随帧变化的锚点，否则所在块每帧都会变化。

- **锚点的含义**：锚点的值处于 Host 的帧空间。Client 用最近已应用 world 帧头中的 game_frame 作为展示时钟，由锚点还原计数（第 13 步 13.6）。
- **已确定的锚点**：
  - `Entity+0x328` 帧基准：FrameCount = 时钟 − 基准，基准原值发送（第 3 步）；
  - `Room+0x11F0` 帧基准：`Room:GetFrameCount()` = 时钟 − 基准，基准原值发送（第 7 步）；
  - Effect Timeout：换算为到期帧 = 全局帧 + Timeout（第 4 步 4.1.5）。
- **其他字段**：其余帧计数类字段在第 3—8 步按同一规则逐个核对。
- **空帧**：静止场景中虽然没有块变化，Host 仍要发送空帧，用来推进展示时钟（第 11 步）。

## 5. 布局表格要求

每个产生状态的类型都要画两张**单行表格**：

1. **原生字节归属表**：覆盖从 0 到对象大小的全部字节，按偏移排列，不留空隙。
2. **状态二进制布局表**：按线上顺序列出每个字段在状态中的位置。

格式规则：

- **只有一行**：表头行就是唯一的数据行，下面只放分隔行，不再加其他行。
- **单元格内容**：每一格写偏移、类型、名称和语义。
  - 原生表的格式是 `偏移 类型 名称【类别】语义`。
  - 状态表的格式是 `+偏移 类型 名称：语义`。
  - 未知或填充区也单独占一格，写明长度和现有判断。
- **嵌套类型**：如果嵌套类型有自己的表，可以占一格并注明“见某某表”，例如 `Vector`、`ColorMod`。变长容器写成 `u16 count + count×元素类型`，元素类型另有表。
- **头部和帧结构也要画**：记录头、状态帧头、成员表项等线格式结构同样画单行表（第 9 步）。
- **生成方式**：第 1 步的生成器从布局源产出表格，放在 `layouts/<类型>.md`。各步设计稿中手写的表要标“草稿”，生成器建成后以生成结果为准，避免文档与代码不一致。

### 5.1 示例：EntityEffect（草稿）

**已被取代**：EntityEffect 的实际字节归属和块划分，以[第 4 步 4.1](4-entity-classes.md#41-entityeffect-块划分修订稿2026-11-08) 为准。本示例按 SDK 声明的 0x490 和单个状态块写成，对象大小（J273 为 0x4C0）、未知字节数、0x420 变体复用区和块划分均已过时，现在只作表格格式示例。

偏移来自 `entity-application.json` 的 `class_native`（`EntityEffect.*` 13 项和 `NativeEffect` 0x420），语义取自本地 Wiki `IsaacDocs/docs/EntityEffect.md`。未知区来自 SDK `EntityEffect.zhl` 的对象大小 0x490，尚未经 J273 反编译核对。

**EntityEffect 原生字节归属（子类部分 0x410—0x48F）**

| 0x000—0x40F Entity【—】公共部分，见 Entity 表 | 0x410 f32 m_Height【状态】离地高度，粒子作纵向速度 | 0x414 f32 FallingSpeed【状态】下落速度 | 0x418 f32 FallingAcceleration【状态】下落加速度 | 0x41C—0x41F 4B【未知】尚无证据 | 0x420 BitSet128 VarData【状态】按效果变体解释的 128 位数据 | 0x430—0x44F 32B【未知】尚无证据 | 0x450 i32 State【状态】效果自定状态，初始为 0 | 0x454 f32 MinRadius【状态】冲击波最小半径 | 0x458 f32 MaxRadius【状态】冲击波最大半径 | 0x45C i32 Timeout【状态】剩余时间，每帧递减 | 0x460 i32 LifeSpan【状态】寿命 | 0x464 bool IsFollowing【状态】是否跟随父实体 | 0x465—0x467 3B【未知】疑似对齐填充，待证实 | 0x468 Vector ParentOffset【状态】相对父实体的偏移，见 Vector 表 | 0x470 i32 DamageSource【状态】伤害来源的实体类型 | 0x474 f32 Rotation【状态】旋转 | 0x478 f32 Scale【状态】缩放 | 0x47C—0x48F 20B【未知】尚无证据 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

**EntityEffect 子类状态块（69 字节，草稿）**

| +0x00 f32 m_Height：离地高度 | +0x04 f32 FallingSpeed：下落速度 | +0x08 f32 FallingAcceleration：下落加速度 | +0x0C BitSet128 VarData：按变体解释的 128 位原样字节 | +0x1C i32 State：效果自定状态 | +0x20 f32 MinRadius：冲击波最小半径 | +0x24 f32 MaxRadius：冲击波最大半径 | +0x28 i32 Timeout：剩余时间 | +0x2C i32 LifeSpan：寿命 | +0x30 u8 IsFollowing：是否跟随父实体，0／1 | +0x31 Vector ParentOffset：相对父实体的偏移 | +0x39 i32 DamageSource：伤害来源实体类型 | +0x3D f32 Rotation：旋转 | +0x41 f32 Scale：缩放 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

从示例可以得出三点：

- **复制次数**：这 13＋1 个字段只需 4 次 memcpy，区间是 `[0x410,0x41C)`、`[0x420,0x430)`、`[0x450,0x465)`、`[0x468,0x47C)`。0x465—0x467 在证实为填充前不能并入区间，否则 memcmp 可能因无意义字节误判变化。
- **未知字节**：子类部分 128 字节中有 59 字节未知，需要第 4 步核对。
- **布局待调整**：状态块不保留原生对齐，解码一律用 memcpy 读取。字段按什么频率变化、如何拆块，由第 3、4 步决定，这里只演示表格格式。

## 6. 取代与保留

| 旧内容 | 位置 | 处理 |
| --- | --- | --- |
| 公开 Lua 优先、自有只读观测 Lua 作为正式采集来源 | [规范第 15 节](../../refactor/project-conventions.md)、[观测扩展总览](../observation-expansion/README.md)、[REPENTOGON 适配设计](../repentogon-refactor.md) | 正式同步改为原生二进制。Lua 观测只保留两种用途：E2E 对照，以及 Mod 域例外（第 8、14 步） |
| Host 采集性能修复方案 | [tx-04-21 采集性能方案](../tx-04-21/tx-04-21-capture-performance-plan.md) | 整体取代。成本口径 stage11—18 和有界交接的思路继续沿用 |
| 逐成员实时目录、`field_ids`、按字段名分层 | [tx-04-21 基础](../tx-04-21/tx-04-21-foundation.md)、`stream-catalog.json`、`mod/tools/gen_stream_catalog.py`、`mod/src/replica/stream_partition.cpp` | 由第 1 步的布局源取代；成员目录改为回映射核对表 |
| `Field`／`Atom`／`Layout` 范围模型，“永不隐式继承旧动态字段” | `mod/src/replica/stream_model.h:30-58,88`、[tx-04-21 网络合同](../tx-04-21/tx-04-21-network-contract.md) | 改为状态帧、块变化版本和显式继承已确认版本（第 9、11 步） |
| JSON 目标与计划：`Parse{Entity,RoomGrid,Player}Target`、`DomainModel`、`SubjectValues` | `mod/src/replica/*_target.*`、`*_plan.*`、`stream_materialize.*` | 由影子状态的类型化视图取代（第 12、13 步） |
| Client 按字符串成员名写入 | `client_entity_common_native.cpp:43-50` 等 | 改为生成的索引分派表（第 13 步） |
| 18—20 应用目录 | `tx-04-18/room-grid-application.json`、`tx-04-19/entity-application.json`、`tx-04-20/player-application.json` | 保留，作为应用职责和回映射的输入；不再驱动采集 |
| tx-04-21 的 E01—E26 | [tx-04-21 E2E 方案](../tx-04-21/tx-04-21-e2e-plan.md) | 修订后作为第 15 步的验收清单；tx-04-21 的暂停进度不再按原方式续跑 |

保留并复用：

- **身份**：`IdentityTable`、`Registry`、key／use／shape 的语义。热路径改用数值接口，见第 10、11 步。
- **采集窗口**：已核的 ordinary／PlayerPost 采集窗口和规范第 11 节的读取规则。
- **Hook**：统一 Hook 清单，常驻并按会话逻辑启停。
- **传输**：通道、`ReliableSender`、`Reassembler`、合包、zstd、`AppliedAck`、Resync，以及 512／128／128 MiB 三池。
- **Client 原生写入层**：原生写入执行器和场景结构事务。描述结构中依赖 JSON 的部分需要替换。

历史文档的正文不改写。第 0 步只在受影响文档顶部加一行说明，指向本目录。

## 7. 步骤

整个重构是一个推进目标：0—14 是实现步骤，期间不运行任何测试；第 15 步是唯一的统一整体验收。

| 步 | 文档 | 内容 | 依赖 |
| --- | --- | --- | --- |
| 0 | [起点、取代范围与规范同步](0-baseline.md) | 代码起点、旧文档标注、术语 | — |
| 1 | [布局源、字节归属模型与生成器](1-layout-source.md) | 唯一布局源，生成复制计划、编解码表和文档表格 | 0 |
| 2 | [基础值类型与嵌套规则](2-base-types.md) | Vector、ColorMod、ANM2 等基础类型，以及容器、引用、资源规则 | 1 |
| 3 | [Entity 公共与主 Sprite](3-entity-common.md) | Entity 0x410 与内嵌 ANM2 的字节归属、块划分 | 2 |
| 4 | [Entity 子类](4-entity-classes.md) | NPC、Tear、Projectile、Bomb、Laser、Knife、Pickup、Familiar、Effect、Slot 及专用形态 | 3 |
| 5 | [EntityPlayer](5-player.md) | Player 0x2EA0、内嵌 Sprite、容器、身体快源块 | 3 |
| 6 | [GridEntity](6-grid.md) | Grid 公共部分和各子类 | 2 |
| 7 | [Room、Level、RoomSave 与 Exploration](7-room-level.md) | 房间、楼层、描述表和保存集合 | 2、6 |
| 8 | [Run、Presentation、Random 与 Mod](8-run-presentation.md) | 整局、HUD／地图、RNG 权威和 Mod 例外 | 2 |
| 9 | [线格式](9-wire-format.md) | 状态帧、记录、块、成员表、资源表和校验 | 1 |
| 10 | [Host 采集器](10-host-capture.md) | 主线程复制、换算、引用、容器和交接 | 1—9 |
| 11 | [Host 变化检测、增量与发布](11-host-publish.md) | 块比较、变化版本、按 Peer 组帧和 Baseline | 9、10 |
| 12 | [Client 解码、校验与影子状态](12-client-decode.md) | 校验、版本接受规则和影子状态 | 9 |
| 13 | [Client 原生应用](13-client-apply.md) | 冷热路径写入、描述结构替换和漂移校正 | 12 |
| 14 | [退役旧路径与产品边界](14-retire-old-path.md) | 移除 Lua／JSON 热路径，划清产品与 E2E 构建边界 | 10—13 |
| 15 | [E2E 适配与统一整体验收](15-e2e-acceptance.md) | Lua 对照、未知区探测、E01—E26 修订、新增 E27—E35 和唯一整体验收 | 0—14 |

步骤 3—8 是字节归属研究，需要对 J273 做静态反编译，可以交叉进行；第 9 步只依赖布局源格式，可以与 3—8 并行。

## 8. 推进规则

这些规则遵循[现行项目规范](../../refactor/project-conventions.md)：

- **不设 gate，不校验哈希**：布局兼容靠整数布局版本号，不计算任何哈希。
- **只在第 15 步测试**：0—14 期间只做资料分析、静态反编译、设计、实现和纯编译。不运行游戏、E2E、校验脚本或带测试的构建。第 15 步进入后连续修复、续测，不另开编号。
- **不另写单元测试**：实现后不补写单元测试，以 E2E 为唯一测试手段。必须隔离验证的系统，先写出全部失败方式再写代码；各步文档的“失败方式”一节即用于此。
- **注释**：代码为功能、核心函数和数据结构写中文注释。
- **实机测试前沟通**：每次启动前告知用户需要配合的操作和停止时机，需要手动配合时先确认用户已准备好。
- **记录如实保留**：失败和 UNKNOWN 照实记录，不留 artifact，只在中途停下需要恢复时保留必要材料。
- **隔离约束不变**：保持 C/S 与原版 Online、Isaac Sync 的隔离，线程与生命周期约束、Hook 常驻和实际操作授权范围不变。
- **提交**：只在用户要求时提交。

每步文档统一包含：目标、输入事实、设计、改动范围、失败方式、完成内容、未决问题。

## 9. 未决问题与需要用户决定的事项

1. **代码起点（2026-11-08 已定）**：用户已提交为 `87bae3e`（157 个文件，+6265／−532），含第 70—75 轮修复和本计划文档，作为重构起点，见第 0 步。
2. **Mod 域**：没有原生布局，计划保留只读 Lua 来源并以低频自描述二进制块发送，见第 8 步。这是“按字节归属”的唯一例外。
3. **tx-04-21 验收（2026-11-08 已定）**：tx-04-21 未完成的统一验收并入第 15 步，不再按原 tx 续跑；[进度文档](../tx-04-21/tx-04-21-e2e-progress.md)顶部已标注。
4. **依赖其他状态的计算结果**：例如 `GetCollectibleNum` 的结果依赖库存、效果等多处状态。它们能否全部由已发送的字节在 Client 重建，要在第 5 步和回映射中逐项确认；不能重建的，按“换算”或“权威”归类。
5. **未知字节**：第 3—8 步的工作量取决于各类型的未知字节数量。EntityPlayer 0x2EA0 字节、Game 0x680E8 字节的完整核对规模最大，第 5、8 步需要先给出范围划分。
