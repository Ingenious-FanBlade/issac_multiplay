> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 Native Join Active：实施记录 v1

日期：2026-08-31。用户批准实施的范围是证据补全、专用实现及独立自动验证；**没有新的实机授权**。

## 冻结边界

- 前置为 plan v6 / passive decision v3，仅表示工作包资格。历史 source、失败结论、N/A scope 与 149 文件基线不得改判。
- 本包固定 P0 Keyboard、唯一未分配 XInput B、普通 Isaac。只允许原生 join 的两次 UI edge；不包含 gameplay input、scheduler、transport、virtual device、普通 B 退出或 T0-06。
- 用户已批准新增基于证据的限域 prejoin Offline 判据；不把 passive 的非零 backend 后见证例外升级为主动权限。
- native 与测试由不同子智能体负责；逆向证据不自我验收。

## 原生路线

候选事务是旧 53 槽加 RVA `0x5BB058`、`0x5BB38C` 两个 `push imm32` callback operand，共55槽。原 callback 为 RVA `0x6209E0`。这不是 REL32 修改，必须核对完整指令、上下文、重定位和目标所有权。

自然 manager pass 中，第一次仅将 B 的 action19 原 false 返回覆盖为 true；原 manager 返回后核对单 Player 和唯一 selector（type=0、selectionIndex=1、scale=1、autoConfirm=0）。下一自然 pass 重新验证，再覆盖一次 action14。保留完整原生创建、assignment、HUD、初始化及 selector 退休，不另调 manager，也不拼接三个 callee。

四参数 cdecl callback 必须 original-once，所有权释放仍由 original 执行。wrapper 只在调用前捕获必要身份，调用后不解引用已释放 control。自然 true、非预期设备/action、线程/owner/depth/generation 变化或重入均不计主动成功；不得压制原行为或自动重试。

## 证据门与实现顺序

1. 保存旧三生产二进制及历史绑定文件。已创建 `cs-t0/coop-native-join-predecessor-v1.json`，快照位于 `artifacts/cs-t0/t0-05-04-predecessor-v1`；164文件包含149基线及迁移链。
2. 冻结 callback/selector/prejoin 的静态原始指令证据，追踪 ordinary Player init 与 join 尾部的持久化可达性。
3. **零持久污染必须先闭合**：覆盖内存 dirty/stat/unlock、磁盘、Cloud/Steam。Offline 或文件哈希不变均不构成证明。证据不足时 active gate 保持 `BLOCKED_BY_EVIDENCE`，不试跑、不发布 runtime-complete。
4. 仅在该门通过后，接入独立 runtime、IPC、授权/claim、55槽事务及完整事件链；不得用测试 fixture 使实机门通过。
5. 独立测试实际生产 wrapper、Shutdown、真实 Win32 mapping、公开 CLI/analyzer、逐槽故障恢复、历史重放和权限拒绝。新生产哈希与历史生产哈希分开验证。
6. 完整自动门通过后才具备申请单次实机的条件。正式 source、恢复、零污染和独立 replay PASS 才更新矩阵第11项并解锁05-05，仍不解锁T0-06。

## 计划接口

`coop-join-preflight`、`coop-join-arm`、`coop-smoke --probe native-join-active --scenario native-join-active`、`coop-native-join-gate`，以及 `coop-analyze` 的独立source分派和 `verify-coop-native-join.ps1`。

接口按证据门落地；文档列出接口不代表其已实现。缺少前置证据时不生成可用 grant/claim、mapping 或注入入口，不借用旧T0-04 active授权。

## 实际交付：证据前置，非主动 runtime

- [静态交接](native-join-active-reverse-v1.md)和 `coop-native-join-static-v1.json` 固定53个原始字节锚点。证据不只覆盖callback，还记录成就/dirty路径、persistent+0x484直接写入、Lua初始化回调及尚未排除的延迟保存。
- `coop-native-join-catalog-v1.json` 保存55槽候选结构；`runtimeImplemented=false`，不得把候选数当作已安装或已恢复槽数。
- `coop-join-preflight` 是已实现的公开入口。它先检查冻结静态/目录/前置文件，独立读取目标磁盘PE，重放五个旧PASS及旧exit BLOCKED，并再次校验哈希。正常结果仍为 `BLOCKED_BY_EVIDENCE` / `STATIC_PREREQUISITE`，退出码1。
- 默认只输出stdout；`--output`必须create-new且位于仓库的`out/cs-t0`或`artifacts/cs-t0`下。文件名仅接受ASCII字母、数字、点、下划线、短横线及`.json`后缀，拒绝Windows设备别名和备用数据流。所有既有source目录及其子目录、冻结快照和现存文件均禁止写入。它不枚举或打开进程，也不创建mapping、token、claim或Native状态。
- `verify-coop-native-join.ps1`验证的是此阻断门和全部继承回归；其成功状态为 `PASS_BLOCKED_GATE_VERIFICATION`，不是05-04正式PASS。新55槽Native ABI、IPC、Shutdown及主动source测试仍未执行，不能由旧Native回归替代。

