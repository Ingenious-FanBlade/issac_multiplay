# tx-01-96：操作7静态分析与主智能体逐项审阅

日期：2026-10-20（Asia/Shanghai）。本轮为只读源码分析、配置schema审阅和离线场景设计审阅；未构建、未运行场景、未访问游戏或调试器。按计划启动独立只读静态审阅子智能体，主智能体逐项复读生产路径并记录处理。

## 1. 审阅范围

覆盖[现行规范](../../project-conventions.md)、[操作7计划](tx-01-94-remaining-operations-plan.md)第3/6节、Rust `coop_active.rs`/`main.rs`、C++ `input_core`/`coop_input`/`authority_tick`/`authority_tick_pipeline`/`coop_active_ipc`/`coop_active_runtime`，以及操作5/6的tick/离线链路。重点核对：

- CLI与manifest的字段、默认、范围、未知字段和冲突处理；
- C/S IPC v3/v4前缀与v5追加config的精确布局；
- 初始化复制/冻结与运行时热改拒绝；
- lead、future window、max age、script drain和admission attempt预算分别进入真实计算；
- 不改变24/64字节输入布局、Event/Shared旧前缀和Hook事务。

## 2. 结论与处理

| 审阅问题 | 主智能体复核与决定 |
| --- | --- |
| 如果只把配置写进manifest或Rust显示，不进入Participant validator，future命令仍会被固定64默认接受。 | 采纳。`Participant::Configure`在Initialize前复制配置，`AcceptInput`和`Progress`使用`runtime_config_.future_window_ticks`；默认未配置路径仍为2/64/0。 |
| 如果只改Participant，clock仍按固定64检查上界，runtime future window更大或更小会产生不一致。 | 采纳。`AuthorityTickClock::ConfigureFutureWindow`和`CheckStart`使用runtime window，并在first scope前冻结；默认64边界与旧tx-01-90场景保持同一等价值。 |
| 如果只改显示/构造器，脚本target、A物理target和恢复target仍使用硬编码lead。 | 采纳。Native `SubmitFresh`、`ScriptInput`、`PublishPhysical`和conflict控制提交均改用`NextInputTarget`；startup first target改用`NextStartupTarget`。 |
| script step预算与admission attempt预算是不同单位，不能共用一个数字。 | 采纳。Native script循环使用`ScriptDrainAllowed(config.script_drain_budget)`；`AuthorityTickPipeline::Configure`只写admission budget，`kMaximumAdmissionAttemptBudget=4096`，config校验要求admission至少`2+4*script`。 |
| v5若直接覆盖旧v3/v4前缀或长度不精确，会破坏world/event布局。 | 采纳。`SharedV5 = SharedV4 + InputRuntimeConfig(48)`，大小4,205,144；v3 4,204,904/v4 4,205,096继续使用default config。`ReadInputRuntimeConfig`严格检查version/size/config字段，v5错误在初始化早期fail closed。 |
| 运行时二次Configure或Active后热改会让同一session组件看到不同值。 | 采纳。Participant初始化后冻结；Pipeline只允许Idle时Configure一次；Clock在首个scope后拒绝改变window；Rust只在显式配置时使用v5。Native在Hook安装前复制并应用配置，新增EventKind36记录实际生效值。 |
| manifest未知字段、同字段manifest+CLI冲突、负数/溢出/类型错误若交给serde/默认值静默处理，会违反fail-closed。 | 采纳。manifest `deny_unknown_fields`；所有数字按u64解析后转换为明确范围；manifest已提供字段与CLI同字段冲突报错；错误信息含字段/原因；无效输入不创建输出。 |
| max_input_age_ticks正值如果被接受，会改变操作6的立即中性策略。 | 采纳。Rust和C++都只接受0；非0明确拒绝，不回退0或静默保持旧输入。 |
| 旧v3/v4 Rust controller路径没有显式config时不能被迫升级或改变历史入口。 | 采纳。没有config来源时保持v3/v4默认行为；只有显式manifest/CLI配置时创建v5 mapping。`coop-input-config`仅做同源解析/记录，不冒充实机。 |
| 配置值进入Native但申请budget可能在第一个tick之后才验证，导致Hook已安装。 | 采纳。初始化顺序为mapping/identity/config读取验证与`ApplyInputConfig`在session/Prepare/Hook安装前；EventKind36在Hook安装前发布，失败不进入Active。 |
| 离线驱动只构造一个独立配置对象，不能证明v3/v4/v5布局、Participant/Pipeline/Clock使用同一值。 | 采纳。离线驱动直接调用生产`ValidateInputRuntimeConfig`、`ReadInputRuntimeConfig`、`Configure`、`NextInputTarget`、`ScriptDrainAllowed`；分别检查v3/v4默认、v5 non-default、错误size/version/config，以及实际future/clock/admission行为。 |
| 测试预期必须与本轮实现的容错口径一致。 | 已固定：fresh/validation无domain状态伪造；admission budget按准入尝试数计；clock用future window的安全上界；v5无效config保留旧错误分类，不伪装Accepted。 |

