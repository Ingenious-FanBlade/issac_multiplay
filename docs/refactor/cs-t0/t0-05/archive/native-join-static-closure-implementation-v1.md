> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04：限域持久化条件审查 v1

本轮从已发布的 plan v7 / join progress v1 继续，只使用磁盘 PE 和只读静态逆向。逆向与测试由不同子智能体承担；没有枚举、打开或控制游戏进程，没有新实机授权。

## 本轮解决的问题

上一轮已经证明普通 join 可以到达持久化写入，但尚未证明可用的原生保护场景。本轮将问题收窄到普通显式 seed、新 run 起始房、Keyboard P0、唯一未分配 XInput B 和普通 Isaac，分别检查 Player Init/tail/cache 与原生 readonly、保存和生命周期路径。

普通显式 seed 是候选场景限制，不是用户必须立即进行的游戏操作。没有通过写 readonly、清除统计、强制保存开关、修改 seed 或压制回调制造实验条件。原生 readonly 只能排除受它保护的 setter；直接持久写、Lua 回调及保存入口必须分别处理。未知条件不能由文件哈希不变或尚未发生的 join 见证替代。

两份新静态交接独立于冻结的 static v1、followup v1 和 plan v7。它们把可成立的条件结论与剩余静态数据流缺口分开，并保留 `runtimeObserved=false`。具体条件以这两份交接及其独立审计为准，不由本记录授予运行权限。

本轮得到的正向条件包括：

- 普通显式 seed 经原生 SeedMenu/New Run 路径建立 readonly；特殊 seed 分支另有保存行为，不能混用。
- 已知 achievement、statistics 和 item-collection setter 的 readonly 保护可以组合使用。普通模式下 `GameRoot+0x26630=0` 排除了已定位的两处临时清除 readonly 的调用，但不代表 readonly 全生命周期恒定。
- Init/tail/cache 的11项局部分支条件已拆开。直接计数写、九处 AddCollectible、callback 8/9/20、派生计数和 familiar 递归不能仅用“普通 Isaac”或界面空道具栏推定安全；受共同 readonly 保护的 health setter 不再被重复列为独立阻断。
- manager 返回后，同帧仍可进入 GameUpdate。计时器 `GameRoot+0x67788`、`+0x68D6C` 可到达转场请求，房间请求还会经别名写入 `+0x1B83C`。readonly 不阻止所有 flush，因此下一次 manager sample 才发现 drift 不能排除已发生的保存。

Init [机器交接 v2](../../../../../cs-t0/coop-native-join-init-closure-v2.json)及[修订说明](native-join-init-closure-v2.md)包含83个锚点；[窗口交接](native-join-persistence-window-v1.md)及其[机器证据](../../../../../cs-t0/coop-native-join-persistence-window-v1.json)包含63个锚点。这146个证据锚点不是新增Hook槽；候选事务仍固定55槽，实际主动事务尚未实现。

## 新增只读接口

~~~powershell
pwsh -NoProfile -File .\cs-t0\inspect-coop-native-join-static-closure.ps1 `
  -GameExe <disk-isaac-ng.exe> -RepositoryRoot <repository> `
  -Output <new-diagnostic.json>
~~~

`Output` 可省略。该脚本固定绑定 `coop-native-join-static-closure-contract-v1.json` 及两份新交接，核对严格 JSON、相对路径、目标 SHA、完整字节窗口声明、所有声明的直接分支和常量引用，检查 condition proof 的锚点引用，再次核对输入哈希。合法输入仍输出 `BLOCKED_BY_EVIDENCE / STATIC_CONDITION_REVIEW`，退出码1；无效输入不输出结构化报告。

指令解码由独立验证器另用 objdump 完成，包括从已声明函数入口到窗口末端的连续边界、窗口指令大小、助记符、所有直接分支目标和浮点常量引用。公开 inspection 的 `instructionDecodingVerified=false` 明确表示它本身不提供独立解码证明。

诊断文件必须 create-new，位于已有 `out/cs-t0` 或 `artifacts/cs-t0` 目录内，使用简单 ASCII JSON 文件名。脚本拒绝现存文件、source 目录、历史快照、设备别名、ADS 和 reparse 路径。报告不包含目标绝对路径、PID、裸指针、设备 key 或平台身份。

