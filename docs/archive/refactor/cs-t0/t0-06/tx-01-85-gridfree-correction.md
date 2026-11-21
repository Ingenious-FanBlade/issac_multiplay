# tx-01-85：岩石基线和专项证据汇总修正

2026-10-19。目标：修正 tx-01-84 已暴露的 fixture 和汇总实现错误，解决 retained Player／Replace 与 GridFree 两个场景。保留 tx-01-84 FAIL 与 tx-01-83 总账。Native、70 槽、controller、协议、输入 scheduler 均不再修改；复用 tx-01-84 Native。

## 修改前登记的失败方式

- SpawnGridEntity 传入 variant=0，原生 Init 可以选择实际外观 variant；初次观测即为 2。把传入值当实际固定值会阻止 D12。必须保存创建后的实际 type/variant/seed/index，使用前逐项比较；不忽略变化。
- 原生普通 Rock 的 ToRock 必须非空，房间中不得有其它 D12 可重掷 Grid；新诊断要在拒绝前记录实际描述和每个检查结果。失败后不能重试或清理原有 Grid 来满足条件。
- 探针 subject=0/1 是入口/原函数的两个副本，不能算两次调用。只按 subject=0 取参，并同时要求 entry/original/return 各 3 次及三次成功返回。
- ActiveItem canonical 位为 2，Bomb 为 1。只接受实际 Submit/Resolve 的 ActiveItem 位，保存 command/target/tick，不能从 Lua 回调推断输入。
- PlayerRestore 可撤销当前出生映射可用性。历史 birth_event 必须从已有出生和生命周期事件保留；样本 null 不补值、不外推跨 Restore 状态连续。源/目标物理 ID/use 仍须随实际 roster/retained 转换保留，不能把重新发布算新生。
- 先前 GridFree fixture_error 不应使新的独立 Replace 证据审阅凭空失败；只允许明确的旧 gridfree-use 拒绝作为历史隔离错误，任何其它 fixture/trace 错误仍失败。本轮新增 GridFree 则不得有任何 fixture_error。
- 唯一新验收入口同时汇总旧 Replace 原始采集和一段新 GridFree 采集，输出新结果；不改旧 result，不重复旧实机动作，不在修改过程中运行其它测试或校验。

## 计划实现

fixture v1.2 保存原生创建后的实际 Grid variant，使用前记录完整检查结果，仍按公开 D12 效果调用触发原生 GridFree。单一入口 `cs-t0/accept-gridfree-scene.py` 新进程只执行 enable / gridfree-spawn / gridfree-use；正常恢复后一次汇总两个场景。旧 Replace 来源固定为 `artifacts/cs-t0/tx-01-84-trigger-e2e-01`，新结果单独输出，不替换历史。

具体输入汇总按现行代码：Decision::Accepted=1；Submit 用 target，Resolve 的 target 未使用，实际用 tick。按相同 generation 的 Submit.target=Resolve.tick 对应三次 ActiveItem 边沿，原 command 仍保存；同 tick 合并的连续输入可使 CurrentSequence 更新，不强求 Resolve.command 等于最初边沿 command。

准备阶段实现完成并静态阅读；准备过程中没有运行 Lua/Python、游戏探查或试验，本轮无需构建。随后唯一入口完成一次 E2E，**专项 2/2 PASS**：旧 Replace 只读汇总通过，新 GridFree 实际触发及 70 槽恢复通过。见[实际结果、原始证据和复现步骤](tx-01-85-e2e-results.md)。旧结果均保留。
