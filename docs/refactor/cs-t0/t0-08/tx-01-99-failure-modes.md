# tx-01-99：T0-08 前四项，实施前失败方式

日期：2026-10-20（Asia/Shanghai）。范围：独立实验协议标识、公共 envelope、四类消息和双端会话建立。本文先于本轮生产实现记录。用户随后确认无哈希、保留包 CRC，仅独立协议核心和一次内存双端验收；不接入真实网络/游戏/Hook。

遵守[现行规范](../../project-conventions.md)：分析期间零测试，不计算或校验哈希，不使用 gate。全部实现与审阅完成后，只执行一次统一验收，保存原始包、双端状态、源码、构建记录、实际结果和复现方式；失败不自动重跑。

## 既有生产约束

- `AuthoritySession` 的配置/绑定阶段与 gameplay Active 分开；协议握手完成不能伪造 Player 绑定或原生模拟就绪。
- 输入必须使用现有 `coop_active::InputStateDraftV2`（24 字节）及 `InputCmdDraftV2`（64 字节），不能退回旧 V1，也不能借 reserved 携带协议字段。
- session 内 A=ordinal0、B=ordinal1，输入来源对应 Participant 由可信本地元数据选择；包里的 client id 不具有重新路由权限。
- network owner 只能解析和有界入队；Participant/AuthorityTickPipeline 的变更属于主线程。新协议核心不操作游戏对象或 Lua。
- 不进入原版 Online/Isaac Sync；独立 magic/version/session/状态，protocol 5、channel0/0xFD 不复用。

## 必须在编码前考虑的失败方式

1. magic/version 与 ISCS 或旧 IPC 混淆，错误协议进入 C/S decoder。
2. 最大包、header/payload 长度未冻结；整数溢出、截断、扩展、尾随字节、空指针或非精确长度越界读取。
3. 原生结构 padding、host endian、enum 大小直接成为 wire ABI；编码失败留下半成品输出。
4. kind、role、flags、reserved、schema、capability 的未知值被静默接受。
5. 仅检查 CRC 而遗漏语义；或包损坏后在验证完成前修改 session/输入状态。
6. session/client/message sequence 为零或最大值回绕；错误 session/sender、同 id 双角色、伪造 ordinal 改写冻结身份。
7. HELLO 未携带双端 nonce 关联；旧 HELLO/ACK 在新 session 重放仍可激活。
8. build/schema/domain/capability 只在一端检查；另一端已 Active 后才发现不兼容。
9. nonce 被当作密码认证；同端 nonce、缺少回显或旧回显使握手串线。
10. 首 ACK 未明确确认对方 HELLO 和 nonce；ACK 在 HELLO 前、来自 client 或普通 ACK 冒充建立会话。
11. 收到 HELLO 就直接激活，绕过首 ACK；仅构造但未交付 ACK 却把双方都写成 Active。
12. 重复包、乱序包或失败包推进接收水位/发送序号；不同消息种类错误依赖同一 lane 的顺序。
13. 终态 Failed/Retired 被新 HELLO 或输入重新打开；重复初始化重置 nonce/session/sequence。
14. 发送失败/容量不足仍推进状态；序号耗尽回绕复用；失败后自动重试创建新 session。
15. INPUT 的 client id 与 envelope sender 不一致，canonical 非法或错误 V1 被接受。
16. INPUT 最后 ACK 指向另一个 session/发送者，或把网络消息序号当作 input sequence/processed 进度。
17. ACK 混淆 accepted 高水位、实际 processed 来源与连续前缀，声称未处理输入已处理；错误单调性隐藏乱序 target。
18. ACK 的 session state 与握手阶段不一致；rejection summary 缺少边界、溢出或未知编码。
19. 状态消息将 partial、odd-only、read failure、未知来源或不同 phase/tick 声称为完整可比世界。
20. 状态载荷偷带 Player/device/平台指针，或为了小包截断原始值却没有显式完整性/缺口。
21. 网络线程调用生产队列、触碰 Player 或向游戏写世界；握手把真实 Native session 直接 Active。
22. 本地 A 和远端 B 分用两套输入 validator/codec；新协议核心绕过共同生产准入。
23. 验收只测替身协议、以单端 round-trip 冒充双端生命周期，或者把离线结果说成实机/网络完成。
24. 测试执行器自动重跑、覆盖旧 artifact、计算哈希、调用历史 gate，或漏存失败时的原始双端事件。

操作5–9的输入冗余/seq 交付策略、完整故障 corpus 和 FaultInjectionTransport 属于后续范围；本轮仍须对实现本身的边界与握手失败行为作一次统一场景验收。
