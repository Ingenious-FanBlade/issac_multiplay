# tx-03-06：Authority 协议发布终点修复

日期：2026-10-21。状态：**修复完成，唯一生产链路集成验收 73/73 PASS；双机 F0 待实机复验。** 下方实施前失败方式保留登记内容。

## 目标与范围

修复 tx-03-05 的实际反例：计划发布 10,000 tick，终点 14795，却持续发布至 14819，造成五个终点条件失败。`AuthorityWireLink::Publish` 在终点仅设置完成标志，等待控制面 STOP 期间没有关闭发布窗口。修复只约束 C/S ACK、STATE、心跳及该窗口的远端输入接纳；游戏 Authority 模拟继续，正常 STOP/恢复仍由原有 owner 执行。不修改旧失败记录，不放宽双机分析器，不实现 VM observer-simulator 或 Replica World。

用户本轮无法准备双机。因此本轮唯一验收采用生产链路集成入口，直接链接实际 `t09_authority_link.cpp`、Wire IPC、protocol Endpoint，通过真实 Windows mapping 完成协商、输入、发布和退休，保存逐包字节、解码记录、检查和源码/构建快照。fixture 以单进程中的控制端视图提供有界 ring 传递；不是双游戏、UDP、真实游戏 tick 或实际 Hook 恢复证据。后续双机 F0 仍待实机复验。旧离线 endpoint scenario 自己实现终点，不能证明本次生产链路修复。

## 实施前失败方式与预期行为

以下在实现及新增集成场景代码之前登记；不在修改过程中执行测试。

1. 完成标志已设置但后续 tick 仍发送：终点 ACK 与 STATE 成功后锁定完成，后续发布成功返回但不再写包、增加协议 sequence、改变最终心跳或暂停游戏模拟。
2. 第一条发布偏移一格、仅一 tick 窗口或非零起点计算错误：以首条发布 tick 减一作为 start，恰好发布 `start+1..start+run_ticks`；检查单 tick、10,000 tick、非零起点和长 STOP 延迟。
3. 重复、倒退、跳号及 ACK/STATE 的 tick 不同：窗口尚未完成时拒绝不连续或不一致的发布，不填补、不裁剪、不伪造成功；完成后重复调用只做成功 no-op。
4. start/end 计算溢出、无效 tick：失败关闭，不能设置虚假 target-reached。覆盖首 tick 为零、UINT64_MAX、end 溢出。
5. ACK 入队失败或 ACK 成功而 STATE 失败：不能提前锁定完成、更新终点心跳或宣告成功；错误后不重发、不恢复发布。真实有界 ring 的满队列分别覆盖这两处失败。
6. 完成后收到迟到输入：继续有界解码/排空输入 ring，但不再调用 admission，避免最终 ACK 固定后输入计数继续变化。完成前仍忽略 target 超出 end 的记录，保持相同 callback/generation 语义。迟到输入不会补写最终 ACK。
7. RUN_START 前发布、协商未完成、提前 STOP：不提前开始发布；提前 STOP 不补足样本、不宣称 target reached；正常退休不改变最后成功心跳。
8. STOP 延迟或恢复被发布逻辑抢先触发：完成窗口时保持 mapping 打开、run running，后续游戏 tick 可继续调用；只在正常 Retire 设置 stopped/closed/restore-complete。
9. 未初始化、已有链路错误、mapping 被外部关闭：不能因完成 no-op 掩盖错误或复活退休链路。
10. 仅测试 fixture 而未链接生产代码，或仅比较内部布尔值：集成程序直接调用生产类；原始 packet 经实际协议解码，检查连续 tick、packet sequence、最终 ACK/state、ring 计数、停止前后 header，并留下可重复产物。

## 单次整体验收安排

先编写上述集成场景与统一入口，再修改生产代码。全部完成并静态审阅后仅执行 `accept-t09-publication-boundary.py` 一次；入口内纯编译生产 Native/Helper/Rust 与新增集成程序，执行集成程序一次，写统一结果。无单元测试、无预跑、无实机操作、无哈希检查；失败保留产物，不自动重跑。

## 实现与静态审阅

