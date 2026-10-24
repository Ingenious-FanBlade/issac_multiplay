> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-03 device-rebind v2 取证计划

当前状态提示（2026-08-31）：以下保留 device v2 与 shutdown 修复各次实施时的范围和授权记录。device live-11 与 menu live-12 后续均已正式 PASS；新的五源＋限域 N/A gate 独立实现，旧六源 gate 保持不变，当前验证与权限见 [gate 迁移记录](native-exit-applicability-gate-v1.md)。本提示不改变设备因果契约、历史证据，也不构成新实机授权。

本修订只授权实现和自动验证，不授权注入、实机采集或自动重试。旧 prelive-block、live-06/08/09、plan/verification 和四份离线反例保持不变。

## 为什么修订

旧 device v1 只要求 `RELATION_CHANGE` 与自报 assignment-match，无法区分真实原生重分配和 manager-only 拓扑变化。新 analyzer 对所有旧 device v1 返回 `BLOCKED_BY_EVIDENCE`；其他 v1 场景按原判定只读重放。旧 slot49 的 self 来自紧前 spawn，不能证明既存 Player 重分配。

规范入口是 `cs-t0/coop-device-rebind-contract-v2.json`，静态证据为 `cs-t0/coop-device-rebind-static-evidence-v1.json`。三条新增 REL32 的 ordinal 是 53/54/55，目标均为 `SetControllerId` RVA `003A6450`，ABI 为 `void __thiscall(Player*, int32, uint32)`、`ret 8`，原生 callsite 的 propagate 固定为 1。它们仅在 device v2 场景安装；其他场景仍安装原 53 槽。

## 正向因果证据

初始必须已经得到 A→0、B→1 的双 BOUND。观察器通过实际 wrapper self 在完整双读 main vector 中的唯一位置确定 endpoint，同时核对原生 ordinal、既存 Player 身份和另一端关系；不依赖菜单选择值的猜测。

仅接受两条原生语法：

- 菜单：slot53 解除分配 → slot54 同一 Player 的一次 `-1→-1` 提示确认 → slot55 重新分配。
- 断连：slot54 解除分配 → slot55 重新分配。

每项都有 original 前 BEGIN 和完整返回后 END，独立读回实际 assignment。初次解除分配先推进一次 generation、发布 A/B UNBOUND；所有设备 witness 使用该 generation，BEGIN/END 不可乱序、重复、跨端或复用。

解除期间只允许严格结构双读确认的目标 `-1`/缺失设备状态保留临时事务，绝不发布 binding。另一端、Player/HUD 身份、offline roster/frame/backend screen 仍须满足原有约束。只有先前真实 ordinary route 资格可以限域移交，不能用设备事件创建在线资格。

重新分配完整返回且读回匹配实参之后，首个完整 coherent manager return 才发布 slot55/phase10 的 `RELATION_CHANGE` 和 A/B Candidate；下一连续完整 pass 的拓扑完全相同才发布 A/B Bound。允许同一个手柄重新接回；摘要差异既不是必要条件，也不是充分证据。manager-only delta 只能发布 `UNCLASSIFIED_RELATION_CHANGE`，source 不得通过。

任一生命周期、退出、teardown、其他 assignment、异常读取、wrong-thread、重入、第三/重复 Player、其他 endpoint 变化、online 信号、ring gap、旧 generation 或最终双采不连续均使设备因果无效。本 source 最多接受一次成功事务。

## 自动验收

逆向子智能体负责 Native/static；测试子智能体通过公开 CLI、序列化 source、真实 Win32 mapping、注入真实 original spy 和 raw event 流独立验收；主智能体集成 Rust、版本链和自动门。测试不得以生产私有“通过”布尔替代观测。

