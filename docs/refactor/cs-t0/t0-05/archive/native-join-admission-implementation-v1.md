> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 静态补证与准入规格实施 v1

2026-09-01。从 plan v8 / join progress v2 接续；本轮止于新静态证据、准入规格、离线模型和独立磁盘门。不实现生产 reader、55 槽 runtime、IPC、arm/claim，不启动、枚举、打开、附着或操控游戏进程。

## 证据与结论分层

新 Init v3 与 persistence-window v2 由两个静态逆向实例分别产出，独立测试由另一实例承担。主线整合规格和 inspector；不能以自身模型接受替代逆向的原始指令依据或测试实例的验收。

Init 新补证包括 pending item-config 来源、pill-history、familiar carry、容器容量及局部初始化零值。pending pointer 为零可以短路对应 trinket 来源，不额外假设其 flags 为零。familiar 匹配涉及 Room 实体查询及 owner 关系，不能虚构一个 Player 自有 familiar vector。具体原始窗口、来源和未覆盖调用以 [Init v3](native-join-init-closure-v3.md) 为准。

窗口补证扩展到自然 Room/实体虚调用、额外 callback 和 readonly 重算入口；以 [窗口 v2](native-join-persistence-window-v2.md) 为准。局部条件能排除某个直接分支，不表示全部传递调用无副作用。真实可达污染反例与覆盖不足分别记录；不将未知冒称反例，也不将两次干净采样、dirty 归零、文件不变冒称无保存证明。

新 [admission contract](../../../../../cs-t0/coop-native-join-admission-contract-v1.json) 明确保留原生静态闭合、完整 reader 和 native ABA 的未决状态。它在证据已冻结后 create-new；旧 static/Init/window、旧脚本、旧测试目录、生产源和三份二进制均保持原字节。

## 固定场景与窗口

沿用普通显式 seed、普通 Isaac、Keyboard P0、唯一未分配 XInput B、新起始房、原生初始保存已完成和干净 persistent 基线。还必须满足空特殊 seed effects、既有 config/解锁/直接计数条件、无相关外部 callback、无物理 gameplay/menu/reset/console 输入及零 pending/timer/fade。无 mods 不是 callback 完整排除的替代条件，不修改存档或原生保护状态来制造条件。

时间顺序固定为：

1. publication 前及选择 callback 的独立准入；
2. action19 选择 pass 返回，确认 selector；
3. 下一连续自然 pass 重新准入、action14 确认、A/B Candidate；
4. 再下一连续完整 pass 达到双 BOUND；
5. disable、drain、retire、restore、terminal。

窗口至少跨三个 manager pass，包含中间及停止等待期间的自然更新。证明目标是在固定前提下每次自然更新保持条件；没有经验秒数或 tick 数可替代它。若某个 callee、别名或 lifecycle 无法闭合，则保留具体 UNKNOWN，不能追加 Hook、压制保存或直接调用普通 B 删除路径。

## Reader 规格与离线模型

契约枚举已有 owner/标量字段来源，区分 PREEXISTING、STATIC_FRESH_B 和 POSTJOIN_ONLY。B 尚未创建时，其安全条件必须由构造与 Init 数据流导出；事后 B 状态或 future route witness 不能满足 prejoin。InputManager 固定为静态对象 RVA `0x857B18`，不能多解引用一次；online roster 只有三指针 canonical-zero 才合格。

保守的离线模型预算为每次采样最多512次读取、65,536字节；main/secondary 模型容量上限4、本地设备32、树模型64。它们是 `DESIGN_ONLY` 的拒绝上限，不是引擎容量逆向结论。预算超出即拒绝，不能截断后接受。标量 alignment=1 表示按字节安全读取的最低要求，不声称已证明原生对齐。未知的完整容器、config owner、callback registry 与实体查询布局不填虚构偏移，完整 reader 标志保持 false。

`cs-t0/admission/admission_contract.py` 提供两项仅供磁盘工具和独立测试使用的接口：

