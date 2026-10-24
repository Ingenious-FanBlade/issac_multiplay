# tx-01-71：A 身份生命周期与 E 原生诊断生产链

日期：2026-10-18。本文是 [tx-01-70](tx-01-70-static-analysis-handoff.md) 工作包 A01–A13、E01–E10 的原生静态分析补充；B/C/D 由同轮其它文档回答，最终合并由主智能体完成。结论只适用于本轮已打开的 32 位 `isaac-ng.exe` IDB（imagebase `0x00400000`，横幅 VA `0xB7E1B4` 为 `Binding of Isaac: Repentance+ v1.9.7.17.J460`）。没有修改 IDB，没有启动/操作游戏，没有构建或测试，也没有计算或校验任何哈希/校验值。

## 1. 证据来源与结论标签

- `STATIC_CONFIRMED`：当前 IDA 指令、伪代码、字符串引用或保存的当前静态证据直接支持。
- `PRIOR_STATIC`：本仓库 tx-01-63 至 tx-01-69 已保存的当前构建静态证据；本文未重新导出或覆盖它们。
- `STATIC_INFERENCE`：由可见控制流和数据流推出，但不等于运行时实际覆盖。
- `DESIGN_PROPOSAL`：建议的本地事件/身份语义，不是已实现功能。
- `UNKNOWN_RUNTIME`：静态分析不能证明的真实覆盖、无漏事件、异常和跨进程性质。

[本轮 IDA 只读导出](../../../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json)保存函数伪代码、VA/RVA、每条已解码指令及字节、调用点、字符串引用和数据交叉引用；[导出脚本](../../../../artifacts/cs-t0/tx-01-71-static/export_native_evidence_tx0171.py)使用新文件名与新目标路径，不覆盖旧产物。共请求 28 个入口、导出 27 个唯一函数；请求 `0x903670` 落在 `sub_9032D0` 内，故不是第 28 个独立函数。详细 ring 的既有底层证据见 [tx-01-63 原始 IDA 导出](../../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json)与[生命周期报告](tx-01-63-entity-lifecycle.md)；Player 备用/保留来源见[类别覆盖](tx-01-64-category-coverage.md)。

本轮保存的伪代码受 Hex-Rays 命名与类型恢复影响；涉及偏移、宽度、条件、调用顺序时以 JSON 中原始指令为准。静态阅读 checksum 算法和调用数据流没有实际计算或校验结果。

## 2. A：身份与生命周期的原生依据

### 2.1 可用于事件建模的路径