自动门必须包含：Rust fmt/check/clippy/all-targets/release，既有 Native 39/262/407-case Debug/Release，新 device Native Debug/Release，原有与 v2 真实 mapping，原生 raw stream 到公开 analyzer 的闭合，56 槽逐点失败/后写失败回滚及逆序恢复、callback drain/Unsafe，六场景离线 fixture、因果/身份/权限/隐私 mutation、四份旧假阳性拒绝，bootstrap/static/C0/active 回归，历史三份 PASS source 逐字节只读重放。

新 pre-live 链是 `coop-catalog-v4` → `coop-plan-v5r1` → 自动门成功后 create-new `coop-verification-v5r5`。未跑继承回归或没有实际目标文件的部分门只能报告 `PARTIAL_NOT_ELIGIBLE`，不能发布 verification PASS。所有主动能力、`csT00504Unlocked` 和 `t005Unlocked` 均保持 false。

## 后续实机操作（尚未授权）

自动门全部通过后重新申请一次 `device-rebind-passive` 授权。授权前确认新 source/output 名称、目标版本和当前游戏状态；不沿用上一轮“完成剩余两项”的笼统授权。

采集时先建立可验证的普通双人 BOUND，再提示用户做一次原生 Change Controller 或断连/重新选择操作。非零 backend 若需要 ordinary join witness，应在同一观察窗口中完成真实 join 预热；不伪造资格。若不能在窗口中建立初态或原生提示未出现，就保留证据失败，不猜测额外 Hook、不自动重试。

用户手动操作只负责触发行为，不作为 causal PASS 的替代证据。原生 wrapper 的实际 self 决定操作了 A 还是 B，不能从“B 按了暂停”推定目标 ordinal。完成后保持同房间稳定，等待自动 shutdown、drain、`55..0` 恢复、56 槽 before/after 复核、producer-close、analyze 和只读 replay。

任意 Unsafe 禁止卸载或自动重试；source/output 已存在、preflight 或证据失败均消费当次授权。设备 source 完整收尾后，才单独申请 `menu-teardown-passive`。真实进度仍为 3/6，native-exit 的 workflow skip 不计入六源 gate；本修订不解锁 CS-T0-05-04。

## live-10 后的 shutdown 一致性修订

live-10 已记录真实 B 断连/重连及 gen3 双 BOUND，但 private cleanup 把两个 terminal event 推至 gen4，manifest 仍为 gen3，因此正式结果永久保留为 `BLOCKED_BY_EVIDENCE`。本次修复授权仅包含实现与自动验证，不授权新的实机。冻结修复约束见 `cs-t0/coop-device-rebind-shutdown-repair-v1.json`。

device-v2 的 shutdown 必须在关闭 admission、第一次 drain 后清除 private candidate/bound、设备/普通 route 身份及 token secret，同时保留最后观测 generation；清理不是新的游戏生命周期。实际生命周期 `Invalidate`/`Retire`、legacy 53 槽、IPC/codebook、catalog v4、plan v5r1 和 analyzer 不变。两个 terminal event、shared owner 与 manifest generation 必须一致；任一 terminal Publish 失败保持 Unsafe，不能继续写 producerClosed 或覆盖为 Stopped。

独立回归必须经过真实生产 Shutdown 控制流与 Event/Publish，禁止 fixture 补 terminal event 或重编号/归一化 generation；可以注入离线 embedded observer 依赖，但必须明确区分测试内存的真实56槽事务与游戏实机。覆盖已完成、candidate、unbound、未完成设备事务、retired、generation 边界、drain/restore/验证失败、首/次 terminal ring 失败、关闭后 original-once 与无 publication/复活。真实 raw stream 经独立序列化到公开 CLI，完整路线通过，未完成路线和 terminal 漂移拒绝；live-10 原始分析逐字节保持拒绝。

修订后的 capture 只接受完整自动门新建的 `coop-verification-v5r6.json`，它以 v5r5 为 predecessor，绑定未变的 catalog/plan、修复约束与新生产文件。门失败或只跑部分回归不得发布 verification PASS。自动门通过也只恢复申请新 device 单次授权的资格，不创建授权、不改写 live-10、不解锁后续 active。
