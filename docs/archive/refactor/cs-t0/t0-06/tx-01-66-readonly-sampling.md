# tx-01-66：只读世界快照采样静态依据

日期：2026-10-18。范围：T0-06 位置、速度及完成只读采样所需的最小静态依据。GPT6 Luna / max 子智能体完成初稿和 IDA 导出，主智能体重新读取关键指令、补全控制流并修订；整体静态文档审阅见[本轮记录](tx-01-66-review.md)。没有实现采样器，没有运行游戏、构建或代码测试，也没有修改 IDB 或运行时代码。

原始 [export_tx_01_66.py](export_tx_01_66.py) 生成的 [ida-evidence.json](../../../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json)保留了 35 个函数、21 处调用点上下文及 8 个实体虚表。主智能体另存 [primary-evidence.json](../../../../../artifacts/cs-t0/tx-01-66-static/primary-evidence.json)（11 个函数、11 段指令与 Room 正常控制流）及 [control-flow-evidence.json](../../../../../artifacts/cs-t0/tx-01-66-static/control-flow-evidence.json)（driver 控制流和实际转场 owner），分别可用[复核导出脚本](export_tx_01_66_review.py)和[控制流导出脚本](export_tx_01_66_control_flow.py)重复提取到新文件。原始 JSON 未覆盖；初始脚本已移入本主题目录，注释、code-item 筛选和后续导出的转场目标已修正。以下 VA/RVA 均针对 imagebase `0x400000`。

字段访问与调用顺序标为 `STATIC_CONFIRMED`；采样点的适用性属于这些事实支持的静态推导；以下 marker、epoch、复制和跳过策略是 `DESIGN_PROPOSAL`，当前没有相应采样实现。未知场景保留为 `UNKNOWN`，不把全部 writer 或生命周期状态机的穷尽作为本轮研究要求。执行规范见[现行项目规范](../../project-conventions.md)。

## 可采用的采样边界

建议只为标准完整房间模拟生成“全房间快照”：要求同一次外层更新确实走过标准分支，`Room::Update` 正常返回，且随后标准尾部和 `MC_POST_UPDATE` 正常完成；随后在顶层更新已返回、render 尚未开始时复制所有目标容器和字段。该结论不表示每个容器成员都在本轮被更新，也不表示成员之间具有相同模拟年龄。

不要把 `StandardRoomExit`、单个 `Entity::Update` 返回、顶层更新无条件返回，或输入 scheduler 的 previous 提交直接称为“整个世界最终状态”。`6FADC0` 在到达 Room 更新前存在早退；标准 Room 更新本身也只提交其局部 staged remove；`MC_POST_UPDATE` 仍可后续触发 Remove。采样应保留当时的来源成员关系和对象 `valid/exists/dead` 状态。

## 位置与速度字段

| 语义 | 地址/类型 | 当前 IDA 依据 | 采集含义 |
|---|---|---|---|
| `Entity.Position` | `Entity+0x33C`：`x:f32`；`+0x340`：`y:f32` | `0x866960` 注册 `Position` getter `0x417290`、setter `0x4288F0`；getter 将两个连续 32-bit 分量复制到返回 `Vector`，setter反向写入 | 直接复制 8 字节；不要调用 Lua 属性 getter |
| `Entity.Velocity` | `Entity+0x360`：`x:f32`；`+0x364`：`y:f32` | 同一注册函数的 getter `0x4172D0`、setter `0x428910`；访问器直接读写两个连续分量，调用代码按 float 运算 | 直接复制 8 字节；它是当前速度字段 |
| staged/可恢复位置 | `Entity+0x344`、`+0x348`：两个 32-bit 分量；标志 `+0x175` | `Entity` 基类 vtable slot 4（字节偏移 `+0x10`）指向 `0x6B0EB0`。低位 flags `+0x168` 的掩码 `2` 已置位时，该函数清除此位并直接返回；否则把当前 `+0x33C/+0x340` 复制到 `+0x344/+0x348`，置 `+0x175`，再按速度和倍率改写当前位置。`0x954CD0` 标准分支仅对 update list 中标记对象把 staged 值写回当前位置并清标志 | 不是另一个当前 `Position`，也不据此称为上一模拟帧位置。`+0x10` 按当前数据流称“位置阶段/slot4”；旧注释中的 `Interpolate` 不能证明它是纯渲染插值 |
| `Player.VelocityBeforeUpdate` | getter `0x6DAD60` 返回 `Player+0x1660` 内嵌向量的地址 | 原始指令是 `lea eax,[ecx+1660h]`，不是读取一个 pointer 成员；Lua 注册名为 `GetVelocityBeforeUpdate` | 与 `Entity+0x360` 的当前速度不同，不能替代它 |
| `Entity.PositionOffset` | `Entity+0x34C`、`+0x350` | getter `0x417310` 返回 `this+0x34C`；setter `0x4172F0` 写两个分量 | 与 `Position` 分开记录；不是当前世界位置 |

