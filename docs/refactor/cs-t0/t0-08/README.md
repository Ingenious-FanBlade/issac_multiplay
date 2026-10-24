# T0-08 最小实验协议

2026-10-20。整体状态：**PASS（操作1–9当前范围）**。

最新`tx-02-04`：验收器持有 A 物理输入会话后完成唯一整体 E2E，**33/33 PASS**、controller exit0、未自动重跑。初始4步和连续64步均真实发送且全程游戏前台；连续输入从首个真实commit tick3后立即开始，覆盖tick160..179 outage。A有646/6269个非零PhysicalSample并观察到真实位置变化。协议trace 56,070/56,070、lost0，6,268个真实commit的ACK/STATE/身份配对通过，70槽恢复、正常stop/清理、B中性/恢复和3/3 Drop边沿通过；离线19,022/19,022 PASS。见[实际结果/全部产物](tx-02-04-results.md)和[输入会话实现](tx-02-04-owned-physical-input.md)。

输入时序修复链：`tx-02-02`唯一E2E 29/31 FAIL，两项均因A全零；根因是连续输入在Authority激活前结束，见[输入窗口设计](tx-02-03-input-window.md)。`tx-02-03-e2e-01`又因外部输入接力超时在controller启动前失败（live=NOT_RUN，游戏未注入），原失败保留。`tx-02-04`改为验收器自身执行Win32 scan-code SendInput，并在每个按下/保持/释放点核对前台PID/HWND后通过。

历史`tx-02-01`：协议日志消费与失败收尾修复后唯一E2E为**23/30 FAIL**（trace4007/lost0、70槽恢复通过，A全零/停止末帧Publication7未解决），见[修复与失败方式](tx-02-01-log-recovery.md)和[历史结果](tx-02-01-results.md)。历史`tx-02-00`的trace overflow及前三次失败见[执行记录](tx-02-00-results.md)，均未重写。

- [tx-02-04 实际结果与全部产物](tx-02-04-results.md)
- [tx-02-04 owned 物理输入实现](tx-02-04-owned-physical-input.md)
- [tx-02-03 Authority 激活同步设计及外部接力失败记录](tx-02-03-input-window.md)
- [操作5–9实现与历史入口](tx-02-00-implementation.md)
- [实施前失败方式](tx-02-00-failure-modes.md)
- [E2E清单和操作步骤](tx-02-00-e2e-checklist.md)
- [历史协议静态复核](tx-02-00-static-review.md)
- [tx-02-00 历史执行记录](tx-02-00-results.md)

历史`tx-01-99`范围为操作1–4：独立magic/version/包上限/domain、envelope、四类消息和双端HELLO/首ACK握手；原wire v1与当次结果保留：

- [协议定义、状态机与复现入口](tx-01-99-wire.md)
- [实施前失败方式](tx-01-99-failure-modes.md)
- [静态分析与主智能体审阅](tx-01-99-static-review.md)
- [实际结果与复现](tx-01-99-results.md)：唯一一次内存双端验收 **305/305 PASS**，退出码0，保存30条原始包和双端状态。

操作1–4的独立协议核心和操作5–9的真实Authority接入均已通过当前范围验收；wire v2、IPC v6、无哈希/保留包CRC。`Established`仅表示协议握手完成；实机拓扑仍为一个真实游戏Authority及同进程自有逻辑Remote，不宣称socket/Steam、双游戏、真实网络断线、可玩Replica、完整世界、snapshot/baseline/delta apply、rollback或late join；这些属于T0-09/后续工作。
