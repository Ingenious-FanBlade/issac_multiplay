#pragma once
#include "net/worker.h"
#include "net/stream_session.h"
#include <memory>

namespace cs::net::live {
replica::Limits Limits() noexcept;
std::uint64_t Random();
bool IsPacket(std::span<const std::uint8_t>) noexcept;
using Packets = std::vector<std::vector<std::uint8_t>>;
// 通知只包含自有纯值；world持有独立allocator/范围租约，不含游戏或SDK借用。
struct Event final {
  enum class Type {
    Welcome,
    Progress,
    TickProgress,
    InputAck,
    World,
    Control,
    Stopped,
    Failed
  } type = Type::Failed;
  replica::Scope scope{};
  std::uint64_t route = 1;
  unsigned ordinal = 0;
  protocol::Ack progress{};
  std::uint64_t received_ms = 0, round_trip_ms = 0, publication_age_ms = 0;
  std::shared_ptr<const stream::Message> world;
  char reason[128]{};
};
// 生成时冻结路由；worker绝不把旧命令套上当前世界标签。
struct Intent final {
  core::InputStateDraftV2 state{};
  std::uint64_t generation = 0, sequence = 0, target = 0;
  replica::Scope scope{};
  std::uint64_t route = 0;
  std::uint64_t input_epoch = 0;
};
// 正式采集的共享信封；一次采集/编码供全部Peer，版本与身体来源保持各自时间线。
struct Publication final {
  std::shared_ptr<const stream::Publication> baseline, latest;
  std::vector<std::shared_ptr<const stream::Publication>> bodies;
};
class HostPeer final {
public:
  HostPeer(std::uint64_t session, unsigned ordinal);
  bool Receive(std::span<const std::uint8_t>, std::uint64_t, ReceivedInput &, const HostIdentity &);
  Packets Reply();
  Packets Progress(const protocol::Ack &);
  Packets Bye(const char *);
  Packets Reject(const char *);
  void Offer(const Publication &, std::uint64_t now);
  unsigned Pump(std::uint64_t now, EncodingCache &, const stream::ReliableSender::Send &);
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  bool Initializing() const noexcept { return publication_.Initializing(); }
  bool OwnsPacket(std::span<const std::uint8_t>) const noexcept;
  bool NetworkSettled() const noexcept { return operations_.Settled(); }
  unsigned PublicationPhase() const noexcept { return unsigned(publication_.State()); }
  const Operations::Statistics &OperationStatistics() const noexcept {
    return operations_.Status();
  }
  const std::string &FirstFailure() const noexcept {
    return operations_.Failure().empty() ? publication_.Failure() : operations_.Failure();
  }
  bool RequestBaseline() noexcept {
    const auto value = resync_;
    resync_ = false;
    return value;
  }
  bool Established() const noexcept { return established_; }
  bool Stopped() const noexcept { return stopped_; }
  std::uint64_t LastReceive() const noexcept { return last_receive_; }
  std::uint64_t Nonce() const noexcept { return nonce_; }

private:
  stream::Reassembler assembler_{Limits()};
  stream::Route route_;
  replica::Scope scope_{};
  Operations operations_;
  PublicationSender publication_;
  unsigned ordinal_;
  bool established_ = false, stopped_ = false, reply_ = false, resync_ = false, ending_ = false;
  std::uint64_t nonce_ = 0, server_nonce_ = 0, transfer_ = 0, control_ = 0, roster_ = 0,
                timing_ = 0, last_receive_ = 0, client_control_ = 0, roster_at_ = 0, timing_at_ = 0,
                echo_client_ms_ = 0, echo_server_ms_ = 0, prepare_sequence_ = 0;
  protocol::Ack sent_{};
  std::uint64_t eligibility_revision_ = 0;
  std::shared_ptr<const stream::Message> welcome_;
  stream::Message Message(stream::Kind, std::uint64_t);
  Packets Pack(const stream::Message &);
};
// 每连接独立线程；输入、可靠控制、世界依赖与可合并动态各有容量，终态不可丢。
class Client final {
public:
  Client() noexcept = default;
  ~Client();
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  const char *Start(const std::string &, std::uint16_t, std::uint32_t) noexcept;
  bool Send(const Intent &) noexcept;
  void RevokeInput() noexcept;
  bool Poll(Event &) noexcept;
  bool PollWorld(Event &) noexcept;
  // 主线程完成准备/原生操作后才发布结果；接收线程从不自行生成AppliedAck。
  bool SendControl(std::shared_ptr<const stream::Message>) noexcept;
  bool Authorize(stream::Route, std::uint64_t generation) noexcept;
  bool Complete(const Event &, OperationStage, const char *reason) noexcept;
  void Stop() noexcept;
  bool Finished() const noexcept;

private:
  struct State;
  std::shared_ptr<State> state_;
  void *thread_ = nullptr;
};
} // namespace cs::net::live
