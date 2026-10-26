# tx-04-08：纯值模型、身份、缓冲与 writer（第一项工程改动）

日期：2026-10-27，Asia/Singapore。状态：**代码已编写；新静态库完成一次纯编译（Release Win32，`/W4` 无警告）；未执行测试、未启动游戏、未构建 DLL**。范围见[迁移设计第 10 节](tx-04-08-implementation-plan.md#10-工程顺序与验收边界)所列实现顺序的第一项。

本轮只做“纯值模型/身份/缓冲和 writer”的代码编写，不包含主线程桥、post-state、Hook 清单、命令、harness 或统一 E2E；这些仍是后续推进项。下文 `CURRENT_SOURCE` 指本次新增源码，`DESIGN` 指沿用迁移设计的目标行为，`UNKNOWN` 指尚未取得运行证据的部分。

## 1. 交付文件

| 文件 | 内容 |
| --- | --- |
| `mod/src/world/model.h/.cpp` | 纯值类型、字段/原因/分类枚举、预算常量与账本、`min-read-v1` 的 78 项字段声明、manifest 输入值 |
| `mod/src/world/identity.h/.cpp` | `(domain,address)` 二叉 radix 关联、local_id/use/birth/revision/readiness 状态、批次成员关系、引用解析、A/B 锚点与当前 Player 读取依据 |
| `mod/src/world/storage.h/.cpp` | 三个槽的所有权状态机、通知、有界值队列、生命周期事件环、gap 控制环、预算记账、DLL 自有 raw reader 线程 |
| `mod/src/world/writer.h/.cpp` | JSONL 编码、manifest/result/诊断文件、编码写盘线程、flush/sync 与终态判定 |
| `mod/CMakeLists.txt` | 新增独立静态库 `isaac_cs_world`（C++20、不依赖 SDK/游戏头），并加入 DLL 链接 |

`isaac_cs_world` 与 `isaac_cs_core`、`isaac_cs_net` 平级：不含 `windows.h` 以外的游戏依赖、不引用 `runtime.h`、不调用任何原生函数。SDK-facing 接线仍留给后续 `game/world_bridge`。

## 2. 值布局与预算（CURRENT_SOURCE）

| 类型 | 字节 | 说明 |
| --- | --- | --- |
| `RawField` | 48 | 字段 ID/状态/编码/宽度/引用状态/参考目标/16 字节原始值 |
| `RawObject` | 2592 | 身份 + 分类 + 52 项字段 |
| `RawMembership` | 16 | `(object, source, index)` 加保留 |
| `RawSource` | 24 | 状态、计数、4 word 容器描述符（仅诊断） |
| `BatchHeader` | 472 | 序号、epoch、Authority 类型/tick、计数、15 项来源、`event_watermark` |
| `RawBatch` | 43,516,376 | 头 + 16,384 对象 + 65,536 成员关系 |
| `LifecycleEvent` | 104 | 事件序号、run/room/outer、对象/use/出生/revision、call/parent、观察入口、进入/异常、成员位集 |
| `GapRecord` | 80 | attempt/epoch/范围/原因/Authority 类型 |
| `Identity` | 136 | 地址、local_id/use/birth/revision、origin/lifecycle/readiness/ordinal、出生描述、成员位集、批次关联 |

`PlannedWorldBytes()` 在 `model.h` 中用编译期常量登记默认上限的总计划量：三个批次槽 130,549,128 + 值队列 67,108,864 + 事件环 6,815,744 + gap 环 327,680 + 身份表 4,456,448 + radix 13,369,344 + 两个 1 MiB I/O 缓冲 + 8 MiB 管理余量 = **233,112,968 字节**，小于 256 MiB 总预算，并由 `static_assert` 固定。`BudgetLedger` 在准备阶段原子预占；超限使准备失败并记录首因，不在游戏线程继续分配。

`RawObject`/`RawField` 故意不写默认成员初始化器：`RawBatch` 的 42.5 MiB 对象数组在准备阶段一次性分配时不能被零初始化触碰（32 位进程的页提交）。写入前必须调用 `Reset()`，这是采集器的前置条件，已写在类型注释里。

## 3. 线程与所有权（CURRENT_SOURCE / DESIGN）

```text
主线程            Begin → 只写自有 WRITING 槽 → Prepare(PREPARED) → Release(committed)
                    ↓ SetEvent（自动复位，零等待，不取队列锁）
raw reader        取得 attempt 最小的 READY 槽 → 预占队列预算 → 复制有效前缀
                    → 立即归还槽(FREE) → Push owned values
writer            只消费队列 → 编码 → 写入并周期 flush → 收尾 sync + result.json
```

- 槽状态严格按 `FREE → WRITING → PREPARED → READY → READING → FREE`：`PREPARED` 在 core 提交前对后台不可见，未提交的批次回到 `FREE` 并记录 `UNPAIRED_SCOPE` gap，不留下带新 tick 的“完整”样本。
- 主线程路径只做原子操作与 `SetEvent`；状态查询（`StorageStatus`/`WriterStatus`/`QueueStats`）读取的计数全部是原子值，不获取队列/文件锁，保持“主线程停止等待 0”。
- 队列同时受 64 MiB 与 64 项两个上限约束，计费覆盖 reader 预留、排队、writer 在途与事件；预算不足时只有 reader 有界等待，游戏生产者从不等待，槽位耗尽记为 `BUFFER_BACKPRESSURE` gap。
- 停止：主线程只发信号；reader 在期限内排空已发布内容后关闭队列生产端；writer 见生产端结束且队列空即收口。两个线程都用 `CreateThread` 创建，句柄在显式 `Close` 或析构时关闭，**不 join、不等待、不释放在途状态**；超时保留 incomplete，线程持有 `shared_ptr` 直到自身结束，符合“不强制终止线程或卸载 DLL”。
- `FaultSwitches` 只由 E2E 配置与匹配 token 设置（开关本身在本层，门禁在后续命令层），默认全关；`pause_reader` 只暂停本 DLL 的 raw reader，`drop_capture_file` 关闭 writer 自有 captures 句柄。

## 4. 身份规则（CURRENT_SOURCE）

`IdentityTable` 只在主线程使用，分配发生在准备阶段：

1. `(domain,address)` 以 33 位字面位串做二叉 radix 关联（每身份最坏 34 节点，容量 32,768×34），不计算摘要；地址只是当前录制的临时关联，`local_id` 才是本地身份。
2. `Acquire`：同址再次取得增加 `use`；物理构造/复用或已销毁时换新 `local_id`；`origin=OBSERVED_CREATION`、`lifecycle=ACQUIRED`、`readiness=0`、`candidate=true`。不伪造出生。
3. `ConfirmBirth`：仅当已取得、尚无出生且 `readiness>0` 时冻结 `birth_event` 与出生描述；`Spawn/PlayerCreate/ManagerRestore/GridSpawn/Door` 才能把 `readiness` 抬到 2（创建 owner 完成）。
4. `OnUnknownInit`：在创建/Morph 上下文之外的 Init 撤销不成立的出生连续性（`born=false`、`origin=FIRST_OBSERVED`、`revision++`），由调用方发布 `Continuity` 事件；上下文内只更新 Init 依据。
5. `MarkInitReturned` 把 `readiness` 覆盖为 1：Morph 内层 Init 之后，派生字段要重新等到外层 owner 完成，这是保守选择，与 ST3/ST6 的“不能提前获得派生字段读取资格”一致。
6. `AnchorPlayer` 只接受 ordinal 0/1；解绑快照会清 `ordinal` 并撤销 `readiness=3`。`MarkCurrentWitness` 仅在已唯一锚定、且 `readiness<2` 时置 3；`RevokeCurrentWitness` 为 ST7 的独立撤销提供入口，不依赖 binding 层是否触发共同失效。
7. `ResolveReference` 只处理 Entity 域：0 → `NULL`；未知地址 → `UNRESOLVED`；已退休 → `RETIRED`；同址复用 → `AMBIGUOUS_REUSE`；只有“请求者已有出生且 revision 为 0、目标已出生且出生更早、目标在本批内”才给 `KNOWN_CURRENT` 与 local target。同批出现不能消除复用风险。
8. 成员关系：每批 `BeginBatch(attempt)` 后按 `source` 累积位集；提交前用 `MembershipMask/MembershipChanged` 判断是否需要发布快照事件，再 `CommitMembership`。退出来源只更新 membership，不等于物理销毁。

## 5. 文件格式（CURRENT_SOURCE）

新 domain 为 `isaac.cs.world-record`、`version=1`、`profile=min-read-v1`，与旧 `isaac.cs.t0.world-record` 明确区分；输出目录由固定根目录 + 受限 ASCII label 组成，最终目录与所有文件都用 create-new，不覆盖已有产物。

- `manifest.json`：domain/version/profile、recording_generation、session、phase、producer（component/build/game/repentogon）、hook 清单引用、启用的观察入口、9 个类别、15 个来源、78 项字段声明、按类别的 `required`、`layout`（声明状态 + 字段偏移/来源/分类证据，由 `binding/world_layout` 提供）、预算（设计上限 + 本次实际生效值 + 计划/实际分配）、事件类别/readiness/lifecycle/origin/relation/field_status/encoding 图例、浮点比较与 HP 假设、未确认字段、假设/未知、被排除的原生诊断、`runtime_acceptance=NOT_RUN`。
- `captures.jsonl`：`record=capture` 或 `gap`；capture 携带 epochs、attempt/capture、outer/scope、`event_watermark`、Authority 有无/tick/kind、`complete/reason`、15 项来源、对象（含字段、成员关系、本地身份与只诊断地址）、required 计数；gap 携带 attempt/原因/候选 tick，`capture_seq`、`authority_tick` 为 null，对象为空。
- `lifecycle-events.jsonl`：`sequence`、epochs、local_id/use/birth/revision、`call/parent`、`observation`、`entering/exceptional`、`event_kind`、`reason`、成员位集、诊断地址/返回值。
- `diagnostics.jsonl`：`prepare`、`backpressure`（槽位耗尽/预算等待/队列峰值/未交付计数）、`fault`（首因 + 代码 + 阶段，E2E 注入标 `TEST_INJECTED`）、`close`。
- `result.json`：`CAPTURE_CLOSED` / `CAPTURE_ERROR` / `CAPTURE_UNCONFIRMED`、readiness（producer/reader/writer/drain/flush/outcome）、生产/复制/写入计数、逐原因统计、未交付范围、故障、预算与队列峰值、`runtime_acceptance=NOT_RUN`。

编码约定：u64 一律规范十进制字符串；raw32/raw64 固定宽小写十六进制；其他宽度按 `bytes_le`；浮点保留原始位；地址/vptr/线程/描述符只进入 `diagnostics`。字段必须与 profile 的 ID/宽度/编码一致，否则整条记录判为 `INVALID_RECORD` 并停止写入，不把非法数据写进新 domain。

## 6. 失败方式与归属

| 情形 | 处理 |
| --- | --- |
| 槽位耗尽 | `attempts+1`，记录 `BUFFER_BACKPRESSURE` gap，主线程不等待 |
| 队列预算不足 | reader 有界等待并计数；到期重试，不绕过预算复制 |
| 事件环/gap 环满 | `EVENT_OVERFLOW`，录制停用，已发布内容仍排空 |
| 对象/成员分配失败 | `BUDGET_EXCEEDED`（Reader/Queue），录制停用 |
| 文件打开/写入/flush 失败 | `FILE_ERROR`，已写前缀保留，不再写新记录，`result.json` 报 `CAPTURE_ERROR` |
| 编码与 profile 不符 | `INVALID_RECORD`，停用而不是静默丢弃 |
| 线程创建/未捕获异常 | `THREAD_FAILED` |
| 通知失败 | `NOTIFICATION_FAILED`（进程内条件变量本身无 SetEvent 失败路径，主线程→reader 的 Win32 事件仍会真实失败） |
| 停止后未在期限排空 | `drain=INCOMPLETE`，终态 `CAPTURE_UNCONFIRMED`，不冒充 Closed |

原因枚举 0–18 沿用原型数值，19–23 为本轮追加（`FILE_ERROR`、`NOTIFICATION_FAILED`、`PREPARE_FAILED`、`THREAD_FAILED`、`INVALID_RECORD`），只追加不改写。

## 7. 构建与验证状态

- 新增静态库 `isaac_cs_world`：MSVC 19.44 / Win32 / C++20 / Release，`/W4` 无警告，链接生成 `isaac_cs_world.lib`（本次在临时构建目录完成的纯编译，不属于测试）。
- **DLL 完整链接未完成，且不是本次改动导致**：当前本地 `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/ASMPatcher.hpp:302` 的 `friend static` 在现有 C++20 编译选项下被当前 MSVC 拒绝（`error C2216`），受影响的是既有 `mod/src/hooks/odd_patch.cpp`、`host_patch.cpp`。仓库内历史 `build/repentogon` 产物早于该工具链状态。该问题留给“必要纯编译链接”推进项处理（例如把这些 SDK-facing 编译单元按既有约定改为 C++17），本次未修改既有编译选项，也未改动 SDK。
- 按现行规范，本次没有运行测试、没有启动游戏；2,000 次稳定段与双进程比较仍为 `NOT_RUN`。

## 8. 给后续推进项的接口约定

1. 准备顺序：`WorldStorage::Prepare` 成功后才能 `WorldWriter::Open`；两者都在 Preparing 后台线程执行，主线程只在 Manager 边界接收 Ready。
2. 一个 `BudgetLedger` 必须由采集器创建并通过 `StorageConfig::ledger` 交给 storage；`WorldWriter::Open` 使用 `storage.Ledger()`，`IdentityTable::Initialize` 使用同一账本，三项合计才是 256 MiB 上限。
3. 主线程每轮收束顺序：`BeginBatch(attempt)`（身份表）→ 填 `BatchHeader`/对象/成员 → `Prepare()` → core `FinishPostState` → `PublishCompleted` 后 `Release(committed)`；`header.event_watermark` 取 `EventWatermark()`，`authority/commit_kind/tick` 取本轮真实提交值。
4. 计数归属：`attempt_seq/capture_seq/complete_capture_count` 由采集器写入 `Diagnostics`（`attempts/captures/complete_captures/last_capture`）；storage 只记录它自己产生的槽位背压与 gap。
5. 停止顺序：主线程 `RequestStop`（`Close(0)`）→ reader 排空并 `CloseProducer` → 控制口/收尾线程 `WorldWriter::Close(close_timeout_ms)`。storage 与 writer 的引用必须比 writer 线程活得久；`FaultSwitches` 同样必须比线程活得久。
6. 故障注入开关只允许 E2E 配置与匹配 token 打开；本轮实现开关本体，不实现门禁与命令入口。

## 9. 保留的 UNKNOWN

- 32 位进程内 233 MB 计划占用的实际可用性、峰值与碎片（未运行）。
- 真实 2,000 次采集下的队列/写入吞吐、背压频率与 `copy_max_ns`（未运行）。
- E2E 注入的 `drop_capture_file` 与真实磁盘错误的组合路径（未运行；仅实现开关与真实失败路径）。
- 生命周期事件的 `call/parent` 编号与事件 watermark 的最终接线（属于后续桥接项）。
- 布局声明（字段偏移/证据、RVA）尚未填入，manifest 会明确记录 `UNDECLARED`（等 `binding/world_layout` 提供）。
