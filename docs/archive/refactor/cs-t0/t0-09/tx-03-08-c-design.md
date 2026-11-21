# tx-03-08：T0-09-C observer、共享脚本与故障窗口工作设计

日期：2026-10-21（Asia/Shanghai）。状态：**DESIGN_COMPLETE / STATIC_REVIEW_PASS；生产实现 NOT_STARTED，P2 E2E NOT_RUN。**

本次推进 `tx-03-08` 的完整目标是完成 T0-09-C 的可实施设计、实施前失败方式和入口同步，预期交付可直接指导后续实现的职责、数据、时序、比较及验收约定。本次只修改文档，全部文档完成后只做一次静态文档审阅；不构建、不运行程序或游戏、不执行代码测试。后续实现另用一个推进编号说明完整目标，全部实现结束后仅执行一次统一 E2E。

依据：[现行项目规范](../../project-conventions.md)、[T0-09 总设计](tx-03-01-design.md)、[B 的正式 P1/F0 结论](tx-03-07-live-f0-formal-acceptance.md)、[本轮实施前失败方式](tx-03-08-c-failure-modes.md)。本文是 C 的当前工作设计；总设计中被本次用户决定细化的 C 范围、采样和验收安排以本文为准。B 的历史结果不改写。

## 1. 本次用户决定

以下三项均在本会话先询问用户并得到明确选择，记录为 `USER_CONFIRMED_DESIGN`：

| 决定 | 当前约定 |
| --- | --- |
| C 的验收规模 | 一次统一入口，按 F0→F1→F2→F3→F4→F5 顺序运行；F0 10,000 Authority tick，F1–F5 各一个 seed、各 5,000 tick。合计 6 个 case、35,000 Authority tick。 |
| 每场准备 | 每个 case 使用新的一对游戏进程；由用户重启双端并准备相同基线。统一 runner 保持一次调用，逐场等待准备。代理不自动退出、重启或终止游戏。 |
| 采样范围 | 复用现有普通 Room 完整采样边界；每个 Authority tick 有记录，普通完整样本比较，odd-only 明确记 gap，要求普通样本无遗漏。不新增 odd-only 专用 Player 采样研究。 |

完整每种故障至少 3 个 seed 的矩阵、双机 C0/C1、新鲜完整 C3 和 T0-09 最终报告仍属于 D。C 的 P2 PASS 不代表完整 T0-09 PASS。六个 case 是同一个推进的一次整体验收的必要用例，不分成六次验收，不提前试跑其中一场。

## 2. 目标与范围

完成 VM 的真实本地 A/B observer-simulator，与 Host 共享确定性输入来源，按逻辑 tick 生成最小状态观察，接入真实 UDP F1–F5，输出输入与首次状态分歧的有限因果窗口。

- Host 保持唯一 Authority，A 本地输入和 B wire 输入进入同一 canonical/Submit/queue/dedupe/Resolve/apply。
- VM 本地重放 A/B；只有 B 的输入副本经 wire 发送。VM 世界不接收 ACK/STATE 写回，本地世界不回写 Host。
- Authority 缺 B 输入时继续真实 gameplay tick，`max_input_age_ticks=0`，缺失立即中性，edge 不复活。
- C 只比较声明的 Player 最小 subset，完整世界、实体引用相等、snapshot/baseline/delta apply、预测、rollback、late join、Replica 和 Steam transport 均不在本次范围。
- C/S 保持独立 session、IPC、日志与可变状态，不进入或触及 Original Online / Isaac Sync；不增加 C2 或相关实机对照。
- 不设 gate，不计算或校验任何哈希。既有包 CRC 继续作为 wire 损坏检测，不作为状态比较或项目推进条件。

## 3. 已有事实与实现差距

代码位置是 `STATIC_OBSERVED`；只有 tx-03-07 已实际记录的内容标为 `RUNTIME_OBSERVED`。

| 位置/事实 | 已有内容 | C 需要补齐 |
| --- | --- | --- |
| tx-03-07 | Host+VM 双真实游戏；P1/F0 Analyzer 38/38，Host 51/51 恢复，用户裁定正式通过 | 不外推为 observer、本地 VM co-op、状态相等或故障通过 |
| `native/src/t09_remote_runtime.cpp` | resident worker、Remote Endpoint、heartbeat+固定 lead=8；交替生成单轴输入，sequence100 的 Drop edge | 共享脚本来源、完整 VM 本地 A/B、独立采样和双端停止恢复 |
| `src/t09/authority_game.rs` | SharedV7，script_count=0，world slot 空，future window=64 | Host A 脚本源、双端 world mapping 与采集控制 |
| `native/src/coop_active_runtime.cpp` | T09 禁用旧 B ScriptInput；A 仍接物理源；T09 发布 typed gap STATE，绕过 world publication | C 专用源选择、正常 world publication、同 token 的最小值导出 |
| `native/src/minimal_protocol.cpp::Endpoint::Receive` | 每 kind 保留最新包；旧序且非最新 exact duplicate 返回 Sequence | T09 的有界重复/乱序接收处理，保留严格 decoder 与准入 |
| `src/t09/relay.rs` | 已有 delay/jitter/loss/duplicate/reorder 排程；每方向 xorshift(13,7,17) 状态流 | 显式故障启用、tick 触发 outage、确定的排队包策略、有界排程和分析复算 |
| `src/t09/manifest.rs` / `orchestrator.rs` | manifest v1、F0 专用校验、单场 P1 编排 | C manifest、六场生命周期、双方完成条件及 P2 报告 |
| `world_record` Native/Rust | 主线程普通完整采集、current-player witness、局部身份、odd gap、world mapping v2 | 从同一批自有拷贝提取最小 subset，与逐 tick receipt 精确关联 |

