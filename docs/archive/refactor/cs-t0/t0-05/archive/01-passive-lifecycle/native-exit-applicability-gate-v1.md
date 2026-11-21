> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-03 原生退出适用性 gate 迁移 v1

日期：2026-08-31。用户授权：实施 gate 迁移并重跑自动门，不包含实机操作。

当前状态：实现、完整自动门 28/28 和独立最终审计 15/15 均 PASS；8 份 create-new 产物已发布，仅解锁 CS-T0-05-04 工作包资格。active 实机和 T0-05 总解锁仍关闭。

## 1. 范围与证据

本实现承接 [适用性文档修订](native-exit-applicability-revision-v1.md)，使用独立的新命令、contract 和 decision identity，不改变旧六源 gate 或 source analyzer。

- 固定五份 PASS：native join live-06、death/respawn live-08、room live-09、device live-11、menu live-12。
- 原 native exit live-07 继续是 `BLOCKED_BY_EVIDENCE`，只作已绑定哈希的反证，绝不改成 PASS。
- [机器适用性契约](../../../../../../../cs-t0/coop-native-exit-applicability-v1.json) 精确绑定目标、等价关系、五源/旧 exit 的身份与三份文件哈希、各自历史 producer 版本及 [149 文件不变基线](../../../../../../../cs-t0/coop-passive-applicability-baseline-v1.json)。
- [新增独立静态证据](../../../../../../../cs-t0/coop-native-exit-applicability-static-v1.json) 包含 29 个精确字节锚点。自动门直接读取已批准目标的磁盘 PE，检查全部锚点、REL32 和 image pointer 目标，不打开游戏进程。

N/A 仅限已冻结 ordinary 构造、main-vector Variant0 Player 与 slot48 guard。旧 graph 的 positive 标签证明分支存在，不证明普通 B 可达。未证明 Variant 永远不变，未证明整个游戏无其他退出入口；新 caller、morph、构造方式或目标变化均需新证据。

death 保留“用户确认＋继承生命周期”，不升级为专用 death Hook 证据。menu 的人工 UI 确认仍为 `NOT_RECEIVED`，新版另从原始事件核验 teardown 前同 generation/pass/topology 的 A/B 双 BOUND；固定 live-12 及原 analyzer 的退休/无复活约束不变。

## 2. 新 CLI 的两层结果

命令为 `coop-passive-applicability-gate`。必选参数是 `--repository-root`、`--applicability`、`--native-exit-source`、五个适用场景的同名 source 参数以及 `--output`；只额外接受可选的 `--verification`。旧 `coop-passive-gate` 仍精确要求六份 source，不接受 N/A 参数。

| 输入资格 | 新 decision 状态 | `csT00504Unlocked` |
| --- | --- | --- |
| 五源＋限定 N/A 验证通过，但没有新版完整 verification | `PASS_EVIDENCE_ONLY` | false |
| 同上，且新版完整 verification 与当前生产二进制、代码和测试哈希全部匹配 | `PASS` | true |
| 缺项、漂移、未知/重复字段、不安全 source 或无效 verification | 退出码 1 或 2，不创建 decision | 不授予 |

新 decision 为 schema 3、`IsaacCsT0-CoopPassiveApplicabilityDecision-v3`。适用性 contract 必须与编译时冻结的完整字节一致，不能重写为通用 skip；verification 使用必填字段、未知字段拒绝和重复权限键拒绝。报告必须包含精确有序的 28 项自动检查及全部 artifact/production binding，当前执行的 CLI 也必须匹配被验证的生产 EXE。

正式 gate 的 `--verification` 输入是本轮 `coop-verification-v5r7.json`；`coop-verification-v6.json` 是 gate 之后的收尾报告，不是该参数的替代品。

