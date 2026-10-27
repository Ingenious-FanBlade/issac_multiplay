# tx-04-08：离线 replay／compare 工程实现

日期：2026-10-27，Asia/Singapore。范围为迁移设计第 8 节的离线工具代码；不是整个 tx-04-08 的完成声明。使用现行规范，不运行局部 replay、compare、单元测试或游戏；唯一整体 E2E 仍待命令、夹具和编排等完整目标收束后执行。

## 实现与边界

`tools/harness/src/world/` 新增纯文件模块，依赖 serde/serde_json 和标准库。`main.rs` 在 remote/E2E 分派前识别两个 world 命令，不经过 Win32、网络、进程内存、Helper 或旧 mapping。Rust 只编排和处理离线文件；C/S 输入、completed/Ack 和 Hook 启停行为不变。

| 文件 | 职责 |
| --- | --- |
| `model.rs`、`profile.rs` | 严格纯值模型、78 项字段 ID/名称/位宽/编码、类别/来源/原因 |
| `validate.rs` | manifest、capture/gap、recorder 终态、诊断的 schema 与语义检查 |
| `io.rs` | 各层重复 JSON 键拒绝、128 MiB 单记录上限、1 MiB 缓冲、create-new 与显式 flush |
| `dataset.rs` | 全文件流式读取；attempt/文件位置/出生元数据索引；生命周期及 checkpoint 对账 |
| `replay.rs` | 全量 canonical 重编码、保留原终态和 legacy sidecar、I/O 计数 |
| `compare.rs` | 对齐依据、构建/布局/采样兼容检查、逐对随机读取、分层比较与冲突整组 |
| `mod.rs` | 参数、独立新产物目录、invocation/failure 报告 |

新格式要求 manifest、captures、lifecycle-events、diagnostics 和 recorder result。writer 的实际 `recording_generation`、`event_watermark`、可空 `grid_index` 均有对应列。生产者失败或未收口的完整合法前缀可重编码，原始失败仍保留，不能获得健康录制资格。截断 JSON、重复字段/对象/来源/成员槽、错宽度/原始位长度、u64 溢出、额外结构列和 domain/profile/version 不匹配会拒绝；diagnostics 内的原生地址等不作比较 key。

批次槽与 gap 环分别消费，**文件行顺序不保证 attempt 有序**。索引拒绝重复 attempt/capture，但按 attempt 元数据顺序归约覆盖缺口；replay 保留原文件行顺序。生命周期文件保持严格递增 sequence，出生编号与 sequence 分开保存；事件缺号或 watermark 超过已交付事件会保留覆盖未知。compare 只同时读入一对世界，索引不缓存全部对象字段。元数据索引随记录/出生数量增长，单行读取受 128 MiB 上限限制。

静态核对发现 `writer.cpp::FlushFaultAt` 原来与诊断外壳重复输出 `at_ms`。本次将首因时间列改为 `fault_at_ms`，外壳数值 `at_ms` 保留；严格解码器不放宽重复键规则。历史已有重复键文件仍拒绝，不覆盖历史原件。并行的命令实现所追加的 checkpoint 诊断和终态计数被读取并保留。

## 命令

以下只说明将来统一 E2E 使用的调用方式，本次没有执行这些命令。

```powershell
harness world-replay --capture <world-dir> --artifact-dir <new-dir>
harness world-compare --left <world-dir> --right <world-dir> --alignment <json> --artifact-dir <new-dir>
harness world-replay --capture <legacy-world-dir> --artifact-dir <new-dir> --legacy-t0
```

参数不接受重复项或未知选项，输出目录必须不存在；源文件只读。`--legacy-t0` 是无值开关：replay 要求所选域确为 `isaac.cs.t0.world-record`；compare 可显式读取旧/新双方，但当前没有旧版本的构建语义映射，涉及 legacy 时输出 INCOMPARABLE，不宣称跨版本 equal。

legacy 保留原域、版本、manifest、captures、生命周期、原 recorder result；存在时还重编码 `consumer.json`、`restoration.json`、`consumer-failure.json`、`controller-failure.json`，不启动旧 runtime，不导入历史运行 PASS。完整 legacy 输入需要生命周期文件；仅有旧 replay 输出的 captures 而无生命周期，不视作完整旧录制。

