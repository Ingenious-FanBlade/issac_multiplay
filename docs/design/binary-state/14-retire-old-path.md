# 14：退役旧路径与产品边界

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

从正式同步路径中移除 Lua getter 采集、JSON 树、逐成员目录和字符串分派。划清产品构建与 E2E 构建的边界，使产品 DLL 中不再包含旧路径的代码。

## 输入事实

### 构建边界

`mod/CMakeLists.txt:94` 的 `ISAACCS_BUILD_E2E_TOOLS` 选项区分产品构建和 E2E 构建。E2E 构建的工具、夹具和故障注入只进入 E2E DLL（`CMakeLists.txt:151-157`）。

### 旧路径主要文件

| 位置 | 内容 |
| --- | --- |
| `mod/src/game/world_sources/lua_entity.cpp`（708 行） | Lua 正式采集与 `CopyLuaSource` |
| `mod/src/game/world_sources/observation.cpp`（797 行） | 十一个域的 Lua 正式采集 |
| `mod/src/replica/stream_capture.cpp`（417 行） | 后台树构建与身份投影 |
| `stream_partition.cpp`、`stream_source_value.*`、`projection_graph.*`、`stream_materialize.*` | 分层、纯值块、Lua 图和还原 |
| `entity_plan.*`、`room_grid_plan.*`、`player_plan.*`，以及对应的 `*_target.*` | JSON 目标与 A→T 计划 |
| `stream_binary.cpp` 的字段表部分、`EncodePayload`／`DecodePayload` | 树形正文编码 |
| `mod/tools/gen_stream_catalog.py`（231 行）及其产物 `stream_policy.lua`、`field_ids` | 逐成员目录生成 |
| `client_entity_fields.inc` 的字符串分派 | Client 按名写入 |

### Lua 观测 Mod

`mod/observation/isaac-cs-observation`，包含 11 个域。

## 设计

### 14.1 移除顺序

1. **新路径完整编译**：第 10—13 步的新路径完成后，先确认它们可以独立编译，不依赖旧路径。
2. **逐个移除**：对上表中的每一项，先用 grep 和编译确认已经没有调用方，再删除。
   - 删除前要看清目标文件的实际内容，确认它的职责已被新路径完全接管。
   - 部分接管的文件，只删除已接管的部分。例如 `stream_binary.cpp` 中仍被控制消息使用的编码保留。
3. **同步构建与生成**：同步修改 `mod/CMakeLists.txt` 的源文件列表和生成命令，移除 `gen_stream_catalog.py` 的生成步骤。

### 14.2 Lua 观测 Mod 的新角色

| 部分 | 产品构建 | E2E 构建 |
| --- | --- | --- |
| `domains/mod`（具名只读 adapter） | 保留，作为 Mod 域唯一的正式来源（第 8 步） | 保留 |
| 其余十个域 | 不加载 | 只作为 E2E 对照（第 15 步）加载，不进入同步路径 |

### 14.3 保留不动的部分

- **Hook**：统一 Hook 清单保持常驻、按会话逻辑启停。新采集使用现有窗口，不新增 Hook。其中 `world_delirium_owner` 保留，只负责 Delirium 变身内 Init 之后的读取资格；仅由真实公共shape字段比较决定结构换代；BossType／Variant自版本42留Host，旧路径退役时须去掉它无条件升shape的逻辑（第 4 步 4.2.6）。
- **身份**：`IdentityTable`、`Registry`。
- **传输**：通道、可靠发送、分片重组、合包、zstd、内存池。
- **Client 原生写入层**：原生写入函数、结构事务和退休流程。

第4步的关系职责按版本29保留，参数与容器以版本37为准，不能用旧应用目录要求恢复已排除同步：Tear保留三份辅助Sprite及StickTarget，Pickup保留两份Sprite／公开Ghost和MegaChest三关系，未来循环数组自版本35起留Host，Laser保留BounceLaser／原始有符号采样模式／optimized与unoptimized两组最终点列及实际展示标量；Knife四槽、Tear六槽、Pickup私有一槽、Laser十二槽的旧Source／安装／原值比较在接线时退役。Familiar +DAC仅variant240布局130保留展示引用，其他变体不发。被排除的目标仍是独立成员；撤边和本地空槽／正常析构保护不可随旧表删除。几何不调用CalculateSamples，FlipSaveState不复活。TearPlus／PickupPlus／LaserPlus的派生声音、重抽和重采样请求仍属Host。见[关系职责复核](internal-entity-relations-review.md)，早期实现记录保留原时点。

