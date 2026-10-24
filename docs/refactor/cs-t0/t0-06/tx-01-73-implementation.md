# tx-01-73：T0-06 只读采集、身份及结构化比较实现

后续运行更新（2026-10-18）：tx-01-74 已按统一执行器完成一次 E2E，整体 **FAIL**，未自动重跑。51/70 槽正常恢复、编码复现及单字段比较通过；W07 只有 2/10,000 次连续完整采集，仍有类别/生命周期/独立证据未覆盖。见[本轮结果](tx-01-74-e2e-results.md)。本说明中的“未执行”保留 tx-01-73 交付时的历史事实。

日期：2026-10-18。执行约定见[现行规范](../../project-conventions.md)。本次采用 tx-01-71，并按用户确认补入 Game::Spawn 完成边界：实现运行时代码和离线工具，代码完成后编写 E2E 清单；**运行验收未执行**。纯编译不执行生成的程序。本轮没有测试、游戏操作、注入、哈希计算或校验，没有新增单元测试。

## 1. 实现与入口

- `cs-t0/native/src/world_record.cpp`：同 owner 的 19 个函数入口观察、阶段 token、身份/代次、有界字段复制和三缓冲 producer。
- `cs-t0/src/world_record.rs`：内部 mapping consumer、固定 profile manifest、JSONL、离线重编码及结构化比较。离线命令不打开游戏进程。
- `hooks`、`loop_runtime` 与 `coop_active_runtime`：入口 trampoline 加入现有事务，阶段观察位于现有 wrapper 内；Authority 提交和已确认 A/B ordinal 通过内部值接口传递。

采集入口保留现有全部必需参数，追加：

```text
IsaacCsT0.exe coop-active --game-exe <exe> --native-dll <dll> --helper <helper>
  --artifact-dir <new-dir> --script <json> --authorization <actual-user-authorization>
  [--b-character <choice>] --world-record min-read-v1
```

上述是使用语法，本轮没有执行。沿用现有 unique-process 发现和虚拟 B 创建流程；没有独立只读启动模式，也不启动第二个游戏。两份比较材料可以来自不同机器，或先后启动的两个不同进程。实机操作仍按实际授权进行。

未传 `--world-record` 时使用原 coop IPC v3、51 槽和原收尾分析。传入时使用内部 v4：在完整 v3 prefix 后追加 96 个 UTF-16 code units 的 mapping 名称；v3 各 offset 不变。世界数据为独立 version-1 mapping，不进入输入事件 ring、ISCS、正式 Lua API 或旧 Lockstep 状态。可选模式输出 `CAPTURE_CLOSED` 或 `FAIL`；`CAPTURE_CLOSED` 只说明收尾健康，不代表 T0-06 E2E PASS。

离线入口：

```text
IsaacCsT0.exe world-replay --capture <world-dir> --artifact-dir <new-dir>
IsaacCsT0.exe world-compare --left <world-dir> --right <world-dir>
  --alignment <alignment.json> --artifact-dir <new-dir>
```

输出目录必须不存在。命令严格解析输入，并保存 invocation 和失败原因；不覆盖输入、既有结果或历史证据，不自动重试。

## 2. 原生接入与生命周期

19 个入口的 ordinal 是 51–69；原有 0–50 槽保留，总数 70，不以单个 u64 mask 表示新槽。沿用 tx-01-71 的 18 项入口、ABI 和完整指令跨度。新增 Spawn：VA `0x6FE410`，RVA `0x2FE410`，入口 6 bytes `55 8B EC 83 E4 F8`，ECX=this、8 个 raw32 栈槽、`ret 0x20`；已在计划阶段通过当前 IDA 静态读取确认，不是运行时观察。

入口 patch 仅复制已确认无相对寻址的固定 prologue，创建 RX trampoline，并保存原始字节。安装/恢复检查原字节及挂起线程指令位置；函数入口跨度或 wrapper 尚未登记在途计数的前 8 bytes 内存在指令指针时返回具体失败，不覆盖未知 owner。安装失败回滚已安装前缀，恢复逆序执行；没有新增重试。模块、已发布 trampoline 和原型自有资源保持进程生命周期有效。