上述访问器只证明属性映射和原始读写方式，不构成所有 Position/Velocity writer 的闭包。

Grid 没有使用 `Entity.Position` 字段。`Room+0x24` 起的 448 个 32-bit slot 是独立 Grid 来源。`Grid Position` getter `0x709DF0` 读取对象自身 `Grid+0x24` index，并从当前全局 Room 的 `+0x0C` 读取宽度；`Room::GetGridPosition 0x812F00` 使用调用方传入的 index 和该 Room 宽度。其公式为 `x = 40*(index % width)+40`，`y = 40*(index / width)+120`（原生 signed 整数除法）。复制方案应同时记录来源槽号和对象存储的 index，不能假定二者永远相同；在已确认同一 Room 且 width 为正时，可在自有缓冲中计算坐标，不调用原生 getter。常量 40、120 已按原始字节确认。Grid 没有在本轮定位到通用 Velocity，不能以零冒充字段值。本轮不扩展到各类 Grid 派生模型。门视图在 `Room+0x724` 有 8 个 slot；普通造门路径可以让门视图与 Grid slot 指向同一对象，见 [tx-01-64](tx-01-64-category-coverage.md)。

## 标准完整模拟路径与顺序

当前 IDA 的 `_main` `0x931050` 在 `0x9312EC` 调用顶层更新 `0x9AB6D0`，返回地址 `0x9312F1` 同时也是随后调用 render `0x9555C0` 的指令地址。`0x9AB6D0` 只在 `0x9AB6F2` 调用一次 `0x954CD0`，随后返回。主智能体另对 `0x954CD0` 正常控制流检查：标准 call block `0x95522F` 与 odd call block `0x955241` 均不存在返回自身的路径，也不能从一者走到另一者。因此在该正常控制流内，这两类调用至多择一执行一次；这不覆盖任意间接调用或 Mod 重入，观察 token 仍需记录嵌套和调用次数。该次更新可能是标准完整模拟、奇数帧 Player-only 模拟，或没有进入模拟路径，不能仅以外层调用计数作为模拟 tick。

`0x954CD0` 检查 `Manager+0x4ABBC` 的最低位：

- 偶数分支先遍历 Room update list（`Room+0x1218` 中 update descriptor 的 data/size），对 `Entity+0x175 != 0` 的对象执行 `Position(+0x33C/+0x340) = staged(+0x344/+0x348)` 并清 `+0x175`，之后在 `0x955231` 调用 `0x6FADC0`。
- 奇数分支检查 `Manager+0x2A3C0`；非零才于 `0x955241` 调用 `0x6FD3F0`，否则该路径没有模拟调用。

仅看到 `0x955231 → 0x6FADC0` 不够：`0x6FADC0` 开头调用 `0x68BBA0`，真值即早退；函数中另有状态分支。完整标准快照的最小可见标记应限定为同一主线程、同一外层调用内的以下正常调用链：

