# tx-01-73：T0-06 实现后 E2E 测试清单

2026-10-19 当前清单（tx-01-88）：按用户要求永久移除 W01.isolation，当前共 32 子项；C/S 不进入原版 Online/Isaac Sync，不再要求真实在线窗口。W01 其余开关/ABI/恢复检查保留，旧 33 项结果均为历史记录。W08 性能修复和唯一全量离线验收 PASS，当前清单累计 **32/32 PASS**（30 项历史沿用，W08 两项本轮检查），无剩余测试项；见[最终结果与证据](tx-01-88-e2e-results.md)。下文历史 PASS/排除按原运行时点理解。

最新运行（2026-10-19）：tx-01-79 **INCOMPLETE：21 PASS / 0 FAIL / 12 NOT_COVERED**，补齐八类字段和独立失联恢复；消费暂停只完成 W12 的部分分支。18 PASS 历史沿用、3 项本轮实际检查，前两个注入前观察器 FAIL 保留。全量 replay 仍排除，完整未覆盖表见[本轮报告](tx-01-79-e2e-results.md)。下文“最新/当前/NOT_RUN”均为较早交付或运行的历史语境。

最新运行更新（2026-10-18）：tx-01-75 的连续完整计数 13,567、背压 0、消费管线及正常恢复子项通过；用户在第一次全量 replay 期间选择结束，最终 INTERRUPTED（10 PASS / 0 FAIL / 23 NOT_COVERED），完整 E2E 未完成。见[本轮报告](tx-01-75-e2e-results.md)，不以旧轮次已通过项替代本轮未执行项。

2026-10-18 后续实现补充：tx-01-75 修改即时消费，未来统一验收还需检查通知合并、复制后释放共享槽、队列满时背压和停止时排空/flush，保存 `world/consumer.json`。具体准备、预期及失败判据见[消费端修改说明](tx-01-75-immediate-consumer.md)。本次没有执行新一轮验收。

当前执行更新（2026-10-18）：tx-01-74 已按统一入口执行一次，整体 **FAIL**，32 子项为 17 PASS、1 FAIL、14 NOT_COVERED；W08/W09 整组通过，W07 未达标，其余组均有未覆盖项。见[本轮逐组结论](tx-01-74-e2e-results.md)及[原始报告](../../../../../artifacts/cs-t0/tx-01-74-e2e-01/result.json)。下文 NOT_RUN 为 tx-01-73 及执行器初次交付时的历史状态；未勾选的准备项不等同于本轮完全未执行，以报告为准。

日期：2026-10-18。状态：**清单已编写，全部用例 NOT_RUN**。本清单在采集器、身份、编码/比较命令实现后编写；本轮没有执行其中任何命令、游戏操作或测试。代码与接口见[实现说明](tx-01-73-implementation.md)，执行约定见[现行规范](../../project-conventions.md)。

## 1. 未来统一执行安排

2026-10-18 tx-01-74 补充：现已新增[统一执行器](tx-01-74-unified-executor.md)，采用用户确认的人工场景检查点。该执行器本轮未运行；本清单和各用例仍全部 NOT_RUN。下文“本次只交付清单”描述 tx-01-73 当时的交付范围。

- 整个目标完成后，由**一个统一入口、一次执行**组织下列全部适用用例；准备场景、采集、复制副本、调用生产 CLI、整理结果都属于这一轮。不把各小节分别试跑，不自动重跑失败项。tx-01-73 当时只交付清单；tx-01-74 的执行器见上述补充。
- 执行前记录实际授权：目标游戏/版本、输入、允许的 Join/B 退休、换房/暂停/死亡恢复/菜单操作及持久副作用范围。两个游戏分别运行在不同机器，或先后启动为两个不同进程；当前 controller 仍要求本机恰好一个用户启动的目标，不擅自放宽。
- 使用本次纯编译产物或未来整体验收入口中的明确构建步骤。不要调用旧 `build.ps1`、verify 链、单元测试或含哈希/gate 校验的入口。
- 探讨期不做进程检查/预注入试跑。正式窗口内记录 PID、创建时间、实际模块、输入来源、角色、run/room 与场景标记。未知 ABI/线程、恢复失败或事件丢失保留失败记录，不强制卸载、重注入或继续写世界。
- 需要受控异常的条目只能修改**采集器自有副本/资源或 controller**，不得破坏游戏指针、RNG、容器、存档或未知全局状态。真实场景和已授权手段不能覆盖时记 NOT_COVERED，不能用离线副本冒充 native 失败分支已覆盖。

