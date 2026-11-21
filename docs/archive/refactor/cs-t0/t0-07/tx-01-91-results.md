# tx-01-91：操作 3 的结果与复现

2026-10-20（Asia/Shanghai）。**T0-07 操作3已实现；唯一一次统一离线验收PASS：22个场景、148条主命令、4357条批量输入记录、152/152项检查，退出码0、errors=[]。** Native与离线场景程序纯编译成功。本轮没有游戏查询、输入、注入、Hook安装/恢复或自动重跑。

## 交付内容

生产Participant新增 `ClientInputProgress` 只读快照：本地生成序号、已接受高水位、显式未来时间窗、最近有效连续状态、可归属接收与Resolve/正常处理进度、逐动作边沿账本和12种拒绝分类计数。进度均保留身份/代际语义；失绑/退休不清会话高水位、history和诊断历史，旧状态返回中性且标为无效。

received tick取到达时authority Completed，与packet target分开。收到非法/旧代/重复/退休后尝试也可以有可归属接收事实，不构成有效输入、timeout或ACK依据。accepted只表示成功入队，Begin只表示已解析；只有正常End且session/绑定/代际仍有效才确认processed。fresh binding未收到首条输入时不虚构已处理状态，普通缺输入继续按既有规则立即中性。processed最高序号与当范围source_sequence分开，不能当作连续确认前缀。

同tick合并edge时保留Bomb/Item/Pill/Drop各自来源，accepted、resolved和processed分别记账；重复Read/End不会增加处理次数。编码失败短路也进入统一计数；失绑清队列和已接受后过期分别记为discard/drop，不混作Submit拒绝。新增scope_pending保护不可见范围重入，持久last_resolved排除已经解析的tick，future上界及edge target加一不回绕。

Native只在退休处输出EventKind33，每client最多28行；逐次提交与查询不增加这些事件。Event仍128字节，Shared v3/v4布局及24/64字节输入codec保持。未新增Hook或网络协议，未修改原版Online/Isaac Sync路径；lead=2、启动例外、max_input_age_ticks=0及操作2的时钟约束保持。

## 静态分析与主智能体审阅

按用户要求调用 **GPT-6 Luna / max** 子智能体，限定只读分析。主智能体重读修改前tx-01-90源码快照、当前Participant、Session/Clock、Native Submit/退休与只读查询、IPC布局和controller序列化后逐项审阅。重点补充：received的可归属定义、fresh binding不虚构processed、last_resolved在失绑后仍阻止迟到、processed最高序号不等于连续前缀，以及编码失败/过期丢弃计数。没有新增IDA连接需求，见[逐项审阅](tx-01-91-static-review.md)。

## 实际覆盖与限制

唯一入口运行生产Session/Clock/V2 codec/Participant，覆盖生成/接收/接受/处理分离、合并edge来源、正常缺输入中性、全部拒绝类别、冲突退休、旧代/重绑/history保留、在途失效、异常/终态状态屏蔽、不可见范围迟到/重入、队列满、4100次正常生命周期跨4096项history淘汰、u64时间窗及edge溢出、target与sequence次序不同的processed高水位。

其中148条主命令各一项结果检查，另外4项检查完整输出及两批记录；4357条批量输入逐条记录并在批次检查中核对顺序、target、decision和completed，不能表述为4357项独立顶层检查。全进程约0.082秒。

`accepted-then-expired-fault`是显式离线Participant API故障注入：跳过较早Resolve来观察已接受后过期的队列丢弃；不代表Native正常链允许漏掉单个client。极值场景只构造离线Clock初值。绑定为自有地址标记，不操作或解引用游戏对象。

**真实Native运行、线程时序、Hook/恢复、事件环背压和世界动作效果均为NOT_RUN。** edge processed只证明生产输入状态在有效的正常结束范围确认，不能证明世界内炸弹/道具一定成功。计数u64饱和分支经静态审阅，未制造u64次到达来运行覆盖。操作4–9未实施，也未重跑操作1/2的旧验收。

## 可复查产物

- [机器结果与每项检查](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/result.json)
- [实际输入](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/commands.txt)、[完整状态事件](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/events.jsonl)、[逐条批量输入结果](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/batch-events.jsonl)
- [实现前场景副本](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/sources/cs-t0/testdata/tx-01-91-client-input-state.json)、[源码与静态审阅快照](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/sources/)
- [Native/离线二进制与编译日志](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/build/)、[出处记录](../../../../../artifacts/cs-t0/tx-01-91-client-input-state-01/binary-provenance.json)（只记录路径、长度、时间，不计算或校验哈希）

## 复现入口

使用新目录，保留本轮原始产物；不自动重复已经完成的验收：

```powershell
& ./cs-t0/build-client-input-state.ps1 -OutputDirectory out/cs-t0/NEW-CLIENT-STATE-BUILD
python cs-t0/accept-client-input-state.py `
  --build-dir out/cs-t0/NEW-CLIENT-STATE-BUILD `
  --artifact-dir artifacts/cs-t0/NEW-CLIENT-STATE-ACCEPTANCE
```

纯编译使用VS2022/v143，Hostx64→Win32 Release；本次Python路径为 `D:\miniconda3\python.exe`。失败清单、场景、驱动与统一验收入口在生产修改前完成；期间仅静态阅读和纯编译，最终只执行一次入口，没有实现后补写单元测试。旧历史事实与失败记录保持。
