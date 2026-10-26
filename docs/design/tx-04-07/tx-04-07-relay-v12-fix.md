# tx-04-07 中继收尾与 V12 采证修复

> 修复后 e2e-08 已执行：第二轮 Bye 收尾及前三轮严格对账通过，terminal_rx=2；V12 已有正对照和提前发射证据，仍 FAIL，Lua 回调的 loop 还存在帧尾快照偏移。S20 因 A 死亡、should_save=false 而不可 Continue，整体仍 FAIL。见[实际结果](../../artifacts/repentogon/tx-04-07-e2e-08/README.md)；原报告不重归约，未自动重跑。

> 后续测试安排：用户确认后执行了 e2e-07 单入口，但因遗漏 Download Helper 临时禁用步骤在启动前中止；未部署或启动游戏，修复仍未实机验证。原报告另有缺少 crash_before 时把历史 dump 误报为本轮崩溃的问题，见[记录](../../artifacts/repentogon/tx-04-07-e2e-07/README.md)。没有自动重跑或本轮测试后功能修改。

日期：2026-10-26，Asia/Shanghai。用户要求先修复中继收尾错误，再核对和修复 V12 证据不足。本轮修改与统一纯构建已完成；没有部署、启动游戏或运行测试，没有改写 e2e-06 的原报告。后续验证仍使用整个 tx-04-07 的唯一整体入口，不拆分试跑，也不自动重跑。

## 中继收尾

e2e-06 第 2 轮中，远端主动发送三份相同 Bye。主机以 PeerBye 正常退休，主机 stop 记录 rx=189、tx=206，远端 tx=191、rx=206；relay 第 0 方向实际接收/成功转发 191，第 1 方向 206，pending/drop/send_error 均为零。随后 relay 的 recv 收到 Winsock 10054 并被一律当成致命错误，导致 finish 失败，S17–S21 未执行。原始记录见 [e2e-06](../../artifacts/repentogon/tx-04-07-e2e-06/README.md)。

UDP 的 10054/10052 是异步连接/网络重置通知，不能当成协议 Bye。现有 native Worker 已按非致命通知处理它们；Rust relay 现在采用相同分类，写出 `udp_notice`（方向、原生错误、累计次数），summary 增加 `udp_notices`。其他接收错误仍失败；每方向每轮收到通知即结束该轮接收，避免忙等。不把通知计为收到的报文，不关闭协议或伪造 Bye。

主机收到首份 Bye 后原先只处理同一 ReceiveBatch 已可读的副本，随即关闭 socket。对于随后到达的副本，这会造成真实的接收计数缺口，并可能触发 ICMP 通知。native Worker 现在先完成逻辑退休，再保留 100 ms 的有界传输收包窗口。它只累计真实 datagram/字节和来源，另记 `terminal_received_packets`，stop trace 输出 `terminal_rx`；不解析或提交输入、不刷新存活时间、不发布 Ack、不回复 Bye、不访问游戏对象。每轮预算和短暂休眠保持有界，Stop 原有 1000 ms 上限不变；协议/内部失败不进入该排空路径。

V4 的严格对账不放宽：源端 TX 仍须等于 relay 实际 RX，relay 成功发送仍须等于目的端实际 RX。100 ms 是本地收尾策略，不能保证任意网络时延下副本全部到达；超出窗口的真实缺口仍失败，不能凭成功 send 推定接收。

代码：[relay](../../tools/harness/src/relay/mod.rs)、[Worker](../../mod/src/net/worker.cpp)、[状态](../../mod/src/net/worker.h)、[stop trace](../../mod/src/game/peer.cpp)。

## V12 的旧证据说明了什么

V12 是第 2 轮角色 7（阿萨泻勒）的跨房射击观察，不是版本号。检查从切房到真实松键前没有归属 B 的新激光，松键后有限窗口有归属 B 的新激光，且非 tick / 阻断窗口的 Shooting original 为零。

e2e-06 有以下直接记录：