一次执行的结果目录建议为 `artifacts/cs-t0/<one-invocation>-t0-06-e2e/`。所有子目录 create-new，统一 manifest 记录开始/结束、实际授权、工具命令、输入脚本、场景、用例编号及产物路径；不记录或校验哈希。

## 2. 实机采集与生命周期

### W01 — 可选开关、版本与原生调用保持

- [ ] 准备：同一构建的 controller/Native/Helper，已授权的本地 Authority 场景，原有输入脚本。
- 操作：统一入口先记录未启用 `--world-record` 的对照，再在独立目标进程启用 `--world-record min-read-v1`；检查原生 ABI/返回值和调用次数保持。真实 Online/Isaac Sync 场景已从当前清单移除。
- 预期：默认仍为 coop v3/51 槽；可选模式为 v4 prefix 扩展、独立 world mapping、70 槽。原版 channel 0、Isaac Sync 0xFD、正式 API/IPC 和 scheduler 输入语义不被世界记录器改写。
- 失败判据：默认启动世界采集/新增 19 槽，覆盖旧 TopExit 单槽，模式冲突仍激活，采集失败改变原生返回值或停住 scheduler。
- 产物：`W01/default/`、`W01/world/` 的 invocation、输入、loop/co-op/world 事件、模式与恢复记录；不要求在线窗口，不将被移除项记为 PASS 或 NOT_COVERED。

### W02 — 完整阶段与 Authority tick

- [ ] 准备：可正常运行的 A/B，足够完整的普通更新；同轮场景安排 odd、暂停/恢复、loading/换房和 teardown。
- 操作：按标记完成普通移动/射击、静止、暂停恢复和已授权的生命周期转换；保存原始 loop 及 world 记录。
- 预期：每份完整样本对应同 outer token 的 owner→Room→tail 正常完成，复制位于 top update 返回后、coop TopExit 前；本次普通提交的 tick/kind/scope 精确对应。odd-only 为 gap，不生成全世界状态；无提交标 unavailable，禁止贴最近 tick。
- 失败判据：重复/缺失 Room 或 tail 仍为完整样本；跨 token 拼接；生命周期进入后批次复活；render/墙钟当 Authority tick；odd 被当完整世界。
- 产物：`W02/capture/`、场景标记及逐 token 对齐表，列出每个 gap 的原始原因、attempt/capture 序号。

### W03 — 八类字段、来源与别名

- [ ] 准备：授权窗口中可自然出现 Player、NPC、Tear、Projectile、Pickup、Familiar、Effect、Grid 的场景；不能出现的类别提前列为待覆盖。
- 操作：采集八类、八 EL 视图、roster/Esau/保留 Player 关系、448 Grid 槽与 8 门槽。选择门与 Grid 指向同对象的样本。
- 预期：按固定 field ID/位宽保存原位值；同批别名合为一个对象并保留全部来源。未知类别明确输出已知头部；Grid 不套 Entity 字段；专用 State/Timeout 未确认项保留 manifest 声明，不补零。
- 失败判据：未知类别被丢弃或自动归 NPC；门重复计对象；447 槽截断；将 vector 的 +C 当 size；读 getter/查询/RNG/原生诊断 producer 代替值复制；缺字段却删减 profile。
- 产物：`W03/class-coverage.json`，逐类原始样本/字段/来源索引，别名对照和未覆盖原因。

### W04 — 临时身份、创建完成及短命对象