1. 从 `0x954CD0` 标准分支的 `0x955231` 进入 `0x6FADC0`。
2. 确认该标准调用随后在 `0x6FBA4D` 进入 `0x802980`（`Room::Update`），并由 `0x6FBA52` 正常返回。Room lifecycle 的 `0x92FC77` 和普通换房路径的 `0x6FE37C` 也是 Room 调用点；其返回地址分别是 `0x92FC7C`、`0x6FE381`，不能冒充此标准调用。
3. `0x802980` 在 `0x803E62` 明确 `push 0`，再于 `0x803E70` 调用 `0x4186C0`（`EntityList::Update`），返回地址为 `0x803E75`。当前正常 CFG 的唯一 return block 是 `0x806884`；排除包含该调用的 block `0x803E62` 后，入口无法到达该 return，支持正常返回必经过这次列表更新。列表更新按自身条件提交 staged Remove；`0x4196A4/0x4196A8` 读取参数低字节，为零时于 `0x4196AC` 调用 `0x419E70`，再于 `0x4196B3` 调用排序/渲染次序处理 `0x419750`。这说明所选普通 Room 路径包含实体更新和空间/碰撞阶段；列表提交仍只处理这次局部提交，不闭合所有 Remove 时点。该正常 CFG 结论不扩展到异常展开或未解析间接重入。
4. `0x6FADC0` 在 Room 更新返回后继续 `0x9BB5D0`、`0x4257B0` 和其它后处理，并于 `0x6FBBA5` 调用 `0x8607A0`（标准尾部 `MC_POST_UPDATE`）。要求 Room 更新与该尾部均正常返回，再要求 `0x6FADC0` 和外层 `0x9AB6D0` 正常返回。
5. 在 `UpdateHook` 内，`g_update()` 已正常返回、但尚未调用现有 `g_top_update_exit_observation`（`coop::TopExit`）前，放置单独的只读复制点。当前 `TopExit` 会创建/注册设备、处理退休和控制状态等写操作；不能把调用该函数本身当作只读采集。

这个选择把复制点放在本轮 render (`0x9555C0`) 之前，也晚于标准 Room 更新、尾部后处理及 `MC_POST_UPDATE`。它不保证每个成员的 `+0x33C` 都在本轮被全房间更新：update list 的 staged 恢复只覆盖该列表中的标记对象；main/persistent 等其它来源可能有不同更新年龄；回调还能在局部提交后新建或 Remove 对象。快照必须按来源容器记录成员，不应把 `exists == false` 等同于已摘表/已析构，也不应调用 Update 或清理来整理列表。对象仍在当前有效容器中时，记录 `valid(+0x170)`、`exists(+0x172)`、`dead(+0x173)` 与字段；不要跨采样阶段或换房缓存对象指针。Remove 的局部提交证据与析构边界详见 [tx-01-63](tx-01-63-entity-lifecycle.md)。

## 奇数帧 Player-only 路径

`0x6FD3F0` 先经过 `0x68BBA0` pause/state-machine 检查及 `0x6FD350` gameplay guard；普通 Player 更新调用在 `0x6FD609`（`vtable+0x0C`），之后对该 Player 集合调用 `vtable+0x10`，实际 callsite 为 `0x6FD62D`。基类 slot 4 `0x6B0EB0` 会写 `+0x344/+0x348`、置 `+0x175` 并改变 `+0x33C/+0x340`，因此它有可见的位置阶段副作用，不能只凭 `Interpolate` 名称将其当作只读渲染步骤。

该 helper 不走 `0x954CD0` 的全 update list staged 恢复，也不执行标准完整 Room 更新与标准尾部。若产品需要，可另行定义带 `odd-player-only` 标签的 Player 增量记录；本任务建议不从该路径输出“全世界模拟快照”。这意味着最小全房间快照并不覆盖每个 60Hz Authority/input tick。

## 只读复制源与禁用查询

在主线程同一边界一次性读取 root、Room、容器描述符/成员指针和字段；复制前后复读 root/Room 与 lifecycle epoch，任一变化就丢弃整批并在下一个稳定边界重新获取。该策略是设计建议，不是当前代码实现或 runtime 安全结论。

