#pragma once

#include "net/protocol.h"
#include "replica/stream_config.h"
#include "replica/stream_metrics.h"
#include "replica/stream_transport.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include "core/replication_memory.h"

namespace cs::net {
namespace live {struct Publication;}

inline constexpr std::size_t kReceiveCapacity = 256, kSendCapacity = 256;
// 配置只在Start前设置；Authority同时接受ICSR真实游戏Client与既有输入工具。
struct WorkerConfig {
  std::string bind_address = "0.0.0.0";
  std::uint16_t port = 27090;
  std::uint32_t peer_timeout_ms = std::uint32_t(replica::stream::RuntimeConfig{}.peer_timeout_ms),
                heartbeat_ms = std::uint32_t(replica::stream::RuntimeConfig{}.heartbeat_ms);
  replica::stream::RuntimeConfig stream;
  protocol::Text build = protocol::TextId("isaac.cs.tx-04-09");
};
// 系统随机数生成的会话身份；主线程据此构造 Session，不能用对象地址替代。
struct HostIdentity {
  protocol::Id session{}, nonce{};
  std::array<std::uint64_t, core::kMaximumParticipants> clients = protocol::kClients;
};
enum class StartError : std::uint32_t {
  None,
  AlreadyStarted,
  Configuration,
  Allocation,
  Random,
  Winsock,
  Socket,
  SocketOption,
  Bind,
  Thread
};
const char *StartErrorName(StartError error) noexcept;
struct StartResult {
  StartError error = StartError::None;
  std::uint32_t native_error = 0;
  HostIdentity identity{};
  explicit operator bool() const noexcept { return error == StartError::None; }
};
enum class StopResult : std::uint32_t { Stopped, TimedOut, WaitFailed };
enum class TerminalReason : std::uint32_t {
  None,
  Stop,
  PeerBye,
  PeerTimeout,
  PeerProtocol,
  RunTeardown,
  InternalFailure,
  // tx-04-07：主线程因远端 Player 被确认移除/归属矛盾而主动结束（U10）。
  PeerLost
};
const char *TerminalReasonName(TerminalReason reason) noexcept;

// 只含网络值的输入信封；主线程取出后仍须按当前绑定代次经统一 Submit 入队。
struct ReceivedInput {
  protocol::Input input{};
  std::uint64_t packet_sequence = 0, received_ms = 0;
  // 由成功发送 Ack 的本地记录查得，不从 Input payload 解码或访问游戏对象。
  std::uint64_t acknowledged_generation = 0;
  core::SessionState acknowledged_state = core::SessionState::Warmup;
  // 区分“未命中 Ack 记录”和“命中的合法 Warmup generation=0”；两者都不读游戏对象。
  bool ack_stamp_known = false;
};
// 原子字段的独立快照，计数可能相差一个工作线程迭代，不承诺整结构同一瞬间。
// terminal 是粘性状态，不经过可丢弃队列；停止后也必须让主线程读到真实原因。
struct WorkerStatus {
  // 共享socket在选定Peer之前的拒绝计数，不能误用某个Peer的foreign计数代替。
  std::uint64_t routing_rejected_packets=0;
  std::array<memory::Usage,3> replication_memory{};
  // 线上字节包含IPv4/UDP开销；lane之和严格等于此值，重传同样计费。
  std::uint64_t uplink_wire_bytes=0,uplink_budget_blocks=0;
  std::array<std::uint64_t,4> uplink_lane_bytes{},uplink_lane_packets{};
  unsigned publication_phase=0;
  std::uint64_t binary_world_ack=0,binary_body_ack=0,binary_event_ack=0,binary_event_dropped_capacity=0,binary_event_dropped_expired=0,binary_unmatched_acks=0;
  bool stream_stale=false;
  replica::stream::RuntimeConfig stream_config;
  replica::stream::Metrics stream_metrics,encoder_metrics;
  replica::Limits stream_limits,stream_fast_limits;
  replica::stream::TransferStatistics stream_transfer;
  std::uint64_t uplink_packets=0,uplink_payload_bytes=0,uplink_bundle_messages=0;
  std::uint64_t operation_retransmissions=0,operation_cancellations=0,operation_completed=0,operation_failed=0;
  std::array<char,128> replication_first_failure{};
  bool running = false, peer_locked = false;
  protocol::PeerState state = protocol::PeerState::Dormant;
  TerminalReason terminal = TerminalReason::None;
  protocol::Error protocol_error = protocol::Error::None;
  protocol::ByeReason peer_bye = protocol::ByeReason::Stop;
  std::uint32_t native_error = 0, last_send_error = 0;
  std::array<std::uint8_t, 4> peer_address{};
  std::uint16_t peer_port = 0;
  std::uint64_t received_packets = 0, sent_packets = 0, received_bytes = 0,
                sent_bytes = 0, valid_packets = 0, duplicate_packets = 0,
                stale_packets = 0, bye_send_attempts = 0, bye_sent_packets = 0,
                foreign_packets = 0, receive_errors = 0, send_errors = 0,
                receive_queue_dropped = 0, send_queue_dropped = 0,
                last_receive_ms = 0, rtt_samples = 0, rtt_last_ms = 0,
                rtt_max_ms = 0, wait_timeouts = 0;
  // 网络线程发布终态的真实时刻，用于超时 E2E；不使用主线程稍后的观察时刻。
  std::uint64_t terminal_at_ms = 0;
  // 终止后短暂排空实际收到的报文数；已包含在 received_packets，不作有效输入。
  std::uint64_t terminal_received_packets = 0;
  std::array<std::uint64_t, static_cast<std::size_t>(protocol::Error::Count)>
      errors{};
};

// 主线程持有的单会话句柄；不包含游戏、REPENTOGON、Lua 或 runtime.h。
// Start/QueueAck/TryReceive/RequestStop/Stop 仅由同一主线程使用。
// 析构只发停止信号，不等待；显式 Stop 必须在 DllMain 外调用。
// 超时线程持有自己的状态强引用，退出前不访问已析构的 Worker 或游戏对象。
class Worker final {
public:
  Worker() noexcept = default;
  ~Worker();
  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &) = delete;
  StartResult Start(const WorkerConfig &config) noexcept;
  bool QueueAck(const protocol::Ack &ack) noexcept;
  bool PublishWorld(std::shared_ptr<const live::Publication>) noexcept;
  bool TryReceive(ReceivedInput &input, unsigned ordinal = 1) noexcept;
  // 单 Peer 退出只关闭自己的 Endpoint，监听线程和其他远端继续运行。
  void RequestPeerStop(unsigned ordinal, protocol::ByeReason reason) noexcept;
  void
  RequestStop(protocol::ByeReason reason = protocol::ByeReason::Stop) noexcept;
  StopResult Stop(protocol::ByeReason reason = protocol::ByeReason::Stop,
                  std::uint32_t wait_ms = 1000) noexcept;
  WorkerStatus Status(unsigned ordinal = 1) const noexcept;
  bool TransportFailed() const noexcept;

private:
  struct State;
  std::shared_ptr<State> state_;
  void *thread_ = nullptr;
  bool started_ = false;
};
} // namespace cs::net