版本30另按[Entity公共职责](entity-common-synchronization-review.md)退役旧公共计时、伤害、物理输入及历史位置的Source／安装／原值比较。版本31再移除公共valid字段／handler／热比较，由本地构造与发布owner及Player P7结构接管；P7尚未完成前保留旧Player结构源的独立激活资格信息，不得把“移出E2”误作删除该信息。exists／dead继续同步，不调用Kill／Remove替代最终状态。见[生命周期再复核](entity-lifecycle-synchronization-review.md)。

版本32另按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)接管资源存在性、路径／实际Image及本地准备owner后，退役旧事件位原值比较、loaded直接写入／跨端原值相等要求，以及ColorParams队列与颜色锚点的Source／安装／漂移比较。保留事件位本地清理、空↔非空资源正常退休、加载失败不ACK、完整selector及最终11分量颜色；保留is_playing／playback_speed的官方绘制依赖。新具名资源handler未接通前不能删去旧资源owner或声称替代完成；历史记录不改判。

版本33按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)退役Familiar11个参数范围的旧Source／安装／原值比较。保留FireCooldown／State／Coins等真实展示源；80接入最终amount缓存／展示adapter，240接入typed payload、逻辑deque和430E40 owner后，才算接管相应缺口。新节点／payload未知仍不写入；不得因新表存在就删除旧生命周期保护或宣称可玩完整。

版本34按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)退役42个参数范围的旧Source／写入／原值相等要求，撤销Bomb ExplosionCountdown与Tear WaitFrames／StickTimer锚点及漂移比较。保留Knife拖影输入、Tear效果位／DeadEye和Room Light、全部主／辅助Sprite及公共最终绘制偏移；Bomb pending清除和读回继续保留。不得通过调用LoadCostumes、ResetSpriteScale、Height／Scale setter、Update或Interpolate重建已排除输入的效果。通用entity_counters仍服务其他线上计数，不连带删除。

版本35按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)退役Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait锚点及未来循环数组的Source／安装／原值比较，以及Slot八项交互／状态／奖励字段。保留Price与WaterClip、DropDelay展示时钟、所有主／辅助Sprite和真实引用；本地循环安全空初始化、奖品动画／is_playing、价格Sprite资源owner不能随旧字段一起删除。不得用交互、定价、开奖或SetPrizeCollectible重算最终画面；整局HUD／进度域自己的捐款总额等来源独立保留。

版本36按[Projectile运动与展示参数复核](projectile-synchronization-review.md)退役Projectile八项运动／效果输入的旧Source／安装／原值相等要求；Damage／Change保持原排除，PR1预留。保留Height、公共EntityFlags／最终偏移／尺寸／Sprite和DepthOffset单份来源，以及Game帧展示借用；Height临时改写须精确恢复。不能通过Interpolate、Update或本地+470回调恢复曲线／摆动。私有展示UNKNOWN及其未完成接线仍如实保留，不因旧字段减少而称完整。

### 14.4 文档同步

- **索引**：更新 `docs/README.md` 和 `docs/hooks/README.md` 中受影响的入口。
- **旧方案与历史资料**：旧方案文档已在第 0 步加了顶部说明；历史记录正文不改。
- **Repo 中的 `stream-catalog.json`**：作为历史资料保留在 `docs/design/tx-04-21/`，不再被构建引用。

## 改动范围

上表列出的文件、`mod/CMakeLists.txt`、观测 Mod 的加载方式，以及相关的文档入口。

## 失败方式

- **误删被依赖的代码**：删除仍被控制消息、E2E 或冷路径使用的代码，导致编译失败，或功能在运行时缺失。
- **旧路径残留**：旧路径仍被某个分支调用，产品 DLL 中保留着 Lua 采集入口。
- **产品依赖 E2E 内容**：E2E 对照 Lua 被产品构建加载，或者产品功能依赖 E2E 文件。
- **生成残留**：旧的生成产物仍然留在构建目录中，被意外包含。
- **Hook 被误删**：顺带删除或改动了 Hook，破坏“Hook 常驻”约束。