| 对象/路径 | `STATIC_CONFIRMED` 事件顺序 | 身份含义及限制 |
|---|---|---|
| NPC、Tear、Projectile、Pickup、Familiar、Effect 的常规生成 | `Game::Spawn` `0x6FE410` → `EntityFactory::Create` `0x6F0090` → 工厂返回池槽 → 设置生成关联/运动 → 虚 Init（调用点 `0x6FE610`）→ 递增 Room ordinal 并写 `Entity+0x20` → `EntityList` 发布 `0x418500` → Spawn 后续处理并返回。调用点 `0x6FE610` 的指令为 `FF 50 04`；`0x418500` 前的完整顺序保存在 tx-01-63 导出。 | 工厂非空仅表示取得了槽，不代表 Init 或发布已完成。建议先建候选，只有同一调用正常完成并确认发布时才提交 `birth/use_generation`。通用工厂明确拒绝 Type=1；Game::Spawn 不能替代 Player/Grid 路径。直接调用工厂或其它构造路径是否覆盖完整，静态 caller 导出不穷尽。 |
| 常规池槽逻辑复用 | 工厂找 `Entity+0x170==0` 的槽。池满覆盖路径可先写 `+0x177=1`，虚 Remove、虚 ClearReferences，清 `+0x177` 后把同址槽交回调用者；随后新 Init，再发布。基类 Init 在 `0x6A954C` 恢复 exists 等状态；构造/析构不必再次发生。 | 同地址跨 Init/复用可代表两个逻辑使用。`valid`、`exists`、池占用、容器成员资格都不能单独充当永久身份。需把新 Init/候选事件与同次发布/正常返回配对；地址只用于当前进程内定位对象，不能跨 tick 或跨进程当身份。 |
| 普通 Co-op Player | `sub_9B9CD0` `0x9B9CD0` 普通分支：单独分配 `0x2F00` 字节、Player 构造 `0x753DA0`、写 PlayerIndex/插入 Manager roster、虚 Init，再从当前 Room 递增 ordinal 并通过 `0x418500` 发布。分配数为 12032 十进制。 | roster 插入早于 Init；看到 roster 不等同 Init 完成或 Room 发布完成。观察正常调用返回和发布才提交候选逻辑使用。 |
| 特殊 Player (`a2==-1`) | 同一 `0x9B9CD0` 的特殊分支单独分配/构造，写入 Manager 的另一 vector（当前反编译为 `this[52..54]`，该结构不等同普通 roster），随后虚 Init(1,2,0,派生 seed)并直接返回；该分支没有普通分支的 Room ordinal / `0x418500` 发布序列。 | 是保留/备用 Player 对象，不能按普通活跃 roster 计数，也不能把“没有普通发布”误读为分配失败。伪代码在构造结果为 null 后仍可继续解引用；没有 OOM 安全证明。 |
| Player 恢复/备用形态 | `PRIOR_STATIC`：`0x9BCAA0` 会为需要备用形态的对象单独分配、构造 Player、写 `Player+0x1E6C` 并恢复状态；同一恢复过程另遍历 Manager `+0x80` 的四个保留槽，按条件创建/初始化。相关 `0x7AC7E0` 分支会查 `Game+0x1BB28` 的保留槽并进入 Player 替换。证据位置见 tx-01-64。 | 恢复可以新建对象，也可以维护保留对象关系；不能仅凭玩家槽位或角色外观声称物理对象延续。新分配对象开始新本地逻辑使用；此前已存在的保留对象仍以原本地对象映射为准。完整存档恢复语义不在本轮范围。 |
| Player 替换 | `sub_9BEE80` `0x9BEE80` 有两类：当目标 `+0x170` valid 为真时，交换 Manager roster 中的 Player 指针/PlayerIndex 及关联字段；否则查找旧 Player 指针并替换成目标指针，迁移受追踪引用和备份关联，随后通过 `0x418500` 发布目标 Player，并处理 Familiar owner 等关系。反编译中的 `+368` 是十进制，不能写成 `+0x368`。替换窗口向双方写入 `+0x26C8` 临时关联，正常可见路径结束时清理；本函数体没有旧 Player 的直接 deleting-destructor 调用。 | 明确新构造对象登记新的本地对象 ID；目标若是已登记的保留对象，则延续其自身 ID，不继承源对象 ID，也不因重新发布再造身份。slot/index 交换时对象 ID 随对象走，玩家槽位关系另记。异常传播后的临时关联清理与旧对象的全部后续回收不由本函数单独证明。 |
| Grid | `PRIOR_STATIC`：Room 的 `+0x24 + index*4` 是独立 Grid 指针槽；`0x7EBCA0` 创建路径会复用同槽时先 Free 旧对象，再写入新 Grid 并 Init；`0x81E980` 是立即移除路径；Room 析构也释放 Grid。 | Grid 不是 EntityList Entity；必须用自己的 slot/creation/removal 事件。指针槽同址不等于同一代。具体构造函数正常返回与所有间接创建调用者没有在本轮新导出中穷尽。 |
| Entity Remove/提交/释放 | `Remove 0x6ACB00` 改 exists/dead、分派原生移除回调和释放 Lua registry 关联，不负责摘表/释放对象。`EntityList::Update 0x4186C0` 在阶段条件满足时摘 update/render/wisp；main 清引用、置 invalid 后摘表；persistent 对 Type=1 且原 valid 的对象先清引用、置 invalid，再虚 Free(1) 后摘表。 | Remove 请求、exists=false、摘表、清引用、valid=false、析构和物理释放是不同事件。只有明确的新逻辑使用证据或已观察的对象域退休边界才结束本地关联；单次未枚举到不是析构证明。普通池槽通常保留内存，Player 某些提交路径才实际 Free。 |
| EntityList Reset / Room 变化 / Run 结束 | `0x418230` Reset 重建若干视图、按条件保留或移除对象，不等同全体逐个析构。Room/Run 生命周期入口与细节已在 tx-01-63/tx-01-66 及本轮 B 设计中分开处理。 | Room epoch、Run epoch 与本地对象逻辑身份分开。Room 变化时被保留的对象可延续本地对象 ID；发生明确池复用/新构造时增加 use generation。Run teardown 退休观察域及其未结关联，不据此声称所有地址立即释放。 |