- B 的 player_type=7、player_index=1、实体 Index=46，动画结束字段存在，前轮夹具的玩家身份问题没有重现。
- Lua 有 4 条 laser-baseline、0 条 laser-spawn。baseline 不能当成新激光出生；没有证据证明实际发射。
- S15.begin 是 tick 12，而脚本 base=14；实际连续 held 从旧代次 tick 17 开始。旧归约使用 begin+3 的硬编码偏移，将脚本生效前 tick 16 的中性误纳入按住段。
- 重绑新代次的 tick 81 确实是 input_sequence=0、held=0、aim=0，Shooting 未暂停。tick 82 起才消费新的 held 命令。这是真实输入空档，不能当作误报去掉。
- `charge_ticks=60`；实际推进率约 60 tick/s，两段各约一秒。实现方案第 7 节要求跨房到达后继续按住至少两秒，原编排不满足时长要求。

时长偏短与真实空档是已确认问题；它们对“完全没有 laser-spawn”的各自影响尚未被隔离，不能静态断言唯一原因。原场景没有同角色普通蓄力/松键的正对照，因此也不能由零事件区分武器是否发射、观测链是否工作与跨房行为是否出错。

## 采证与归约修改

1. 同一次 S15 先执行同角色的普通蓄力/松键对照，再执行原跨房场景。所有输入继续经远端脚本、canonical、Submit/Resolve 和原生 Shooting；不直接造激光，不读写蓄力字段。对照激光只作诊断，不代替跨房后的激光。
2. `charge_ticks` 默认改为 180；配置范围 120–600。三个按住段的脚本等待增加 4 tick 的安装/lead 余量。归约从实际消费到的首条匹配 held 开始，要求至少配置数量的连续有效样本，并用 DLL 微秒时钟证明至少两秒；配置值或 Ack 推进本身不能证明已经蓄满。
3. 跨房连续性从 S15.charged 开始；实际松键以同代次、匹配 Accepted 命令的首次中性 tick 为界，覆盖脚本安装与消费之间的时间。脚本安装前的中性不再误报，重绑后真实缺输入则逐条记录为 `continuity_gaps`。
4. 新增 `laser-evidence.json`：包含前后激光、未知归属、实际松键 tick、三段时长证据、对照激光、连续性空档、阻断分派和下一代次源记录。报告 V12 引用相同 detail。已观察到提前激光、真实连续性空档或阻断窗口错误调用时判 FAIL，不能再被“缺激光”掩盖成 NOT_OBSERVED。缺少必要世界证据仍不能 PASS。

现行规范第 9 节与本任务实现方案第 7 节明确要求：有效 tick 缺输入仍消费中性，不保持旧攻击。故本轮**没有**增加重绑首 tick 的射击暂停例外、等待远端输入、复用旧 held 或放宽连续性判定。tick 81 的空档在既定协议时序下可能再次出现，届时 V12 应明确失败；这项行为不能靠修改采证隐藏。此次修复的是场景时长、诊断对照、证据边界与错误分类，尚不能宣称跨房射击已经通过。

代码：[S15 编排](../../tools/harness/src/tx07.rs)、[V12 归约](../../tools/harness/src/tx07/evidence.rs)、[场景配置](../../tools/harness/scenarios/tx-04-07.json)。

## 构建与后续验收

统一纯构建 `scripts/build.ps1` 已成功：Win32 DLL 与 i686 Rust harness 编译链接完成，Rust 无 warning；C++ 有既有 SPSC queue 对齐填充 C4324 提示。第一次构建被沙箱对本机 SDK 目录的读取限制挡住；取得构建访问权限后完成。日志及相对 e2e-06 源码快照的差异见 [修复产物](../../artifacts/repentogon/tx-04-07-relay-v12-fix/README.md)。未运行单元测试、harness、游戏或其他阶段测试。

后续完整 E2E 需要在新产物目录保存四轮报告、严格 network-ledger 与新增 laser-evidence。应检查 S16 能否完成并进入后续轮次、终止排空的真实收包计数、正对照激光，以及 V12 的实际连续性与发射结果。每次游戏启动仍按现行规范第 8 节另行告知配合时间并确认准备；本轮没有启动游戏。