- [ ] 准备：覆盖普通 Spawn、虚拟 B 创建、Grid/door 替换、可观察的池复用、嵌套生成与短命实体。
- 操作：保留 factory→Init→publish→owner 返回的完整事件；观察重复发布、Remove/清引用、同址再用及明确析构/替换。
- 预期：同次创建只确认一次，发布不冒充 Spawn 后处理已结束；嵌套对象独立，短命对象即使没有相邻快照也保留事件。重复发布/Remove 幂等；同址新 use 不复活旧身份。物理对象、use、membership 与退休分别解释；来源覆盖完整时保存 membership bitset 观测，空 membership 不当作物理销毁。
- 失败判据：以地址/seed/Index 作为跨代身份；每次 Init 都计出生；一次缺枚举即销毁；失败候选伪造出生；清引用或 Remove 一概等同释放。
- 产物：`W04/lifecycle-cases.jsonl`，每例对应事件序号、local_id/use、冻结出生描述和相邻样本；未真实触发的复用/短命路线记 NOT_COVERED。

### W05 — Morph、Restore、Replace、房间/run 退休

- [ ] 准备：已授权且实际可达的 NPC/Pickup Morph、Player Restore/Replace、房间和菜单场景。
- 操作：观察分类/seed 变化、同对象 Init、保留 Player 重新发布、普通门过渡和 run teardown。
- 预期：已知 Morph 保留本地 use 与冻结出生描述，当前分类/seed 单独变化。Restore/Replace 批次作废，保留对象不因地址不同一律算新生；未知连续性撤销映射。嵌套房间信号合并事务，run 退休后旧 key/关系不复活。
- 失败判据：Morph 重写出生 key；恢复旧输入/对象关联；相同指针被当作生命周期未变；持久 Player 因换房自动换物理 ID；保留对象被当活跃 roster。
- 产物：`W05/transactions.jsonl`，逐事务进入/退出、epoch、对象关系和首个恢复后样本。不能自然覆盖的特殊角色/Restore 路线不能由普通换房代替。

### W06 — FIRST_OBSERVED 与用户采纳的 Player 读取规则

- [ ] 准备：A 在启用前已存在，B 在启用后创建；有一次绑定失效/重绑窗口。
- 操作：记录首批 A 的状态，随后记录有效 Authority 窗口中原生 PlayerUpdate 正常返回前后的 exact pair/generation；执行已授权的绑定失效，再观察重新取得依据。
- 预期：A 始终保留 FIRST_OBSERVED，不补出生序号；满足规则后只把当前字段可读依据记为 `3/USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS`。绑定失效撤销该依据；错线程、无 tick、暂停/loading、异常返回或 generation/绑定变更不能赋予该依据。B 的已知构造/创建 owner 证据独立保存。
- 失败判据：将 Update 记录成历史 Init/出生；roster 下标直接产生备用 Player 语义锚；B 退休后残留 ordinal；通过本规则把 odd 更新升级为完整世界样本。
- 产物：`W06/readiness.jsonl`，初始化/读取依据、PlayerUpdate witness 事件、origin/birth/ordinal 和绑定前后值。

### W07 — 连续 10,000 次完整采集与只读行为

- [ ] 准备：同轮前段已取得字段读取依据、场景稳定且在操作授权范围内；记录 warmup/不可比较样本，不删掉它们制造连续性。
- 操作：保持一次捕获窗口，在同一稳定 run/room 取得连续 10,000 次完整普通采集。并行保存输入、loop/native 调用次数、线程/阶段、来源和对象 RNG 的原始值及实际读取路径记录。
- 预期：capture/attempt/token/tick 各自含义正确。tx-01-74 用户于 2026-10-18 确认：明确的 odd-only gap 保留记录、不计次数，也不打断普通完整采集区间；其 Authority kind/tick、序号、run/room、mutation 和线程必须符合执行器的明确判据。其它 gap、字段/来源不完整、读失败、背压缺号或生命周期变化均打断区间。native 世界的改变可归因于正常模拟与输入；collector 仅读，没有额外 RNG、查询、lazy getter、创建/清理调用。
- 失败判据：不足 10,000 次却 PASS；把 odd/gap/NOT_READY 算完整；未明确的 gap 或缺号被当作 odd 例外略过；重复旧快照补足；只凭首尾两个值相同宣称整个窗口无 mutation；collector 导致线程阻塞或改变原调用次数。
- 产物：`W07/coverage.json` 指定精确序号区间、普通完整次数、区间内未计数的 odd-only 数量、全部原始记录和逐项依据，附采样耗时/背压实际观测及证据边界。当前执行器对应文件为根目录 `primary-coverage.json` 与逐记录的 `primary-record-index.jsonl`。不把静态代码检查当作这一运行结论。