既有 `Shared::script` 只有 256 个 step，不能直接塞入 10,000 tick 的逐 tick 双 ordinal 表。现有 worker 按 heartbeat 变化生成一条输入，也不能等同于共享绝对 target 表。实现必须显式解决这些限制。

## 4. 组件、线程和所有权

| 组件 | 允许职责 | 输出 |
| --- | --- | --- |
| Host Native 主线程 | 维护唯一游戏 Authority；ordinal0 脚本准入；解析 wire 后 ordinal1 准入；Resolve/apply、采样、commit/publication | tick receipts、最小 STATE、world batch、正常恢复 |
| VM Native 主线程 | 一个本地观察模拟器，维护自己的 A/B、owned virtual B、scheduler、采样和正常恢复 | VM receipts、world batch；不消费 wire 作为本地输入 |
| VM wire worker | 一个 Remote Endpoint；从不可变脚本与 Host 时标副本构造 B wire 命令；收 ACK/STATE | wire 日志和进度；不触碰 Player/device/Lua/world/游戏队列 |
| 双端 controller | 有界搬运 Wire IPC，drain 已复制的 event/world 数据，落盘和可靠控制 | 双端 artifact、restore 与 process identity |
| Host relay | 原字节 UDP 转发、固定 seed 决策与实际排程 | receive/decision/schedule/forward/drop 日志 |
| Host suite runner/analyzer | 六场手动准备编排、静态脚本生成、日志关联、结果与资源预算 | suite、case、first-divergence 与未覆盖清单 |

VM 主线程和 wire worker 使用不同的源游标/序列状态；共用不可变脚本，不共用可变 Participant。不能同时安装两个各自管理 Hook 的 runtime。C 的 VM 初始化是一个协调 owner 下的本地模拟与 Remote worker 组合，不是在旧 Remote export 旁再注入一套互不协调的 active owner。

新增 Hook（若确有需要）纳入既有统一安装、失败回滚和恢复事务；完整 world observer 的既有 19 槽与基础 catalog 一并由同一 owner 管理。以实际安装清单计算恢复，预期两端各为基础 51 加 world 19，不把历史 51 或计划 70 硬当实际结果。未知 ABI/线程/恢复状态停止相关修改，不猜测后继续。

## 5. 场景基线和安全脚本

沿用 B 的场景标识：双端同 build、以撒、seed `PY1JDB20`、offline/local 初始房间、起初仅原生 A；由各端 Native 正常创建完整普通 B。C 的 VM 本地 B 是新增实现，不能从 B worker 的存活结论推定它已经可靠。

每场准备记录角色、seed、room、配置/Mod 列表、游戏 role/PID/creation time。两端配置须一致；不为通过而自动换 seed、换角色、补物品、重置 RNG、写位置或对世界作额外归一化。任何不可比初始条件记失败/未准备，不能把 pointer 相同或最终文件没变当作整个操作窗口无持久副作用的证据。

共享脚本 `t09c-input-v1`：前 300 tick 中性 warmup，末 300 tick 中性 cooldown；中间固定地安排 A 四向小幅移动/停止、B 四向小幅移动/停止、A/B 相反方向、瞄准/射击 hold/release、一个 Drop edge，以及 B 静默 1/5/30/120 tick 与新 sequence 恢复。300-tick warmup 不等待 B，不把全员输入准备条件放入日常 tick。

- 移动采用有限短脉冲和中性间隔；具体幅值、持续、目标 tick 在实施前固化在脚本表，限制在初始房间中心区域，不以实机试跑调参。中段按固定块延续到 cooldown，确保 outage 附近有 B 输入和恢复观察。
- 唯一 edge 使用现有 `edge_flags=8` 的 Drop，基线要求没有可被 Drop 释放的持有物；不使用炸弹、药丸、主动道具或解锁相关操作。原生 consumer 被查询/处理一次是 edge 依据，不要求人为制造物品变化。
- 没有意外跨门、死亡/复活、暂停或 run/room 变化的预期；发生时保留生命周期证据并停止该场，不重新定位 Player 或续接旧 generation。
- 输入走 canonical 源；操作员在运行窗口不按游戏键。C 模式的物理 poll 仍可履行原生观察职责，但不把物理样本提交为矩阵 A/B 命令。

