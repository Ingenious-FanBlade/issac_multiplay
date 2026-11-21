# tx-01-75：即时消费修改的统一 E2E

日期：2026-10-18，用户时区 Asia/Shanghai。状态：**用户明确要求结束本轮，最终报告为 INTERRUPTED；33 子项为 10 PASS、0 FAIL、23 NOT_COVERED。完整 E2E 尚未完成。** 使用新 Controller/Native/Helper 启动同一个统一执行器；没有在验收中重新编译或修改生产代码，没有另开测试入口、自动重跑或计算校验哈希。

[最终结果](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/result.json)、[计数及覆盖范围](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/report-counters.json)、[收尾记录](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/finalization.json)、[逐项记录](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/checks.jsonl)、[组汇总](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/cases.jsonl)、[文件目录](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/file-inventory.jsonl)和[时间线](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/timeline.jsonl)均已保存。13 组均仍有未完成子项，不能将已通过的采集子项外推为整组通过。

本轮在原有 32 子项上增加 `W07.consumer_pipeline`，共 33 项；统一入口仍为 tx-01-74 的执行器，验收目标和二进制为 tx-01-75。[目标构建记录](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/target-build-record.json)、二进制副本和全部源码快照已经保存，新消费模块 `world_record_live.rs` 也包含在快照中。旧 tx-01-74 的 FAIL 和原始材料不变。

## 已完成的实机阶段

| 段 | 游戏 PID | controller | 当前确认的结果 |
| --- | --- | --- | --- |
| default | 27368 | 38568 | 用户确认 A/B 行为正常；v3、51 槽恢复子项 PASS，nativeError=0；旧较大套件的 NOT_OBSERVED/exit=1 原样保留 |
| primary | 30112 | 3172 | 用户确认普通换房正常；CAPTURE_CLOSED/exit=0，70 槽恢复子项 PASS |
| secondary | 26904 | 48588 | 用户确认同一路径普通换房正常；CAPTURE_CLOSED/exit=0，70 槽恢复 |

primary 的 [producer result](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/captures/primary/capture/world/result.json) 和 [consumer.json](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/captures/primary/capture/world/consumer.json)显示：

- attempts=33,050，captures=16,427，backpressure=0，native_error=NONE。
- 原始批次复制与写入均为 33,050；事件复制、写入和 produced/consumed 均为 5,187。
- 自有队列峰值 299,768 bytes、29 项，预算等待 0；上限仍为 256 MiB / 256 项。
- 原始复制最长耗时 159,900ns；这不是发布到唤醒的延迟，不能据此证明完整读取时延或零干扰。
- reader_done=true、files_flushed=true，outstanding items/bytes=0，failure=null，Native callbacks=0，producer_closed=true。

实时进度在普通换房后从 429 增长到 4,333、7,764、10,972 次连续完整普通采集，达到目标后正常停止。随后同一执行器正式扫描确认：**W07.continuous_10000 PASS，最长 13,567 次；W07.consumer_pipeline PASS；W13.normal_restore PASS。** 最长区间为 attempt 5918–33050，同一 run/room，中间 13,566 条明确 odd-only gap 保留、不计数。全部 33,050 条记录/5,187 条事件无序号、profile 字段或事件缺失错误。

可读类别为 Effect、Grid、NPC、Pickup、Player、Tear；Familiar、Projectile 尚未覆盖。W07 的完整只读/RNG/原调用及唤醒延迟证据仍不足，不能把两个自动子项的 PASS 当作 W07 整组通过。

secondary 原始批次复制/写入均为 6,290，事件为 4,819，队列峰值 299,944 bytes / 20 项，预算等待 0，最终 flush 和排空成功。两段均未出现消费者 failure。

## 用户中断及报告收尾

用户已确认 secondary 准备和普通换房，之后正常停止。用户明确选择跳过受控 controller-loss，原因是未准备控制器退出后保存实际恢复字节的手段；没有执行该故障动作。

primary 的 captures.jsonl 为 2,297,381,939 bytes；第一轮全量 replay 从 UTC 14:54:41 开始，到 UTC 14:57:50 约输出 114,384,897 bytes，约 0.6 MB/s。离线 replay 仍直接对 File 做 JSON 小块写出，尚未使用本轮 live writer 的 BufWriter。按当时速率，两次全量重编码估计约 2 小时；这只是观察速率估计，不是性能契约。发现明显耗时后先向用户询问，没有中途修改代码或替换二进制。

用户询问全量 replay 的用途，获知它用于验证全部记录的字段/原始位串、重复编码字节及输入保留后，明确选择“结束本轮，保留结果并终止自有离线子进程”。普通 Ctrl+C 没有让执行器完成中断收尾。随后按命令行、专属二进制路径、创建时间及父子关系确认身份，先终止本轮 Python 执行器 PID 23764，再终止其当前 replay 子进程 PID 35076；两者均确认退出，游戏进程未被操作。见[终止请求](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/user-termination-request.json)和[终止结果](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/user-termination-result.json)。统一 shell 会话随后退出码为 1，不是游戏 controller 的退出码。

第一轮 replay 保留 454,168,146 bytes 部分输出，没有完成 REENCODED 结果；子进程实际退出码未被原执行器记录，不补造 exit.json。其[外部终止记录](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/commands/001-replay-a/external-termination.json)明确 exit_code=null。第二轮 replay、逐字段/字节核对、离线变体、secondary 正式扫描、显式跨进程对齐/比较及最终人工审阅均未执行；本轮 W08–W11 相关子项保留未覆盖，不能借用旧 tx-01-74 的 PASS。

由于原执行器已终止，使用[仅整理已保存材料的收尾脚本](../../../../../artifacts/cs-t0/tx-01-75-e2e-01/finish-interrupted-report.py)恢复原 checks 状态、记录未完成原因，并生成报告和文件目录。该脚本没有目标进程访问，没有运行生产 CLI、重扫描样本或执行新测试；finalization 明确 `original_executor_self_finalized=false`、`REPORT_FINALIZED_AFTER_USER_TERMINATION`。这表示报告已整理完成，不表示 E2E 正常执行完成。

10 个自动 PASS 为：W01.default_51、W01.optional_70、W02.record_invariants、W03.profile、W03.sources_alias、W04.creation_events、W06.bootstrap_witness、W07.continuous_10000、W07.consumer_pipeline、W13.normal_restore。其它 23 子项为场景/证据不足、用户选择跳过，或因本轮结束未执行；没有新增功能断言 FAIL。

## 后续未完成内容

离线写出性能问题尚未修复。后续实质修改后再按用户安排进行一次完整验收；不自动重跑本轮。Familiar/Projectile、特殊生命周期、完整 ABI/只读/RNG/时序、故障隔离、严格双进程依据及受控失联恢复仍需实际证据。