replay 输出 `manifest.json`、`captures.jsonl`、`lifecycle-events.jsonl`，新版另有 `diagnostics.jsonl`；原 recorder 终态另存 `source-result.json`，工具自己的 `result.json` 为 REENCODED。重复 replay 可识别 `source-result.json`，不会把上一轮工具状态冒充 recorder 状态。只排序 source/object/field/membership 集合；所有 raw32/raw64/raw128、未知状态、u64 字符串和诊断原值保留。±0 的归一化只发生在 compare 的临时比较值中。

compare 输出 `comparisons.jsonl`、`key-collisions.jsonl`、`result.json`、双方 manifest、alignment 原值、复制的依据文件及其来源索引、`birth-evidence.json`。错误返回非零并尽力写 `failure.json`，已写前缀留在新目录中；正常返回零只说明工具完成，语义结果仍需读取 result。

## 对齐格式

`version: 1` 延续 scope/ticks/births/players 语义，新增必需的 `evidence`，不能用地址、seed 或最近 frame 自动生成配对。示例：

```json
{
  "version": 1,
  "left": {"run_epoch": "1", "room_epoch": "1"},
  "right": {"run_epoch": "1", "room_epoch": "1"},
  "ticks": [{"left": "100", "right": "120"}],
  "births": [{"left": "3", "right": "5"}],
  "players": [{"left": 0, "right": 0}, {"left": 1, "right": 1}],
  "evidence": {
    "input": {"left": "p1/input.jsonl", "right": "p2/input.jsonl"},
    "scenario": {"left": "scenario.json", "right": "scenario.json"},
    "births": {"left": "p1/world/lifecycle-events.jsonl", "right": "p2/world/lifecycle-events.jsonl"},
    "players": {"left": "p1/bindings.jsonl", "right": "p2/bindings.jsonl"}
  }
}
```

四种依据的左右路径均相对 alignment 所在目录解析，也允许绝对路径；必须是实际普通文件，逐份复制到新产物的 `alignment-evidence/`，不计算摘要。工具保存原件与引用，不以“存在一个文件”证明输入计划相同，具体场景/输入消费/绑定关系仍由整体 E2E 归约核对。ticks 必须两侧严格递增，births 一对一且必须能在对应录制中找到同 run 的 BIRTH_CONFIRMED；保留对象允许出生在更早房间。players 保持 A=0/B=1 的冻结 ordinal，一对一，不接受互换。未映射对象及缺失的出生连续性保持未知。

## 比较语义

- producer 的 component/build/game/repentogon、layout、profile 和采样/读取语义要求一致且已声明；缺少或不兼容时 INCOMPARABLE，不降级按同名 profile 比较。
- 有效 StandardRoom capture 才比较完整世界；缺样本、重复 tick、gap/odd、来源缺失、不可用字段、UnknownEntity、未解析/退休/复用歧义引用、非有限浮点均不能凭相同错误获得 equal。
- Player 使用显式冻结 ordinal，其他对象使用显式出生映射及 domain。local ID/address/vptr/thread/容器描述符不参与跨进程 key；出生描述作为已知值单列比较。
- 相同 key 多对象保留整组，跨类别冲突也计入；引用到冲突组不可比，不任意选取对象。
- 对有限浮点按原始位精确比较，±0 归一化；raw64/raw128 保持全部位宽。已知字段差异可与更早未知同时存在，输出 `first_observed_difference` 和 `earlier_coverage_unknown`，不丢掉已知差异。
- 总结果依次为有已知差异时 DIFFERENT，否则存在未知时 INCOMPARABLE，否则 EQUAL_WITHIN_PROFILE。`known_field_comparisons` 为整体 E2E 证明实际可比区域提供计数，不以 CLI 退出成功替代覆盖。
- 本工具不负责 V3 的 2,000 次稳定段证明。odd-only 对完整世界 compare 仍是缺口；稳定计数允许跳过 odd-only 的专项规则由未来统一 E2E 归约实现。

## 构建与尚未执行项

Rust harness 使用 i686 MSVC Release 纯编译，构建记录见 [离线工具构建记录](../../../artifacts/repentogon/tx-04-08-world-offline-build/README.md)。未执行 replay/compare、单元测试、mutation 或游戏，L1–L7/V10/V11 尚无本版运行结果。共享工作区的命令、主线程桥和关闭协调改动由并行工作保留，本次不以 Rust 编译替代其验收。