### 2.2 Morph 对象身份

`sub_6C0F30`（VA `0x6C0F30` / RVA `0x2C0F30`，导出长度 `0x1C1`）是当前 NPC Morph 体。可见主体用同一 `this` 比较/写入 Type、Variant、SubType；若这些值相同且条件参数相同则返回 0，否则改分类、更新冠军/配置相关字段，随后执行多个虚回调和 helper。当前导出体没有直接写 `Entity+0x3EC` InitSeed 或 `Entity+0x3DC` DropRNG，也没有直接调用通用工厂或 `0x418500`。本轮早期曾把 `this[793]` 误读成 seed；更正为 `this+0xC64` 的冠军/配置相关字段，不能据此声称 NPC Morph 改写 seed。未穷尽的 helper/回调可能改变其它状态，所以不能进一步宣称 seed 永不变化。

`sub_6E30A0`（VA `0x6E30A0` / RVA `0x2E30A0`，长度 `0x759`）是 Pickup Morph 体。它在同一 `this` 上保存一批状态字段，执行可能消费 Pickup 自身 RNG 的条件分支，并经虚 Init/后续回调重建逻辑状态；当 `!KeepSeed` 分支成立时，直接更新 `this[251]`，即 `Pickup+0x3EC` 的 InitSeed，再调用同对象的虚 Init。可见主体没有通用 `Game::Spawn`/`EntityList::Publish` 调用。它存在大量保存/恢复和状态重置，不应把 Morph 当成纯分类写入；但对象地址不变的事实也不等于必须分配新身份。

**`DESIGN_PROPOSAL`：** NPC/Pickup Morph 记为当前 `object_id/use_generation` 上的 `classification_mutation` 或 `reinit` 事件，附变更前后快照。除非实际路径另有分配并成功发布新对象，不递增对象使用代次。Morph 回调中可能嵌套 Spawn，内外事件要分别产生候选/发布事件，不能因外层 Morph 事件代替嵌套新生。

### 2.3 可实施的身份事件语义与覆盖

**`DESIGN_PROPOSAL`：** 为当前采集器观察域分配不回绕的本地 `object_id`，并对同一物理槽每次确认的新逻辑使用分配新的 `use_generation`。身份记录至少有 `(run_epoch, object_id, use_generation)`；序号在耗尽前停止该观察域并明确报错，不能回绕重用。创建时冻结 `class/type/variant/subtype/init_seed` 等描述字段及“已观察创建事件序号”；分类、seed、PlayerIndex、Room ordinal 和地址是后续快照/关系属性，不是稳定 key。Room ordinal `Entity+0x20` 是房间序号，不是跨房/跨进程身份。

候选事件在工厂返回或 Player 分配后建立，成功 Init/发布路径正常完成时提交；未发布、空返回、控制流未正常返回的候选记为取消/不完整。观察启用前已存在对象标记 `first_observed`，出生序号为 unknown，不编造历史 ordinal。跨进程匹配只能使用冻结描述、创建事件序列和可证实的参与者映射作为候选；重复 seed、相同分类和状态相同都可能碰撞，无法唯一配对时输出 unmatched/ambiguous，不用地址补齐。