它不是进程 preflight、arm API、主动 smoke 或正式 join gate。它不创建 mapping、grant、claim、注入状态或事件 source；不会因条件审查通过而改变任何主动权限。

## 验证及历史隔离

本轮没有修改上一轮已验证的 Rust/Native 生产源或三份生产二进制，也没有重签其 verification。原 `coop-join-preflight` 继续按冻结 static v1 报告七项缺口；新条件进展由新的 inspection、verification 和 progress 表达，不改写历史报告。

独立门负责新静态数据、新公开脚本的真实黑盒及对抗测试，并在前后核对164文件快照、149文件基线、三份生产二进制和旧 Release 阻断行为。本轮不重跑或冒称重新完成上一轮完整 Rust/Native 自动门，也不把旧53/56槽回归充当55槽主动 runtime 验收。

完整新静态门实际退出0，create-new 发布 [verification v1](../../../../../cs-t0/coop-native-join-static-closure-verification-v1.json)，状态为 `PASS_STATIC_CONDITION_REVIEW_VERIFICATION`，其 `blockingStatus` 仍为 `BLOCKED_BY_EVIDENCE`。14组独立工具对抗和9组公开黑盒全部通过；共核146个锚点、1,994条指令、456条边、5处常量引用和4个浮点常量，并机械核对 Init v1→v2 的修订范围。

验收前后均核对164文件快照、149文件基线、三份生产二进制，以及旧发布18项、旧verification20项和继承引用16项绑定。新报告绑定32个输入，明确没有重建生产程序、重签旧verification或重跑继承完整自动门。旧Release preflight实际仍退出1并保留原七项缺口。命令、环境、失败与通过范围见[独立测试结果](native-join-static-closure-test-results-v1.md)。

通过验证后，公开 inspection 另行 create-new 生成[审查诊断](../../../../../out/cs-t0/native-join-static-closure-inspection-v1.json)，实际退出1。新schema按静态、实现和未来实机义务保留12个缺口代码；它与旧preflight的七项codebook不同，不回写旧结论。诊断中全部 runtime、process、mapping、arm/claim、active及解锁字段为false。新的 plan/progress 只发布本轮条件审查进展，不能用于任何主动授权。

Init v1 初次独立解码被拒绝：5个锚点中的22条 padding 指令助记符为空。磁盘解码确认其中1条为 `nop`、21条为 `int3`。create-new 的 v2 仅修正这些元数据并绑定 v1 与失败审计，不改原始字节、分支、条件或权限。原 v1 及失败报告全部保留；窗口初次检查器的 RVA 格式适配失败也单独保留，没有将工具失败或部分诊断计为通过。

## 不变的出口条件

只有完整静态无污染与独立 prejoin 契约闭合后，才进入专用 runtime/IPC、两次原生 UI edge、单次授权消费、55槽事务、Shutdown 和严格事件重放的实现与独立自动验证。完成这些自动门后，仍须就具体进程、场景和验证版本取得新的单次实机授权。

当前不修改矩阵第11项，不解锁05-05、T0-05整体或T0-06；不使用旧授权、不自动重试实机，也不调用普通 B 的 native-exit 回滚。

后续静态工作已经收窄为两项：

1. 沿 `0x353DA0 / 0x355470 → 0x3BBBD0 / 0x3BC740`，把 fresh B 的 constructor/Init 状态传递到 `0x3706E0 / 0x370CA0 / 0x363370` 等有效计数和 desired-familiar 输入。B 在准入前尚不存在，不能用创建后读回替代这项事前证明。
2. 把该 Init 路径与自然 GameUpdate 组合，闭合上述计时器及 `0x2FDC10 / 0x2FD7C0 / 0x42EE40` 请求路径，证明声明窗口内不会调度持久写或清除 readonly。只搜索顶层函数中的字段位移不能证明 callee/alias 不写。

字段宽度、读取边界、owner/vector/device 替换及 generation/ABA 失效策略另列为实现契约义务。它们不得依赖未来 passive witness；也不会因为运行观测尚未发生，就要求无界证明整个游戏永不保存。