## 3. 实现与验收边界

- 默认值仍是2/64/0/64/258；非默认只通过显式manifest/CLI配置，不改变产品常量或旧路径。
- `coop-input-config`只进行Rust CLI/manifest解析、range/冲突/unknown字段验证和create-new记录，不启动游戏；C++场景只使用生产配置/state类，不加载Native DLL。
- v3/v4兼容读取在离线场景中验证layout/默认值；真实cross-process IPC、Hook安装、输入采样内存、通知、事件环和网络仍未运行。
- 未计算/校验任何哈希；未创建或读取gate/decision/unlock字段；旧失败与历史source保持原样。

独立静态子智能体的初步/最终清单将并入本表；主智能体在最终构建前复核Rust类型/借用、C++ warning-as-error风险、v5 offset/长度和testdata预期。

## 4. 独立静态审阅追加发现与处理

独立只读静态审阅子智能体返回以下问题，主智能体逐项复读并修正：

| 发现 | 处理 |
| --- | --- |
| clock的`ConfigureFutureWindow`虽在first scope后拒绝，但第一次scope前可第二次调用，和“一次性冻结”契约不符。 | 采纳。增加`window_configured_`；成功配置后任何再次配置都失败。离线runtime-nondefault在该scenario首个begin前增加reconfigure拒绝检查。 |
| manifest字段使用`Option<T>`时，显式JSON `null`会被当作缺省并回退默认，违反非法类型fail-closed。 | 采纳。为每个字段增加`deserialize_with`，缺省仍使用default，显式null返回含字段名的错误；新增numeric null和schema null反例。 |
| CLI未知/重复选项只返回泛化参数错误，没有指出具体字段。 | 采纳。option parser现在分别报告`unknown option <flag>`、`duplicate option <flag>`、`option <flag> requires one value`；新增unknown/duplicate CLI反例。 |
| tiny admission budget=6虽通过range校验，但A启动2次+B script fault1启动4次+可选conflict-b 2次可能在第7次被Abort。 | 采纳。保留`2+4*script`为必需准入下限，新增`AuthorityTickPipeline::RemainingAdmissionAttempts`；Native仅在剩余预算至少2时提交可选conflict-b，否则跳过该诊断控制，不让合法A/tick和script被诊断项拖垮。tiny场景在耗尽前断言remaining=0。 |
| 离线场景未断言Progress future window和EventKind36的具体字段。 | 采纳。Snapshot输出`future_first_tick/future_last_tick/future_window_valid`；IPC header新增生产`FillInputRuntimeConfigEvent`，Native和离线场景共用，config-event断言kind/lead/future/max-age/script/admission/mapping/version/flags。 |
| 仅测试v3/v4读取，没有测试default配置应用到Domain。 | 采纳。新增shared-defaults场景，Domain配置来自同一default解析，断言v3/v4读取与Domain/Participant/Clock/Pipeline的default值一致。 |
| v5 config读取依赖mapping header自报size；若实际view更短但header伪造大size，读取尾部可能先AV。 | 采纳。Native在读取header前用`VirtualQuery`验证view至少有完整`Shared`区域，再验证`Shared.size <= RegionSize`，之后才读取v4/v5扩展。 |
| 声称支持范围上界但未覆盖最大值。 | 采纳。Rust新增256/256/0/256/4096 manifest case；C++ validation和runtime-maxima场景覆盖lead/future/script上界、admission4096和target256边界。 |

独立审阅未发现v3/v4/v5 offset、`InputRuntimeConfig`字段偏移或4205096+48=4205144尺寸不匹配；最终产物仍以一次纯编译和一次整体离线PASS结果为准。
