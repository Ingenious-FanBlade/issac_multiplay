# tx-03-01：T0-09 设计静态审阅

日期：2026-10-20（Asia/Shanghai）。这是设计文稿的一次静态审阅验收；没有运行游戏、构建、协议场景、probe 或测试。审阅对象：

- [T0-09 README](README.md)
- [T0-09 主设计](tx-03-01-design.md)
- [T0-09 实施前失败方式](tx-03-01-failure-modes.md)
- [任务计划 T0-09 章节](../isaac-cs-t0-task-plan.md#12-t0-09双进程与网络故障矩阵)

## 1. 审阅清单与结果

| 检查项 | 结果 | 说明 |
| --- | --- | --- |
| 用户决策完整性 | PASS | 双机 host+VM、两个真实游戏、host UDP relay、VM observer-simulator、双端脚本输入、完整矩阵分阶段、VM 共享 artifact、C0/C1 新鲜、C2 删除均已写入。 |
| T0-08 前置状态 | PASS | 明确引用 tx-02-04 的 33/33 PASS，并明确它是单进程逻辑 Remote，不是 T0-09 双进程证据。 |
| 双进程/网络边界 | PASS | 设计定义 host/VM 两端、独立 UDP data plane、host relay、control plane、wire IPC 和双端 artifact。 |
| 线程与所有权 | PASS | 网络 worker 只做 socket/ring；Native 主线程处理 Player/canonical/scheduler/world/Lua；已写明 ring fail-closed。 |
| 输入语义 | PASS | A/B 使用共享确定性脚本；A 本地 ordinal0 与 remote ordinal1 共用 codec/queue/admission；B observer 与 wire send 不重复 apply。 |
| observer-simulator 边界 | PASS | B 本地重放 A/B 仅作同 tick 观察，不应用 snapshot、不作为权威；只有 B input 走 wire。 |
| 故障模型 | PASS | F0–F5 配置、固定 seed、双向独立决策流、bootstrap 不注入、实测网络与配置 impairment 区分。 |
| 状态比较 | PASS | 最小 canonical subset、observation gap、`INPUT_LATE_OR_MISSING`/`APPLY_EFFECT`/`UNCOPIED_STATE`/`OBSERVATION_GAP`/`UNKNOWN` 分类，不计算哈希。 |
| C0/C1/C3 | PASS | C2 删除；C0/C1 双机新鲜 controls，C3 完整 custom 矩阵；不把“未触碰 Original Online”写成通过。 |
| 矩阵规模 | PASS | 完整 F0 10,000 tick、F1–F5 每 case ≥3 seed ×5,000 tick；F5 outage 前后各 ≥1,000 tick。分阶段但不设 gate。 |
| Stop/恢复 | PASS | 双端正常 stop、relay flush、两端实际 target 恢复和进程存活；失败路径只正常 stop 一次、不自动重跑。 |
| Artifact/隐私 | PASS | host/vm/relay/control/state/analyzer 目录、VM 共享文件夹、去身份化 manifest、磁盘预算与失败保留。 |
| 规范一致性 | PASS | 无 gate、无 SHA/hash 校验、无 Original Online/Isaac Sync 操作、无强制子智能体、无自动重跑。 |
| 引用可定位性 | PASS（修正后） | 审阅发现任务计划原有 3 个相对链接失效，已改为当前路径；T0-09 文稿新增链接均可定位。 |
| 失败方式前置 | PASS | [失败方式](tx-03-01-failure-modes.md) 60 项覆盖拓扑、IPC/线程、relay、输入、状态比较、控制和 artifact。 |

## 2. 审阅中发现并修正的问题

1. **任务计划 C2 与现行规范冲突**：旧 T0-09 仍列 Original Online 双端只读对照。已在该章节标为按现行规范删除，并改由 C0/C1 承担双机新鲜 offline/disarmed controls。
2. **Endpoint B 输入描述歧义**：旧固定拓扑只写 `local input source = B`。已明确 `wire input source = B`，本地 A/B 确定性脚本仅用于 observer replay，不是 authoritative apply。
3. **任务计划仍残留 `hash`/OriginalOnline 操作描述**：已将 T0-09 目的改为最小状态观察，ACK/hash 改为 ACK/STATE，状态 hash 比较改为最小 canonical subset/observation gap。
4. **C3 transport 描述过旧**：已改为 host 侧独立 UDP impairment relay，A loopback、B VM 虚拟网络。
5. **任务计划 3 个历史相对链接失效**：修正架构规划、T0-03R/T0-04 状态和 T0-04 reverse handoff 的当前路径；修正后 T0-09 文稿和 T0-09 任务计划均无缺失相对链接。
6. **设计稿缺少可实施 schema 草案**：新增 manifest、control envelope、Wire IPC v1、输入 step、ObservationFrame 和 first-divergence 六类最小草案，明确实现前仍需冻结实际 ABI/JSON schema。

## 3. 未完成/不宣称

- 本轮没有实现 endpoint 分离、wire IPC、UDP relay、control plane、observer-simulator、C0/C1 或矩阵 runner。
- 没有启动 host/VM 游戏，没有注入、输入、建连或修改任何 artifact。
- 第 16 节 10 个 UNKNOWN 仍然是实施前置研究项，不能从本设计直接推导为已验证事实。
- 设计 PASS 不等于 T0-09 PASS；T0-09 仍为 `NOT_STARTED` 的实现阶段。
- C2 已删除，未做 Original Online/Isaac Sync 操作，也不以“未触碰”登记为测试通过。

## 4. 审阅结论

**设计文稿静态审阅 PASS。** T0-09 设计可作为后续实现入口；实施前需要用户复核本设计和 10 个开放问题。没有运行任何测试，也不存在 gate/hash 前置条件。