| 直接读取源 | 已知布局 | 建议复制内容 |
|---|---|---|
| Game root | 全局 slot `0xC71678` | 当前 `Game*`；每批开始和结束都重读 |
| Room | `Game+0x18300` | 当前 `Room*`；每批开始和结束都重读 |
| Room `EntityList` | `Room+0x1218` | 对象枚举以 main/persistent 为基础，update 为更新视图；其余视图按需要补记来源，不要求最小实现全部读取。descriptor 16 bytes：flag `+0`、data `+4`、capacity `+8`、size `+0xC`；复制 `data[0..size)` 的指针。slot：main `+0x20`、persistent `+0x30`、update `+0x40`、render `+0x50`、effect `+0x60`、temp `+0x70`、buffer `+0x80`、wisp `+0x90` |
| PlayerManager vector | `Game+0x1BAA8` embedded vector | begin/end/capacity 三个指针及 `[begin,end)` 成员；保留与 Room entity list 的别名/来源关系，布局见 [tx-01-64](tx-01-64-category-coverage.md) |
| Room Grid slots | `Room+0x24`，448 slots | 一次复制全部 slot 指针，记录来源槽号及 `Grid+0x24` 存储 index、当前 Room `+0x0C` width；可在自有缓冲派生坐标，不假设独立 Entity Position |
| Door view | `Room+0x724`，8 slots | 单独保存 door-view membership；同一 Grid 门对象可由两个视图指向，不重复解释成两个对象 |
| Entity row | 上述容器成员指针 | 直接复制 `+0x33C..+0x343` Position、`+0x360..+0x367` Velocity 和 `+0x170/+0x172/+0x173` 生命周期字段；保存来源容器/slot。需要一批内对象去重时以地址关联，但保留每个来源 membership |

不要调用 `0x86FB20` `GetRoomEntities`：其路径创建 Lua table 与 userdata。不要调用 `0x41AF60` `QueryType` 或 `0x41A790` radius query：它们会使用/修改 scratch、cache 或统计数据且会过滤集合。不要调用 Lua 属性 getter、Update、Remove、清理或任何用于重建容器的函数；也不为判断能否采样而重新调用 `0x68BBA0` 等原生状态机。直接复制 descriptor 和成员指针后，在同一阶段读取字段；不要逐实体跨回调/阶段采样。

最小实现仍需正常数组边界保护：EL 的 size 不超过 capacity、非空数组必须有 data；Player vector 的 begin/end/capacity 顺序和元素跨度须有效。自有缓冲不足、读失败或来源异常时标记整批不完整，不静默截断为完整样本。指针仅用于同批别名关联和诊断，不据此建立操作 4 的实体身份或网络 key。以上是尚未实现的复制规则，不是本轮已验证的运行时行为。

主线程依据是 `_main` → `0x9AB6D0` 路径以及当前 `UpdateHook` 在 `g_update()` 返回后调用 observation 的结构；代码入口还以 `EstablishOrCheckMainThread` 限制观察线程。采样器应在同一线程执行同步复制，不要把复制任务异步排队后跨生命周期读取。此处仅为静态线程路径依据，没有做并发或安全运行验证。

## 暂停、加载、换房与 teardown 策略