脚本安全数值和未知字段可在实施阶段通过已有静态依据确定；不能通过新增提前测试来解决。文档不授予未来实机输入、注入或世界修改权限，实机按届时实际授权和用户准备执行。

## 6. 脚本身份、命令与三个时间域

### 6.1 不可变脚本

脚本按 `(relative_target_tick, ordinal)` 唯一排序。每条记录含 `purpose`、V2 24-byte canonical state、`emit`、`edge_identity`；静默是 `emit=false`，不同于发送 neutral。同一目标最多一条，未知字段/非法状态/冲突 target 拒绝。两端保存同一原件、展开表与 canonical bytes，以逐字段/逐字节内容比较说明来源，不使用哈希。

在不可变表展开时，按每个 ordinal 的 emitted row 顺序分配 sequence，从 1 递增；静默不分配 sequence，恢复取下一合法新 sequence。前 300 tick 明确为 `NEUTRAL_WARMUP_NO_SOURCE`，两 ordinal 均 `emit=false`，由缺输入策略保持中性，不分配或漏发 sequence；第一条 emitted row 从 r=301 开始。这个启动区间与中段专门考察的 B 静默区间分别标记。Drop edge 只有一个逻辑 identity。local apply 与 wire 副本使用该表的 identity，不因 heartbeat 合并、重排、丢包或 outage 创建新 identity。C 发送 `redundancy_count=0`，不新增旧 edge 恢复重发策略；relay duplicate 仍保留整包原身份。

### 6.2 时标

- `host_tick`：既有 Host AuthorityClock 的真实累计 gameplay tick。
- `vm_tick`：VM 本地观察 scheduler 的累计 gameplay tick，不是网络 ACK 或 render 时钟。
- `r`：本 case 的逻辑相对 tick，`1..N`。分别有 `host_tick=host_origin+r`、`vm_tick=vm_origin+r`。

两个 origin 分别取 RUN_START 后本端第一个合格普通完整 commit 的 tick，作为 `r=0` 基线帧；该帧不计入 N。之前只保持中性与初始化，不能消费矩阵脚本。之后每个成功 gameplay commit 都使 r 增加，包括 odd；不能只计普通采样、删除 gap 或把两个不同 tick 拼成一个。

对应命令的 24-byte state 必须相同；本地完整 64-byte command 的 target 需要按各端 origin 翻译，不能要求不同本地时标的 target 原字节相等。比较命令使用 `(suite/case, ordinal, sequence, r, canonical_state)`；wire target 必须是 Host 绝对 tick。session/client identity 的映射也在 manifest 明示，局部 observer session 与网络 session 的可变状态隔离，不靠 pointer 或局部 entity id 跨端关联。

### 6.3 准入与发送

- 本地 A 和 VM 本地 A/B 均在自己的 Authority-pre，经相同 codec/Submit/queue 提前排入脚本目标，不直接覆写 Player/device/cache；本地正常 input lead 固定 2。
- 网络发送提前量固定 8，沿用 B 的实机参数；未来输入窗口固定 64，缺输入年龄固定 0。参数分工明确，不再混用一个 `lead_ticks` 同时代表本地 scheduler 与网络提前量。
- RUN_START 后各端上报 origin；控制面只传 role、origin、Authority heartbeat 等元数据。VM wire worker 以 Host heartbeat 计算合法发送窗口，按脚本游标逐条发送；heartbeat 跳号时有界处理仍未过期的目标，过期目标记 `SEND_DEADLINE_MISSED`，不滑动 target 或补新 edge。
- 300 tick warmup 按表不生产命令，逐 tick 记录既有缺输入中性来源；最早 emitted target 的发送期限之前必须已建立 origin 与发送窗口，未满足则记启动失败。不能凭空填入 accepted/processed 序号。
- VM 本地 replay 按自己的 r 和不可变表执行，包括相同静默区间；不按实际 Host ACK/apply 结果修改本地 replay。Host 缺包与 VM 已重放的差异由 analyzer 解释。
- lead=8 不是 WAN 必达承诺；F1–F5 的迟到/缺输入是需要解释的实际结果。不得为通过某场动态增大 lead、未来窗口或 max_input_age。

## 7. 启动、运行和每场准备

suite runner 一次调用内执行以下状态机：

`WAIT_USER_PREPARE → DISCOVER → INITIALIZE → READY → ARMED → RUN_START → ORIGINS → RUNNING → TARGET_COMPLETE → STOP → RESTORE → COLLECT → NEXT_CASE/FINISH`。