- 新增 `publication_complete_`：只在终点 ACK 和 STATE 均成功写入并提交后锁定。`Publish` 仍先检查初始化、健康、退休和外部 closed，完成窗口之后成功 no-op，不更新任何 packet/心跳。此返回值保持 `AuthorityTickPipeline::PublishCompleted` 正常推进，不提前 Retire。
- 完成前要求逐 tick 连续且 ACK/STATE 同 tick；首条发布在改写 start/end 前检查端点范围，拒绝无效 tick、溢出和 UINT64_MAX 终点。失败 helper 同时设置对象健康状态与现有 Wire 错误，错误不能转成完成后成功。
- 完成后 `DrainInbound` 仍执行现有 bounded receive/协议合法性检查，跳过 admission；完成前 end 过滤、callback 和 generation 传递保持原语义。
- 实施前失败方式、集成场景及验收入口先写，随后修改生产代码；没有添加单元测试。新 scenario 与生产 Native 编译同一份 AuthorityWireLink，旧离线 endpoint scenario 只编译不执行。C++ Remote 仅有 Authority 发出 first ACK，fixture 按现行协议协商。
- 满队列由真实发布占满：128 条未消费使最终 ACK 失败；先仅消费首个进度 ACK、留下 STATE，再累积至 127 条使最终 STATE 失败。不篡改 ring 计数或插入伪包。
- `report_t09_b_live.py`、游戏 loop、Hook catalog/恢复、Rust 控制面及 Remote 游戏功能均未修改。原始双机失败 33/38 仍保留，不能以本轮单进程集成替代实机结果。

执行入口（一次，新目录）：

```powershell
python cs-t0/accept-t09-publication-boundary.py --artifact-dir artifacts/cs-t0/tx-03-06-publication-01
```

## 结果

统一入口只执行一次：2026-10-21 16:53:23–16:55:29（Asia/Shanghai），退出码 0；纯编译与场景退出码均为 0。Native、Helper、Rust 控制器、旧离线驱动与新增生产链路场景编译通过；只运行新增集成场景一次。C++ 场景 67/67，加逐包字节独立检查及整体完成检查 6/6，合计 **73/73 PASS**，无自动重跑。

| 实际观察 | 结果 |
| --- | --- |
| 原反例窗口 start=4795、end=14795 | 进度 ACK 与 STATE 各恰好 10,000 条，tick 4796..14795；含 HELLO/first ACK 的 outbound produced=consumed=20002 |
| 终点后继续调用至15307，延迟512 tick 才退休 | 心跳仍14795，outbound仍20002，admission仍1；等待 STOP 时 mapping 开放、run running；退休后才 closed/stopped/restore-complete |
| 单 tick 与非零短窗口 | 0→1 与900→907正确；额外24/1024调用均不扩大发布窗口 |
| 完成后迟到与冗余 INPUT | 协议解码并排空，无新增 admission；完成前正确传递 generation=7，过滤 target>end |
| 重复/倒退/跳号、ACK/STATE错tick、无效/溢出终点 | 均失败关闭，错误0xE301，未写额外包或误设完成 |
| 最终 ACK 满队列失败 | 最后心跳64、目标65，Remote仅收到64组；完成标志未设，无重试 |
| 最终 STATE 满队列失败 | Remote收到ACK65但STATE仅到64；最后心跳仍64，完成标志未设，无重试 |
| 提前 STOP、重复 Retire、未初始化或外部 closed | 未补写、未假报目标完成，已关闭状态不能被完成 no-op 掩盖 |

产物入口：

- [统一结果](../../../../../artifacts/cs-t0/tx-03-06-publication-01/acceptance-result.json)、[全部73项检查](../../../../../artifacts/cs-t0/tx-03-06-publication-01/checks.json)。
- [原始包字节与解码记录](../../../../../artifacts/cs-t0/tx-03-06-publication-01/evidence/packets.jsonl)、[共享内存状态](../../../../../artifacts/cs-t0/tx-03-06-publication-01/evidence/events.jsonl)、[场景结果](../../../../../artifacts/cs-t0/tx-03-06-publication-01/evidence/scenario-result.json)。
- `artifacts/cs-t0/tx-03-06-publication-01/source/` 是验收时源码与本文件的实施前/待验收快照；`build/` 保存编译日志、产物与对象文件，`commands.json` 和 `scenario-command.json` 保存执行命令。本节是执行结束后补充，不覆盖当时快照。

边界：fixture 单进程、手动驱动模拟 tick、admission 为记录 callback；不证明真实 scheduler/world、UDP/控制面时序、51槽实机恢复或双游戏世界相等。没有操作或注入游戏，也未运行原版 Online/Isaac Sync。用户本轮不能准备双机，因此双机 F0 未复验；tx-03-05 的 **33/38 FAIL** 原样保留，T0-09 仍 IN_PROGRESS。