## 3. 使用实际产物的离线生产 CLI E2E

以下属于同一次统一执行的后半段，使用 `world-replay` / `world-compare` 真实入口及文件 I/O；只改自有 artifact 副本，不改 native 世界或原 source。所有比较均保留完整固定 profile，不删除实体/字段来获得 equal。

### W08 — 冻结副本编码与解析复现

本轮结果：PASS；真实 1,310 条记录两次 replay 对照及宽整数保留通过。

- [ ] 准备：一份真实 `world/` 及其 manifest，两个新输出目录。
- 操作：对同一冻结输入分别调用两次 `world-replay`；保存两份 captures.jsonl，再进行逐字段与可读字节对照。保存解析后的 u64、raw32/raw64/bytes_le 和字段顺序。
- 预期：同字段、同顺序、同可读编码；u64 超过 2^53 时仍为精确十进制字符串；不落盘 C++ padding 或地址排序结果作为跨进程 key。
- 失败判据：重编码丢弃 gap/未知状态/原位值，精度损失，不确定顺序，源被覆盖，非法长度或字段被宽松接受。
- 产物：`W08/replay-a/`、`replay-b/`、逐字节/逐字段对照及 source 文件内容对照；不得用 digest/hash 代替。

### W09 — 已知字段首次差异及浮点规则

本轮结果：PASS；自有副本单字段定位、±0、NaN/±Inf 规则通过。

- [ ] 准备：有明确映射的实际样本副本；选一个已有 VALUE 的已知字段。保留原版对齐文件与未修改 source。
- 操作：只修改右侧自有副本的一项 raw32，例如 `position.x` 的一个有限值；调用 `world-compare`。同轮再用独立副本覆盖 +0/−0、另一有限位值、NaN 和 Inf。不得改 native 内存。
- 预期：已知不同值定位至 run/room/class/match key/field 与对应 tick；其它未知区域继续保留。±0 不产生该字段差异，原位值仍保留；NaN/Inf 为不可比较，不变成 0 或 equal。其他 finite 不使用 epsilon。
- 失败判据：只有总校验值；错字段/错 tick；已知差异被未知区域掩盖；相同 NaN 或相同缺项被判 equal。
- 产物：`W09/<variant>/` 的输入副本、变更说明、comparisons、result 和首次差异路径。整体 equal 只有在该固定 profile 全部条件满足时才允许，不能仅凭零值字段归一化断言。

### W10 — 冲突、引用、不完整、缺口和 decoder

- [ ] 准备：真实产物的自有副本及明确的逐项修改记录。
- 操作：同轮覆盖同 key 多对象、未映射 FIRST_OBSERVED、NULL/KNOWN_CURRENT/UNRESOLVED/RETIRED/AMBIGUOUS_REUSE 引用、READ_FAILED/NOT_READY、缺源、缺 tick、重复 tick、非同阶段，以及错误 schema/version/字段 ID/位宽/长度/十进制字符串/多余载荷。
- 预期：冲突保存整组；引用目标冲突也不可比较；缺失存在性为 PRESENCE_UNKNOWN；缺映射为 ALIGNMENT_UNAVAILABLE/IDENTITY_UNRESOLVED 或对应 gap；非法 wire-like 文件载荷拒绝解析并保存 failure。确定的其他字段差异仍可输出 DIFFERENT，同时保留未比较区域。
- 失败判据：按地址打破冲突或就近强配；相同错误判 equal；静默跳过未知类别；错误状态被填零；错误 profile 被逐样本缩减；非法输入造成无结果却退出成功。
- 产物：`W10/cases.jsonl`、每例输入副本/修改记录及 CLI 输出目录、冲突组、failure/比较报告。上述离线 READ_FAILED 只验 decoder/analyzer，不冒称 native 访问失败分支已覆盖。

### W11 — 双进程同 seed/input、显式对齐与层级比较

