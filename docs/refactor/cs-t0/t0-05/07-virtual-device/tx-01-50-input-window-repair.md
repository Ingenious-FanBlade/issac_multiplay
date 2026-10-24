# tx-01-50：输入被打断的来源与修复

日期：2026-10-16。用户要求“检查输入被打断的来源，然后修复该问题”。目标是识别 tx-01-49 中额外 Player::Update 的实际 owner，修复 A/B 输入窗口覆盖，最后在一次统一验收中检查 A（阿萨泻勒/键盘）移动、持续瞄准/蓄力及原计划的 B 死亡/复活。历史捕获和失败保留。

## 修改前失败判据与场景

不得用 A 即时物理旁路、放宽 CurrentEligible、重放旧 edge、跳过未理解的原生更新，或直接把窗口外 Update 全部认作 UI。必须先识别额外调用方，区分实际世界推进、原生维护、Mod/API 重入与 ABI/Hook 错误。A/B 继续共用 scheduler，每 participant 每 Authority tick 至多 Resolve 一次，原生 original 的调用次数和 ABI 保持正确。

修复后的整体场景：新进程、隔离 Offline 初始仅 A；单次注册/原生 Join；A 连续移动和各方向持续射击输入，原生消费期间不被窗口外中性覆盖；能直接观察到的蓄力/发射结果与尚未观察到的字段分别报告；B 脚本与 A 隔离；B 死亡失效、中性、A 继续、B 复活唯一重绑及新输入恢复；菜单 teardown、pair 缺席、root/DLL 常驻、全部原始槽字节恢复（来源采集 47 槽，修复新增后 48 槽）。缺场景不计通过，错误或失败不自动重跑。

FAIL 包括：没有识别额外 owner 就改世界时序；持续输入仍被中性覆盖；重复 Resolve/world commit/edge apply；越界、错误线程、读失败或传输丢失后继续写世界；退休/恢复不完整；原版/Isaac Sync/V1 行为改变。完整 OOM/EH/worker 加固仍不在范围。

## 现有证据与缺口

tx-01-49 #51863/#51898、#51928/#51963、#51993/#52028、#52058/#52093 已观察同 tick 的有效 Shooting 后又有不可见 current 下的 Shooting，原生 +0x162C 派生方向从有效值清回 -1。旧事件没有 Player::Update 的 caller/phase/stack，因此不能从“同 tick”直接推出第二次调用发生在同一次顶层 loop。

本轮静态核对 32-bit isaac-ng.exe，base=0x400000。Player vtable +0x0C 指向 782AF0；六个 consumer callsite 在 785BC2..785BE7。直接 Movement/Shooting 目标为 779B40/77B020，仅该 Player 函数有对应 direct call。Player::Update 只有 vtable 数据引用，实际 virtual caller 必须区分。4186C0 内有多处 +0x0C virtual call；还存在原生状态/UI/API 中的同类调用，不能凭调用槽猜测对象。StandardRoom 入口/正常返回仅包住 6FBA4D→802980；原生后续仍有 9BB5D0、4257B0、MC_POST_UPDATE(8607A0) 等路径。TimeMachine 源码虽然含额外 Player Update，但用户确认未启用，不作为本次归因。

## 可审阅的定位改动

仅在现有 PlayerUpdate wrapper 记录少量不同 caller/phase/ordinal/是否 current eligible 的样本；记录直接返回地址、loop iteration、phase/depth 和有界栈地址。仅使用现有事件 IPC，事件仍为 128 字节，不新增 Hook，不调用游戏对象 API，不修改 current/previous/queue/world，不改变 original 的调用次数。最多 32 个不同来源样本，避免连续刷日志；无来源样本或栈不可读不伪造结论。

定位若获用户允许：在新游戏进程中一次构建/注入，运行短暂 A 移动/持续射击，采集实际 native caller 与 phase，随后正常 teardown/shutdown；不做 B 死亡/复活，也不把定位采集标为功能通过。目录 `artifacts/cs-t0/tx-01-50-source-capture-01`；保存源码改动、输入顺序、原始事件、调用来源解释与恢复结果，不计算哈希。

定位后按实际 owner 完成生产修复，准备最终统一入口，在再次完全重启的隔离场景运行本轮一次整体验收，目录 `artifacts/cs-t0/tx-01-50-live-01`。前置诊断须用户明确允许对现行项目规范 §2 作本轮例外；此授权已取得，采集现已完成，下方保留事实。

定位入口为 `cs-t0/capture-native-player-source.ps1`，使用现有统一构建/注入/恢复链；fixture 只有 B 中性输入。前置的 actual operations 为：新进程 A-only → 注入/Join → A 短暂持续移动/射击 → 用户主菜单 → 正常 stop。原始功能 result 不改写，单独 `source-result.json` 只能记 SOURCE_CAPTURED/NOT_OBSERVED/FAIL，不是功能 PASS；没有实际窗口外来源或未恢复，不自动重试。该入口已执行一次。

## 状态

### 已授权的一次来源采集结果

