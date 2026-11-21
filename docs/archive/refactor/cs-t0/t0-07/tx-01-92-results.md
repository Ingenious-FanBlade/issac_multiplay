# tx-01-92：操作4结果与复现

2026-10-20（Asia/Shanghai）。**T0-07 操作4已完成：A/B共用生产AcceptInput核心；Native最终纯编译成功；唯一一次统一离线验收40场景、303/303项检查PASS，退出码0、errors=[]。** 实际保存288条场景命令、4734条批量输入结果、4846条逐次准入字节记录，用时0.166秒。未访问游戏、未运行Hook/注入/实机输入或真实网络，没有重跑验收。

## 交付与行为

`Participant::AcceptInput(command)` 将本地命令原样复制为64字节，转入与远端字节载荷相同的 `AcceptInput(span)`。严格长度与V2解码后，统一进入原有session/client/generation/sequence/target/edge/容量验证、history去重及逐client队列；底层Submit和编码拒绝方法已收为私有，不能从生产调用方绕过准入。Native原Submit wrapper及A/B物理/脚本/启动/控制输入共同调用此API，保留事件发布与A冲突关闭session、B冲突退休自身的处理。

每次尝试只计一次submit。新增非法长度span仅在完整40字节头可解析且身份匹配时计received；24–39字节不补造sequence/target。完整64字节原有拒绝顺序不变。字节输入不会按payload身份自动重路由，generation/completed仍由可信主线程上下文提供。24/64字节codec、Decision、Participant成员、Event/Shared布局、Hook数量、lead=2、启动例外、缺输入立即中性和时钟语义保持。

按用户要求使用 **GPT-6 Luna / max** 只读静态分析，主智能体复读并逐项审阅；采纳对malformed-span口径和真实远端未接线的说明，另修正短头在无效配置下可能被默认零头误归属的边界。详见[实现前失败清单与最终契约](tx-01-92-input-admission.md)和[主智能体审阅](tx-01-92-static-review.md)。

## 实际覆盖与限制

唯一入口运行生产Session/Clock/V2 codec/Participant，覆盖typed/bytes两个方向的重传、跨入口冲突、A/B相同sequence独立、错误身份不改路由、两种入口的非法编码/生命周期/时间窗、空/39/63/65字节长度、在途非法尝试不改变current、启动两份命令不重复edge、本地及远端队列满、history淘汰与旧代拒绝、同tick逐edge来源，以及B静默120 tick期间A持续完成120次离线生命周期。

303项检查含288个逐步断言及完整性、批量结果和原始字节一致性检查；4846条准入记录不是4846项独立顶层检查。22个原有契约场景在本次新增入口上执行，与18个操作4场景组成同一次调用，没有另跑旧验收。失败清单、场景、驱动及入口均在生产修改前写好；未在实现后补写单元测试。

启动场景提交按既定规则形成的两份命令，没有运行Native物理采样或SubmitFresh；marker绑定只使用自有地址，不解引用游戏对象。边沿processed表示有效正常结束范围中的输入确认，不保证世界动作一定发生。极值clock初值只用于离线场景。

**真实游戏、Hook/恢复、线程时序和真实网络接入均为NOT_RUN。** 当前B生产源仍是脚本；字节入口是未来主线程接收队列可调用的共同核心，没有新增socket、认证/dispatch、网络线程或协议。未来接线必须保留可信client route、接收代际及Native事件/冲突处理职责。操作5–9未实施，不宣称整个T0-07完成。

## 可复查产物

- [机器结果与全部检查](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/result.json)
- [场景命令](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/commands.txt)、[逐步状态](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/events.jsonl)、[批量输入结果](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/batch-events.jsonl)
- [逐次原始载荷、来源、代际与决定](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/admissions.jsonl)
- [实际执行的源码与场景快照](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/sources/)、[二进制及纯编译日志](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/build/)、[路径/长度/时间出处记录](../../../../../artifacts/cs-t0/tx-01-92-input-admission-01/binary-provenance.json)

第一次纯编译因受限环境无法读取Windows SDK安装目录失败，保留[原始日志](../../../../../out/cs-t0/tx-01-92-build/Native-build.log)。随后允许读取SDK的纯编译成功；主审补充短头边界后，最终版本在 `out/cs-t0/tx-01-92-build-final` 编译成功。各次均只编译，没有执行测试；唯一验收使用最终二进制。

## 复现

本轮运行的统一入口：

```powershell
D:\miniconda3\python.exe cs-t0/accept-input-admission.py `
  --build-dir out/cs-t0/tx-01-92-build-final `
  --artifact-dir artifacts/cs-t0/tx-01-92-input-admission-01
```

原目录保留，不自动重复；后续复现使用新目录：

```powershell
& ./cs-t0/build-input-admission.ps1 -OutputDirectory out/cs-t0/NEW-ADMISSION-BUILD
python cs-t0/accept-input-admission.py `
  --build-dir out/cs-t0/NEW-ADMISSION-BUILD `
  --artifact-dir artifacts/cs-t0/NEW-ADMISSION-ACCEPTANCE
```

工具链为VS2022/v143、Hostx64→Win32 Release。入口只创建新产物目录，复制输入/源码/二进制并调用场景程序一次；出处仅记路径、长度与时间，不计算或校验哈希。所有历史结果与失败记录保持。
