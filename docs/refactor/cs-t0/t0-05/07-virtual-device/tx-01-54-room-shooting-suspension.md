# tx-01-54：跨房非模拟窗口暂停射击消费

日期：2026-10-17。目标：修复 T0-05 按住攻击跨房自动发射；保留真实松键、有效 tick 缺输入立即中性、旧 generation 失效和 A/B 统一输入语义。tx-01-50/51 的失败与 52/53 的已有结果保留。

## 修改前的失败判据与唯一验收

分析及编辑过程中不运行测试、构建、诊断采集或旧捕获的验收脚本。全部实现完成后，只运行 `accept-room-shooting-e2e.ps1` 一次，统一构建、注入、操作、恢复和分析同一捕获；失败不自动重跑。不新增单元测试，不计算哈希。

场景：用户启动的新隔离 Offline 进程，初始只有键盘 A（阿萨泻勒 ID7）；一次原生 B Join；A 蓄力后持续按同一攻击键往返跨房，入房后继续按住至少两秒，再实际松键；暂停/恢复时保持攻击；B 同窗口脚本覆盖持续攻击、明确中性、重复和旧 generation 输入，必要时用户操作期间发送新的 script 命令（同一次捕获、新 sequence）。最后主菜单 teardown、正常 stop、保留进程与 DLL，核对全部 48 Hook 恢复。

FAIL：未松键却发射；正常蓄力或真实松键损坏；有效 Authority tick 的缺输入被暂停从而保持旧攻击；使用旧 generation/旧样本/物理旁路补 held；非模拟 Shooting 仍实际执行；实际执行缺少有效模拟窗口；普通 tick/lead/codec/edge 规则改变；重复处理、原生或传输错误、恢复不完整。缺少场景或人工画面事实记 NOT_OBSERVED。输入方向缓存及不变 witness 不充当武器蓄力或激光实体证据。

产物：`artifacts/cs-t0/tx-01-54-live-01`，保留 fixture、操作计划、源码静态依据、构建日志、原始事件、人工画面观察、逐项结果、恢复和进程状态。人工观察在 stop 前落盘，报告只在统一入口末尾执行一次。

## STATIC_CONFIRMED：输入中性不等于暂停武器更新

开始时未发现可调用的 IDA Pro MCP / x32dbg 工具，先用本地 `objdump` 只读反汇编既有游戏 EXE（没有运行或附着游戏）。随后 IDA 工具接入，已读取数据库 input path 为同一 `isaac-ng.exe`、image base 为 `400000`，并保存以下函数的静态伪代码；没有读取或比较哈希：

- VA `6FD4A9` / `6FD54A`（RVA `2FD4A9` / `2FD54A`）的 `FF 50 0C` 仍调用 Player::Update，返回地址分别为 `2FD4AC` / `2FD54D`；前者在 `Game+1B83C` 的 1/4 状态，后者在 `Game+1BA78` 的 1/4 状态。两者位于 `6FD597 -> 6FD350` 普通模拟许可检查之前。
- VA `6FB51C` 的 `E8 7F 63 13 00` 调普通门切换 owner `8318A0`。该 owner 在 `8319C9`、`831A11`、`831A31` 直接调用 Player::Update（返回 RVA `4319CC` / `431A14` / `431A34`）；不能只考虑嵌套 Room::Update。
- Shooting callsite `785BE0`（RVA `385BE0`）调用 `77B020`。该函数在 `77B1ED/77B220/77B248/77B306` 查询攻击 pressed，并在 `77B37C` 读取 shooting vector；这些结果参与后续武器更新。将查询变零仍会执行整个武器状态机，不能把它视为暂停。
- 现有 `Evaluate` 在 `g_tick=0` 时返回零；`Consume` 随后仍执行原生 Shooting。历史 51 捕获中也保留了 `WorldWitness action=0, query=2FD4AC` 的窗口外消费事实。由此推导它能向蓄力武器制造松键；是否解决实际发射以本轮 E2E 和人工画面为准。
- IDA `7826D0` 进一步显示它把输入向量/held 传入 `Player+13DC`（备用 `13E0` 起四槽）武器对象的虚表 `+0C` 更新。这里的 Player 方向缓存并不是武器蓄力计数。

