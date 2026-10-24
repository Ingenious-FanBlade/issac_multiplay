# T0-04 Canonical Input 与 Loopback 实验契约

状态：`IMPLEMENTED_AUTOMATED / BLOCKED_ACTIVE_BY_RAW_APPLY_EVIDENCE_AND_AUTHORIZATION`。T0-03R V5 loop gate 已 7/7 `PASS`；真实 raw sample/apply 被动闭环和第二次主动授权仍缺失。本文只冻结 T0 实验身份，不是 Isaac Sync 产品 wire/ABI。

## 编码

`InputStateDraftV1` 固定 24B、小端：

| Offset | 类型 | 规则 |
|---:|---|---|
| 0 | `u16 version` | 必须为 `1` |
| 2 | `u16 length` | 必须为 `24` |
| 4 | `u32 heldFlags` | 仅 bit0=`shootHeld`，其余拒绝 |
| 8 | `u32 edgeFlags` | bit0 bomb、bit1 active item、bit2 card/pill、bit3 drop；其余拒绝 |
| 12 | `i16 moveX` | Q15，禁止 `-32768` |
| 14 | `i16 moveY` | Q15，禁止 `-32768` |
| 16 | `i16 aimX` | Q15，禁止 `-32768` |
| 18 | `i16 aimY` | Q15，禁止 `-32768` |
| 20 | `u32 reserved` | 必须为零 |

`shootHeld` 必须与 `aimX != 0 || aimY != 0` 等价。键盘、XInput、controller index、vtable、实体指针和进程地址均不进入 canonical bytes。

`InputCmdDraftV1` 固定 64B：`sessionId[16]`、`u64 clientId`、`u64 sequence`、`u64 targetTick`、24B state。session/client 不能为零，sequence 从 1 开始且 `0/UINT64_MAX` 拒绝。

## 量化

raw axis 非有限值拒绝；有限值 clamp 到 `[-1,1]` 后先乘 `32767` 并向零截断。每个二维轴对随后在整数域执行：

1. 计算向上取整的整数半径；
2. 半径不大于 `4096` 时归零；
3. 半径大于死区时，按 `(radius-4096)/(32767-4096)` 线性重标到满幅；
4. 每个分量用整数除法向零截断；
5. 最终结果压入 Q15 单位圆。

Rust 与 Win32 Native 共用 golden bytes。示例：

- 正向满幅/全 edge：`01001800010000000F00000001100000FF7F000000000000`
- 对角满幅：`0100180001000000000000007FA5815A815A7FA500000000`

## Queue 与 edge

实验常量固定：lead `2` ticks、future window `64`（含边界）、transport/input queue capacity `256`、drain budget `64/tick`、dedupe history `4096`。

- 只在 Authority tick 的 drain-before-resolve 阶段接收；已 resolve 的同 tick、past target、future `> current+64` 均拒绝。
- 同 identity、同 target、同 bytes 为幂等 duplicate，即使已 apply 或 queue 已满也不再次入队。
- 同 sequence 内容不同为 fatal conflict：client 安全退休，queue/world 不变。
- 更低 sequence 为 stale；sequence gap 可接受并计数。所有拒绝不推进 accepted/edge/world ledger。
- 同 target 的连续状态取最高 sequence；合法不同 edge OR 合并；相同 edge 的不同 identity collision 拒绝；每个 client/target 最多一次 adapter apply。
- Local planner 对每个 edge 单独使用 `max(observedTick+2,lastTargetForAction+1)`，因此同 raw sample 的不同 edge 可合并，同一 edge 的快速第二次 rising 必须进入下一 tick。

## 独立安全边界

- IPC magic 为 `ICSI04V1`，与 T0-01/02/03、现有 `ISCS` protocol 5 完全独立。
- Native exports 为 `IsaacCsT0InputInitialize/IsaacCsT0InputShutdown`。
- loopback 必须携带非零 32B 一次性 arm token；Native 用原子 CAS 消费，重复使用拒绝。
- 该路径不调用或放宽 T0-02 `ModeLatch`。T0-03R gate 未通过时 Native export 只发布 `BLOCKED_BY_EVIDENCE`，不安装 input hook。
- action `12+`、UI/window input、非 C/S 和 OriginalOnline-disarmed 不得创建 canonical/queue/resolve/synthetic apply 事件。

## 证据顺序

自动 codec/queue/analyzer/harness 通过不等于 T0-04 `PASS`。T0-03R passive 实机矩阵已冻结真实 loop envelope、physical-poll identity、controller ownership 和 UI/window 分区，但仍未冻结 primitive → action `0..11` mapping 与真实 offline/local apply。必须先以只读静态分析或无返回值改写的被动观测闭合 raw/apply 证据；之后再次人工确认，才能开启 loopback 主动实验。

2026-08-22 自动 harness 又加入中途 Apply 失败的动态回滚覆盖：relative-call 与 pointer-slot 两类事务分别在首个写入成功后制造第二个写入冲突，并验证所有先前写入、installed 标志与原值完整回滚。随后根据真实 binding alias 反例加入跨 Rust/Native 的 primitive 分类器：只有完整 binding 表中唯一且仅属于 action `0..11` 的 primitive 才能输出 gameplay action；与 action `12+` alias、仅属于 `12+`、布局超界或未绑定都 fail closed。Rust 130 tests、全目标 clippy、Win32 Debug 和 Release/LTCG Native harness 均通过。这些结果只关闭自动化安全覆盖缺口，不解锁游戏内输入。