- [ ] 准备：两个不同游戏进程的同 seed/input 场景声明、实际输入脚本与事件，不从 seed 声明推断读取了全局 RNG 初始 seed。
- 操作：保存双方完整 world 产物；根据实际 run/room、Authority tick 和创建事件建立一对一 alignment.json。通过真实 `world-compare` 比较明确范围，并保留没有映射的对象。
- 预期：当前 process address、vptr、thread、墙钟和容器地址不影响相等；按 run→room→类别→对象→字段给出结构化结果。允许不同或不可比较；有更早缺口时报告 first_observed_difference 与 earlier_coverage_unknown。
- 失败判据：本地 epoch 数值相等即视为同场景；seed 相同即强制身份；自动最近帧配对；只有总 checksum；跳过更早未知窗口却宣称全局首次因果分歧。
- 产物：`W11/left/`、`right/`、输入/场景与对齐依据、alignment.json、双方 manifest、comparisons、key-collisions 和 result。结论仅覆盖显式配对范围。

## 4. 失败行为、停止与恢复

### W12 — 预算、背压、读失败和观察故障隔离

- [ ] 准备：同轮、已授权且不改 native 世界的受控资源/consumer 故障安排；记录实际采用的手段。
- 操作：暂停或延迟 **controller 的消费** 以触发无 FREE；观察可达的对象/membership/关系预算与真实读取失败。对未获授权或不可达的 native 分支只列 NOT_COVERED，不临时修改游戏指针触发。
- 预期：producer 不等待后台；背压/预算/读取失败显式记录，attempt 缺口不冒充成功 capture。观察连续性丢失或错线程只停用自身采集；A/B scheduler 和原生调用仍按原路径运行。
- 失败判据：静默丢事件，覆盖正在读取的 buffer，后台解引用 native 指针，collector 调用 coop::Fail，回放旧快照补洞或冻结 Authority。
- 产物：`W12/faults.jsonl`、原生/consumer 时序、实际错误/背压计数、输入继续推进证据及每项覆盖状态。

### W13 — 70 槽正常停止、在途调用与控制器丢失

- [ ] 准备：至少一份正常停止材料，以及同轮安排的受控 controller 丢失场景；新 PID/目录分别记录，不擅自重注入同一不确定实例。
- 操作：正常 stop；在独立已授权场景停止 controller，让既有失联恢复处理接管。保存停止发布、B 退休、observer disarm、后缀与前缀恢复、在途收敛和 final drain。
- 预期：关闭 publication 后无新样本；19 入口与既有 51 槽同一 owner 恢复，总计 70 个目标原字节/值匹配；在途桥/观察完成后才 closed。DLL、trampoline 和自有设备按既有常驻策略有效。失败时保存未恢复/未关闭事实，不强制卸载。
- 失败判据：只检查 51 槽；遗漏 64 以上 ordinal；在途未收敛就 closed；恢复 owner/字节冲突仍报告成功；控制器丢失后继续未界定采集或重复重注入。
- 产物：`W13/<scenario>/restoration.json`、70 项目标记录、各入口进入/返回计数、callback/producer 状态、helper 日志及失联事实。未知异常展开不得以正常 stop 的成功替代。

## 5. 一次执行的可复查交付

- [ ] 统一 `manifest.json`、`cases.jsonl` 与 `result.json`：每条 W01–W13 及其子项列 PASS/FAIL/NOT_COVERED，关联实际产物与理由；本清单当前全部 NOT_RUN。
- [ ] 保留输入、场景标记、源码快照、构建命令/输出、游戏/Native/Helper 身份描述、原始 capture/gap/lifecycle、双方对齐依据、未修改 source 与修改副本。
- [ ] 比较结果、冲突、失败、首次观察差异和更早未知范围均能回到原始记录；保存生产 CLI 的精确参数、退出码和输出。合法比较得到 DIFFERENT/INCOMPARABLE 时 CLI 可正常退出，不能只看退出码断言状态相等。
- [ ] 保留真实恢复字节与在途/关闭状态。没有真实覆盖的字段/异常/worker/Mod 路线如实列出，不外推跨版本、全部武器或全量世界状态。
- [ ] 不自动重跑。失败记录和部分产物保留；后续修改与新验收按现行推进规范另行说明，不覆盖本次事实。