1. 每场先生成新 case/run id、session、run nonce 和 A/B nonce，新建目录。等待操作员为该 case 提交带 case id 的准备确认；等待时尚未开始采集或注入，不复用旧确认。
2. 本机/VM controller 各确认本 OS 的唯一目标游戏，记录 PID+creation time；后续 case 必须与上一场进程身份不同，不仅比较 PID。用户自行正常退出并重启；runner 不 kill，不自动正常退出/启动游戏。
3. 双端加载相同脚本和角色配置、初始化 world/事件/Wire 映射、唯一 Hook owner 和 owned B。只有双方本地 exact pair、observer 及 wire Established 都真实就绪才发送 ARMED，不能把 worker Ready 冒充 observer Ready。
4. relay bootstrap 无 impairment，RUN_START 只发送一次。两端独立取得 r0 后回报 origin/基线观察标识；第一个非中性脚本目标之前完成时标建立。Authority 不因等 VM origin 或逐 tick 输入而停顿。
5. 所有 r=1..N 有 receipt/observation 索引；普通完整样本按同 r 比较，odd 规则见第 9 节。Host 先到 N 时立即封闭 Host publication/admission 窗口；VM 可以在不同墙钟时间到 N，不要求 render 同步。
6. 双端到 N、日志排空后统一正常 STOP/restore。Host 封闭窗口后游戏继续原生模拟；VM 到自己的 N 后也封闭 replay/observation，保持中性，不继续消耗矩阵脚本。等待较慢一端有明确超时，超时记失败，不能先 STOP 抹掉剩余证据。
7. 完成双端恢复与进程健康记录后进入下一场准备。任一场失败时正常收尾一次并终止 suite，后续 case=`NOT_RUN`，不自动继续、重试或从中间恢复第二次验收。

手动换进程是六个不同用例的准备步骤，不是同一用例重跑。一次调用中保留操作员等待日志；等待超时及取消有明确 `ABORTED/NOT_RUN` 记录，不偷偷重新注入。

## 8. 状态采集、字段和 IPC

### 8.1 采集与 publication

两端创建已有 world mapping v2，复用 T0-06 主线程 token、普通 owner/Room/标准尾部正常返回、render 前且 coop TopExit 修改前的复制边界。Player 专用字段读取依据沿用 `USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS`，绑定/generation 变化时撤销，不补造构造或 Init 历史。

新增最小提取器从**该 token 已复制的自有 world batch**选出 exact-pair ordinal0/1；不在 controller/网络线程重新解引用 Player。必须将 receipt.tick、post capture/attempt、scope、run/room/mutation、绑定代次和 batch 一一匹配。C 路径撤销 P1 对 `world::PublishPostState` 的无条件绕过，正常处理同 token 的 world publication。

普通帧要求整个既有采样判断合格、双方各两个唯一 Player、所有必需字段有效。上游 batch incomplete、背压、读失败或缺 receipt 不能仅抽出恰好可读的两个位置而声称普通完整样本。

### 8.2 最小 subset

| 内容 | 来源/比较规则 |
| --- | --- |
| ordinal 与 client | ordinal0=A、ordinal1=B；按 manifest 对应；原始局部身份只作来源 |
| Type / Variant / Subtype | 已有 common 字段 `0002/0003/0004`，raw u32；基线角色与创建记录另行核对，不猜测字段意义 |
| Position x/y | 已有 `000B/000C`，Entity+0x33C/+0x340，f32；只取有效当前 Player 来源 |
| sequence/target/resolve/apply | 同 tick 原生 receipt、Submit/Resolve/edge 记录；按脚本逻辑 identity 核验 |
| 采集来源 | 每端本地 run/room/mutation/attempt/capture/token、phase、readiness 及局部 id/use；不要求跨进程原数值相等 |

浮点沿用现有 world 比较约定：有限值精确比较，比较时归一化正负零并保留原始 bits；不引入 epsilon。地址、vptr、Player/device/control pointer、平台 handle、局部 entity id/use、完整世界、未确认字段不作为跨端值相等条件。

accepted 是到达高水位，Host wire 与 VM 本地预排队的 accepted 时刻本来可能不同；它属于因果元数据，不能要求原始计数每 tick 相等。必须逐命令核验 accepted→resolved→processed，F0 必需脚本目标的实际输入/edge 消费和 Player subset 才是跨端比较对象。generation 只在本端核验连续性，不按原数值跨端相等。

### 8.3 wire STATE 与本地 frame

不更改 ICT8 wire v2、input schema2、state schema1 和包上限。普通有效 STATE 使用 `PartialValues`，reason=19（完整世界未导出），每端两个 Player 各五个已定义字段，共 10 values，按既有 `(entity,use,field)` 排序。entity/use 保留本端非零局部观察身份，不能改成假造的全局 ordinal；对应 ordinal 映射只在本端 frame 关联中解释。它是完整的本次 subset，仍是 partial world。

odd-only STATE 保持 `Gap`、OddOnly phase、reason=2、count=0。普通失败使用真实非 odd gap 原因并使该场失败，不使用 `ValuesNotExported` 掩盖采集失败。VM 接收的 STATE 只记观察；双端比较读取各自产出的本地 frame，不能靠把 Host STATE 当 VM 本地状态制造相等。

