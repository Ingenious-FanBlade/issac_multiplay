# tx-01-96：T0-07操作7结果与复现

日期：2026-10-20（Asia/Shanghai）。**操作7的CLI/manifest到Native运行配置贯通已完成：Rust配置解析/校验、IPC v5写入、Native初始化冻结、Participant/Clock/Pipeline及Native target/script路径使用生效配置；最终纯编译成功，统一离线验收为20个配置入口case、9个生产状态场景、83条命令、104/104检查PASS。** 真实游戏、Hook、注入、采样内存、事件环和网络未运行。

## 1. 交付行为

- `input_lead_ticks`默认2、支持1..256；`future_window_ticks`默认64、支持1..256；`max_input_age_ticks`只支持0；`script_drain_budget`默认64、支持1..256；`admission_attempt_budget`默认258、支持`2+4*script_drain_budget`..4096。
- `coop-input-config`和`coop-active`共用Rust manifest/CLI解析：manifest未知字段、显式null、错误type、range、lead>future、非法组合、manifest+CLI同字段冲突、未知/重复CLI flag均usage exit2并指出字段；非法输入不创建输出。
- 显式配置创建IPC v5：完整v4前缀4,205,096 + 48字节config = 4,205,144。v3(4,204,904)/v4(4,205,096)继续可读并使用旧default值；v5要求精确size、config version/size/reserved/range。
- Native在安装Hook/进入Active前读取、验证并冻结配置；clock window、pipeline admission budget和两个Participant均一次性配置。clock即使首scope前也不接受第二次window配置，pipeline/participant也拒绝重复或初始化后的配置。
- 实际运行路径：
  - `NextInputTarget`控制普通/恢复/脚本/物理target lead；
  - `NextStartupTarget`控制启动completed+1；
  - Participant validator和Progress使用配置future window；
  - clock安全上界使用配置future window；
  - script drain使用配置step预算；
  - pipeline使用配置admission attempt预算；
  - 可选conflict-b诊断仅在剩余准入预算至少2时提交，避免小预算诊断拖垮正常A/tick。
- EventKind36通过共享`FillInputRuntimeConfigEvent`记录生效config和mapping version，在Hook安装前发布。Event仍128字节、Shared旧前缀和24/64字节输入布局不变。
- Native使用`VirtualQuery`确认实际mapping region至少容纳完整`Shared`，且header自报size不超过region，之后才读取v4/v5扩展，避免伪造v5 size越界读取。

## 2. 最终构建与统一验收

最终纯编译入口：

```powershell
pwsh -File ./cs-t0/build-runtime-config.ps1 `
  -OutputDirectory out/cs-t0/tx-01-96-build-08
```

最终统一离线入口：

```powershell
D:\miniconda3\python.exe cs-t0/accept-runtime-config.py `
  --build-dir out/cs-t0/tx-01-96-build-08 `
  --artifact-dir artifacts/cs-t0/tx-01-96-runtime-config-04
```

结果：`PASS`，20 Rust config cases、9 scenarios、83 command rows、104/104 checks、scenario exit0、errors=[]、约0.05秒。104项为20个配置入口case、83条场景逐step检查和1项完整transcript检查。

覆盖：

- Rust默认、manifest非默认、tiny、CLI override、maxima；未知字段、未知/重复CLI、显式null、非法type、range、lead>future、max-age、admission上下界、冲突、schema version和非法整数。
- C++配置range全矩阵、v3/v4 default读取、v5 non-default读取/invalid config/wrong size/wrong version、EventKind36打包、Participant future first/last、clock runtime window、pipeline budget、初始化前后reconfigure拒绝。
- 默认/非默认/tiny/maxima配置实际进入Domain/Participant/Clock/Pipeline；future边界、lead target、startup target、script drain边界、admission budget耗尽、clock安全上界。
- v3/v4 default与v5 non-default分别应用；tiny场景先耗尽6次准入并断言remaining=0，再观察第7次fail closed。

## 3. 历史失败与修正

所有失败目录原样保留，不覆盖、不改判：

- `out/cs-t0/tx-01-96-build-01`：PowerShell 5把cargo stderr当作终止性NativeCommandError；改为cmd重定向日志后继续。
- `out/cs-t0/tx-01-96-build-02`：Rust `BTreeMap::entry`类型推断失败；补明确String key。
- `out/cs-t0/tx-01-96-build-03`：命令执行超时，未产生可执行最终构建。
- `out/cs-t0/tx-01-96-build-04`：C++ `InputRuntimeConfig`未使用namespace alias，Native编译失败；补`coop_active` using声明。
- `out/cs-t0/tx-01-96-build-05`、`-06`：成功纯编译；对应场景仍有argc/大`Shared`临时对象问题。
- `artifacts/cs-t0/tx-01-96-runtime-config-01`：FAIL；场景程序参数个数检查写成4，实际入口3，exit2，未产生events。
- `artifacts/cs-t0/tx-01-96-runtime-config-02`：FAIL；离线场景对4.2MB `Shared`使用值初始化临时，造成栈溢出0xC00000FD；改为heap byte buffer承载mapping测试。
- `artifacts/cs-t0/tx-01-96-runtime-config-03`：FAIL 101/104；仅3个测试预期字段名/JSON类型不一致（pipeline budget字段名、event u64字段以字符串输出），修正testdata后重跑。
- `out/cs-t0/tx-01-96-build-08` + `artifacts/...-04`：最终PASS。此前每次失败均先定位并实质修正，再使用新目录执行；旧commands/events/source/result均保留。

## 4. 未运行项与限制

**真实游戏、Windows Hook/原生original、注入、输入采样/共享内存、事件通知/事件环背压、网络收包/ACK、真实Active和世界动作效果均为NOT_RUN。** 离线场景直接链接生产配置结构、codec、Session/Clock/Pipeline/Participant和IPC layout helper，但不加载Native DLL，不把helper/构造器结果当实机证据。

B仍是脚本源，操作8完整故障矩阵和操作9真实Authority停发实验未实施。`max_input_age_ticks`正值的保持旧输入行为未实现也未授权。配置范围是T0实验支持范围，不是产品常量或网络协商能力。v3/v4现场混用只做布局/读取兼容测试，真实跨进程mapping/安装仍未运行。

## 5. 产物

- [最终结果](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/result.json)
- [最终场景命令](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/commands.txt)、[逐step事件](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/events.jsonl)
- [Rust配置case输出、manifest、stdout/stderr](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/configs/)
- [最终源码快照](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/sources/)、[二进制及纯编译日志](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/build/)、[路径/长度/时间出处](../../../../artifacts/cs-t0/tx-01-96-runtime-config-04/binary-provenance.json)
- [操作7实现与失败清单](tx-01-96-runtime-config.md)、[静态审阅与修正](tx-01-96-static-review.md)

本轮不计算或校验哈希；实际配置以Rust `effective`记录、Native EventKind36和C++场景快照为准。后续复现须使用新build/artifact目录，不覆盖历史失败。
