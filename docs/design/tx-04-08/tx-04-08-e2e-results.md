# tx-04-08 整体 E2E 结果

> **2026-10-30 V12专项完成**：[v12-e2e-03](../../../artifacts/repentogon/tx-04-08-v12-e2e-03/README.md)入口退出0，V12 PASS，PID16272/37308均正常退出0；3710 tick、104 Ack全量无错，四个输入窗口、5283持久调用、9个原生窗口及六组文件区间通过。切房/换层18/320条上下文UNKNOWN查询均有逐条实际返回见证、有限为零，原UNKNOWN计数保留。存档变化符合原规则、无新crash、清理无错；其它V/L本次NOT_RUN。第一轮第二进程因第三档Steam Cloud读取失败弹窗阻塞并超时，收尾另缺evidence目录；第二轮加载未知值未闭合且原版第三档仅尾Counter/Checksum变化、旧基线不覆盖，均保留FAIL。已按用户“修复后直接继续”授权完成实质修复后续测，不扩大白名单、不回滚存档；同步失败的更深原因UNKNOWN，后四次启动未重现。

> **2026-10-30 V1/V10 专项完成**：用户要求只完成 V1、V10，并明确实机直接开始、故障直接修复继续、结束后统一资料落盘。本次 [V1/V10 联合入口](../../../artifacts/repentogon/tx-04-08-v1-v10-e2e-01/README.md)退出 **0 / PASS**，两次顺序启动，PID 29244/29444 均正常退出0，无强杀或新增崩溃；V1、V10 **PASS**，L1–L5 **PASS**，其余 V 项与 L6/L7 本次 **NOT_RUN**，不代表整个 tx-04-08 已完成。仅本次入口执行一次，未发生运行失败或重跑。下方整体失败记录保持原样。
>
> V1：两进程各79项清单/ABI/注册、19对外层与原生层计数对应；Off窗口实际58/234次观察入口调用，Closed窗口各6次，均正常配对。四份录制逐call核对482组原生四边界，配对/嵌套错误、未闭合调用与事件缺号均0。零调用的Replace/Morph/部分Grid分支明确未观察，不用注册事实补造运行覆盖。
>
> V10：primary全部982条记录（353 capture，其中311完整，629独立gap）、847生命周期事件、17诊断双份生产CLI重编码并逐字节/字段一致；Tear真实完整attempt923/tick656供raw128边界派生。L2共11项、L4共11项非法输入、L5共11项冲突/引用与缺口全部通过，L3精确单字段差异通过；五份原始输入逐字节未改。最终[机器报告](../../../artifacts/repentogon/tx-04-08-v1-v10-e2e-01/report.json)、[摘要](../../../artifacts/repentogon/tx-04-08-v1-v10-e2e-01/report.md)、[离线账本](../../../artifacts/repentogon/tx-04-08-v1-v10-e2e-01/offline/ledger.json)与清理记录齐全。

以下保留截至 2026-10-29 的整体 E2E 历史，Asia/Singapore。工程交付和纯编译见[实现与主审](tx-04-08-defg-implementation.md)。历史失败及当时不自动重跑的安排不追溯改写。

