# tx-01-51：重新绑定后的新鲜输入启动例外

日期：2026-10-17。用户已明确要求“按照启动例外的方案修复这个问题”。目标为消除已唯一重绑后由 C/S lead=2 产生的两步中性间隙；A/B 使用相同规划规则，仍经 codec、Submit、queue、Resolve，正常运行 lead=2、容量和预算不变。tx-01-50 的 FAIL、原始捕获和通过的死亡/复活事实保持。

## 修改前确定的场景、失败判据与产物

一次最终统一入口完成新构建和新隔离 Offline 游戏窗口：A 为阿萨泻勒 ID7/键盘；单次 B 创建及原生 Join；A/B 首次有效绑定和 pause/resume 后启动；A 按住攻击键跨房不松，再在移动中重叠切换相邻射击方向；B 脚本新鲜首包、首次显式 Drop edge、后继普通 lead=2 输入；停止输入中性；菜单 teardown、pair 缺席、48 槽恢复、DLL 常驻。

FAIL：启动使用旧 generation/旧采样；首包绕开队列或查询直接读物理状态；启动后下一步再出现人为缺口；连续量预填充重复 edge；正常 lead 被改变；双键重叠的非零真实输入被投影为零；重复 Resolve/commit/consumer/accept；原生/传输错误、未恢复或失去常驻 pin。原生控制/动画抑制不凭方向缓存中性一概命名为丢蓄力；用户画面观察独立记录。未观察场景记 NOT_OBSERVED；失败不自动重跑。

构建/验收前不运行任何测试、试跑、单元测试或额外前置采集。本轮不沿用 tx-01-50 的一次前置采集例外。实机仍沿用既有授权，待代码/入口/操作清单完成后仅确认新进程和隔离场景就绪。目录 `artifacts/cs-t0/tx-01-51-live-01`；保留 fixture、操作计划、输入决策、原始事件、逐项报告、人工观察、恢复结果，不计算哈希。

## 启动规则

每 generation 的首次新鲜输入可启动一次。设 completed=C，下一 Authority tick 为 C+1，正常 nominal target 为 C+2。生成一条完整输入命令指向 C+1；同一新鲜样本的连续量另以新 sequence 按正常规则排入 C+2，edge=0。后一份是正常 lead=2 的预填充，不是缺输入时延续旧 current。下一次新鲜输入仍排 C+3，因此 C+2 不再空缺。只有实际源的新鲜输入可以启动；没有输入仍立即中性，不等待另一端。

两条命令均使用当前 generation 和现有 Submit/queue；一次 Bind/Invalidate 后的启动资格只由该 generation 的首个即时接受或实际 Resolve 消耗。错误、重复、非法、过期和旧 generation 包不得启动；脚本晚到的旧 target 不滑动到新 tick。正常命令保持原有 target。

A 由当前原生 poll 采样，记录不可变 raw/canonical、实际 directional/held mask、exact binding 与 loop iteration。即使临时禁用，也只更新物理采样基线，不发布命令或快照；Authority 重新唯一绑定后仅可使用同一次 loop、同一 exact pair 的新鲜 poll。跨 lifecycle 不清物理按键基线；旧 frame 的 rising 不带入新绑定，同 frame 已发布的 edge 也不重复。查询仍只读已 Resolve current。

B 只在 ScriptInput 真实产生本 generation 的按期新命令时使用同样的启动规划；不自动重放已消费脚本或已排队旧命令，不自动重建 B。共享规划的普通预填充清掉 edge，首包的显式 edge 不要求 PreviousComparable。

## 范围

修复已确认的重绑启动间隙，并观察相邻重叠换向；换向问题若来自不同源因，不凭本次启动方案声称全部修复。不更改武器蓄力内存、不跳过原生更新、不扩大原版/Isaac Sync/V1 行为、不新增 Hook；48 槽统一 owner 和 NEVER 回收策略保持。

## 已完成实现，待一次整体验收