SOURCE_CAPTURED，目录 `artifacts/cs-t0/tx-01-50-source-capture-01`。PID 45136，creation time 134366388710570699，实际 image base 0x760000。记录 6 个来源签名；#78/#88 的 native caller RVA=0x2FD60C（IDA VA 6FD60C），phase=GameplayUpdate、g_tick=0、未 pause/loading。正常来源 RVA=0x1895C（IDA VA 41895C）位于 EntityList 内、phase=EntityMutation。栈 #79–81 包含 6FD60C→955246→9AB6F7 主循环链。正常恢复 47 槽并保持 DLL pin，无 nativeError/transportFailure；原始 feature NOT_OBSERVED 不改写为 PASS。

静态原始指令进一步确认：954CD0 在 9551D6 检查 n298+0x4ABBC 的最低位；偶数帧在 955231 调 6FADC0，奇数帧在 flag n298+0x2A3C0 非零时经 955241 调 6FD3F0。6FD3F0 先调用原生暂停状态机 68BBA0；6FD597 调 6FD350 检查暂停/菜单/room/loading 等阻断；正常路径遍历 Room 实体，6FD609 的 `FF 52 0C` 调 Player::Update，返回 6FD60C。普通奇数帧真实玩家更新被现有标准 Room-only scheduler 漏掉。

### 修复决定与最终验收（修改前确定）

保留标准 Room 的 Authority-pre/normal-post 行为；新增 955241→6FD3F0 的同 owner wrapper，仅在其正常路径 6FD609 首次即将更新 Main A/B 时开启一次 Authority。此时原生自己的暂停/状态检查已经返回允许，不重复调用有副作用的 68BBA0。奇数帧 helper 正常返回后提交 previous；暂停/FSM 特殊路径 6FD4A9/6FD54A 不推进 scheduler，仍中性。A/B 共用原队列、lead=2 和立即中性策略，序号与 generation 不重置，不使用物理旁路；奇偶两个原生模拟步骤各 Resolve 至多一次。

新 callsite 纳入统一安装/回滚/恢复，co-op 槽总数由 47 变为 48；历史 47 槽结果不更改，V1/原版观察者不增加该 Hook。最终报告必须同时观察标准与奇数帧的有效输入查询、持续 held 不被同来源中性清除、每个参与者/tick 无重复 Resolve/consumer apply，以及 B 死亡/复活、菜单 teardown、48 槽恢复与 DLL 常驻。新绑定首个无命令的中性 tick 是合法状态，不当作漏帧；暂停/loading/死亡期间的中性不当作持续输入失败。

来源采集阶段仅改动 PlayerUpdate wrapper，增加最多 32 个来源签名，每个签名记录 caller/phase/loop/current eligibility 和最多 16 个栈地址；事件编号 20/21，128 字节布局不变，当时未新增 Hook。直接 caller 在 wrapper 中取 _ReturnAddress，传给 noinline 记录函数；CaptureStackBackTrace 的 BackTraceHash 参数为空，不请求哈希。当时输入分支、scheduler、original 调用和恢复路径未改。

用户已明确答复“允许前置采集，已重启并准备好场景”，授权本轮一次前置来源采集，随后修复并运行一次最终 E2E；操作范围沿用既有授权。来源采集已正常结束；此后仅进行静态分析和编辑。

### 已完成的生产改动，待最终验收

`coop_active_runtime.cpp` 新增 cdecl/no-args PlayerOnlyUpdate 包住已核对的 E8 AA 81 DA FF；原生 helper 恰调用一次，未改变原生暂停状态机。PlayerUpdate 仅在 return RVA=0x2FD60C、位于该 helper、首次 normal Main A/B 且拓扑唯一时开启 Authority；不依赖旧 binding，因而死亡/复活可由 Authority 失效/重绑。正常返回时统一 End/commit。Resolve/AuthorityCommit detail=1/2 区分标准与奇数路径；WorldWitness action=0/1/2 表示窗口外/标准/奇数，query 记录 native Player caller RVA，原 128 字节布局保持。

新 relative slot ordinal=40，action/Player 指针槽顺延至 41–47，全部走 loop 同 owner 安装、回滚、排空及恢复。旧 V1/原版路径不增加该槽。最终恢复目标为 48 槽，来源采集保留历史 47 槽结果。

统一入口 `cs-t0/accept-input-window-e2e.ps1` 完成一次新构建和同一实机窗口；使用原死亡/复活 fixture，不额外消耗道具。完整操作顺序和失败判据为 `cs-t0/testdata/tx-01-50-input-window-operations.json`。报告沿用 capture 原始事件，增加双方两路径 move/shoot 查询、held 原生字段保持、A 至少连续 60 步持续瞄准、普通奇数 caller CurrentEligible、无重复处理及 48 槽恢复。已理解的暂停/FSM 特殊调用单独识别。原完整 feature 报告仍保留，缺项不被专项 PASS 覆盖；原生 aim 消费不能冒充激光实体/蓄力完成证据。

最终 E2E 已执行一次，结果见下。来源采集进程 PID 45136 未重注入；用户完全重启后使用新进程。