所有 source 参数必须对应 repository-root 内冻结的仓库相对路径；临时测试仓库可复制资料保持相对布局，但路径逃逸或场景替换均拒绝。输出必须 create-new，且不能位于任一 source 目录内。前后均检查全部不变基线，不在持久 decision 中写入调用者绝对路径。

## 3. 独立测试与红→绿

逆向子智能体只提交静态证据与 scope 复核；测试子智能体通过公开 CLI、只读测试导出和临时副本编写 [17 项独立黑盒测试](../../../../../../../cs-t0/tests/coop_passive_applicability_contract.rs)。主线独占 Rust gate、CLI、自动门和版本链。

测试覆盖 176 项 contract scalar、16 项 contract shape、152 项递归文件、152 项 verification 结构/字段、18 份 source 文件漂移及旧 exit 假 PASS、27 项 source 安全/零能力/隐私反例，以及 CLI、路径、重复/缺失 source 和 create-new。正式分支的正向 fixture 只存在于临时测试仓库，不冒充真实自动门证明。

独立测试发现 verification 的普通 map 会接受重复权限键：`activeInputAllowed:true` 后跟同名 false 时，未修复 CLI 返回 0。[红证据记录](../../../../../../../out/cs-t0/passive-applicability-duplicate-permissions-red-v1.json) 保留实际观测，不伪填未捕获的输出资格字段。主线将权限改为 14 个严格必填字段；修复后 14 项重复权限键全部拒绝，同步伪造 Controller 文件及其 hash 也不能绕过当前 EXE 检查。

## 4. 完整自动门

入口为 [verify-coop-passive-applicability.ps1](../../../../../../../cs-t0/verify-coop-passive-applicability.ps1)。它完整调用字节未变的旧 passive verifier，覆盖 Rust test/fmt/check/clippy/Release、既有 39/262/407 Native 与 device v2 harness 的 Debug/Release、真实跨语言 mapping、六场景离线 fixture/mutation/旧 gate、bootstrap/static/C0/T0-04 及 MCP 回归。

新 wrapper 要求继承 `cargo test --all-targets` 输出中 17 个指定黑盒测试都有实际成功记录，不能用过滤后零测试通过代替。继承脚本自身的重复测试仍照常执行。随后逐一重放五份 PASS 与 live-07，检查旧六源 gate 仍 BLOCKED，并执行新 evidence-only 门；只有这些步骤全部成功，才发布新版 verification 并运行正式新 gate。

完整 v3 运行已正常退出 0，[verification v5r7](../../../../../../../cs-t0/coop-verification-v5r7.json) 为 28/28 PASS；原始 [v3 日志](../../../../../../../out/cs-t0/passive-applicability-automatic-gate-20260831-v3.log) SHA-256 为 `714FD964E4B0315041D6FD4AC1BE097B405261823E58F59A8CCBA3D3386F2F9F`。旧 verification v5r6 的 22/22 只代表当时的 pre-live 自动门，不能代替本次迁移的 28 项检查。

失败尝试均按 create-new 保留。第一份 [v1 日志](../../../../../../../out/cs-t0/passive-applicability-automatic-gate-20260831-v1.log) 记录构建访问 SDK 目录受限；获准的构建上下文中读取已恢复。第二份 [v2 日志](../../../../../../../out/cs-t0/passive-applicability-automatic-gate-20260831-v2.log) 中完整继承测试通过，五份 PASS 与 live-07 BLOCKED 按原结果重放不变，但 wrapper 把 scenario flag 与 source 路径合并为一个 argv 元素，旧六源 CLI 因参数格式拒绝，未发布任何 verification 或解锁产物。

[拼参修复记录](../../../../../../../out/cs-t0/passive-applicability-invocation-repair-v1.json) 保留复现与公开 CLI 冒烟结果。现使用唯一的逐元素参数 builder；只读测试导出 `-TestOnlyExportInvocation` 输出 `TEST_ONLY_INVOCATION_NOT_VERIFIED`，不读取游戏、不执行控制器、不创建文件，且禁止与 verification/final 发布参数组合。它不是部分自动门或 skip 模式，不能授予资格。独立回归实际执行导出的参数，完整门仍须从头运行。