`Participant` 增加每 generation 的 startup_pending；Invalidate 重置资格，接受 completed+1 或首次实际 Resolve 后消耗资格。`SubmitFresh` 为双方共用规划，完整首包和清 edge 的正常 continuation 都经现有 codec/Submit/queue。旧序号去重与高水位不重置。

物理 poll 即使在临时绑定失效/暂停时也只更新真实采样基线；Authority 唯一重绑后，仅复用同 loop/exact pair 的观察。physical rising 历史只因真实物理 device/control/K 更换而重置；同一 loop 发布过的 edge 用 mask 记录，不随 generation 重置。Raw snapshot 不写 participant current/previous，也不从 synthetic 查询取值。

IPC 仍为 128 字节事件，新增 StartupInput=22（首包/continuation sequence/target/exact pair/loop）及 PhysicalObserved=23（真实动作 held mask、canonical、loop、已消费 edge）；未增加 Hook，仍为 48 槽。

统一入口：`cs-t0/accept-startup-input-e2e.ps1`；fixture 为 `testdata/tx-01-51-startup-script.json`，首个显式 Drop edge、120 步移动/射击、重复包和旧 generation 拒绝；操作清单 `testdata/tx-01-51-startup-operations.json`。报告 `report-startup-input-e2e.py` 仅分析同一捕获：首包与 nominal 接受/Resolve 配对、物理源同 loop/exact pair、常规 lead=2、后继无缺口/无重复 edge、双方两原生路径、跨房重绑和相邻真实重叠、恢复与传输。蓄力画面 PASS/FAIL/NOT_OBSERVED 从独立人工观察文件读取，未观察不能据原生方向字段代记通过。

## 一次最终验收结果

用户确认完全重启并准备好后，统一入口执行一次。Rust Release、Native/helper v143 Win32 构建成功；PID14796、creation time134366437759460762。336,848 条事件，6,524 Authority tick；nativeError=0、transportFailure=null，Resolve/commit/consumer/accept 无重复、查询无不一致。未运行其他测试、未自动重跑。

自动专项 `artifacts/cs-t0/tx-01-51-live-01/result.json` 为 NOT_OBSERVED。52 条 A 启动记录，其中 51 个完整首包/nominal Resolve 配对，45 次对应 room 后启动；未出现首包把空档搬到下一步、普通 lead=2 改变或旧 edge 复制的报告违规。双方共用规划的 A 路径实际通过；B 路径尚未验证。

本轮设备成功单次注册 #21；首 Join pass #113 中 action19 true 查询 #114/#115，第二 pass #172 的 action14 为 NOT_CALLED #174，当前两 pass 范围随后耗尽。原生 Main 列表曾出现与自有 K/device/control 关联的 Player，但未观察到受控 creation callsite，g_attempt=0，#123 因 OWNED_IDENTITY 拒绝绑定；没有 Joined 发布。早先口头“B 未生成”过于概括，准确结论是没有接纳为生产 B，不能认定没有原生对象，也不能认定受控创建成功。未追加创建、未切换备选路线。

用户画面反馈：相邻方向重叠不再丢蓄力；始终按住攻击跨房仍会自动结束蓄力，经追问确认是自动发出激光。相反攻击方向同时按住仍丢蓄力，属于当前向量相消编码的另一个问题。不能据 startup 配对通过声称整个输入问题修复完成。

一次 mark/status 写入因自动审批无法完成而未执行，返回账户用量上限；没有绕过审批。用户退回主菜单后，使用原控制器正常 stop 成功。原生 teardown、pair 缺席/root=1 #336799，48 槽恢复汇总 #336848；pinned=true，游戏同进程仍存活。

人工结果落盘晚于自动分析器结束，因此原自动报告保留 NOT_OBSERVED，没有重跑分析器改判。人工事实为 `manual-charge-observations.json`，补充最终判定 `operator-result.json` 为 **FAIL**（跨房未松键却自动发射）。两份结果的来源和时序明确区分。

后续未完成：跨房过程中原生射击/蓄力消费的完整窗口、反向双键语义，以及本轮未受 owner 接纳的原生 Join 路径。需要实质修复后再安排下一轮；本轮不重跑。
