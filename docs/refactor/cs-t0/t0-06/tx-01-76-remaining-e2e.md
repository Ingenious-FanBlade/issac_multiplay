# tx-01-76：中断验收的剩余项续跑

目标：按用户要求继续 tx-01-75 尚未完成的测试，只排除全量 replay；允许单条样本的宽整数重编码及非法 decoder 用例。生产实现与已冻结二进制不修改、不重构建。上轮 INTERRUPTED 和更早 FAIL 保留。

统一入口增加 `-ContinueFrom <旧 artifact>`、`-SkipFullReplay`、`-SupplementalCapture`。只接受已中断的来源，二进制必须使用其 `binaries/` 副本。新 artifact 独立创建；来源只读。上轮 PASS 明确标作 `CARRIED_FORWARD`，不声称本轮重新执行。

一次执行依次完成：读取已保存 primary coverage/index，选择完整普通样本，运行单字段/正负零/非有限值、宽整数、冲突、未知与缺失、早期 gap、非法 decoder；扫描 secondary 后等待人工提供明确对齐与依据；在全新人工启动进程采集 Familiar/Projectile；最后收集人工证据并统一输出结果。单条样本仍保留全部实际对象与固定 profile，不缩减字段来获得通过。候选样本在任何比较用例执行前固定，不在失败后更换重跑。

用户已确认可准备跟班和敌方弹幕，特殊流程需另定。Morph/Restore/Replace/复用、ABI/只读/RNG 动态跟踪及受控故障/失联恢复缺少具体场景或取证定义时保留 NOT_COVERED；不猜测触发或替用户操作。跨进程不自动按 tick 距离、地址或位置匹配，单纯指定观测对比须用户明确认可其范围，不等同严格同 seed/input/模拟时刻。

产物包括 `continuation.json`、`prior-evidence/`、`frozen-sample.json`、原始新采集、各 CLI 输入/对齐/输出、逐命令退出记录、补充扫描和恢复记录、最终 ledger 与文件清单。不计算或校验哈希。以上是运行前确定的范围。

2026-10-19 运行收尾：一次统一续跑已完成，**INCOMPLETE：19 PASS / 0 FAIL / 14 NOT_COVERED**；9 项新增、10 项显式沿用。Projectile 可读，Familiar 缺出生/Init 依据而保留 NOT_READY，70 槽正常恢复。用户选择保留缺口收尾，见[实际结果与十四项未覆盖](tx-01-76-e2e-results.md)。没有自动重跑。
