# tx-01-99：T0-08 前四项实现与验收结果

2026-10-20（Asia/Shanghai）。**操作1–4已实现，唯一一次内存双端统一验收305/305 PASS，退出码0，errors=[]。** 程序运行0.149秒，保存30条原始包及双端协议状态记录。当时T0-08整体为IN_PROGRESS，操作5–9尚未实施。

## 实际交付

- 独立实验身份：magic `ICT8`、protocol1、64字节header、1200字节包上限、文本观察domain；没有使用ISCS/protocol5/channel0/0xFD。
- 公共envelope：完整session、sender role/id、message sequence、精确长度、flags/reserved、CRC32；逐字段小端，encode/decode失败不改输出。
- 四类消息：HELLO、V2 INPUT、区分accepted/processed高水位与实际source的ACK，以及保留历史名字但只有局部原始值/明确缺口的STATE_HASH。
- 一次冻结的双端HELLO/Authority首ACK握手：精确协商build/schema/domain/capability及双方nonce；prepare和成功交付commit分开，终态不复活。协议Established不调用游戏AuthoritySession::Activate。
- 先登记失败方式和双端场景，再写协议生产代码；按用户要求调用GPT-6 Luna / max只读分析与新代码审阅，由主智能体复读、修正。中性tick ACK和负向包构造的E2E场景修订在首次运行前完成。

核心源码：`cs-t0/native/include/isaac_cs_t0/minimal_protocol.hpp`、`cs-t0/native/src/minimal_protocol.cpp`。独立场景工程链接同一核心和已有V2 codec；没有修改或运行Native DLL/Hook路径。

## 唯一运行及可核对产物

[统一结果JSON](../../../../../artifacts/cs-t0/tx-01-99-protocol-01/result.json)记录本轮实际范围、调用、检查和退出码。配套材料：

- [逐项检查](../../../../../artifacts/cs-t0/tx-01-99-protocol-01/checks.jsonl)：305条，全部PASS；其中200个HELLO截断长度边界。
- [原始包与双端状态](../../../../../artifacts/cs-t0/tx-01-99-protocol-01/packets.jsonl)：30条，保留实际wire十六进制、双端状态及结果。
- [文件来源记录](../../../../../artifacts/cs-t0/tx-01-99-protocol-01/provenance.json)：构建源路径、文件长度和时间，没有摘要。
- `artifacts/cs-t0/tx-01-99-protocol-01/sources/`：本轮协议、既有codec依赖、场景、工程、统一入口及实施前文档副本。
- `artifacts/cs-t0/tx-01-99-protocol-01/build/`：实际场景exe、build.log和toolchain.json；stdout/stderr另存。

实际统一入口只调用一次：

```powershell
python cs-t0/accept-minimal-protocol.py --build-dir out/cs-t0/tx-01-99-build-02 --artifact-dir artifacts/cs-t0/tx-01-99-protocol-01
```

覆盖完整双HELLO→首ACK→INPUT→缺输入中性ACK→局部值→odd gap→退休；兼容字段失败、nonce/身份、pending取消/错误commit、重复与同序号冲突、输出原子性、CRC、header/长度/reserved、非法canonical及状态字段边界。新增首ACK序号晚于HELLO、协议固定版本和文本超长拒绝等检查另有源码审阅依据，不把未独立注入的分支宣称为运行覆盖。

纯编译有两份成功记录，未执行程序：`out/cs-t0/tx-01-99-build/`为初次编译；随后源码复读修正中性ACK语义和负向场景构造，最终`out/cs-t0/tx-01-99-build-02/`编译成功，只有这一份进入上述唯一运行。未发生运行失败或自动重跑，初次构建原样保留。

## 复现

在保留相对目录结构的源码副本中，使用VS2022/v143、Win32和Python；选择全新输出目录，不覆盖本轮artifact：

```powershell
pwsh -NoProfile -File cs-t0/build-minimal-protocol.ps1 -OutputDirectory out/cs-t0/tx-01-99-reproduce-build-01
python cs-t0/accept-minimal-protocol.py --build-dir out/cs-t0/tx-01-99-reproduce-build-01 --artifact-dir artifacts/cs-t0/tx-01-99-reproduce-01
```

这是可重复入口，不表示本会话已再次执行。完整wire和状态机定义见[协议文档](tx-01-99-wire.md)，静态分析和修正见[审阅记录](tx-01-99-static-review.md)。

## 明确限制

- 无哈希路线，CRC仅检测包损坏；显式build标签不能证明二进制实际相同。nonce由调用者提供可信新鲜值，本核心不生成随机数或认证身份。
- 没有socket/Steam/跨进程transport、真实游戏、注入、Hook、Native主线程接入或世界动作；没有改变既有Authority scheduler。内存双方的环境和原样值是场景输入，不冒充实机采集。
- STATE_HASH仅为partial观察/Gap，无完整世界、snapshot/apply或跨进程对象配对；entity/use仅为来源进程的观察标识，不能据此判断两个世界相等或定位世界分歧。
- 本轮只有逐kind消息序号水位和有限最近包去重，没有完成输入冗余、业务ACK/state进度单调策略、完整golden/fuzz corpus或FaultInjectionTransport；这些仍属操作5–9。

没有计算或校验文件、构建或状态哈希，没有新增gate/解锁要求，也没有进入原版Online/Isaac Sync。