### 最终实机结果（2026-10-17）

目录 `artifacts/cs-t0/tx-01-50-live-01`，统一入口只执行一次。Rust Release、Native/helper v143 Win32 均构建成功。PID 45788，creation time 134366398527567897；650,671 事件、12,624 Authority tick，两路径各提交 6,312 次；nativeError=0、transportFailure=null，Resolve/commit/consumer/accept 无重复、查询无不一致。A 持续瞄准最长 205 步，两端两路径的移动/射击消费均有观察。

用户先反馈移动和蓄力恢复正常；自动按键前置收到 FOREGROUND_DENIED，未发送按键、未自动重试。随后用户手工完成四方向移动/攻击，并完成 B 死亡和复活；操作 marks、工具失败和人工观察分别保留。用户额外要求再发一轮 B 脚本，仅在同一窗口发布新序号，没有新构建或重注入。

B Death #168243/tick3185/generation28；死亡期间 2,950 个中性 Resolve/Authority commit，A 有效 2,936 tick；Respawn #307251/tick6135/generation103，#307253 重新唯一绑定同 K/device/control/ClientId/ordinal。后续 generation105 新命令被接受/Resolve/query/原生移动字段变化，旧 generation 明确拒绝；用户观察到 B 行动。全部死亡/复活项 PASS。

菜单 teardown #650619/#650620，pair 缺席及 root=1 #650622；48 个原始 Hook target 全部恢复、回调排空、DLL pinned，进程仍运行。没有强制卸载或再次注入。

**专项总结果 FAIL（19 PASS、1 FAIL）**，保留 `result.json` 原文，不自动重跑。失败为 `no-held-aim-neutral-interruption`，21 个样本；其中 #197..#734 属 B 创建后原生 Shooting 不读取方向输入的前 14 步，before/after 均已为中性；#265116..#265404 为换房后 A 的 7 步，查询确实返回非零，但 before/after 仍中性。报告把“held 下派生字段中性”一概命名为“cleared”，不能单凭该词证明发生了有效→中性覆盖；原生 Shooting 自身有 ControlsEnabled/ControlsCooldown、动画、物品和其他抑制条件。此限制不改写 FAIL。

用户最终补充两种残留现象：一直按住攻击键跨房会失去蓄力；移动中切换相邻攻击方向、两键短暂重叠时偶发丢蓄力。现有 +0x162C/+0x1630 是 GetFireDirection/GetAimDirection 派生字段（IDA getter 4316D0/4316E0），不是直接蓄力计数，不能将上述 21 条直接冒充这两个现象的完整证明。输入缺陷仅修复了奇数帧漏窗，蓄力连续性整体尚未完成。

只读回看现有捕获：A 无命令 Resolve 共 26 步，均为 13 次 initial/rebind 后连续两步；没有观察到同 generation 中间漏 Resolve 命令。换房 tick4148/4149、4226/4227、4518/4519、4870/4871、5138/5139、5252/5253 中性后恢复非零 held，与清空 pending + lead=2 的固定启动间隙一致。另有移动保持但瞄准为零的已接受采样，例如 physical #115599/#115652→Resolve tick2185/2186，随后 ↑→右；缺少 primitive 按键记录，不能判定这些源端零值是否对应用户描述的实际双键重叠。

原完整功能报告单独保留 `full-feature-result.json`，为 NOT_OBSERVED，不能将本次死亡/复活 PASS 写成完整 T0-05 PASS。后续不重跑本轮；需要先明确新 generation 输入启动语义并完成实质修改。

### 残留蓄力问题的后续澄清

用户确认相邻攻击方向、短暂双键重叠；换房保持攻击键不松。前述无命令间隙不能单独解释所有换向问题；raw PhysicalSample #115599/#115652 的零输入来源仍缺 primitive 证据。原版查询的 before 值中还可见多个攻击方向同时为 true，因此不能把 canonical 零值直接归因于用户松键或硬件。

静态对照原版本地链：Keyboard primitive +0x24/+0x2C（A6C510/A6C580）读取当前 src 表，逻辑 Pressed/Scalar（A1E8D0/A1EA20）结合原生 action cache 和绑定的 primitive；779620（Lua GetShootingInput）按 action4–7 算出二维向量，相邻双键仍非零。77B020（Shooting）另有 ControlsEnabled（+0x410）、ControlsCooldown（+0x414）、动画/物品等原生抑制分支。4316D0/4316E0 分别返回 FireDirection/AimDirection；这些字段不是蓄力计数。

原版本地链没有 C/S generation/queue/lead=2；本项目自行失效重绑引入的两步中性不是原版人为安排的输入延迟。原生换房是否保留某项武器蓄力仍须按具体武器状态解释，不由方向缓存反推。

已向用户提出“重新唯一绑定后首个新鲜命令可指向即将 Resolve 的 tick，A/B 相同规则、正常 lead=2 不变”的功能选项；用户追问“原版是如何实现的？”，尚未确认该选项。未修改 lead、未增加物理即时旁路，未在已恢复进程重新操作，也未重跑分析器修改原 FAIL。