本地 `ObservationFrameV1` 必含：suite/case/side、r、本端 tick/origin、Authority kind、完整本地 source tuple、`complete_for_subset`、gap reason、两个 ordinal 的选择依据/field raw bits、两 client 的 input availability/receipt 引用。每 tick 恰好一条 frame 索引，world batch 通过 capture 引用，不按“最新一次样本”关联。

### 8.4 配置和兼容

- 新增 C 专用 manifest v2；旧 manifest v1/F0 runner 继续保留原语义。v2 明示 suite/case/seed、手动准备、N、三种时标、local lead/network lead/future window、输入与观察 profile、outage tick、资源限额。
- Native 采用新内部 SharedV8：完整保留 SharedV5 前缀，追加 C 配置（simulation role、run nonce、build label、N、script mapping 与 profile 标识等），明确版本与总尺寸。它不冒充旧 SharedV7，不悄悄复用 reserved；实施时在改生产代码前写出实际布局及静态大小/偏移断言。
- 双端 C 模拟角色分别为 authority / local-observer；只有 Host 角色连接 AuthorityWireLink。VM local-observer 的可变 session 不得连接 Host admission，Remote worker 单独连接 Wire role B。
- 不可变脚本使用独立只读映射，容量有界（每 ordinal 最多 10,000 个 target）；不要扩写旧 256-step 数组。worker 只接收自有快照及原子 origin/heartbeat 元数据。
- Wire IPC v1 的 packet rings 和原有 prefix 保持；若需要 C 进度/原点通道，使用独立 C status mapping/显式版本消息，不把既有 reserved 直接当新字段。控制面新增字段/消息用显式 v2；旧 T9C1 v1 的严格未知字段拒绝规则不放松。

## 9. 比较覆盖、odd gap 与失败

1. 每端每个 r 必须有唯一原生 commit、frame 索引及正确 kind；Authority tick 数按真实标准/odd gameplay commit 计数，不能改成 10,000 个普通 world capture。
2. `OddOnly` 例外必须有该端 receipt.kind 与采样 reason 共同证明；保留原始 gap、run/room/mutation/thread/token/attempt 连续证据。odd 不获得完整样本资格、不进入普通跨端相等统计。
3. 两端以 r0 为普通完整基线，同 r 普通帧配对。普通帧缺失、字段无效、背压/丢号或无来源均失败；不能把它们放入 odd 例外。两端同 r 的 kind/phase 不匹配也失败，保留 `PHASE_MISMATCH`，不向前/后搜索凑配对。
4. 报告分别列每端 total committed、standard complete、odd gap、其它 gap 和 paired standard；必须覆盖全部普通 tick，且有实际普通比较样本，不能全 gap 得 PASS。odd gap 不算 event gap，也不把 T0-06 W07 的“10,000 普通采集”规则误搬到 T0-09 的 gameplay tick 数。
5. F0 的 r0 与所有可配对普通帧要求声明 subset 一致，必需动作的输入 identity/消费完整；任何位置 mismatch 均判 F0 失败，即使能归因为实际网络迟到，也不放宽规则或重写 B 的 P1/F0 结果。
6. F1–F5 允许有直接因果证据的输入迟到/缺失导致的状态分歧；初次分歧后可以持续偏移，不要求无 snapshot 的 observer 自动收敛。新的输入/状态变化仍要持续可解释，不能以一次解释豁免余下全部窗口。

首次分歧分类沿用：`INPUT_LATE_OR_MISSING`、`APPLY_EFFECT`、`UNCOPIED_STATE`、`OBSERVATION_GAP`、`UNKNOWN`。分类不等于通过：`UNKNOWN`、普通 observation gap 和不明原因的 APPLY_EFFECT 失败；UNCOPIED_STATE 必须指出具体已观察但未复制的状态及影响证据，不能泛称“可能 RNG”。first-divergence 保存前后各至少 16 tick，必要时扩展到最近相同输入/静默/outage 起止，不更换比较规则。

## 10. T09 重复与乱序接收设计

现有 Endpoint 的 strict 最新包语义不能直接承担 F2/F4 的旧序重复和乱序。C 新增仅 T09 使用的 `ReceiveGuard`，放在严格 decoder/peer 身份验证之后、既有 Endpoint 最新进度更新之前；旧协议默认接收行为不改变。