固定 x86 转发桥保留调用者原返回地址及原参数槽，在保持模 16 对齐的影子参数栈调用 trampoline 一次，按各自 ABI 清理参数。观察前后保存通用寄存器、EFLAGS、x87/SSE/MXCSR；观察元数据丢失只停用采集，不据此猜测返回地址。桥的在途计数先于观察逻辑登记，关闭后等待在途桥与观察 callback 收敛。正常路径之外的所有 EH、worker 和第三方改写行为没有运行证明；未知或配对失败不输出安全恢复成功。

同次 outer token 要求普通 owner、Room、标准尾部各进入/正常返回一次，嵌套顺序正确，没有生命周期污染。原 `g_update()` 正常返回后、coop TopExit 前复制。Authority 普通提交直接传入 tick/kind/scope；Player-only 提交生成 `ODD_PLAYER_ONLY` gap，不贴最近 tick。没有提交的阶段样本保留 Authority unavailable，不能用于跨进程 Authority 对齐。

本地关联分开记录 object_id、use_generation、出生事件、revision、来源 membership、初始化依据和退休状态。地址关联使用有界 radix tree；不计算地址或状态哈希。Factory 重用换 use，明确重构/物理复用换 object_id；Remove 请求与成员关系、析构分开。已知 Morph 保留出生身份，未知重新 Init 撤销比较连续性。Restore/Replace 污染本批并保留关系事实，run teardown 退休本观察域。启用前对象保留 `FIRST_OBSERVED`，不补造出生序号。

读取依据 `0` 表示没有观察历史，`1` 表示基 Init 正常返回，`2` 表示已知创建 owner 或受支持 Morph 正常完成。实现中发现从已启动 run 注入时 A 的专用字段会持续 NOT_READY，已先向用户询问；用户于本会话明确采纳补充规则：`3`=`USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS`。仅当 A/B 的 exact-pair 绑定在有效 Authority 模拟窗口内跨过一次原生 PlayerUpdate 正常返回仍保持相同 generation/绑定，并静默重读实际唯一 device/control/key pair 仍一致时，允许读取其当前专用字段；这是用户接受的当前值读取规则，不冒称静态证明或 Init/出生历史。该依据在绑定失效时撤销；FIRST_OBSERVED 与缺失出生序号保持不变。专用字段只有足够依据时才输出 VALUE。A/B 语义锚来自现有 exact-pair 绑定；其它 Player 不由 roster 下标获得网络身份。

## 3. 读取、缓冲及文件

2026-10-18 后续修改：tx-01-75 已将 controller 消费拆为通知驱动的原始读取和独立编码/写入线程；原始复制后立即归还共享槽，另有用户确认的 256 MiB / 256 项自有队列。world 内部 mapping 为 v2、布局不变，min-read-v1 文件 schema 不变；详见[即时消费说明](tx-01-75-immediate-consumer.md)。下面的共享 mapping 大小与 Native 三批预算继续适用。本次运行验收未执行。

按 tx-01-71 的逐字段规格覆盖七类 Entity 与独立 Grid 域，八个 EL 视图、roster、Esau、Player 保留关系、448 Grid 槽及 8 门槽。同一同步批次合并别名，同时保留来源槽；特殊来源不被自动视为 active 世界。未知类别保留可确认头部；Grid 不套用 Entity HP/flags。

只复制值，不调用 Lua 查询、EntityPtr 构造/setter、虚 getter、RNG、Spawn/Morph、原生记录生产或反序列化函数。EL/vector 描述符及条目变化、root/room/epoch 变化、不可读、预算和初始化不足均显式报告。Grid 坐标来自明确的 Grid 来源槽；同对象多个 Grid 槽发生歧义或只有 door 来源时，派生坐标为 NOT_READY。已观察复用、清理或缺少同代事件依据的关系保持未解析，裸地址不进入比较。