关系槽（Parent、Child、Target、SpawnerEntity、Familiar.Player，以及 Player SubPlayer/Twin/backup/replaced）保存原始指针/槽值和解析状态。同一同步观察批次内唯一指针映射仍不足以排除旧关系指向已复用地址；还需已观察写入/清理顺序或足以支持同代的事件，才建立本地引用。NULL、目标未枚举、已退休、地址复用歧义或不确定分别保留原因。不得用裸指针把关系跨代延续。

| 类别 | 支持的入口/覆盖 | 建议的提交与退休规则 | 未闭合范围 |
|---|---|---|---|
| NPC | 通用 `Game::Spawn`→factory→virtual Init→publish；NPC Morph `0x6C0F30` 是同对象变更 | 正常生成完成后开始使用代次；Morph 保持身份并记 mutation；明确复用或退休后结束 | factory 直接 caller 是否全部经 Spawn；所有 Morph helper/重入行为 |
| Tear | 通用 Spawn/factory/Init/publish 路径；由 `Type=2` 与 RTTI 区分 | 与 NPC 同一候选/发布契约；Remove 与提交拆分 | 直接创建及全部派生 Init 覆盖 |
| Projectile | 通用 Spawn/factory/Init/publish 路径；由 `Type=9` 与 RTTI 区分 | 与 NPC 同一候选/发布契约 | 直接创建、短生命周期事件是否完整观察 |
| Pickup | 通用 Spawn；同址 Morph `0x6E30A0`，`!KeepSeed` 更新 InitSeed 并虚 Init | 普通创建/复用在发布后提交；Morph 标为重初始化/分类 mutation；单独快照 seed 变化 | Morph 嵌套回调和所有旁路创建 |
| Familiar | 通用 Spawn；独立池来源；`Familiar.Player` 在 `+0x410` | 对象使用和 owner 关系分别建事件；替换 Player 时更新 owner 关联，不仅按 owner 变化判新生 | 全部 direct callers 和替换/释放完整顺序 |
| Effect | 通用 Spawn；池覆盖先 Remove/ClearReferences 后同址重用 | 新 Init/发布提交新 use generation；不能等析构 | 直接创建/复用 caller 的运行时覆盖 |
| Player | `0x9B9CD0` 普通与特殊分支；`0x9BCAA0` 恢复保留源；`0x9BEE80` 替换 | 普通玩家在成功发布后提交；special/backup 单独作为保留 Player use，不计活跃 roster；纯 index 交换不改变对象 ID；新构造对象新 ID，已登记目标重新发布延续其 ID | 载入/恢复的所有入口与异常清理 |
| Grid | `0x7EBCA0` 单独槽路径；`0x81E980` 立即移除；Room 析构 | grid slot reuse 在旧对象 Free 与新 Init/存槽后新代次；与 Entity 列表分开 | Room 内其它 Grid 更新/恢复入口未穷尽 |

### 2.4 Hook 入口与静态边界

常规 Entity 观测需区分“尝试创建”“取得旧池槽”“Init”“发布”“移除提交”；仅看 `0x6F0090` 会在 Init/发布失败时过早登记，单独看 `0x418500` 又无法说明调用来源/重复 Init。Player 至少另有 `0x9B9CD0` 与 `0x9BEE80`；Grid 是另一对象域；Morph 至少有 NPC/Pickup 各一条同对象变更路径。建议把发布当候选提交凭据，而不是唯一身份来源，并在不完整路径输出 gap。

新建入口必须使用真实 call-site/ABI 证据与项目已有 patch owner。父端设计记录的当前 patch 容量为 relative 17、pointer 12，已用 12/7，仅各剩 5 个；identity 一组如果需要超过余量，必须扩展容量/ordinal 目录并纳入同一事务，不得假设一个通用 Spawn wrapper 已覆盖 Player、Grid、Morph、Restore。阶段内部观察可以复用现有 wrapper，不等于身份入口也无需新 patch。调用约定、精确指令与安装方案以[采样接入设计](tx-01-71-sampling-design.md)和具体原始证据复核为准。

`UNKNOWN_RUNTIME`：真实场景中事件是否无丢失；Mod/虚回调是否重入或从旁路直接创建；异常/OOM 之后列表和临时关系的最终形态；启动采集前历史；不同进程创建顺序是否一致；跨进程是否存在唯一匹配。当前结论不把地址、seed、PlayerIndex、room ordinal 说成稳定跨进程 key。