- 每 session、方向、kind 维护有界最近 256 条已见包身份和原 bytes，不存哈希；高水位只升不降，按高水位正常淘汰旧历史，不把第 257 条包当成容量错误。新鲜且合法的包继续由 Endpoint 完整验证。
- 已见同 kind/sequence 且原 bytes 相同：记 `DUPLICATE_DROP`，不再次 admission，不重复 edge。已见同 identity 不同 bytes：`IDENTITY_CONFLICT`，停止相关 session，不修改高水位或世界。
- 未见但序号低于该 lane 高水位：记 `REORDER_OLD_DROP`，不回退 ACK/STATE、不重新提交旧输入；这与当前 Participant 对低于 accepted 高水位的 Stale 语义一致，不为 C 改写 scheduler 的接受政策。
- 超出有界历史的旧包：记 `STALE_OUTSIDE_WINDOW` 后丢弃，不声称已证明其为 exact duplicate。拒绝路径不进入游戏队列。仍执行长度、CRC、session、role、sender、消息结构和协商身份验证；不能先按序号丢包而掩盖错误会话/冲突。
- ACK/STATE 各自 lane 进度不得相互覆盖；INPUT header sequence 与 canonical command sequence 分别记账。ReceiveGuard 的丢弃不是 relay loss，analyzer 分开统计，并关联被丢原包的 ordinal。
- Guard 的分配失败/越界、序列溢出、坏包、错误线程和不可解释的结构错误是真失败；正常历史淘汰不属这些错误。不能将所有 protocol error 一概当作网络故障忽略。

## 11. relay 与 F1–F5

| Case | base / jitter | loss / duplicate / reorder | outage | N | C seed |
| --- | --- | --- | --- | ---: | ---: |
| F0 | 0 / 0 ms | 0 / 0 / 0% | 无 | 10,000 | 131080 |
| F1 | 25 / 10 ms | 0 / 0 / 0% | 无 | 5,000 | 131081 |
| F2 | 50 / 20 ms | 1 / 1 / 1% | 无 | 5,000 | 131082 |
| F3 | 100 / 40 ms | 5 / 2 / 2% | 无 | 5,000 | 131083 |
| F4 | 50 / 20 ms | 0 / 10 / 10% | 无 | 5,000 | 131084 |
| F5 | 50 / 20 ms | 0 / 0 / 0% | Host r=2000 触发，2,000 ms | 5,000 | 131085 |

延续现有每方向独立 xorshift(13,7,17) 实现及方向 seed tag，记录 `rng_algorithm=t09-xorshift-13-7-17-v1`、整数宽度/溢出规则和每个方向的初始状态；不把旧总设计写的 xorshift64* 误报为已实现算法。保持现有 jitter 含义：均匀非负 `[0,jitter_ms]` 追加到 base，reorder 额外 20 ms，重复副本额外 1 ms。PRNG 调用顺序与条件消费必须写入 profile 并由日志复算，不能只靠 seed 承诺同一墙钟流量完全复现。

### 11.1 故障启用和触发

- bootstrap 始终无 impairment。RUN_START 的控制事务显式向 relay `ARM_FAULTS`，记录启用时间；不因 relay 等待 VM 启动而提前消耗 outage。warmup 位于 case 内，已经适用正常非 bootstrap impairment。
- Host controller 每个 commit 都能提供可靠 heartbeat；orchestrator 第一次收到 Host r≥2000 时，向 relay 发带 case/run/trigger id、requested_r=2000、observed_r 的一次性 FAULT_TRIGGER，relay 返回实际 QPC/monotonic 起止与 trigger id。不能用 `outage_after_ms` 从 relay 创建时计时冒充该行为。
- 同一 trigger 重复接收不再次断网；错误 run、提前触发或 control 断连记失败。requested_r 与 observed_r 不相等时保留控制延迟，不伪造精确 tick2000 已开始。
- F5 是双向数据面实际 forward 静默窗 `[start,end)`：窗口内不发 INPUT/ACK/STATE；触发时及窗口内所有应在 end 前发送的排队包明确记 `OUTAGE_QUEUE_DROP`，窗口内新到包记 `OUTAGE_RECEIVE_DROP`。原已排程且 due≥end 的包保留原 due 和 identity；不能悄悄改时标或让旧排队包在断网中泄漏。
- Authority 在 outage 内继续原生 commit，记录 start/end 对应的最后/首次 Host tick；要求 start 前和 end 后各至少 1,000 Host tick。若 5,000 tick 不足覆盖实际 end+1,000，判失败，不在末尾偷偷延长窗口或改目标。

### 11.2 决策、边界和证据

每个包 receive 时决定一次故障；记录方向、ordinal、packet identity、原 bytes、PRNG 消费、jitter、reorder、copies、scheduled due 与实际 forward/drop。relay 不合并、拆分、重编码或生成协议包，不重算 CRC。既有 bootstrap 身份缓存限制保留，目标地址在该 case 学习后固定，来源改变拒绝。

scheduled heap 新增包数与 bytes 上限；ring、controller 队列、共享目录、日志 flush 和磁盘失败均有界失败，不能靠 drop 模拟正常系统过载。配置故障与 VM 网络/NAT/宿主排程延迟分别报告。独立主机的 QPC 不能直接相减作单向时延；同一 relay 时钟的 receive/due/forward 和端点序号才是复算基础。

## 12. 首次分歧和逐项关联