## 完成内容

- **移除与编译**：旧路径移除，产品与 E2E 两种构建都能纯编译通过。
- **无残留**：用 grep 确认产品代码中没有对旧接口的引用。
- **验证安排**：功能验证在第 15 步完成。

## 未决问题

- 冷路径（场景准备）中是否还有依赖旧 JSON 的部分没有被第 11 步的 Baseline 完全取代，需要在实现第 11 步时逐项核对。

版本37按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)退役十九项Laser更新／运动／计时输入、六项Homing标量、indices中间缓存／Bezier与六f32系数的旧Source、writer、validator及原值／漂移比较；两个Laser锚点和旧indices i32／4B步长假设一并退役。原生indices已纠正为8B点对，旧SDK／执行器错误保留在历史说明，不能将旧路径误写成已修复。L2／L4／L7／L8及Homing子块0／3／4空预留不要求接收或安装。只安装两组最终点列与真实展示输入，保持本地self／空缓存owner、失败候选清理和正常析构；不得恢复采样、更新、半帧、接触或生成模拟。

**版本38 Effect退役**：按[Effect各臂与payload复核](effect-synchronization-review.md)撤销六项公共更新参数、四变体TearFlags完整128位、136描述八成员的旧Source／writer／validator／原值与漂移比较；136旧room-grid `lifted_descriptors`同样退役，不转移到另一同步域。Ghost旧四份固定Sprite、独立i32 count和Offset／Scale原值路径退役，改有效前缀三投影与专用视图134，保留无效槽正常清理。Trail旧base_prefix／producer未定位读取限制由606190正面证据解决，不能继续报已解决的布局Gap；节点仍完整64B且不连续。MrMe四项Host内部范围不新增同步。保留State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale、三种真实payload、引用与资源及本地owner，禁止通过旧路径重演玩法。本轮只改声明和生成链，未声称旧运行路径已经完成退役。

**版本39装备／Plus退役**：按[完整复核](player-equipment-plus-synchronization-review.md)退役ActiveItemDesc来源池／定时充能冷却、Weapon owner／FireDelay／Direction／axis_parameters／NumFired及Knife hitboxSource的旧Source／writer／validator／原值与漂移比较；正式ReadKnifeExtension接口已删除，旧Lua／JSON运行链的实际切换仍归本步。旧MaxCharge／两份chargebar重复负载改由保留字段及已核动态公式重建，不以旧比较要求恢复多余字段。保留Whip真实主引用、五槽类型与存在性、主动槽六项输入、TemporaryEffect三值及外层disabled；外层mask／evaluate请求和本地Player地址不再复制。Plus公共两nullable与同类holder／optional owner保持，已排除的各派生Plus项不恢复；所有旧本地边／容器经正常owner清理，不能memcpy零或恢复Update／Fire／HUD Update／采样／重抽。
**版本40 Effect后续五项退役合同**：见[证据与落地](effect-next-five-evidence.md)。清理旧FX0=8B／FX1=21B常数、102接收113的旧映射、Trail通用16384上限，以及“静止Effect所有锚点必不重发”的判断；统一改用版本40生成表和完整尾窗127上限。旧119回调／120生成／传播请求及113两标量不得作为新同步字段恢复，VarData残余UNKNOWN仍不上线。未知非空／错variant holder须报告，不借旧Source兜底或按空继续。保留三种表现payload及其正常资源／容器／共享owner生命周期，不增加Hook或重演玩法。本轮布局及合同已更新，正式旧运行链的退役仍由本步实施，未运行测试。

**版本42 NPC现行合同**：正式迁移按N0—N6和当前144视图退役旧NPC Lua／JSON字段／资源来源；DL0／DL1均预留，旧BossType／Variant、raw ControllerId及OverlayEffect原Scale不得作为fallback保留传输或写回。保留Delirium Init读取资格owner，移除无条件shape增加；所有已排除AI／命中／本地owner及UNKNOWN边界保持。见[完整复核](npc-synchronization-review.md)。
