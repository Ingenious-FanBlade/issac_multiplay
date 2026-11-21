# tx-01-76：排除全量 replay 的剩余项验收结果

日期：2026-10-19（Asia/Shanghai；artifact 内时间使用 UTC）。按用户要求，只跳过全量 replay，允许单条样本的宽整数重编码与非法 decoder。一次统一执行已正常完成报告收尾，结果 **INCOMPLETE：19 PASS / 0 FAIL / 14 NOT_COVERED**。其中 **9 项是本轮新增 PASS，10 项是上轮证据的 CARRIED_FORWARD**，没有把上轮中断报告改为通过。

本轮仅扩展验收执行器的显式续跑、跳过全量 replay 和补充场景入口；生产实现与二进制不变，没有构建、单元测试或哈希操作，没有自动重跑。使用 tx-01-75-e2e-01 的已冻结三件套。旧 tx-01-74 FAIL 与 tx-01-75 INTERRUPTED 原始材料保持不变。

## 新完成的九项

| 子项 | 实际结果 |
| --- | --- |
| W08.wide_integer | 单条原样副本的 u64 `9007199254740993` 经生产 parser/reencoder 保留精确十进制字符串 |
| W09.single_field | 仅将 A 的 position.x 从 raw32 `440c0000` 改为 `440c0001`，准确定位 tick=4760、Player/player:00、字段 000b；只有这一项确定差异 |
| W09.signed_zero | +0/−0 比较不产生该字段差异，输入位串仍各自保存；其它未知区域保留 |
| W09.nonfinite | NaN、+Inf、−Inf 三种副本均为字段不可比较，没有误报相等 |
| W10.conflicts | 保留整个 key 冲突组，指向冲突组的引用不可比较 |
| W10.unknowns | 13 种缺映射、不可读、缺字段/来源/tick、重复 tick、阶段与引用状态变体符合预期 |
| W10.decoder | 10 种非法 domain/version/字段/宽度/长度/十进制/载荷被拒绝，保存 failure；预期非零退出不是用例 FAIL |
| W10.earlier_gap | 更早 lifecycle 缺口保留，差异标为首次观察到的差异，不声称全局首次分歧 |
| W11.production_compare | 用户明确确认的两个指定观测经生产 world-compare 成功处理，输出 DIFFERENT，未强制相等 |

单条副本取自上轮 primary 的第一条适合本轮覆盖的完整普通采集：tick=4760；所有原有对象、字段及固定 profile 都保留。选样发生在用例执行前，没有失败后换样。31 次单条生产 CLI 加 1 次跨进程比较共 32 次调用，分别保存精确参数、输出、退出记录；没有全量 world-replay。

跨进程范围仅为左 run=1/room=28/tick=32854，右 run=1/room=28/tick=5829，A→A、B→B，其它创建事件不映射。用户已确认只是指定观测比较。确定差异首先出现于 A 的 position.x（`438750d1` 对 `4390cbd8`）。它们不是相同模拟时刻，不能据此证明或否定同步一致性，W11.scenario_basis 保持未覆盖。

## 补充实机记录

用户手动重启并准备隔离本地场景，本轮 game PID=27508、controller PID=23540。用户确认跟班与敌方射弹实际可见、游戏正常并允许停止。控制器正常退出 0、CAPTURE_CLOSED，70 槽原始目标恢复通过。

- 3,921 条采集尝试/持久化记录，1,797 capture、2,124 gap；其中 odd-only 1,797、阶段不完整 318、生命周期 9。
- 背压 0，原始批次 copied/written 均 3,921；事件 produced/consumed/copied/written 均 5,537，最终 flush 完成、队列占用为 0、consumer failure=null。
- Projectile 已取得 VALUE 字段。Familiar 虽有记录，local_id=2/use_generation=1 的出生事件为 null，专用字段 0301–0303 为 NOT_READY；本轮未取得它的出生/初始化依据，未放宽读取规则。
- 全部 1,797 条 capture 因仍有 NOT_READY 不能作为全 profile 完整样本。没有重新要求 10,000 次；上轮已通过的 13,567 次证据单独沿用。
- 结合上轮，已有七类可读：Player、NPC、Tear、Projectile、Pickup、Effect、Grid。W03.eight_classes 仍未覆盖，不能把“画面存在跟班”计为专用字段已读通过。