分析顺序固定为：本地日志连续性→tick/采集来源→脚本/命令身份→wire/relay/接收处置→admission/Resolve/apply→同 r 字段比较→分类。不得因先看到位置不同就全部归因于网络。

逐命令链至少可定位：`script row → VM local replay / wire encode → controller send → relay ordinal/copy/decision → Host receive/guard → Submit decision → resolved/processed receipt → ACK`。STATE 另用独立 lane 关联到本端同 token 的 copied values。命令没发、relay 丢弃、端点 decoder 拒绝、旧序 guard 丢弃、scheduler expired/neutral 必须可区分。

产物包含 `comparison-profile.json`、双方 `frames.jsonl`、`coverage.json`、`input-dispositions.jsonl`、`first-divergence.json`（无分歧时显式 null）、双端前后窗口和 relay/control 切片，以及后续 divergence 窗口索引。每条字段差异携带 r、双方本地 tick/token/capture、ordinal/field、raw bits、逻辑 input identity 和分类依据。

所有有效 packet 都有明确去向；允许的 wire loss 不等于采集丢失。F1–F5 不要求“零拒绝”或 A/B位置永远相等，要求实际迟到/重复/乱序/过期与正确中性/edge 行为有证据；不能要求旧 B/F0 分析器的零故障条件仍全部成立。

## 13. 停止、恢复与构建产物

- 保留 tx-03-06 发布终点锁定：Host 到 N 后不再增加 ACK/STATE/heartbeat，不再 admission，仍有界 drain；游戏继续模拟。VM 不能因为 Host先到终点就提前丢弃自己的 N 以内普通采样。
- 正常 STOP 关闭新输入/publication，处理在途 callback，退休本地/网络 session 和 generation，移出 owned device 并使输出中性，恢复每端实际安装 catalog；同一场只尝试一次正常收尾。
- `NEVER`：BASE_CONSTRUCTED 后对象/control block/有效基类状态不释放，DLL 常驻；不主动进入释放 root 的旧 ZOMBIE 路径，不强制 FreeLibrary、不盲目重注入。原生调用 wrapper original exactly-once 约定继续。
- 每端分别保存 native error、installed/restored 逐槽清单与字节、callbacks/drain、endpoint retired、DLL resident，以及同一 PID+creation time 游戏存活。恢复不确定时不能进行下一场自动操作。
- 修正 runner 的 resident staging 语义：供运行中游戏加载的 DLL 放在保留目录，成功 STOP 后仍保留；游戏由用户退出后再自行清理或另行维护。不能因预期的映射文件占用把功能 PASS 改成 WorkCleanup FAIL，也不能吞掉真正的 stop/restore 错误。tx-03-07 原 FAIL 与用户裁定原样保留。

## 14. C 的唯一 E2E 设计

拟交付 `cs-t0/accept-t09-c-live.py`，一次调用负责纯构建、脚本/manifest 打包、六场手动准备与执行、双端采集和分析、统一结果。该入口及以下模块当前均未实现；本文不是可立即执行的命令说明。

1. 一次构建实际待测产物，不另外运行离线 harness、unit、smoke、probe、verify-existing 或预演。遵守“不在写代码后新增 unit tests”；若实施确需隔离验证，先补全失败方式，再实现，并纳入同一个最终入口，不能提前执行。
2. 按第 1 节的 6 case/35,000 tick 顺序运行；每场用户准备新的双端身份，同一个 runner 调用继续。没有全套环境/授权时不把单场试跑当成 C 的验收。
3. F0：普通采样零遗漏、subset 精确相等、必需输入/edge 正确、10,000 Host 与对应 VM tick 完成、无 ready stall/event loss；odd gap 单列。
4. F1–F5：采集同样完整；实际故障与 PRNG/排程/接收结果对应；Authority 持续 tick；合法新输入恢复；edge从未重复；分歧有界可解释。F5 另外满足真正的双向 2 s forward outage 和前后 1,000 tick。
5. 每场双端停止/恢复/游戏存活成立，所有被跳过/取消/未执行项如实记录。任一失败则 suite FAIL，不重跑；可解释的状态分歧是否允许按第 9 节判定，不能由 analyzer 自动放宽字段或容差。
6. 只在全部 6 case 满足 C 条件时给 `T0-09-C/P2=PASS`；同时写 `T0-09=IN_PROGRESS` 与 D 的未覆盖列表，不输出完整 T0-09 PASS。

复现分为两类：保存原始输入/配置/源码与构建记录，使人工能按同一准备步骤重新发起后续新实验；保存精确日志/原 bytes/分析规则，使既有结果可离线解释和复算。固定 seed 不承诺不同 host/VM 调度下包的实际到达交错完全相同。本次 C 验收不额外执行一次 replay。

## 15. 产物和资源