## 3. E：三条原生记录链必须分开

### 3.1 详细 Entity framestate：真实 ring producer 是 `EntityList::Update`

`STATIC_CONFIRMED`：`EntityList::Update` VA `0x4186C0` / RVA `0x0186C0` 在普通 Room 更新调用中执行；Room 内嵌 EntityList 位于 `Room+0x1218`。此前的描述把 `Room+0x1310` 与 `EntityList+0xF8` 误写成两套详细 ring，现更正：它们是**同一个地址**（`0x1218+0xF8=0x1310`）。Room 更新 call-site `0x803E64` 的指令字节是 `8D 8F 18 12 00 00`（取 `Room+0x1218`），`0x803E70` 调 `0x4186C0`，字节 `E8 4B 48 C1 FF`。

该详细 history 位于 EntityList `+0xF8`，含 32 个、每个 24 字节的 frame-slot header；32 槽之后，head 位于 EntityList `+0x3F8`，count 位于 `+0x3FC`。每槽保存原生 frame 标记、一个动态记录 vector 的 begin/end/capacity 及两个摘要字；consumer 将这两个摘要字复制到传输 header。每个已记录 Entity 的记录宽度为 56 字节。producer 的窗口条件是 global Manager 的 `field[77046] != field[77047]`，即其内嵌 NetManager roster begin/end 不同；这只限定该详细 ring 的更新窗口，不能推广成所有 RNG-history 或所有 frame 输出的全局条件。

| 56-byte row offset | 来源/宽度 | 已确认的含义 | 覆盖限制 |
|---|---|---|---|
| `+0x00` / 4 | 当前 EntityList 更新迭代 | update iteration ordinal | 不是 Authority tick、Room frame 或跨进程 birth ordinal |
| `+0x04/+0x08/+0x0C` / 各4 | Entity Type/Variant/SubType | 更新时分类三元组 | 是可变快照，Morph 后可改变 |
| `+0x10` / 4 | Entity DropRNG 首 dword，即 Entity `+0x3DC` | 当前 Drop seed 的完整 32-bit raw word | 不是 Entity InitSeed（`+0x3EC`），也不是完整 16-byte DropRNG 状态 |
| `+0x14` / 4 | `sub_6EF1F0` 返回值 | 全局 MT 状态摘要 | helper 读取全局 MT 状态但首次可能懒写 CRC 表；未推进 MT。不是 Entity 自身 RNG 或完整 RNG state |
| `+0x18..+0x27` / 16 | Entity `+0x38C..+0x39B` | 更新前四个 raw dword（producer 的 pre 区） | 只能按原始字段保存，不将它擅自命名成 HP/flags/owner |
| `+0x28..+0x2F` / 8 | Entity `+0x33C` | 更新后 Position `(x,y)` | 浮点 raw bytes；不证明量化语义 |
| `+0x30..+0x37` / 8 | Entity `+0x360` | 更新后 Velocity `(x,y)` | 浮点 raw bytes；不证明量化语义 |

producer 把 row 关联到其 frame-slot vector，并计算原生摘要。已确认摘要输入包括 row ordinal、Type/Variant/SubType、DropSeed、post-position、post-velocity，以及帧结束的 global RNG 摘要；该摘要覆盖**不包含** HP、完整 flags、State/Timeout、Parent/Owner 引用、Grid、Player heart/backup 等本项目字段。具体 24-byte header 的两个摘要字在消费者协议中作为 GlobalSeed 与 framestate checksum 携带；它们不是每 row 另一个 56-byte 状态字段。

### 3.2 Online summary ring 与序列化流 CRC

这条链与上述 EntityList 详细 row ring 不同。NetManager 的 summary ring 由 `sub_90D640` 生产，frame checksum 来源于 `EntityList+0xF0`，即 `Room+0x1308`；`sub_90D950` 根据 frame 查找环内记录并返回对应摘要。`sub_910980` 的 ready 路径调用该生产/同步路径，随后递增 NetManager `+0x0C`。发送/持久状态记录为 12-byte tuple：

