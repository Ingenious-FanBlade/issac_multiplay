# tx-01-96：T0-07 操作7，CLI/manifest到Native的参数贯通

日期：2026-10-20（Asia/Shanghai）。完整目标：把T0-07实验输入配置从CLI/实验manifest解析、校验并写入独立C/S IPC v5，在Hook发布和Active前复制冻结到生产`Participant`、`AuthorityTickClock`、`AuthorityTickPipeline`及Native脚本/输入target计算中。只完成操作7；完整故障矩阵和真实Authority停发实验留给操作8/9。

本轮按现行规范先写失败方式、场景、驱动和统一入口，再修改生产实现。阶段内不运行构建、测试或游戏；全部目标完成后纯编译Rust/Native/离线场景程序，并只执行一次整体离线验收。

## 1. 配置schema与支持范围

CLI直接参数和实验manifest使用同一组字段；manifest为严格JSON，未知字段拒绝。CLI同类字段重复、或同字段同时出现在manifest和CLI时拒绝，不静默覆盖。

| 字段 | 默认 | 支持范围/约束 | 单位与实际用途 |
| --- | ---: | --- | --- |
| `input_lead_ticks` | 2 | 1..256，且不得大于future window | Authority tick；启动后的普通输入、A/B脚本/物理target、恢复target |
| `future_window_ticks` | 64 | 1..256 | Authority tick；Participant准入上界、future进度展示、clock安全上界 |
| `max_input_age_ticks` | 0 | 本操作只支持0 | Authority tick；仍立即中性，不支持正值保持 |
| `script_drain_budget` | 64 | 1..256 | 每个Authority tick最多处理的script step数，不是准入尝试数 |
| `admission_attempt_budget` | 258 | `2 + 4 * script_drain_budget` .. 4096 | 每个Authority tick所有AcceptInput尝试数，包含启动、重复、非法和受控故障 |

`admission_attempt_budget`与script step预算分开。默认64个script steps对应保守最小258次准入尝试；任一值不是静默截断/回退，而是明确报错。这里的范围是T0实验支持范围，不是产品常量。

## 2. CLI与manifest入口

`coop-active`新增：

```text
--input-config <json>
--input-lead-ticks N
--future-window-ticks N
--max-input-age-ticks 0
--script-drain-budget N
--admission-attempt-budget N
```

`coop-input-config`是同一解析/校验实现的离线入口，不需要游戏或注入：

```text
IsaacCsT0.exe coop-input-config \
  [--manifest <json>] [--input-lead-ticks N] [--future-window-ticks N] \
  [--max-input-age-ticks N] [--script-drain-budget N] \
  [--admission-attempt-budget N] --output <new-json>
```

manifest字段使用同名snake_case，可选`schema_version=1`。不合法类型、未知字段、越界、lead>future、正值max-age、admission低于script下限或超过上限均以usage exit 2失败；错误文本指出字段与原因，不创建输出。合法输出同时保存`requested`、`effective`、每个字段的`source`、支持范围、单位、mapping version和冻结点。

`coop-active`的`invocation.json`保存同一配置记录与真实`coopIpcVersion`。显式配置使用v5；未提供任何配置来源的历史调用保持v3/v4并用默认配置，避免无必要地改变历史入口。

## 3. IPC v5兼容与冻结

v5完整前缀：

```text
Shared (v3 prefix, 4,204,904 bytes)
+ world_name[96] (192 bytes, v4 prefix)
+ InputRuntimeConfig (48 bytes)
= 4,205,144 bytes
```

`InputRuntimeConfig`固定为`version=1,size=48`，字段偏移8/16/24/32/36/40。旧v3（size4,204,904）和v4（size4,205,096）继续被接受并使用默认配置；v5必须精确匹配4,205,144和config字段，错误版本/长度/字段/range在改动任何Hook或Active状态前返回Arguments。v5允许world name为空；非空时仍沿用原world mapping。

Rust controller只在显式配置时创建v5 mapping并写config；Native初始化先读取并验证mapping config，再一次性配置clock、pipeline和两个Participant。配置不能在运行时热改；第二次Configure、初始化后Configure或已开始tick后的clock window修改都失败。Native新增EventKind 36记录生效配置和mapping version，事件在Hook安装前发布。

## 4. 实现前失败方式