```text
artifacts/cs-t0/<implementation-tx>-c-e2e-01/
  suite-manifest.json  operator-preparation.jsonl  suite-result.json
  sources/  build/  runtime/                 # resident DLL 保留
  scripts/input.json  scripts/expanded.jsonl  scripts/canonical.jsonl
  cases/F0-seed131080/                        # F1..F5 各独立子目录
    manifest.json  baseline.json
    host/{events,wire,udp,control,frames}.jsonl
    vm/{events,wire,udp,control,frames}.jsonl
    host/restore.json  vm/restore.json
    relay/{relay.jsonl,ready.json,stopped.json}
    control/{control.jsonl,origins.json,fault-trigger.json}
    analyzer/{checks.jsonl,coverage.json,input-dispositions.jsonl,
              comparison-profile.json,first-divergence.json,windows/,result.json}
  analyzer/{case-summary.json,uncovered.json,report.md}
```

所有目录/文件 create-new；共享目录按 suite/case/run id 区分，VM 完成落盘后发 artifact-closed，再由 Host 收集/分析，不从半写文件下结论。失败保留原始原因、已落盘数据、第一次分歧和停止恢复结果；清理失败不覆盖最初错误。

复用 world mapping v2 现有容量 `136,316,688 bytes/端`，双端消费者及时 drain，写入 compact Player subset 和来源证明，不默认把每 tick 全世界写盘。预算同时覆盖内存/队列峰值、两端事件、三类包原 bytes、35,000 双端 tick 的行上界、分歧窗口和失败尾部，开始前保存估算和可用磁盘，逐场重新确认剩余空间；空间不足停止，不能滚掉仍用于因果配对的必要原始日志。

公共报告不含 SteamID、用户名、完整游戏路径、原始 handle 或无关进程信息。需要完整 invocation 的运行资料放私有目录；字段比较与 packet 配对不依赖这些身份信息。

## 16. 实施任务顺序与交付

以下为同一个实现目标的工作顺序，均不安排中途测试：

| 顺序 | 实施内容 | 主要代码位置/新增组件 |
| --- | --- | --- |
| 实现1 | 严格 manifest/control/Native 配置、脚本展开与时标契约 | `src/t09/manifest.rs`、`control.rs`、新 script 模块、SharedV8/独立 script/status 映射 |
| 实现2 | Host A 脚本源、VM 本地 A/B 与 Remote worker 的协调 owner | `coop_active_runtime.cpp`、`t09_remote_runtime.cpp`、`authority_game.rs`、新增 observer driver |
| 实现3 | 双端 world mapping、同 token subset 导出、frame/receipt 对应与覆盖统计 | `world_record.cpp`、`world_record_live.rs`、`t09_authority_link.cpp`、新 observation 模块 |
| 实现4 | T09 ReceiveGuard、真实 relay 故障控制与 queue 语义 | `minimal_protocol` 的复用校验接口、T09 guard、`relay.rs`、`endpoint.rs` |
| 实现5 | 六场用户准备编排、双端终点等待、resident staging 和恢复记录 | `orchestrator.rs`、game driver、统一 acceptance runner |
| 实现6 | 因果 analyzer、状态比较、窗口、失败与 D 交接报告 | 新 C analyzer、coverage/first-divergence 输出、使用说明 |
| 实现7 | 全部完成后一次统一 E2E | 6 场，35,000 Authority tick，真实 Host+VM artifacts |

无需新增全映像别名闭包、旧 gate/哈希认证或独立审计流程。实施期间发现新的实际 ABI/来源问题，先静态研究并修订设计；不能用提前实机 probe 解决，也不能把未解决问题写为已验证。

## 17. 未决事实及完成边界

- `UNKNOWN_RUNTIME`：VM 本地 owned B、双端同 seed/角色/room 的最小位置可比性；r0 对齐后标准/odd 序列是否同相；固定网络 lead 在 VM 调度下的实际迟到率。通过唯一 E2E 观察，不预设必然成功。
- `UNKNOWN_IMPLEMENTATION`：新增配置映射实际字节布局、helper/observer driver 接线、source/frame 通知和有界队列大小；由实现前静态契约补齐，不是需要用户重新决定的产品选项。
- `OUT_OF_SCOPE_D`：双机 C0/C1、新鲜完整 C3、多 seed 覆盖、完整 T0-09 报告。C2 已删除，不列为未覆盖/待测。
- `OUT_OF_SCOPE_GENERAL`：完整世界相等、snapshot apply、预测/rollback/Replica、晚加入、专用服务器，以及已有 virtual-device 异常/OOM/worker 并发闭包的全部证明。

若上述 UNKNOWN 在唯一验收中失败，按实际失败提交有限窗口和未完成清单，实质修复再开展新的推进；不只换编号重跑原验收。

## 18. 本次文档验收记录

本次仅在全部设计、失败方式和入口同步完成后进行一次静态文档审阅，检查三项用户选择、代码事实、接口一致性、采样与计数、输入/线程/恢复、C/D 范围及引用。结果记录见 [tx-03-08 静态审阅](tx-03-08-c-static-review.md)。未运行代码、构建或游戏。