| 产物 | 入口结果 | 实际游戏启动 | 停止原因 |
| --- | --- | --- | --- |
| [e2e-01](../../../artifacts/repentogon/tx-04-08-e2e-01/execution-notes.md) | FAIL，退出码1 | 0 | 启动前发现第三方 gameplay mod `arknights_terra_a_journey_3631023221` 仍启用 |
| [e2e-02](../../../artifacts/repentogon/tx-04-08-e2e-02/execution-notes.md) | FAIL，退出码1；用户明确允许的重新执行 | 1 | 最初 counters 先于首帧主线程服务，5000ms超时；5859ms时被主线程按过期规则丢弃 |
| [e2e-03](../../../artifacts/repentogon/tx-04-08-e2e-03/execution-notes.md) | FAIL，退出码1；用户明确允许修正后再执行 | 1 | 启动等待/选档/新局到达；S0错误按平面schema读取tx08的结构化players数组而拒绝合法A；L7实际PASS |
| [e2e-04](../../../artifacts/repentogon/tx-04-08-e2e-04/execution-notes.md) | FAIL，退出码1；2026-10-29 用户重新要求开始并确认第一次启动 | 1 | S0与两项setup动作完成；world准备在root_directory失败，attempt=0，Host/第二进程未启动；L7实际PASS |
| [e2e-05](../../../artifacts/repentogon/tx-04-08-e2e-05/execution-notes.md) | FAIL，退出码1；用户要求修复路径后继续测试并确认第一次启动 | 1 | 扩展盘符路径修复通过，world实际Recording并正常flush；远端首包后10054，未握手；13条gap、0完整capture；L1/L7实际PASS |
| [e2e-06](../../../artifacts/repentogon/tx-04-08-e2e-06/execution-notes.md) | 实机采集FAIL；最终自动报告未完成，用户要求停止缓慢归约 | 1 | 监听等待66ms后握手/Active成功；240秒内完整capture=0，主要UNKNOWN_CLASS；world收口与环境恢复完成，停止归约保留证据，未启动第二进程 |
| [e2e-07](../../../artifacts/repentogon/tx-04-08-e2e-07/execution-notes.md) | FAIL，退出码1；最终自动报告正常生成 | 1 | Grid全部识别、无UNKNOWN_CLASS；Effect读取依据NOT_READY，240秒完整数0；全量索引核对28,672条tick/480Ack无匹配错误，L1/L7通过，第二进程未启动 |
| [e2e-08](../../../artifacts/repentogon/tx-04-08-e2e-08/execution-notes.md) | FAIL，退出码1；最终自动报告正常生成 | 1 | 开局前Recording生效，首份完整样本23个Effect均有初始化资格；30秒取得797份完整样本，未达该次冻结的2,000目标；L1/L3/L4/L5/L7通过，第二进程未启动，环境恢复 |
| [e2e-09](../../../artifacts/repentogon/tx-04-08-e2e-09/execution-notes.md) | FAIL，退出码1；最终自动报告正常生成 | 1 | 主250目标的稳定段单项通过，实际254完整+253 odd-only；第25项Grid直接替换返回false，夹具grid_spawn_false；L1/L3/L4/L5/L7通过，第二进程未启动，环境恢复 |
| [e2e-10](../../../artifacts/repentogon/tx-04-08-e2e-10/execution-notes.md) | FAIL，退出码1；最终自动报告正常生成 | 1 | 主稳定段254/250通过；第24项首次Grid创建返回true，但请求variant0、实际variant2，grid_spawn_parameters_changed；便便替换未执行，第二进程未启动，环境恢复 |
| [e2e-11](../../../artifacts/repentogon/tx-04-08-e2e-11/execution-notes.md) | FAIL，退出码1；最终自动报告正常生成 | 1 | 主稳定段254/250通过；Grid创建/删除后生成便便/独立移除及原生重建关联通过；切房、B死复活、换层动作到达；启动首份故障录制误读旧generation的Closed而中止，第二进程未启动，环境恢复 |
| [e2e-12](../../../artifacts/repentogon/tx-04-08-e2e-12/execution-notes.md) | FAIL，退出码1；用户要求修复后直接测试，无准备询问 | 1 | 身份限定等待实测14次start各跳过旧快照成功；主稳定段253/250通过，故障10/11通过（thread首因不符）；Continue后703动作等待新Player Init超时，第二进程未启动，完整报告生成且环境恢复 |

用户已确认第一次启动准备。入口在键盘环境与第三方 mod 检查处退出，尚未部署、进入人工检查点或启动游戏。Download Helper 临时禁用已恢复，清理错误均为null。原[JSON报告](../../../artifacts/repentogon/tx-04-08-e2e-01/report.json)和[自动摘要](../../../artifacts/repentogon/tx-04-08-e2e-01/report.md)保持不变；缺材料导致的 V/L FAIL 不冒充运行反例。完整实机与离线功能验收均仍未取得，tx-04-08 未通过验收。

后续重新执行必须有用户明确指示；不能把启动前失败静默删除后称为首次验收。若临时禁用指定 mod，记录并恢复其原启用状态，不修改 mod 源码。

e2e-02 的第一进程失败清理以 WM_CLOSE 正常退出0，没有强杀、第二次启动或 world 录制；清理与指定 mod 的恢复日志均无错误。统一入口随后实际执行的 L7 CLI 也失败：旧gap带15个零值来源头，新解码器错误地将它当成新版非空来源而拒绝。已静态对照旧编码器，限定修正显式legacy的空头读取与原值保留。失败后另将 tx08 原有的 `wait_startup` 菜单等待前移到最初 counters 之前。两项只纯编译，尚未再运行入口或CLI。工程功能仍未通过完整验收，不能用修正替代运行结果。

e2e-03 按用户许可验证了上述两项：启动就绪等待通过，L7完整重编码15820条记录（6335完整capture、9485gap、93事件），CLI退出0且flush成功。S0的原始A满足全部条件，但共享harness只将tx07识别为结构化观测，遗漏tx08；已将tx08加入同一读取分支，不放宽条件。失败后仅纯编译，未再启动。第一进程正常失败清理，第二进程/world仍未执行；整体迁移未验收。

