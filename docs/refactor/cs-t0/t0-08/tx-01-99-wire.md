# tx-01-99：T0-08 操作1–4，无哈希实验协议

2026-10-20（Asia/Shanghai）。用户确认：无哈希、保留包 CRC；本轮仅独立协议核心和内存双端场景验收。操作5–9、真实网络、Native/Hook 接入、游戏 Active 和世界写回均不在本次范围。遵守[现行规范](../../project-conventions.md)，不计算/比较构建或状态哈希，不设置 gate。

生产代码：`cs-t0/native/include/isaac_cs_t0/minimal_protocol.hpp`、`cs-t0/native/src/minimal_protocol.cpp`。验收驱动和统一执行器先于生产代码写入；[实施前失败方式](tx-01-99-failure-modes.md)先于全部实现登记。

## 独立身份与 envelope

| 项 | 值 |
| --- | --- |
| magic | ASCII `ICT8`，字节 `49 43 54 38` |
| protocol | 1，实验专用，不发布兼容承诺 |
| 最大包 | 1200 字节，含64字节header |
| state observation domain | `isaac.cs.t0.observation.v1`，文本标识 |
| input/state schema | 2 / 1 |
| capabilities | bit0=canonical V2、bit1=raw observation、bit2=packet CRC；当前集合7 |
| transport channel | 不分配，无 socket/Steam transport；不借用0或0xFD |

所有整数为小端。结构逐字段编码，只有现有明确小端且固定24/64字节的 V2 canonical codec 被复用。文本为32字节ASCII，非空、可打印且无空格、NUL终止，尾部全零；build 是显式实验配置标签，不能证明两个二进制实际相同。nonce 各16字节、非零、两端不同，由调用者以可信配置提供新鲜值；本核心不生成随机数、不提供密码认证。

| offset | bytes | 字段 |
| ---: | ---: | --- |
| 0 | 4 | magic |
| 4 | 2 | protocol version |
| 6 | 2 | kind：HELLO=1、INPUT=2、ACK=3、STATE_HASH=4 |
| 8 | 2 | header size=64 |
| 10 | 2 | sender role：Authority=1、Remote=2 |
| 12 | 4 | flags=0 |
| 16 | 16 | session，非零 |
| 32 | 8 | sender id，非零 |
| 40 | 8 | message sequence，1..UINT64_MAX-1，不回绕 |
| 48 | 4 | payload size，精确等于实际剩余长度 |
| 52 | 4 | reserved=0 |
| 56 | 4 | CRC32，小端 |
| 60 | 4 | reserved=0 |

CRC-32/ISO-HDLC：反射多项式0xEDB88320，初值/末异或0xFFFFFFFF；覆盖整个实际包，56..59视为零。先检查总长与精确payload长，再计算CRC；它只检测包损坏，不校验构建、artifact或世界状态。decoder 对错误不改 caller output 或 endpoint；兼容性/握手确认失败在明确归属当前peer后使 endpoint Failed。全部 encode 使用临时包，失败不改输出。

## 四类载荷

HELLO 固定136字节：build[32]、domain[32]、local_nonce[16]、expected_peer_nonce[16]、input_schema:u16、state_schema:u16、protocol:u16、role:u16、capabilities:u64、maximum_packet:u32、reserved:u32、A id:u64、B id:u64。A/B id冻结且互异；envelope sender 必须对应其role。双方精确比较同一配置，并互换 role/local_nonce/peer_nonce；不做隐式能力降级。

INPUT 固定72字节：原有64字节 `InputCmdDraftV2` 加 `last_ack_message_sequence:u64`。command 的session/client必须与envelope相同，canonical V2必须有效；input sequence 与 message sequence 分开。只有Remote可发送，Authority本地A仍由既有 loopback 路径负责，未被本轮修改。last_ack只是接收ACK的网络序号字段，本轮不实现重传/丢失恢复。