1. CLI显示默认而Native仍使用硬编码2/64/64/258；或只改manifest不改生产链。
2. manifest未知字段被忽略、CLI/manifest同字段静默覆盖、负数/溢出/回绕被截断、非法组合回退默认。
3. `input_lead_ticks`未进入启动/普通/恢复/脚本/物理target计算，或lead>future但被接受。
4. `future_window_ticks`只改显示，不进入Participant validator/Progress和clock上界。
5. `max_input_age_ticks`非0被静默忽略或真的保持旧输入，改变操作6已采纳的立即中性语义。
6. script step预算和admission尝试预算混用，或admission小于`2+4*script`导致生产链最坏展开被提前截断。
7. 配置在Hook发布后或Active后仍可热改；同一session不同组件看到不同值。
8. v5 mapping尺寸/偏移写错，旧v3/v4前缀被覆盖，或world_name/config互相覆盖。
9. 错误version/size/config在安装Hook、绑定、消费script或进入Active后才被拒绝。
10. 新config事件/日志保存原始地址、peer identity、输入布局字段或改变Event/Shared/codec大小。
11. 配置写入24/64字节输入command/state，或借用输入reserved字段。
12. v3/v4默认路径被破坏；旧controller/旧Native混用产生静默截断或错误字段解释。
13. clock上界仍按固定64判断，runtime future window更大时产生target/edge加一溢出风险。
14. 用离线构造一次budget值冒称Native配置贯通；或实现后补测试、失败后自动覆盖重跑。
15. 把离线配置/编译结果冒充实机Hook、Active或网络证据。

## 5. 生产落点

- `input_core.hpp/.cpp`：`InputRuntimeConfig`、支持范围、三态错误、`NextInputTarget`、`NextStartupTarget`、`ScriptDrainAllowed`。
- `coop_input.hpp/.cpp`：Participant一次性`Configure`，future window进入AcceptInput与Progress，初始化冻结。
- `authority_tick.hpp/.cpp`：clock runtime future window及安全上界。
- `authority_tick_pipeline.hpp/.cpp`：pipeline一次性配置admission attempt budget；max cap与配置一致。
- `coop_active_ipc.hpp`：SharedV5及v3/v4/v5严格读取/拒绝；EventKind36。
- `coop_active_runtime.cpp`：初始化读取/冻结配置，启用配置target/script/clock/pipeline，发布生效配置事件。
- `coop_active.rs`：严格manifest/CLI合并、range/conflict/unknown-field处理、`coop-input-config`入口、v5 mapping写入、invocation记录。
- 新增离线驱动/入口：`runtime_config_main.cpp`、`accept-runtime-config.py`、build/script/testdata。

## 6. 唯一整体验收范围

场景先于生产实现写入`cs-t0/testdata/tx-01-96-input-runtime-config.json`：

- Rust配置入口：默认、manifest非默认、tiny预算、CLI覆盖；未知字段、range、lead>future、max-age、admission上下界、冲突、错误类型、schema version和非法CLI整数均fail closed且错误含字段。
- C++离线生产链：默认/非默认/tiny配置实际进入Participant/Clock/Pipeline；lead target、startup target、future window边界、script drain边界、admission budget耗尽、clock安全上界、初始化后不可热改。
- IPC：v3/v4 default读取、v5 non-default读取、v5 config非法、错误mapping size/version拒绝；旧前缀和固定布局由静态断言与运行检查覆盖。
- 一次统一入口组合Rust config CLI检查与C++场景执行，保存命令、manifest、输出、逐step状态、源码、二进制、路径长度时间；不计算/校验哈希。

真实游戏、Hook、注入、输入采样内存、通知/事件环、网络和世界动作均为NOT_RUN。最终状态与限制在结果文档单独记录。

## 7. 审阅后的收紧

- manifest的显式`null`不再等同缺省；缺省用default，显式null按字段名报错。未知/重复CLI选项同样报具体flag。
- clock future window使用独立`window_configured_`，即使尚未开始首scope，第二次配置也拒绝。
- `AuthorityTickPipeline::RemainingAdmissionAttempts`公开当前drain剩余尝试数。可选的`conflict-b`诊断只在剩余至少2时提交；预算不足时跳过该诊断，不冲击正常A/tick和B script。
- Native首读header前用`VirtualQuery`验证实际mapping view至少包含完整`Shared`，再验证header自报size不超过实际region；v5 config尾部在通过长度检查后才读取。
- EventKind36字段打包抽成生产`FillInputRuntimeConfigEvent`，Native与离线场景共用；场景同时断言Participant future first/last、clock window、pipeline budget和事件字段。
- 支持范围上界加入Rust manifest、C++ validation和runtime-maxima场景；tiny预算是明确受限实验，admission上限与script/控制预算分开。