```text
native_frame : 4 bytes
frame_checksum : 4 bytes
serialized_online_state_crc : 4 bytes
```

第三字的来源链在 `0x90D771`：serializer 参数包含 NetManager `+0x1018` 与 global Manager `+0x14`；这证明输入是当前 Online/lobby 状态序列化源。`0x91ACC0` 对序列化缓冲执行压缩/复制相关处理，`0x683580` 对 stream 计算 CRC。由于没有闭合它覆盖整个 Game/Room/Entity 世界状态的证据，本文统一称 `serialized_online_state_crc`，不称 world-save checksum 或世界快照校验值；不展开完整 Lobby/schema，也没有执行 serializer 或 CRC。

### 3.3 Desync 收发、NetInputDevice 缓存和 dump consumer

`sub_8FE3A0` 是 `MSG_DESYNC_NOTIFY` 本地构造/转发处理，不是详细 ring producer。它通过 `sub_90B070` 找本地 NetInputDevice，先释放旧的 framestate 指针并清零保存摘要，再由 `sub_90D950(frame)+8` 取得该 frame 的 summary checksum；从 `Room+0x1310` 对应 EntityList `+0xF8` 详细 ring 复制匹配 frame 的 56-byte rows 和摘要 header 到 NetInputDevice。只有该 handler 的 `n2==0` 分支写 `desync_framestate.txt`。因此文件输出是后续 dump/consumer，不是原始更新时的生成点。

原先把缓存地址写成“Player+956”是对象基址误判。正确基址是 **NetInputDevice+956 decimal = `+0x3BC`**（framestate header/pointer）与 **NetInputDevice+960 decimal = `+0x3C0`**（`serialized_online_state_crc`）。Entity_Player 自身 `+0x3BC` 是 Parent 引用槽，完全不同对象域。NetInputDevice 构造 `0x9015B0` 安装 vftable 并清这两个字段；释放路径 `0x901B70` 清理/释放 `+0x3BC` 并清 `+0x3C0`。

远端解析在 `sub_8FF600`（从 `sub_8FED60` 调用），按 protocol version 2 解析 presence、frame、checksum/global-seed、count 和 `count*56` row 数据；它清理先前缓存并构造新的 NetInputDevice header/vector。`sub_8FD900` 将本地/远端 framestate header `[5]` checksum、`[4]` GlobalSeed 与 `sub_90D950(frame)+8` 的独立 summary checksum 比较，再调用 `sub_8FFC70` 做 row 位置差异输出和 `sub_9032D0` 做 RNG/额外 desync 日志。`sub_8FFC70` 将 row `+4/+8/+0xC` 打成 Type 三元组、`+0x10` 打成 Seed、`+0x14` 打成 GlobalSeed，并对 `+0x18..+0x37` 的 pre/post position/velocity 做报告；日志标签是 consumer 的显示名，不证明记录可逆或字段具有更强语义。

此前一条搜索线索 `0x95999E` 实为读取/比较全局 Manager `+0x4B3E4` 的 frame 字段，不是向 framestate 写帧的位置。应以 `0x4186C0` 详细生产链和 `0x90D640/0x910980` summary 生产链为证据，不把这个 cmp 当 writer。

### 3.4 RNG call history 真正写入与开关

`sub_6EEF60`（VA `0x6EEF60` / RVA `0x2EEF60`，范围到 `0x6EF1E5`）是 RNG history node producer；`sub_904E70` 是把链表写成 `desync_rng_history.txt` 的 dump consumer。`sub_9032D0`（调用点 `0x903670`）和上游 `sub_8FD900` 在 desync 差异处理中触发输出。不能把 dump 字符串或旧解析器当成 history 生产证据。