ACK 固定216字节：8个u64依次为server_tick、client_id、accepted高水位、processed高水位、processed_tick、实际processed_source、processed generation、rejected_count；随后rejection_mask:u32、session_state:u16（沿用现有枚举）、purpose:u16（First=1/Progress=2）、confirmed HELLO sequence:u64；尾部重复136字节完整Authority HELLO协商身份。processed不大于accepted，processed_source不大于processed，processed_tick不大于server_tick；有processed高水位必须有generation/tick。缺输入中性tick允许processed_source=0且保留旧processed高水位，不能假装又处理了旧输入。首ACK固定Warmup和零进度；Progress的HELLO sequence=0。它不声明accepted/processed为连续前缀，也不建立业务ACK单调性策略。

STATE_HASH 保留历史消息名，内容为**状态观察**，没有hash/digest。前缀96字节：domain[32]；tick/run/room/attempt/capture各u64；schema/phase/status/reason/count各u16；reserved:u16、reserved:u32；source_token:u64。后跟最多24条40字节原始字段：entity observation id:u64、use generation:u64、field:u16、FieldStatus=Value:u8、encoding:u8、width:u16、reserved:u16、raw[16]。按(entity,use,field)严格递增，零尾部；Raw/F32/Derived支持，引用存储不支持。F32只能4字节有限值。最大字段包1120字节，不隐式截断。

state status仅PartialValues=1、Gap=2，不提供Complete或快照资格。PartialValues只允许标准完整更新后的采样phase，带非零原因和至少一条字段；Gap无字段，保留明确原因。phase OddOnly=2必须是reason OddOnly=2的Gap。reason沿用T0-06的1..18语义；schema/domain必须与会话配置一致。entity/use是来源进程内的观察标识，不证明跨端稳定身份，不传Player*/device*/address/vptr；本轮未接入采样器、未建立跨进程字段比较。

## 会话建立与提交点

`Endpoint`单owner、一次初始化；状态Dormant→Negotiating→Established→Retired，失败进入Failed，两终态不复活。调用者冻结session、A/B、build/schema/domain、能力及互为期望的nonce。协议Established不调用 `AuthoritySession::Activate`，真实游戏仍要求主线程Warmup和exact pair绑定。

1. 双方PrepareHello只准备有界自有包，不改序号；交给自有有界传输队列成功后CommitSent，提交本地HELLO。
2. 双方Receive对方HELLO，先验证整个包和归属，再比较所有协商字段。错role/build/schema/domain/nonce/capability在Established前拒绝。
3. Authority仅在本地HELLO已成功发送且对方HELLO已通过后，PrepareFirstAck；首ACK回显Remote HELLO sequence和双方nonce及完整协商字段，ACK envelope序号必须晚于Authority HELLO。
4. Authority将首ACK成功入队后CommitSent进入Established；Remote只有收到并验证匹配首ACK才Established。两端之间存在传输时延，不声称全局原子Active。

每端最多一个pending；发送失败CancelPending不消耗序号。CommitSent精确匹配准备的包，拒绝改写包与二次commit；序号耗尽不回绕。接收水位按kind保留，避免跨lane排序；最近同kind相同字节包为Duplicate，旧/同序号不同字节为Sequence错误。完整冗余、重排/丢失策略和ACK覆盖策略留给操作5–6。Failed/Retired拒绝后续包。

## 唯一整体验收

先完成实现与静态审阅，再纯编译，最后只执行一次统一入口：

```powershell
pwsh -File cs-t0/build-minimal-protocol.ps1
python cs-t0/accept-minimal-protocol.py --build-dir out/cs-t0/tx-01-99-build --artifact-dir artifacts/cs-t0/tx-01-99-protocol-01
```

执行器只启动一次已编译场景程序，保存逐检查JSONL、原始包十六进制与双端状态、stdout/stderr、源码、二进制、工具链及文件长度/时间。覆盖完整握手→INPUT→ACK→局部状态→odd gap→退休，pending取消/失败原子性、兼容性失败、nonce/身份/序号与边界。不是完整操作8的golden/fuzz corpus，也不是真实网络或游戏验收。结果另记。
