# tx-01-99：静态分析与实施审阅

2026-10-20（Asia/Shanghai）。按用户要求，由GPT-6 Luna / max只读静态分析；主智能体直接读取源码复核和实现。没有运行游戏、调试器、程序实验或测试。本轮没有可调用IDA/x32dbg；协议任务未依赖新游戏ABI逆向。

## 原有生产事实与处理

| Luna结论 | 主智能体复核与实际处理 |
| --- | --- |
| AuthoritySession::Activate要求Warmup及A/B绑定，协议完成不能代替 | 直接读取authority_session.cpp；新增Endpoint为独立PeerState，只提供Established，不持有或调用AuthoritySession对象。 |
| 当前输入为V2的24/64字节，旧V1语义不同 | 复读coop_input.hpp/.cpp；INPUT复用V2 Encode/Decode/Valid，外层session/client交叉校验，不新增输入队列。 |
| envelope不能memcpy任意原生结构 | 全部协议header、HELLO、ACK和状态前缀/字段逐字段小端编解码；V2保留既有已固定的小端codec。 |
| accepted/processed是高水位，实际来源另列 | 复读Participant::End/Progress和pipeline::Receipt；ACK字段分开，不称连续前缀，不触发输入消费。 |
| 双HELLO和首ACK须校验完整协商身份与nonce | HELLO精确matcher，首ACK回显Remote HELLO sequence及完整Authority HELLO，含build/domain/schema/capability/双方nonce和冻结A/B。 |
| ACK入队与Remote接收之间可能单侧Established | Prepare/CommitSent分离；验收明确检查Authority已建立、Remote尚协商的中间状态，不称双方原子Active。 |
| 文本build标识不证明二进制内容相同 | API和协议说明明确限制，不读取PE或计算文件摘要。 |
| STATE_HASH不能恢复hash，局部字段不证明完整世界 | 只提供PartialValues/Gap；字段带原始字节，拒绝完整状态、odd局部值、非有限F32和引用编码。 |
| Object.id/use尚未证明跨进程稳定，address/vptr不可传 | wire里的entity/use仅为来源进程观察标识，不接采样器，也不声称跨端可比；不含address/vptr/Player或device指针字段。 |
| 现有Native无可复用CRC | 新增专属PacketCrc，明确CRC-32/ISO-HDLC与56..59清零规则，检测包损坏；不读取构建或状态以计算摘要。 |

## 实现期间主智能体复读修正

- 文本helper超过31字符返回非法空标识，避免不同build名称被静默截断成相同identity。
- rejection_mask只允许现有Decision拒绝值3..14，duplicate不计拒绝；count/mask零状态一致。
- 即使双端都误配相同的未知schema/domain/capability，也不能握手Established；当前支持V2/schema1/指定观察domain/capabilities7。
- INPUT的last_ack不得超出本端实际收到/对端实际发送的ACK水位；message/input sequence分离，0/UINT64_MAX输入或message序号拒绝。
- encoder/decoder输出先存临时对象；endpoint接收只有全包、identity、phase和协商检查成功才保存接收水位和输出。失败的当前peer握手只允许进入Failed，不打开输入。
- 复读Participant::End发现缺输入中性tick会保留processed高水位、推进processed_tick、将processed_source置0。ACK validator已允许这一真实生产组合，现有双端E2E生命周期的progress段对应更新为tick3/source0/high-water1；不是重放旧来源。没有新增单元测试或提前运行场景。
- 复读E2E负向包构造时，修正wrong-session/wrong-sender段：同时更改envelope和内部command，使其先成为自洽、可编码的错误来源包，再交给另一个endpoint拒绝；检查Encode成功并保存实际包，避免编码失败后错误复用上一包而产生假阳性。

## 新实现的Luna只读审阅与主智能体处理

- **采纳：首ACK envelope序号应大于Authority自己的HELLO序号。** 已补检查；Remote除匹配本地HELLO echo，还核对当前ACK晚于已收到的Authority HELLO，失败进入Failed。
- **采纳收紧：HELLO内部protocol明确限定kVersion。** 原Config没有protocol可配置成员，LocalHello固定1，原PeerHello精确比较已阻止双方协商到其他协议；仍在共享HelloValid中明确固定1，避免孤立codec接受无效实验协议。
- **确认范围：ACK/state的业务进度单调性尚未实现。** 当前只保存逐kind消息序号水位，ACK字段有局部合法性约束；不会据此宣称server_tick或accepted/processed跨包单调。该策略属于后续操作5–6。
- **确认限制：原样字段只作partial诊断观察。** process-local entity/use没有class/ordinal/coverage证明，不能用作跨进程相等、分歧定位或完整世界证据；文档和类型注释均保留此限制。

Luna最终只读复核确认中性ACK修正，未发现其他阻止本轮验收的具体缺陷。上述新增约束经过源码复读，没有为了提前确认而运行程序。分析结束后执行的唯一统一验收为305/305 PASS，见[实际结果](tx-01-99-results.md)；该结果不改变静态审阅本身的证据性质。

## 未覆盖与后续

本轮没有游戏/Hook/真实网络接入、生产主线程运行验证、密码认证、nonce随机生成、跨进程对象身份、完整world snapshot或世界值比较。核心按单owner调用，无并发API保证。完整packet corpus/fuzz、输入冗余和FaultInjectionTransport属于操作5–9，不能据本轮边界场景宣称这些任务已完成。