producer 的第一处分支决定是否记录：VA `0x6EEF8A` / RVA `0x2EEF8A`，bytes `80 3D 68 AC C7 00 00`，比较 `byte_C7AC68`。非零路径跳到 VA `0x6EEF93` 的 `rand()` 调用，bytes `FF 15 94 89 B1 00`，并直接返回；该次不进入此 helper 的自定义 MT 更新和 history 链。零值路径才推进自定义 MT、分配/追加 node、捕获 stack 并剪裁旧 node。因此不能写成“每个游戏 RNG 调用都在 history 中”，也不能声称该函数仅在 Online 模式执行：`sub_6EEF60` 函数体没有 Online mode gate；它由哪些调用者进入、以及 suppression flag 在那些窗口的全局覆盖仍有范围限制。

节点与操作的证据如下：

| 字段/操作 | 原生数据流 | 静态结论 |
|---|---|---|
| 分配与链表 | VA `0x6EF104` 调 `sub_A0F4C0`，bytes `E8 B7 03 32 00`，请求 276 bytes (`0x114`)；节点 `+0/+4` 链接前后节点 | 每次记录路径追加 linked-list node；不是固定长度逐帧数组 |
| 随机返回值 | node `+8` 写当前生成 uint32 | 保存该 helper 的输出；没有输入参数、所属 Entity 指针或 Entity seed 字段 |
| frame | VA `0x6EF157` bytes `8B 80 E4 B3 04 00` 读 Online/global manager `+0x4B3E4`，写 node `+0x0C` | 记录一个当前 manager frame 字；不能单独映射到 Authority tick / 每个实体 |
| 调用栈 | VA `0x6EF16A` 调 `RtlCaptureStackBackTrace(0,0x40,node+0x10,0)`；VA `0x6EF173` bytes `89 86 10 01 00 00` 将返回 count 写 `node+0x110` (`+272`) | 最多 64 个地址，实际 count 单独保存。没有保存原始调用参数，也没有对象到 node 的显式关联 |
| 清理窗口 | 当前 frame 来自 manager `+0x4B3E4`，阈值另读 `+0x4B3E8` (`field[77050]`)；旧节点 frame `< current - 2*field[77050] - 2` 时 unlink/free | frame-window prune，不是全历史、固定条目数或可重放日志；`field[77050]` 的业务含义不在本报告中猜测 |
| 抑制路径 | `sub_916770` 的相关合并消息 cases 在存在本地 lobby/profile 条件时写 flag=1（VA `0x916E51`，`C6 05 68 AC C7 00 01`），调用 `sub_948FC0`（`0x916E5E`），之后写回 0（`0x916E71`，`C6 05 68 AC C7 00 00`） | 这是已定位的 flag 抑制窗口；不能推出其它线程/路径永不改 flag，也不能把它等价成“所有 Online RNG 历史关闭” |

`sub_6EEF60` 的自定义 MT 更新本身会推进 RNG；每个历史 node 分配内存、捕获调用栈，剪裁旧项并释放。故它不是纯只读 getter。全局 flag 非零时则会调用 `rand()` 旁路，不创建上述节点。`sub_6EF1F0` 是另一个实体 framestate 字段 helper：它读 MT 状态形成摘要，可能懒初始化/写入 CRC 表，但不推进 MT；不要将它与 `sub_6EEF60` call-history node 混为同一个“RNG history”。

### 3.5 字段覆盖、时机与限制表

