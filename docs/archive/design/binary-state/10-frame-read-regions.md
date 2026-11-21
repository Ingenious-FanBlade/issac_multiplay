# 主线程整帧可读区表

**2026-11-17 H1 已实施（E10／E11，包123）**：[设计文档](eventual-consistency-revision.md#36-host-采集配套)、[规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)。D1—D5 已接入。作废后重查成功的复制正常返回，H2 不记单根失败。实施见[第 10 步记录](10-host-capture.md#2026-11-17-h1-和-h2-实施记录)。

初稿日期 2026-11-17（Asia/Singapore）。**状态：H1 源码已接入，运行验证归第 15 步。** 2026-11-17 用户同意第 6 节 D1—D5 全部按推荐执行，已写入[规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)。文中标明“推算”的数字没有实测。

上级：[现行规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)、[总计划](README.md)、[Host 性能优化方案](10-native-capture-performance.md)。本文改动该方案 5.1、14.3、14.14 节的页信息寿命规则，见第 4 节。运行验证归[第15步](15-e2e-acceptance.md)。

## 1. 问题与依据

Host 主线程上，最大的单项开销是页可读性查询，即 `VirtualQuery`（内核调用 `NtQueryVirtualMemory`）。

依据一：主线程采样 `.scratch/step15-profile-run123-host.json`（四身体混合负载）。

| 叶函数（首个本项目帧） | 样本 |
| --- | ---: |
| `cs::world::QueryNativeReadRegion` | 365 |
| `cs::game::ReadMemory`（函数内直接调用 VirtualQuery） | 150 |
| 主线程非游戏叶样本合计 | 891 |

两项合计约占 58%。按调用方归类，`QueryNativeReadRegion` 的 365 个样本中：

| 调用链 | 样本 |
| --- | ---: |
| `ConsumerReadWindow::Copy` ← `StableConsumerSnapshot` ← `RefreshLifecycle`（玩家 hook） | 126 |
| `TopologyReader::Copy`（拓扑批次公共读取与设备读取） | 37＋21＋5 |
| `CaptureReadSession::Find`（二进制采集） | 27＋10 |

run120 的采样分布相同。

依据二：计数 `.scratch/step15-perf-run128.json`（180.7 秒）。

| 计数 | 总数 | 每秒 |
| --- | ---: | ---: |
| `stream.source.read_queries`（采集段内） | 723,062 | 约 4,000 |
| `stream.main.topology.*.region_queries`（两类合计） | 2,017,176 | 约 11,160 |
| 同期 Manager 帧 | — | 约 60 |

已计数的查询约每帧 250 次。`ReadMemory`、`ConsumerReadWindow`、根读取、RTTI 分类这几处没有计数，不在其中。

每秒查询数与负载有关。实体出生要经过 `HW_world_spawn` → `ObserveLeave` → `Birth` → `Collector::Read` → `ReadMemory`，所以泪弹落地等大量生灭的场景，查询数随出生数增长。

## 2. 现状：查询点和缓存寿命

VirtualQuery 返回的是一整段属性相同的连续页 `[BaseAddress, BaseAddress+RegionSize)`。堆段和对象池通常是几百 KB 到数 MB。一帧内实际访问到的不同区间估计只有几十个（推算，没有计数），但现在每个读取点的缓存都很快丢弃：

| 读取点 | 位置（相对 `mod/src/`） | 区表 | 寿命 | SEH |
| --- | --- | --- | --- | --- |
| `ReadMemory` | `game/participants/topology.cpp:18` | 无 | 每次调用都查询 | **无**，直接 `memcpy` |
| `ConsumerReadWindow` | `game/authority/lifecycle.cpp:279` | 8 槽环形 | 一次 `StableConsumerSnapshot` 调用 | 有 |
| `TopologyReader` | `game/participants/topology.cpp:165` | 32 槽 | 一个批次，`Begin()` 清零，约 30 批／帧 | 有 |
| 根读取 `roots` | `game/observation/world_bridge.cpp:368` | 8 槽 | 一次调用 | 有 |
| RTTI 分类 `classify` | `game/observation/world_bridge.cpp` | `thread_local CaptureReadSession` | 每次 `Begin()` 清空 | 有 |
| 二进制采集 `CaptureReadSession` | `world/binary/read_session.cpp` | 1024 槽有序表 | 一个 Prepare 页作用域（包104），下一帧重来 | 有 |

`ReadMemory` 的调用方遍布 Host 和 Client：Collector 读取（`world_bridge.cpp` 的 `result.copy`），输入覆盖查询经 `ReadAt`，`join.cpp`、`device_k.cpp`、`display_observation.cpp`、`exit_patch.cpp`、`client_native_bridge.cpp`、`client_atomic_bindings_native.cpp`。其中 `display_observation.cpp:129,138,144` 逐字节读字符串，每个字节都触发一次查询。

不在本文范围：
- `client_atomic_install_native.cpp:45` 与 `ClientNativeReadWindow`（`client_read_window.h`）：Client 写段的可写性检查和只读窗口，语义不同，另议。
- `binding/definitions.cpp`、`hooks/random_runtime.cpp`、`random_sources.cpp`、`random_runtime_lua.cpp`：启动期或需要 `Type`／`AllocationBase` 属性的检查。
- `ReadProcessMemory`（`copy_image`、`random_owners.cpp`）：前者是冷准备；后者是否在热路径上未核实，记为未知。

## 3. 设计

### 3.1 组件

在 `world/common/` 新增 `FrameReadRegions`（建议名），整个进程只有一份，只由主线程使用。

- **数据**：
  - 1024 条按 `first` 升序的 `NativeReadRegion` 有序表，外加一个“最近命中”项。
  - 当前代次号和计数器。
  - 冷准备时一次性分配，不随帧分配。
- **只缓存“可读”的结论**：
  - 保护属性沿用 `QueryNativeReadRegion` 的明确可读集合：READONLY、READWRITE、WRITECOPY、EXECUTE_READ、EXECUTE_READWRITE、EXECUTE_WRITECOPY；排除 GUARD、NOACCESS 和未提交。
  - 查询失败的结果不入表。
- **不表示对象活着**：表里的结论只说明页可读，不说明对象存活、身份一致或数据完整。这些仍由各来源的身份复核、证言复核和读取序号负责，本文不改动。

建议接口，函数名可在实施时调整：

```cpp
namespace cs::world {
// 主线程整帧可读区：查表；未命中时查询一次并入表；非主线程直接查询，不入表。
bool FindReadableRegion(std::uint64_t address, NativeReadRegion &out) noexcept;
// 受检复制：先确认整段可读，再做 SEH 复制；命中旧条目而复制失败时，整表作废并重试一次。
bool CheckedNativeCopy(std::uint64_t address, void *out, std::size_t size) noexcept;
// 作废原因计入计数；不释放内存。
void InvalidateFrameReadRegions(FrameRegionReset cause) noexcept;
FrameRegionCounters FrameReadRegionCounters() noexcept;
}
```

`StringBlock` 需要区间末端来限制 64 字节块，所以继续用 `FindReadableRegion` 取得 `region.end`。

### 3.2 查表与复制流程

`CheckedNativeCopy(address,size)` 的流程：

1. 用 u64 检查地址加法和 4GB 上限，与现有各读取器相同。
2. 从 `address` 开始逐区间覆盖 `[address,address+size)`：
   - 先比较“最近命中”项；
   - 再在有序表上二分查找；
   - 都未命中时调用 `QueryNativeReadRegion`，成功则入表，失败则返回 false，不触碰内存。
3. 记下本次覆盖是否用到了本次调用之前已在表中的条目，称为“旧条目”。
4. 调用带异常码的窄 SEH 复制叶（见 3.4），成功即返回。
5. 复制失败：
   - 本次全部区间都是刚查询的：返回 false，与现状相同。
   - 用到了旧条目：作废整表并计数，回到第 2 步重做一次。重做时全部重新查询，结果按上一条处理。最多重试一次。

入表时如果新区间与相邻条目重叠，说明内存布局在本代次内变了。此时作废整表，再插入新条目，并计数。

表满时不入表，本次直接使用查询结果，记一次回退，等同现状的回退语义。

### 3.3 作废时机

| 时机 | 位置 | 说明 |
| --- | --- | --- |
| Manager 帧开始 | `ops/command.cpp:488 FrameBegin()` 的入口处，在 `native_frame` 分支之前 | Host 和 Client 都经过此点（Client 忙帧走 `BeginContextFrame`，也先到这里）。每帧一个代次 |
| 任意一次 SEH 复制失败 | `CheckedNativeCopy` 及各读取器的失败路径 | 整表作废，原因记为“访问失败”或“守护页” |
| 入表重叠 | 3.2 节 | 原因记为“布局变化” |
| 显式撤销 | `CaptureReadSession::Revoke` 等现有撤销点 | 保持现有撤销语义，原因记为“撤销” |

建议不随外部资源写水位（`foreign_resource_writes`）作废，理由：
- 该水位守护的是值和证言的有效性，`Good()`、`Seal` 和 `JointSourceCurrent` 继续检查它，本文不改；
- 页信息过期已由第 3.4 节兜底；
- run128 每次 ordinary 约 7 次 sprite 撤销，如果每次都清表，会把节省掉的查询又加回来。

Manager 帧之外的读取（Render、菜单等）沿用上一帧的表，寿命到下一次 `FrameBegin` 为止。如果长时间没有 Manager 帧，条目会活得更久；这种情况仍由第 3.4 节兜底，并另记最长代次时长。

### 3.4 失败与重试：为什么结果与现状等价

现状的保护有两层：先用 VirtualQuery 判断可读，再做 SEH 复制（`ReadMemory` 只有第一层）。改造后的保护：先查表，再做 SEH 复制；表中条目过期时，靠“作废并重查一次”还原现状的判断。

对任意一次读取，逐项比较：

| 实际页状态 | 现状 | 改造后 |
| --- | --- | --- |
| 可读，表中有 | 查询可读 → 复制成功 | 复制成功（省掉查询） |
| 可读，表中无 | 查询可读 → 复制成功 | 查询入表 → 复制成功 |
| 已不可读，表中有旧条目 | 查询失败 → 不复制，返回 false | 复制触发访问异常 → 作废 → 重查失败 → 返回 false |
| 不可读，表中无 | 查询失败 → 返回 false | 查询失败 → 返回 false |

返回值在各种情况下都与现状一致，差别只有三点：
1. 旧条目过期时多一次异常分派的开销。每次作废后最多发生一次，频率记入计数。
2. 守护页，见 3.5。
3. `ReadMemory` 补上 SEH。现状里，查询之后、`memcpy` 之前页被释放会直接崩溃；改造后会安全失败。

SEH 复制叶要新增一个返回异常码的版本：
- 过滤器沿用 `NativeReadExceptionFilter`，即 ACCESS_VIOLATION、IN_PAGE_ERROR、GUARD_PAGE（0x80000001）；
- 叶内不含 C++ 析构，以满足 MSVC 对 `__try` 的限制。

### 3.5 守护页

守护页被读到一次，`PAGE_GUARD` 标志就被系统清掉。现状靠查询拒绝守护页，因此从不碰到它。改造后，只有当一个曾经可读并已入表的区间，在同一代次内被改成 `PAGE_GUARD` 时，才会碰到。

- **线程栈不受影响**：线程栈的守护页位于栈的保留区，查询时就不可读，不会入表；已提交的栈页不会再变成守护页。
- **其余来源**：只剩显式 `VirtualProtect(…PAGE_GUARD)`。游戏和驱动是否会这样做，没有核实，记为未知；J273 的导入调用需要 IDA 复核，当前 ida-mcp 无法连接。
- **防护**：
  - 地址落在本线程栈范围内时不入表。栈范围在主线程冷准备时用 `GetCurrentThreadStackLimits` 取一次；游戏对象不在栈上，正常读取不受影响。
  - 复制叶报告异常码为 GUARD_PAGE 时：计入“守护页”计数，首次写一条 trace，作废整表，本次返回 false，不重试。
  - 第 15 步要求该计数为 0；出现非 0 就停用跨调用缓存，回到局部区表并定位来源。

### 3.6 各读取点的改法

| 读取点 | 改法 |
| --- | --- |
| `ReadMemory` | 改为 `IsMain()` 检查加 `CheckedNativeCopy`。可读集合统一为 3.1 节的集合：现状接受“非 NOACCESS 且非 GUARD”，会放过只执行页（PAGE_EXECUTE）；改造后拒绝这类页。调用方读代码字节时，页属性为 EXECUTE_READ／EXECUTE_READWRITE，仍在可读集合内 |
| `ConsumerReadWindow` | 删除 8 槽表，`Copy` 改用 `CheckedNativeCopy`；所有身份字段仍每次读取 |
| `TopologyReader` | 删除 32 槽表，`Region` 改用 `FindReadableRegion`；`stale` 标志和失败语义保留，SEH 失败另外作废整表 |
| 根读取 `roots` | 删除 8 槽表，逐项用 `CheckedNativeCopy` |
| `classify` | `thread_local CaptureReadSession` 不再持有区表，`Find` 走整帧表 |
| `CaptureReadSession` | 删除 `regions_`（1024 槽）、`recent_`，`Find` 改用 `FindReadableRegion`。`serial_`、`Good()`、外部资源写水位、`Revoke`、读取序号保持不变。页作用域（包104）不再承担区表保留，`read_page_spans`／`read_page_continuations` 标注为已无页含义，或在实施时删除 |
| `display_observation.cpp` 逐字节读字符串 | 本文只让它走整帧表；改成按块读取另议 |

各读取器现有的计数（`read_queries`、`region_queries`、`region_hits`、`region_fallbacks`）保留名字，含义改为“经该读取器、未命中整帧表的查询”。拿改造前后的读数对比时必须注明这一点。

### 3.7 计数与状态输出

主线程累计 `read_regions.*`，由 `stream main 0` 显式查询。

| 计数 | 含义 |
| --- | --- |
| `lookups`、`hits`、`queries`、`inserts`、`full_fallbacks` | 查表次数、命中次数、真实查询次数、入表次数、表满回退次数 |
| `reset.frame`、`reset.fault`、`reset.guard`、`reset.overlap`、`reset.revoke` | 按原因分开的作废次数 |
| `retries`、`retry_success` | 重试次数与重试后成功次数 |
| `faults_fresh` | 刚查询的区间复制仍失败的次数 |
| `high_water`、`max_epoch_ms` | 单代次最大条目数、最长代次时长 |
| `by_reader.<读取器>.lookups`、`by_reader.<读取器>.queries` | 按读取器分开，读取器包括 read_memory、consumer、topology、roots、classify、capture |

改造前，`ReadMemory`、`ConsumerReadWindow`、`roots`、`classify` 没有专用计数，并且每次调用至少查询一次。所以改造后同一读取器的 `lookups` 可以近似当作改造前的查询数，不必为取基线另外跑一轮。

### 3.8 内存

净减少：
- `CaptureReadSession` 的 1024 槽区表（约 16KB）移入整帧表；
- `classify` 的 `thread_local` 实例不再带区表；
- 拓扑、Consumer、根读取的栈上小表删除。

整帧表约 16KB 加计数，在冷准备时计入现有公共额度，不逐帧分配。

## 4. 与现行合同的关系

本设计改动三处已写明的规则，用户已于 2026-11-17 同意（第 6 节 D1），三处原文已加修订注记：

| 条款 | 原文要点 | 改为 |
| --- | --- | --- |
| [性能方案 5.1](10-native-capture-performance.md#51-合并范围与区域查询) | 区域缓存不跨调用原生代码的边界 | 页可读信息在一个 Manager 帧内跨原生调用共用，由 SEH 加作废重试一次保证结果与现状等价 |
| [性能方案 14.3](10-native-capture-performance.md#143-h同一边界内的拓扑批次读取) | 拓扑用局部有界区表，不全局放宽 ReadMemory | 拓扑与 ReadMemory 改用整帧表；ReadMemory 补 SEH 并统一可读集合 |
| [性能方案 14.14](10-native-capture-performance.md#1414-第15步续修prepare内页信息复用) | 页作用域只在 Prepare 内保留，不能延伸到 Commit、FrameEnd 或下一帧 | 页作用域不再负责区表；区表寿命为 Manager 帧，跨帧清空不变 |

以下内容不变：
- 各来源的身份、生命周期、证言、读取序号和外部资源写水位检查；
- “区域可读不代表对象仍存活”的原则；
- 14.3 节列出的拓扑批次边界：Player 更新前后分别读取，`PlayerIsHologram` 单列调用边界；
- `CaptureReadSession` 的 Revoke、读取异常和越界立即撤销。

撤销的是会话自身的有效性，此外还作废整帧表。

## 5. 失败方式

| 编号 | 失败方式 | 处理 |
| --- | --- | --- |
| F1 | 入表后页被反提交或释放 | 访问异常 → 作废 → 重查一次；返回值与现状一致（3.4） |
| F2 | 入表后页被设成守护页 | 计数、首次写 trace、作废、本次失败；第 15 步要求为 0（3.5） |
| F3 | 读取范围落在本线程栈内 | 不入表，每次查询 |
| F4 | 新区间与已有条目重叠 | 作废后插入，计数 |
| F5 | 表满 | 不入表，用本次查询结果，计数 |
| F6 | 非主线程调用 | `FindReadableRegion` 直接查询、不入表；`ReadMemory` 保持非主线程返回 false |
| F7 | 重试后仍失败 | 返回 false，不再重试；单次调用最多两次异常 |
| F8 | 长时间没有 Manager 帧（菜单、加载、hook 未就绪） | 条目寿命变长，仍由 F1 兜底；记录 `max_epoch_ms` |
| F9 | `ReadMemory` 可读集合收紧 | 只执行页现在被拒绝；实施前用 grep 核对调用方是否读这类页，第 15 步看各调用方的失败计数有无新增 |
| F10 | 计数含义变化 | 改造前后对比时注明（3.6） |
| F11 | 过期条目使调试器出现首次机会异常 | 只影响调试观察；计数可查，不改变 E2E 判定 |
| F12 | 异常集中出现（例如资源重载释放大块内存） | 每次作废后同一区间最多一次异常；`reset.fault` 激增时，按读取器分开的计数用于定位 |
| F13 | Client 侧行为随 `ReadMemory` 一起变化 | Client 同样在 `FrameBegin` 作废，第 15 步 Client 读取失败计数不应增加 |
| F14 | 页作用域相关的旧断言或计数依赖 `regions_` | 实施时用 grep 清点 `PageSpanActive`、`page_continuations` 的使用方，逐处改写 |

## 6. 用户决定（2026-11-17，全部按推荐）

- **D1（已同意）**：第 4 节三处条款按本文修订，即页可读信息在 Manager 帧内跨原生调用共用。
- **D2（采纳 3.5 节）**：本线程栈范围不入表；GUARD 计数非 0 即停用跨调用缓存，回到局部区表并定位来源。未采纳的备选是只缓存 `MEM_IMAGE` 和已登记身份的对象池区间，收益较小。
- **D3（已同意）**：`ReadMemory` 补 SEH，可读集合统一为 3.1 节的集合，只执行页改为拒绝。
- **D4（包含共用函数）**：范围包含 Client 经 `ReadMemory` 等共用函数的读取；Client 专用的写段检查（`client_atomic_install_native.cpp`）和 `ClientNativeReadWindow` 不在本次范围，另议。
- **D5（不作废）**：外部资源写水位变化时不作废整帧表；该水位继续守护值和证言（3.3）。

## 7. 实施顺序

按第10步实施记录顺延包号。每项只做纯编译；运行验证全部归第 15 步。

1. 新增 `FrameReadRegions` 及带异常码的 SEH 叶、计数和状态输出；在 `FrameBegin` 入口作废。
2. `ReadMemory` 改用整帧表，补 SEH，统一可读集合（D3）。
3. `ConsumerReadWindow`、`TopologyReader`、`roots`、`classify` 改用整帧表，删除局部区表。
4. `CaptureReadSession` 改用整帧表，删除自有区表，处理页作用域的计数和断言（F14）。
5. 文档：
   - 在性能方案 5.1、14.3、14.14 加修订注记；
   - 在第 10 步实施记录写入包号；
   - 更新 `docs/archive/README.md` 与总计划。

代码按项目约定为组件、核心函数和数据结构写中文注释。

当前低层查表由受检复制和字符串块共用。拓扑保留本批的查询、命中和回退计数，重查成功时也保留原拓扑读取结果。Guard 停用后按单次复制核页边界，计数继续累计。空闲重载暂时释放整帧表的 Values 配额，保留进程计数与 Guard 停用状态。

## 8. 第 15 步覆盖与判定

在同一整体验收内续测，不另开编号。场景沿用现有安排，重点覆盖：

- 四身体混合负载 180 秒窗口，与 run128 对照；
- 100 泪弹下落（有 FX 生灭）和 100 敌怪；
- 切房、换层、重开 run、costume／资源变化、暂停恢复、正常停止与退出；
- Client 同步运行，检查 F13。

判定与报告：

1. **机制计数**：
   - 每 Manager 帧真实查询数（`queries` 除以帧数）；
   - 命中率；
   - 各作废原因；
   - `retries`、`retry_success`；
   - `reset.guard` 必须为 0。
2. **结果一致性**：
   - `stream.source.gaps` 与采集失败原因分布、拓扑 `read_failures`、Client 读取失败，与改造前同类场景相比不应新增；
   - 若新增，按 `by_reader` 定位。
3. **耗时**：
   - 主线程采样中 `QueryNativeReadRegion` 与 `ReadMemory` 的样本占比；
   - ordinary outer、`source_native`、`topology_*` 的均值和分位数；
   - Host tick 频率。
4. 机制计数下降不代替性能指标。ordinary 的 p95≤2ms／p99≤4ms 等目标照旧判定；本项预计不足以单独达标（第 9 节）。

## 9. 预期与不确定项

- **查询次数（推算）**：每帧真实查询从“已计数约 250 次加上未计数部分”降到“本帧实际访问的不同区间数”，估计为几十次。实际区间数在第 15 步由 `inserts` 与 `high_water` 给出。
- **耗时**：单次查询在本机的成本、采样间隔都没有测量，因此不给毫秒数。可以确定的是：采样里这两项约占主线程非游戏叶样本的 58%；在二进制采集段内约占 13%。所以主要收益在采集段之外的拓扑、玩家 hook、输入和实体出生路径；ordinary 采集本身改善有限。
- **不能解决的问题**：每帧约 3.3 万次小复制各自的检查开销、逐字段写入、资源每帧重新解析、证言双读、场景固定成本。这些需要另行设计，见 2026-11-16 的性能分析（采样与计数来源同第 1 节）。
- **未知**：
  - 游戏或驱动是否会把已提交的可读页改成守护页（3.5）；
  - `random_owners.cpp` 的 `ReadProcessMemory` 是否在热路径上；
  - 一帧内实际访问的不同区间数。