内部 mapping 为 136,316,688 bytes（约 130 MiB），包含三批 buffer 和独立事件 ring；这是原型资源预算，不是游戏上限或已测性能。每批最多 16,384 对象、65,536 membership，Player 关系工作表最多 64 个节点。producer 仅主线程写 WRITING，controller 仅消费 READY/READING；状态通过 release/acquire 交接。没有 FREE 时记录背压，不等待后台。身份、事件及序号耗尽停用自身采集，不调用 coop::Fail。

`world/` 产物：

- `manifest.json`：domain/version/profile、固定字段、类别、来源、预算、HP 假设和未确认字段。
- `captures.jsonl`：capture/gap，u64 用十进制字符串；raw32/raw64 为固定宽十六进制，其他字节为内存顺序 bytes_le；地址/线程/容器描述符只在 diagnostics。
- `lifecycle-events.jsonl`：取得、Init、发布、owner 返回、Morph/Restore/退休及连续性事件、完整来源覆盖下的 membership 观测，包括未落入快照的短命对象。
- `result.json`：采集关闭、计数、背压、错误和各入口观察进入/返回计数，不是 E2E PASS。
- `restoration.json`：70 个目标的原始字节/值与恢复记录；异常控制器退出另写 `controller-failure.json`。

## 4. 比较与显式对齐

domain=`isaac.cs.t0.world-record`，version=1，profile=`min-read-v1`。字段 ID/宽度、必需字段和类别顺序固定，不逐样本缩减。保留 raw 浮点；比较视图仅归一化 ±0，其他有限值精确比较，NaN/Inf 不可比较。未知字段状态或引用、相同缺项、相同错误均不等于 equal。

对齐文件格式如下，示例数字只是语法，不是本轮采集事实：

```json
{
  "version": 1,
  "left": {"run_epoch": "1", "room_epoch": "2"},
  "right": {"run_epoch": "1", "room_epoch": "3"},
  "ticks": [{"left": "32", "right": "34"}, {"left": "34", "right": "36"}],
  "births": [{"left": "7", "right": "9"}],
  "players": [{"left": 0, "right": 0}, {"left": 1, "right": 1}]
}
```

tick pairs 在两侧都严格递增；出生和 A/B 映射各自一对一。映射依据由实际输入、生命周期事件和场景记录提供；相同数字不是依据。没有映射不自动按地址、位置、seed 或最近帧补配。

候选 key 使用已映射出生事件及冻结出生描述；A/B 可使用明确映射的当前冻结 ordinal。多对象同 key 保留整组冲突，不以地址打破平局。引用仅在目标同代且 key 唯一时可比较。比较器索引 JSONL 的文件位置，逐对读取样本，不把 10,000 个完整世界同时留在内存。

输出 `comparisons.jsonl`、`key-collisions.jsonl`、`result.json`、原对齐文件及双方 manifest，逐 run/room/class/entity/field 定位。状态为 DIFFERENT、EQUAL_WITHIN_PROFILE 或 INCOMPARABLE。存在更早缺口/未比较范围时只报告 first_observed_difference，并保留 earlier_coverage_unknown；结论只覆盖显式配对范围。离线非法 schema/载荷拒绝解析并保存 failure.json。

## 5. 构建与实际完成状态

仅直接调用 Cargo build 和 MSBuild Native/Helper Release Win32 项目，不调用 `build.ps1`、测试项目或生成的 exe。编译过程中出现并修复：巨大共享布局的聚合默认初始化导致 C1060；调整成 POD 布局时的两处 static_assert 编辑错误。相关失败不记为运行验收失败或 PASS。

实现范围已经落入源码；Native、Helper、Rust Controller 的 Release 纯编译均成功，入口、参数、退出码、日志与产物路径见[构建记录](../../../../artifacts/cs-t0/tx-01-73-implementation/build-record.json)。运行时完整覆盖、时延、10,000 次只读行为、跨进程身份/字段比较、异常路径及真实 70 槽恢复仍待未来一次统一 E2E。该次清单见 [tx-01-73 E2E 清单](tx-01-73-e2e-checklist.md)。bootstrap 和保守关系解析可能使大量样本 INCOMPARABLE，这是真实的覆盖限制，不删字段或强配身份消除。