| 本项目观察域 | 原生详细 framestate 是否包含 | 变化/过滤和可比性 | 结论 |
|---|---|---|---|
| Entity Type/Variant/SubType | 有，row `+4/+8/+0xC` | 更新窗口内按 EntityList 更新迭代记录；Morph 后为当前值 | 仅类别快照 |
| InitSeed `Entity+0x3EC` | 没有独立 InitSeed 字段 | row `+0x10` 是 DropRNG `+0x3DC` 首 dword，不可冒充 InitSeed；Pickup Morph 可改 InitSeed | 不覆盖 |
| DropRNG `Entity+0x3DC` | 仅首 dword | 其余 12 bytes 不在 row 逐字段内；+0x14 是全局 MT 摘要，不补全 Entity 的 DropRNG | 部分覆盖 |
| Position/Velocity | 有更新前部分 raw state 与更新后的 pos/vel | 56-byte row 的范围见 3.1；未证明排序、量化或浮点规范化 | 适合解释部分运动不同，不是完整状态 |
| HP、flags、State/timer、归属/引用、Familiar owner | 没有 | producer 摘要 feed 也不包含这些字段 | 不覆盖，不能因 checksum 相同断言这些字段相同 |
| Player roster/heart/subplayer/twin/backup/replaced | 没有成组 Player snapshot；仅当其 Entity 更新记录落在被记录的更新迭代时可见普通 row | 保留槽、备用 Player、心量和 Player 专用关系不在逐项 row 模式 | 不覆盖项目 C 表的 Player 域 |
| Grid 448 槽与门槽 | 没有独立 Grid row 集 | EntityList 的 Entity row 不等于 Grid 槽；Room grid checksum 输入是否间接吸收它未在此生产路径证明 | 不覆盖 |
| frame/global RNG 摘要 | frame header 两个 summary 字；另有 `sub_6EF1F0` row 摘要 | summary header 与 row 属不同层；frame checksum 与 online serialized-state CRC 又属另一 ring | 只用于原生 desync 诊断，不作为项目 schema |
| RNG call history | 输出 uint32 + manager frame +最多64个 stack address /实际 count | 仅 `byte_C7AC68==0` 时走这条记录分支；按 frame 阈值裁剪；无显式 Entity/参数绑定 | 调试旁证，不是完整 RNG snapshot/replay log |
| Online serialized state CRC | 12-byte tuple 第三字 | 来源是 Online/lobby serializer stream；压缩/复制与 stream CRC；未证明全世界状态覆盖 | 不用于 C/S 采集或判同 |

**`DESIGN_PROPOSAL`：** 项目 collector 应独立复制自己的字段模型，不复用 Online Manager、NetInputDevice、desync ring、frame等待条件或原生诊断 checksum。原生 framestate 只能作为诊断背景与某些时序/字段差异的旁证；它不是可逆 snapshot，也不能代替本项目身份事件。

**`UNKNOWN_RUNTIME`：** Online 窗口中 `field[77046]!=field[77047]` 与实体记录覆盖的精确游戏模式语义、ring 满/跨帧覆盖时是否有丢事件、不同端 frame 对齐关系、出现多个同分类实体时如何与远端 row 一一配对、所有 Mod 是否参与 EntityList 更新、第三方对 suppression flag 的写入、stack unwind 后节点生命以及任何给定 CRC 是否在真实运行中匹配，均不能靠静态代码保证。本轮没有触发或验证这些路径。

## 4. 纠正记录

本轮收束了以下先前表述的误判/歧义，保留旧导出不改写：

1. `Room+0x1310` 与 `EntityList+0xF8` 是同一详细 ring，不是两个 buffer。
2. `NetInputDevice+956 decimal` 应写 `NetInputDevice+0x3BC`；其对象域不同于 `Entity_Player+0x3BC` Parent 槽。
3. 实际指令头 `0x95999D` 是读/比较当前 frame 字段，不是 frame 写入点；详细 ring writer 是 `0x4186C0`。
4. `sub_6EEF60` 的 RNG history 不受函数体内 Online gate 限制；`byte_C7AC68` 的 suppression/rand 旁路才是分支条件。
5. NetManager summary 第三字标为 `serialized_online_state_crc`，serializer 的可见输入是 Online/lobby 状态源；没有证据称之为全世界存档 checksum。
6. NPC Morph `this[793]`=`+0xC64` 不是 InitSeed；本体主体没有直接写 InitSeed/DropSeed。Pickup Morph `!KeepSeed` 分支才直接更新 `+0x3EC`。
7. `0x903670` 是 `sub_9032D0` 内部调用点，请求地址数为 28、唯一函数数为 27。
8. Replace 的目标 valid 偏移是 `+0x170`（十进制368）；目标是已登记保留对象时保持其自身 ID。当前指针唯一映射仍需事件顺序支持，不能消除关系的地址复用歧义。主智能体最终更正与补充见[生命周期和原生记录报告](tx-01-71-lifecycle-and-native-records.md)。