下一次跟班场景需要在观察启用后，按明确的正常流程取得新的出生/Init 完成证据；具体触发方式仍需先与用户确认。当前没有再次注入、重跑或改变 FIRST_OBSERVED 读取规则。

## 仍未覆盖的十四项

| 子项 | 缺少的材料或范围 |
| --- | --- |
| W01.isolation | 完整原版/Isaac Sync/API/输入兼容动态证据 |
| W01.native_results | ABI、栈清理、返回寄存器及原函数次数的完整动态跟踪 |
| W02.phase_order | owner/Room/标准尾部及 Authority 提交的独立阶段跟踪 |
| W02.blocked_windows | 暂停、loading、生命周期窗口与实际缺口的完整场景标记 |
| W03.eight_classes | Familiar 专用字段可读的初始化依据 |
| W03.retained_unknown | 保留 Player、关系歧义、未知类别的实际场景 |
| W04.identity_history | 嵌套、短命、同址复用与成员历史的完整取证 |
| W05.transitions | Morph/Restore/Replace、房间与 run 退休的完整流程 |
| W06.revocation | exact-pair/generation 失效撤销与恢复场景 |
| W07.readonly_calls | 只读、RNG、线程、原调用次数和耗时的完整动态证据 |
| W08.replay | 用户主动排除的全量两次重编码；性能问题未在本轮修改 |
| W11.scenario_basis | 严格同 seed/input/模拟时刻与创建事件映射依据 |
| W12.fault_isolation | 受控消费/资源/读取故障及 scheduler 继续运行的真实证据 |
| W13.controller_loss | 实际失联操作与独立恢复原字节证据 |

用户选择按现有材料收尾，保留上述缺口。除排除的全量 replay 外，尚余 13 项，不能表述为其它测试全部通过。

## 可检查产物

根目录：[tx-01-76-remaining-01](../../../../../artifacts/cs-t0/tx-01-76-remaining-01/)。

- [统一结果](../../../../../artifacts/cs-t0/tx-01-76-remaining-01/result.json)、[收尾记录](../../../../../artifacts/cs-t0/tx-01-76-remaining-01/finalization.json)、[文件目录](../../../../../artifacts/cs-t0/tx-01-76-remaining-01/file-inventory.jsonl)。
- [沿用与选样依据](../../../../../artifacts/cs-t0/tx-01-76-remaining-01/continuation.json)、[完整冻结样本](../../../../../artifacts/cs-t0/tx-01-76-remaining-01/frozen-sample.json)，原始 primary/secondary 路径明确记录，上轮来源只读。
- `offline/inputs/`、`offline/<variant>/` 与 `commands/`：所有自有副本、变更、对齐和生产 CLI 输出；`cross-process/`：明确对齐、用户范围说明和比较报告。
- `captures/supplemental/capture/`：invocation、operations、restoration、world 的 manifest/capture/gap/lifecycle、consumer、producer 结果；根目录 `supplemental-coverage.json` 与 `supplemental-record-index.jsonl` 可回溯样本位置。
- `user-review-evidence.json`、`manual-review-accepted.json`：用户确认范围、跟班缺口与十二项人工证据缺失的逐项理由。

执行器自身的最终状态为 INCOMPLETE、error=null、finalization=COMPLETED。外层工具 PTY 返回码为 1，PowerShell 输出已明确说明“存在未覆盖项”；不把外壳返回码改写为全套 PASS，也不把它当作某个生产用例 FAIL。实际生产命令逐项退出码和实机 controller-exit 独立保存。
