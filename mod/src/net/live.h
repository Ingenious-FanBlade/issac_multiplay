#pragma once
#include "net/worker.h"
#include "net/stream_session.h"
#include "state/host_publish.h"
#include "state/client_receive.h"
#include <memory>

namespace cs::net::live {
replica::Limits Limits() noexcept;
#ifdef ISAACCS_E2E
// 容量负向只借用真实DLL的纯值能力声明；不构造Client、不发送Ready，也不获得原生世界。
replica::Json E2EHandshakeCapabilities();
#endif
std::uint64_t Random();
bool IsPacket(std::span<const std::uint8_t>) noexcept;
using Packets = std::vector<std::vector<std::uint8_t>>;
// 通知只包含自有纯值；world持有独立allocator/范围租约，不含游戏或SDK借用。
struct Event final {
  enum class Type {
    Welcome,
    Prepared,
    Progress,
    TickProgress,
    InputAck,
    Control,
    Stopped,
    Failed
  } type = Type::Failed;
  replica::Scope scope{};
  std::uint64_t route = 1;
  unsigned ordinal = 0;
  protocol::Ack progress{};
  std::uint64_t received_ms = 0, round_trip_ms = 0, publication_age_ms = 0;
  // 单调钟量化误差独立于测得RTT；输入投影与年龄见证必须计入该上界。
  std::uint64_t timing_uncertainty_ms = 0;
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
  // 采集会话/epoch/run/room是主线程核对的连续性键，不含原生地址。
  // 同一键下暂缺新候选不等于撤销已交付网络的历史目标；全零表示明确退休。
  std::array<std::uint64_t,4> source{};
  std::shared_ptr<const state::HostStatePublication> binary;
};
class HostPeer final {
public:
  HostPeer(std::uint64_t session, unsigned ordinal, stream::RuntimeConfig config={}, bool lobby=false);
  ~HostPeer();
  bool Receive(std::span<const std::uint8_t>, std::uint64_t, ReceivedInput &, const HostIdentity &);
  Packets Reply();
  Packets Progress(const protocol::Ack &);
  Packets Bye(const char *);
  Packets Reject(const char *);
  bool Offer(const Publication &, const state::HostStatePublication::View &, std::uint64_t now);
  unsigned Pump(std::uint64_t now, EncodingCache &, const stream::ReliableSender::Send &);
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  const stream::Metrics &Metrics() const noexcept {return metrics_;}
  const stream::TransferStatistics &TransferStatus() const noexcept;
  replica::Limits EffectiveLimits() const noexcept {return route_.world_limits;}
  replica::Limits EffectiveFastLimits() const noexcept {return route_.fast_limits;}
  bool Stale() const noexcept;
  bool Initializing() const noexcept;
  bool OwnsPacket(std::span<const std::uint8_t>) const noexcept;
  bool NetworkSettled() const noexcept { return operations_.Settled(); }
  unsigned PublicationPhase() const noexcept;
  struct BinaryProgress {
    std::uint32_t world=0,body=0;
    std::uint64_t events=0,dropped_capacity=0,dropped_expired=0,unmatched_acks=0;
    // 累计真实完整发送次数和目录对就绪耗时，跨同一 Peer 的场景路由累计。
    std::uint64_t joint_sent=0,world_sent=0,body_sent=0;
    std::uint64_t body_dependency_blocks=0;
    state::HostWorldProgress world_window;
  };
  BinaryProgress StateProgress() const noexcept;
  const Operations::Statistics &OperationStatistics() const noexcept {
    return operations_.Status();
  }
  const std::string &FirstFailure() const noexcept {return operations_.Failure();}
  bool Established() const noexcept { return established_; }
  bool Stopped() const noexcept { return stopped_; }
  std::uint64_t LastReceive() const noexcept { return last_receive_; }
  std::uint64_t Nonce() const noexcept { return nonce_; }
  unsigned LobbySelection() const noexcept { return lobby_selection_; }

private:
  // 保留现有状态输出的数字顺序。
  enum class PublicationPhaseValue { WaitingWorld, Preparing, Baseline, WaitingApplied, Active, Recovering, Stopped };
  stream::Metrics metrics_;
  struct Binary;std::unique_ptr<Binary> binary_;
  stream::Reassembler assembler_{Limits()};
  stream::Route route_;
  replica::Scope scope_{};
  Operations operations_;
  std::array<std::uint64_t,4> source_{};
  unsigned ordinal_;
  stream::RuntimeConfig config_;
  bool lobby_ = false;
  unsigned lobby_selection_ = 0;
  bool established_ = false, stopped_ = false, reply_ = false, ending_ = false;
  std::uint64_t nonce_ = 0, server_nonce_ = 0, transfer_ = 0, control_ = 0, roster_ = 0,
                timing_ = 0, last_receive_ = 0, client_control_ = 0, roster_at_ = 0, timing_at_ = 0,
                echo_client_ms_ = 0, echo_server_ms_ = 0, prepare_sequence_ = 0,prepare_generation_=0;
  protocol::Ack sent_{};
  std::uint64_t eligibility_revision_ = 0;
  std::shared_ptr<const stream::Message> welcome_;
  // 旧路由只保留最后一次小准备确认，丢确认不会重新执行或接纳旧世界分片。
  stream::Reassembler prepare_replay_{Limits()};
  std::shared_ptr<const stream::Message> prepared_echo_;
  bool echo_pending_=false;
  stream::Message Message(stream::Kind, std::uint64_t);
  Packets Pack(const stream::Message &);
};
// 客机连接质量（只作显示）：网络线程发布，主线程按值读取，不参与输入目标或同步判定。
// rtt_ms 为 Roster／TickProgress 往返估计（已扣 Host 持有时间）的 1/8 指数平滑值；
// loss_permille 为最近 loss_window 个 Host→Client TickProgress 序号中未到达的千分比。
struct LinkQuality final {
  bool rtt_valid = false, loss_valid = false;
  std::uint32_t rtt_ms = 0, loss_permille = 0, loss_window = 0;
};
// 每连接独立线程；输入、可靠控制、世界依赖与可合并动态各有容量，终态不可丢。
class Client final {
public:
  Client() noexcept = default;
  ~Client();
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  const char *Start(const std::string &, std::uint16_t, std::uint32_t, stream::RuntimeConfig config={},
      std::shared_ptr<const state::ClientStatePreparation> binary={},
      std::shared_ptr<state::ClientStateMemory> binary_memory={}, bool lobby=false) noexcept;
  bool SetLobbySelection(unsigned selection) noexcept;
  std::uint64_t LobbyRoster() const noexcept;
  // 房主解锁位图，与名单同一消息到达；尚未收到时为 0。
  std::uint64_t LobbyUnlocked() const noexcept;
  bool Send(const Intent &) noexcept;
  void RevokeInput() noexcept;
  // 停止旧世界接纳并撤输入；恢复请求须等主线程真实安全退休后另外排入。
  stream::Metrics Metrics() const noexcept;
  stream::Metrics TransmitMetrics() const noexcept;
  stream::TransferStatistics TransferStatus() const noexcept;
  // 任意线程可读；尚无样本时对应 valid 为 false。
  LinkQuality Quality() const noexcept;
  replica::Limits EffectiveLimits() const noexcept;
  replica::Limits EffectiveFastLimits() const noexcept;
  bool Poll(Event &) noexcept;
  bool PollWorld(Event &) noexcept;
  // 二进制邮箱与旧Range完全分开；主线程只消费不可变批次／资源工作并回报原票据。
  // 按应用通道取票，0 为 World／joint／恢复，1 为普通 Body；总容量仍为两票。
  bool PollBinary(unsigned channel,state::ClientStateApplyBatchRef &) noexcept;
  bool PollBinaryResource(state::ClientStateResourceJob &) noexcept;
  bool CompleteBinary(const state::ClientStateApplyCompletion &) noexcept;
  bool CompleteBinaryResource(const state::ClientResourcePreparationTicket &,state::ClientResourceState) noexcept;
  bool RecoverBinary(const state::StateRoute &,state::StateResyncReason) noexcept;
  // 主线程回报本地应用和退休结果；后台在保留可解码快照后独立发送接收 ACK。
  bool Prepare(const Event &) noexcept;
  bool Complete(const Event &, OperationStage, const char *reason) noexcept;
  void Stop() noexcept;
  bool Finished() const noexcept;

private:
  struct State;
  std::shared_ptr<State> state_;
  void *thread_ = nullptr;
};
} // namespace cs::net::live