[独立补充回归记录](../../../../../../../out/cs-t0/passive-applicability-invocation-repair-v2.json) 记录第 17 项的红→绿：显式 `-PublishFinalArtifacts:$false` 最初被导出模式接受，虽未写文件，仍违反禁止组合的规格；改为按参数是否显式存在拒绝后，四种发布参数组合全部拒绝。该测试还覆盖含空格路径、整个临时目录树不变和实际新旧 CLI 调用，定向 1/1 PASS（33.30 秒）、fmt/clippy PASS；在随后从头完成的完整 v3 门中，四轮全量测试均为 17/17 PASS。

## 5. Create-new 版本链

输出顺序固定，避免循环哈希：

1. `coop-verification-v5r7.json`：28 项完整自动检查，权限全关。
2. `coop-passive-decision-v3.json`：五源＋N/A 正式门，只开放 05-04 工作包资格。
3. `coop-matrix-v3.json`：保留 20 个编号，19 个适用 primary＋1 个 N/A；当前完成 9 个适用条目，11～20 的 active 条目仍待执行。
4. ledger v4、hypothesis index v3：记录受限结论与自定义退出等未证明能力。
5. `coop-verification-v6.json`、`coop-plan-v6.json`、progress v8：绑定上述结果并关闭 passive capture 资格。

每个后继引用已有文件的真实 SHA，旧文件均保留。原六源规则下的 5/6 与 live-07 BLOCKED 作为历史事实保留；新版不得宣称六次实测均 PASS。virtual 条件的两个 primary、两个 repeatability source 和 comparison 都没有被减免。

当前入口为 [decision v3](../../../../../../../cs-t0/coop-passive-decision-v3.json)、[matrix v3](../../../../../../../cs-t0/coop-matrix-v3.json)、[ledger v4](../../../../../../../cs-t0/ledger/isaac-cs-t0-coop-ledger-v4.json)、[hypothesis v3](../../../../../../../cs-t0/coop-hypothesis-index-v3.json)、[verification v6](../../../../../../../cs-t0/coop-verification-v6.json)、[plan v6](../../../../../../../cs-t0/coop-plan-v6.json) 和 [progress v8](../../../../../../../cs-t0/coop-passive-live-progress-v8.json)。矩阵完成的 9 项精确为 8 个字面 PASS＋1 个继承 PASS_COMPOSITE，不改写 C0 的复合证据标签。

独立 [最终审计 v2](../../../../../../../out/cs-t0/applicability-final-independent-audit-v2.json) 为 15/15 PASS，唯一 [Release formal replay](../../../../../../../artifacts/cs-t0/coop-passive-decision-v3-independent-replay-01.json) 除 `createdUtc` 外逐字节一致。8 产物、19 条向前 SHA 依赖边、20 个构建输入、3 个当前生产文件、149 基线和六份 source 的 18 个文件全部一致。审计 v1 对 PASS_COMPOSITE 的错误假设在执行 CLI 前被拒绝；该失败记录保留，仅审计工具另建 v2，正式产物未改写。

## 6. 解锁边界

即使正式新 gate PASS，也仅表示可以开始 CS-T0-05-04 的实现与前置验证；不表示 active runtime 已通过，不允许立即实机。`activeInputAllowed`、输入/世界修改、scheduler/apply、OriginalOnline 正向能力、passive Hook 资格与 `t005Unlocked` 均保持 false。active 实机仍须新前置验证及单次授权。

自定义 Endpoint 逻辑退出与物理移除 B 仍为独立后续能力。保留原 exit Hook、ABI/original-once、53/56 槽事务、停止输入、drain、恢复及 Unsafe 回归，不允许因 N/A 直接调用删除函数或复用历史授权。