| 情形 | 采样处理建议 | 可见条件/边界 | 静态限制 |
|---|---|---|---|
| Pause / 本轮未模拟 | 不输出全房间快照；要求本轮精确标准 Room 更新链标记齐全 | 标准 `0x6FADC0` 在进入 Room 更新前调用 `0x68BBA0`，可能早退；缺少 `0x6FBA4D → 0x802980` 正常返回即不采。奇数 helper 也有 `0x68BBA0`、`0x6FD350` guard | 全局 pause 的全部条件未闭合。不得用 `PauseGapTracker` 的连续空 Authority 计数替代本轮原生模拟证明 |
| Loading | loading bracket 内跳过；旧 Room 代次退休；退出后重新读取 root/Room | 当前 lifecycle callback 将 `AlternateEnvelope` 入/出标为 loading；源码 hook 对应 owner `0x959720`，tail `0x864EE0` | 未闭合全部 loading/FSM 进入条件；只对观测到的 bracket 作条件依据，其他状态 UNKNOWN |
| 普通换房/换层 | transition bracket 内跳过并退休旧采样代次；退出后重新读取 root/Room | lifecycle callback 将 `RoomLifecycle`、`RoomChange` 当作 Room 域；标准 FSM owner 调用点 `0x6FB4F0→0x92F1C0`、nested Room 调用点 `0x92FC77`；door-room-change owner 调用点 `0x6FB51C→0x8318A0`、nested Room 调用点 `0x6FE37C`；新房间入口 hook target `0x865180` | 这些是已定位路径，不宣称穷尽所有传送/房间迁移算法；其它路径 UNKNOWN。初始导出选取的 `0x6FE2F0` 不是 `0x6FB51C` 的目标，实际目标已补充导出 |
| Menu / 无活动 Game | 不采；Game root 或 Room 为空时不保留旧指针 | 只有存在当前 Game、Room 且本批标准链完成才可能输出 | 完整 menu/run FSM 状态条件没有闭合；不能只凭某个 HUD/render 状态值判定 |
| Run teardown | teardown 入口立即退休当前采样代次；新 run 必须重建 root/Room 代次 | run teardown hook target `0x6FA0C0`；当前 lifecycle callback 在 `RunTeardown` entering 时退休会话 | 何时从菜单建立下一 run 的完整条件 UNKNOWN；不能复用旧 Room/entity 指针 |
| 复制期间 root/Room/lifecycle 改变 | 丢弃这批所有字段，不发布部分 snapshot；下次稳定边界重新读取 | 批前、批后比较 root、Room 和本地观察 epoch | 这是最小实现的防陈旧设计建议，静态分析没有验证其实现正确性 |

标准路径标记应是一次外层调用内的 thread-local/nesting 证据，不是单个跨帧 bool。建议记录同一次调用 token 下的标准分支进入/返回、普通 Room 进入/返回和标准尾部进入/返回计数；正常模式各有一对且嵌套匹配才出样本。Room 的普通 caller 返回地址必须是 `0x6FBA52`，标准尾部 caller 返回地址为 `0x6FBBAA`；token 在本次复制后即消费，没有更新的循环不重发旧样本。换房/loading brackets 在同一期间有任一进入/退出、异常嵌套或 epoch 变化，都使该批无效；只比较前后相同指针不足以发现地址复用，因此 epoch 应是采样器自身的生命周期观察序号。这个方案只选择标准完整路径，不承诺每个 Authority tick 均有快照，也不把生命周期追踪包装成全状态机覆盖。

## 证据边界

当前 IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，输入 `isaac-ng.exe`，imagebase `0x400000`，x86；本稿使用当前 banner `Repentance+ v1.9.7.17.J460`，主智能体再次读取了 `0xB7E1B4` 的原始字符串。镜像仍含旧 `1.9.7.16` 字符串，因此不混用旧构建地址。完整相关伪代码、原始指令和 vtable 槽值在 [ida-evidence.json](../../../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json)，更正及正常控制流另见上述两份补充证据。历史容器布局和实体生命周期依据分别见 [tx-01-63](tx-01-63-entity-lifecycle.md) 与 [tx-01-64](tx-01-64-category-coverage.md)。

尚未静态证明：每个来源容器中的实体在当前标准 tick 都刚刚模拟；未进入 update list 的位置都与上次 render/位置阶段完全无关；全部 pause/loading/menu/传送条件；本建议运行时无竞态或安全。当前报告明确保留这些限制，没有闭合全部 writer，也没有以 runtime 样本作为研究前置条件。

本轮完成的是字段定位、普通标准路径的统一候选阶段和最小复制/生命周期建议。HP 研究仍见 [tx-01-65](tx-01-65-hp-analysis.md)；其它 state、timer、owner/parent、身份和采集器实现未在本轮推进。上述未知不改变已恢复的正常路径事实，也不被写成已实现功能。