未接入的计划接口包括`coop-join-arm`、active smoke、join source analyzer及正式`coop-native-join-gate`；55槽runtime、授权消费和实机仍待证据闭合。没有为这些接口提供可被fixture或手填字段打开的临时实现。

后续首先需要证明一个原生可达且无持久污染的实验场景，覆盖Player Init/尾部全部相关写入、初始化和cache回调、整个采集/停止窗口的保存路径，并建立独立prejoin准入。不得通过写readonly、清统计、截断原初始化或抑制未理解的回调来制造通过条件。证据闭合后才能继续原计划中的主动实现与独立runtime验收。

[补核纪要](native-join-active-followup-v1.md)已支持限定ordinary join正向设备路径不依赖原联机，不要求未来passive witness或无Steam背景活动。该补核不回写冻结static v1，也不代替完整准入/稳定性契约及独立验收；当前preflight仍按v1报告其七项缺口。持久化的实际阻断继续存在：低金币只排除个别成就分支，尾部cache更新还有其他成就写和mask `0x100` 回调，不能只检查player-init的mask `0x200`。

本文件记录当前实施；逆向交接与独立测试结果分别由对应文件提供，不能把本记录当作主动PASS证据。

## 独立自动验证结果

完整自动门v3已执行结束，退出码0；create-new生成[verification v1](../../../../../../cs-t0/coop-native-join-verification-v1.json)，SHA-256 `5F2CE26D88D27B3869DE9B01B92DB79737F2A17369A990781BE96FAECDE61CE4`。其唯一通过范围为 `PASS_BLOCKED_GATE_VERIFICATION`：只读阻断门和完整继承回归，不认证主动runtime或实机资格。

- 新公开CLI黑盒8组通过，包括实际目标磁盘检查、输入/历史篡改、旧入口隔离、create-new和Windows输出别名拒绝；所有依赖目标的用例均实际执行。
- Rust全量test/fmt/check/clippy、Release构建，以及继承的Native Debug/Release、真实Win32 mapping和生产Shutdown桥接通过。这里的Native是旧53/56槽和T0-04路径；新55槽runtime尚未实现，未把这些回归算作其验收。
- 历史适用性18组通过，149文件契约与164文件快照哈希一致；五份PASS、live-07 BLOCKED及旧六源结论保持不变。当前Rust EXE由新报告绑定，旧三生产二进制仍保存在前置快照，不更新旧verification使新EXE通过。
- 最终Release preflight实际退出1，报告53个静态锚点、55个候选槽、164个历史文件、五PASS＋exit BLOCKED以及七项缺口；join/目标路径的进程读取、mapping、注入、arm/claim、active及解锁字段均为false。

[完整v3日志](../../../../../../out/cs-t0/native-join-blocked-automatic-gate-20260831-v3.log)保留实际输出。v1的SDK访问失败、v2为补Windows输出拒绝而中止，以及初期测试fixture失败均保留在独立测试记录中；没有将失败、未执行或旧轮次的部分结果计入v3通过。

详细执行证据见[独立验证结果](native-join-active-test-results-v1.md)。[plan v7](../../../../../../cs-t0/coop-plan-v7.json)与[join progress v1](../../../../../../cs-t0/coop-native-join-progress-v1.json)已create-new发布，绑定verification、冻结前置、静态契约与补核审计，明确记录未实现接口及剩余条件。当前工作包仍BLOCKED，矩阵第11项未改，05-05和T0-06未解锁；不能申请单次主动实机。

发布后的[独立终审](../../../../../../out/cs-t0/native-join-publication-independent-audit-v1.json)再次核对新旧绑定与权限，结果为 `PASS_PUBLICATION_BINDINGS_AND_ZERO_ACTIVE_AUTHORITY`。它仅认证本轮阻断产物正确发布，不升级任何主动能力。