e2e-04 在用户本次请求及逐次准备确认后执行。S0结构化观测分支通过，seed为`RXHA AY90`；`cs world start primary`随后返回`PREPARE_FAILED / root_directory`，writer未启动、attempt/complete/gap均为0。静态定位为writer的逐级建目录逻辑未跳过harness传入的Windows扩展盘符路径`\\?\F:\`前缀；具体Win32错误码未由状态导出，保持UNKNOWN。没有改生产代码、重构建或更换路径重试。Host/Join、稳定段和第二进程均未执行，不再请求第二次准备。第一进程PID43964，经WM_CLOSE正常失败清理退出0，无强杀请求、无新增crash；部署/fixture/Helper恢复无报错，保留原先已存在的禁用标记，不回滚存档。独立L7实际再次完整导入通过，15820条记录/6335完整capture/9485gap/93事件，flush成功。原自动FAIL与缺材料结论保留，完整tx-04-08仍未验收。

e2e-05按用户“修复该问题，然后继续测试”实施：writer保留并跳过扩展盘符根，只查询根属性，从根后逐级建目录，并统一分隔符；中间既有节点须是目录、最终录制目录仍create-new。完整DLL纯编译链接成功，未写单测或运行局部验收。获逐次准备确认后一次完整入口实际进入Recording并写出`p1.before_host` checkpoint，路径修复获得运行覆盖。随后远端首次Hello后报10054，tx=1/rx=0/establishments=0；Host命令只返回queued，harness随即启动远端，实际Idle→Hosting在下一帧，静态定位存在监听就绪前启动远端的时序缺口，系统层错误来源未另行探测。world正常stop至Closed，13条gap/0完整capture/1checkpoint均收口，drain/flush/result成功、无未交付项；S1/稳定段/第二进程未完成。第一进程PID20324正常失败清理退出0、无强杀请求或新增crash，环境恢复无报错。L1全量gap录制replay和L7旧格式导入通过，L2–L5因无完整capture未覆盖、L6未运行；整体FAIL保持原样。本次没有为新握手失败再改代码或自动重跑。

e2e-06按用户再次要求实施Host监听时序修复，仅改harness并纯编译：从Idle提交一次Host，等待新代次Hosting/Host、peer_running=1、Negotiating和无terminal/native错误，初始两进程与菜单Host/Continue共用，不重发首包、不忽略10054。第一进程PID37256、seed`JPK1 X37K`，实际等待66ms后才启动远端，握手与A/B Active成功，远端全程error=null。240秒稳定窗口因完整数为0而超时；14,430次attempt中有7,199份不完整capture、7,231纯gap，主要UNKNOWN_CLASS=7,157；首个相关样本的41个未知对象均为Grid域/vptr0167972c，具体类型与被拒检查尚未核定。world正常排空flush，117事件/2checkpoint均交付，游戏退出、环境恢复无报错，第二进程未启动。随后最终trace归约因每tick重复扫描全部events而长时间占用CPU；用户明确要求保留证据并停止，于02:32:23核对身份后仅终止harness PID5056，包装finally完成、子进程退出码-1。原始录制、L1双份replay、L7导入与已生成索引全部保留；report.json/report.md未生成，V1–V12自动最终判定未完成。人工事实摘录与停止记录单列，不伪造自动报告。本轮没有额外测试或重跑，完整tx-04-08仍未验收。

e2e-07在Grid扩表与全量索引修复后按用户“启动新e2e测试”及第一次准备确认执行。第一进程PID18852、seed`LL4Z 042G`，Host/握手/A-B Active成功，原UNKNOWN_CLASS未再出现，样本44个Grid全部识别。240秒完整数仍0，所有7,165份不完整capture均为NOT_READY；样本Effect为录制前已存在的FIRST_OBSERVED/init_evidence=0，公共000a及0401–0403读取被拒。14,361次attempt、122事件、2checkpoint正常drain/flush，无未交付或背压；第一进程WM_CLOSE退出0，无强杀请求或新增crash，环境恢复无报错。最终自动报告正常生成：FULL_INDEXED全量核对189,804事件/28,672 tick/480Ack，bad_tick/bad_ack均0，最后save_after记录后约5.05秒落report；进程收集至报告总约153.96秒包含全量replay和文件扫描，不作跨seed加速倍数结论。L1/L7通过，L2–L5无完整样本未覆盖、L6/第二进程未运行；V12因缺操作窗口/第二进程仍FAIL，首进程全量核对证据已独立保存。原自动FAIL不改写，本次没有改代码/场景或重跑；待解决Effect读取依据/时序，整个tx-04-08仍未验收。