静态原始输出保存在 `artifacts/cs-t0/tx-01-54-static`。不将静态推导标为本轮 RUNTIME_OBSERVED。

## 实现决定

在 C/S 独占 owner 的 Shooting 主动分派处，无 Authority tick、生命周期阻断、暂停、加载恢复或停用期间不进入原生 Shooting consumer。其他原生 Player 更新、观测 wrapper 和正常 consumer 仍按原调用执行。这里是明确的主动射击抑制：被暂停的 Shooting original 调用次数为零；不把它描述为 original exactly-once 的观测 wrapper。

有 Authority tick 且无生命周期阻断时，即使 participant 当前没有命令/绑定，也照常进入 Shooting，查询仍中性。尤其不能用 CurrentEligible、held 或 sequence 是否非零作为暂停条件，否则会把缺输入变成隐式保持。双方共用此分派，不复制旧 current，不写武器/蓄力字段，不从查询读物理输入，不新增 Hook。

新增 128 字节事件 `ShootingDispatch=25`：action 0=暂停、1=原生执行；query=PlayerUpdate caller RVA；detail 位 0=无 tick、1=pause、2=lifecycle、3=loading recovery、4=disabled；target=原生 loop iteration；decision=入口 CurrentEligible；before/after 是既有 Shooting witness，boolean 表示读取成功。临时失效时只为标注事件，从当前 Main 拓扑重新识别 ordinal，不建立 participant 绑定或恢复任何旧关联。

正常 tick 的 `WorldWitness`、查询与分派配对；暂停时不伪造 WorldWitness。事件继续使用现有隔离 IPC v2，24/64 字节 V2、startup/lead=2、source 方向历史与 NEVER 回收规则不变。

## 状态

生产修改、事件和唯一入口/fixture/报告已完成；一次实机专项验收 **18/18 PASS**，未运行其他测试、未自动重跑。

## 一次最终验收结果（RUNTIME_OBSERVED）

用户本会话确认“已重启并就绪，开始本次一次 E2E”后，执行统一入口一次。Rust Release、Native/helper v143 Win32 构建成功；PID3476、creation time134366894271924377。捕获1,074,896条事件、19,027个 Authority tick，nativeError=0、transportFailure=null；重复 Resolve/commit/consumer/accept 与 inconsistentQuery 均为 false，专项 violations=[]。

单次注册 #51、原生创建 #105、B binding #112、Joined #113。A/B 各75次非模拟 Shooting 暂停，正常执行分别19,027/19,025次；双方两原生模拟路径均消费过新鲜非零射击。普通门切换暂停例如 A #145633、B #145643，A 当前 loop held 源对应恢复后的启动 #145755。B 显式中性例如 #6878、缺输入中性仍执行 Shooting；duplicate #787、旧 generation #3352 拒绝。

用户先反馈“表现一切正常”，再明确确认持续按住攻击往返跨房、入房继续按住、真实释放才发射、再次按下可蓄力、保持攻击暂停恢复均正常，并已回主菜单。人工观察在 stop 前写入 `manual-room-shooting-observations.json`，报告独立列出五项画面 PASS，不以方向缓存替代蓄力/发射事实。

菜单 teardown #1074844/#1074845，pair 缺席/root 常驻 #1074847，48 Hook 恢复汇总 #1074896；nativeRestored=true、pinned=true，同一游戏进程保留。正常 stop，未强制卸载或再次注入。

产物：[专项结果](../../../../../artifacts/cs-t0/tx-01-54-live-01/result.json)。同目录保留原始事件/操作、恢复、场景/操作计划、源码副本、人工观察、构建日志和进程状态；静态反编译保存在 `tx-01-54-static`。

宽范围 controller 返回1，原 `full-feature-result.json` 保留 NOT_OBSERVED（完整45功能清单未覆盖）；事先定义的本轮专项18项为 PASS，专项入口退出码0。没有重跑分析器或改写宽范围结果。实际未额外发送 script 指令；原200步 B 脚本完成，B 跨房暂停机制有实测，但未对 B 蓄力武器作跨房画面验证。本轮不外推 loading/换层、所有武器、OOM/EH 或并发闭包。完整 T0-05 E2E 与51未接纳 Join 的历史原因仍未全部解决。