- `check_contract(dict)`：校验固定身份、目标、权限、完整义务、字段和版本规格；成功返回诊断摘要，非法输入抛 `ContractError`。它不是语义定理证明器。
- `evaluate_model(contract, model)`：检查明确标为 `SYNTHETIC_ADMISSION_MODEL` 的来源、逐条操作、容器、读跨度和身份样本；返回 accepted/reason，scope 始终为 `SYNTHETIC_SPECIFICATION_ONLY`、runtimeObserved=false。拒绝暂态写入或失效后恢复的轨迹，不只比较终态。

模型 source channels 覆盖已知 raw/active/temporary/effects/implicit/map/byte/trinket/递归/关联/config/seed/pending-config/pill-history/familiar-carry 来源。零来源模型不是原生 effective-count 运算复刻，不证明完整151处 desired-call 覆盖；供给轨迹可能遗漏 native 操作，因此即使模型接受也不能关闭原生数据流缺口。模型容器是原始数字跨度及节点关系，不是已完成的 native memory reader。

## Generation policy

generation 由 adapter 持有，绝不发明 native generation 偏移。必要采样点为 PREPUBLICATION、SELECT_EDGE、CONFIRM_RECHECK、DOUBLE_BOUND；采样身份包括进程创建身份、GameRoot、onlineRoot、PlayerManager、InputManager、main/secondary/device vector、B device/shared-control。

生命周期、owner/vector/device 替换、control 退役、读取失败、漂移、无法排除 ABA 和进程身份复用均永久失能，不复发 UI edge、不自动重试。已有 callback 的 original-once 义务继续保留给未来 runtime；本轮没有实现或验收 callback wrapper。

相同地址/样本不证明两采之间没有替换；精确可观测失效来源在既有55槽候选边界内尚未完整闭合。因而 `generationContractSpecified=true` 只表示上述失败策略已经规定，`nativeAbaExclusionProven=false` 与旧组合缺口继续保留。停止不确定时保留 Unsafe 和模块，不强卸载，不以 native teardown、退出 B 或重开作为恢复手段。停止后的私有清理不应改变最后观测 generation；真实 lifecycle 失效与私有清理须在未来 runtime 分开。

## 新增磁盘入口及版本保护

```powershell
pwsh -NoProfile -File .\cs-t0\inspect-coop-native-join-admission.ps1 -GameExe '<disk-target>' -RepositoryRoot .
pwsh -NoProfile -File .\cs-t0\verify-coop-native-join-admission.ps1 -GameExe '<disk-target>' -RepositoryRoot . -Output .\out\cs-t0\<new-verification.json>
```

inspector 可选 `-Output`，只允许现有 out/cs-t0 或 artifacts/cs-t0 顶层简单 ASCII JSON 文件名；拒绝覆盖、历史 source 子目录、设备别名、ADS 和 reparse 路径。Python 参数仅选本地运行时，不包含游戏进程操作。合法输入始终输出 BLOCKED/exit1；非法输入非零退出、无结构化报告或输出覆盖，不泄露目标绝对路径。输出的独立解码、原生语义验收、实机和主动权限字段均为false。

inspector 固定 pin 新契约及其两份 evidence，核对直接 predecessor/reused bindings、磁盘目标和新增锚点字节，前后复核输入哈希。它不把旧 snapshot-relative bindings 当作仓库根引用递归解释。完整历史保护、独立解码和语义评估由新 verifier 执行，测试放新目录，避免旧 verifier 枚举旧目录时改变验收范围。

独立结果另行 create-new 记录；本实施记录不预填测试通过。通过只认证实际执行的新静态／规格门及保留阻断，不能认证原生全闭合、生产 reader 或55槽 runtime。旧生产 preflight 仍绑定v1、返回原七项缺口与 BLOCKED/exit1；不重建生产、不重签旧验证、不将旧全套自动门说成本轮重跑。

发布顺序为新证据及本实施记录 → 新契约和封印工具 → 独立验证/结果 → inspection → plan v9 → join progress v3 → 发布后独立审计。不得回填已封印产物或创建SHA循环。矩阵第11项仍为 NOT_STARTED；active/live、05-05、T0-05整体和T0-06继续关闭。
