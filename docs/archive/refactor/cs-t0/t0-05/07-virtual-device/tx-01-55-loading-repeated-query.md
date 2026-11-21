# tx-01-55：Loading／LoadingExit 与同一查询重复读取

日期：2026-10-17。完整目标：在一次真实原生 Continue 加载窗口内，观察 Loading、LoadingExit、输入阻断及重新唯一绑定后的新鲜输入；在加载后的真实 A/B consumer 上下文中，对同一 generation/tick/action/query/consumer 连续读取，确认值相同且没有消费队列、edge 或修改 current/previous。tx-01-54 的结果保留。本次只补充可观测性和受控只读测试接线，不重命名其他场景为 Loading。

## 修改前的场景与失败判据

分析、设计和修改阶段零测试。用户已经确认能完全退出重启并停在有隔离 Offline 存档可 Continue 的主菜单；控制器安装完成后才手动 Continue。初始没有受控 B，加载正常返回、唯一 A 绑定后，设备按原路线只创建/Join 一次。正常游戏中发送 queries 控制请求，再发送新鲜 B script；用户只需短暂移动/攻击。最后主菜单、正常 stop、48 Hook 恢复、root/DLL 常驻和同一进程保留。

FAIL：没有真实 AlternateStateApply/OwnerExit 却发布 Loading 标签；加载/恢复期间提交有效输入、推进 Authority 或应用非中性；LoadingExit 未清旧 generation，恢复输入无唯一绑定/新鲜源；同一查询连续读取值改变或 Participant 任意状态改变；重复读取调用了 Resolve/Submit/原生 consumer，重复 edge/world apply；事件丢失、错误线程、传输/原生错误或恢复不完整。未出现真实加载或足够重复样本记 NOT_OBSERVED，不自动重跑。

唯一入口为 `accept-loading-query-e2e.ps1`，统一构建、注入、操作、恢复和分析同一捕获一次。产物目录 `artifacts/cs-t0/tx-01-55-live-01`，保存原始事件、操作、场景、源码、副本、构建、逐项结果、恢复。没有单元测试、前置试跑或哈希校验。

## 静态依据与实现设计

IDA 数据库 input 为当前 `isaac-ng.exe`、base400000。`959720` 的 alternate owner 在状态恢复路径调用 `6F8140`；该函数包含 `[Continue, %d]` 日志及原生 state apply。现有 Loading 由 callsite RVA559B21 的 state apply 进入、RVA554D5E owner 正常返回退出；只在第一次实际 apply 后打开窗口。普通 owner 空调用不代表 Loading。

加载后的查询仍只读实际 Resolve。新增 queries 控制（现有 retire_b 控制值4）打开有界重复读取：实际 Eligible query 的首读仍返回游戏，在同一主线程/consumer/exact binding 上再调用 Participant::Read 两次；另在该实际允许的 action/consumer 上覆盖其余纯读接口。每组都有三次实际 Read，source1为原生查询拦截，source2为受控 companion 读取，不冒充原生自然重复。连续读取前后逐字节比较整个自有 Participant（current/previous、队列、dedupe、sequence、generation），不比较哈希、不写回状态。使用自有静态诊断缓冲，避免把含4096项历史的 Participant 放入游戏线程栈。默认关闭，每次 queries 最多256组；每 generation/action/query/零或非零类别最多一次。额外读取不再次调用原生设备方法或 consumer。

新增 QueryRepeat=26 记录组结论，LifecycleWindow=27 记录原生生命周期信号、caller RVA、loop、阻断和进入/退出前后的深度及绑定可见性；action2/query2明确记录 Authority 观察释放 loading recovery，便于检查恢复前没有 Submit/Resolve。IPC布局、输入codec及48 Hook不变。loop原始日志补充thread/caller/originalCalls/flags字段，用于对应真实原生事务；观测 wrapper 保持 original exactly-once。

## 状态

已完成一次专项 E2E，**20/20 PASS**；分析/修改阶段零测试，没有自动重跑。覆盖初始 Continue、加载后 A/B 查询，不声称已验证已接纳B在主动run中再次加载。

## 一次最终验收结果（RUNTIME_OBSERVED）

用户确认已完全重启、主菜单可 Continue 且隔离存档初始仅A后，统一入口执行一次。Rust Release、Native/helper v143 Win32 构建成功；PID34696、creation time134366954044543197。260,528条事件、4,645个 Authority tick；nativeError=0、transportFailure=null，重复 Resolve/commit/consumer/accept、inconsistentQuery 均为 false，专项 violations=[]。

原生真实加载事务1次，loop8119：Loading #5/#6，来源窗口 #7（state apply caller RVA559B26）；LoadingExit #24/#25，窗口 #26（owner return RVA554D63）。两个参与者的 generation 都失效递增，窗口绑定不可见；加载/恢复期间无接受输入、Resolve、commit 或非中性查询。loop8120 的 #42 明确释放 loading recovery 后才恢复正常输入。原生 loop 日志同时保存 StateApply/OwnerEnter/Exit、同一主线程和 originalCalls=1，标签有实际原生来源支持。

注册 #37、创建 #86、B binding #93、Joined #94 各一次。加载后 A/B 唯一绑定与 accepted→Resolve 对应，新鲜输入实际消费。控制器 queries 请求在tick1312开启；tick2545的script请求在同一窗口生成新鲜B输入，未重新注入或创建测试进程。

共102组、306次受控实际 Read；A/B 的 Pressed、Triggered、Scalar 均观察到零值与非零值。每组三次值相同、浮点位相同，整个 Participant 前后逐字节不变（队列、历史、sequence、generation、current/previous及edge均在比较范围），现有 controller 的 repeated-query-pure 也为 PASS。source1/2区分原生查询拦截与受控 companion；不把这些组称为引擎自然发起的三次查询。没有额外调用原生 consumer 或推进tick来制造重复。

用户确认“已 Continue，进入 Run 且表现正常”，再确认移动/按住松开攻击正常且已回主菜单。人工事实保存在 `operator-observations.json`。菜单 teardown #260474/#260475、pair缺席/root常驻 #260479、48 Hook恢复汇总 #260528；同一进程保留、DLL pinned，未强制卸载。

产物：[专项结果](../../../../../../artifacts/cs-t0/tx-01-55-live-01/result.json)。同目录保留原始co-op/loop日志、控制请求、fixture、操作计划、源码、构建、人工观察、恢复及进程记录。宽范围 `full-feature-result.json` 仍为 NOT_OBSERVED、原controller退出1；专项入口对事先定义的20项分析同一捕获为PASS、退出0，未重跑分析器或覆盖宽范围结论。

本轮不外推已运行A/B的第二次加载、所有加载路径/武器、完整T0-05其余用例或异常并发闭包；旧失败和未观察项仍保留。
