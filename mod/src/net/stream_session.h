#pragma once
#include "replica/stream_config.h"
#include "replica/stream_transport.h"
#include <functional>
#include "replica/stream_metrics.h"

namespace cs::net::live {
namespace stream = replica::stream;
// 只记录实际物理包；共同头按条目摊销，统计缺失不影响协议处理。
void ObservePacket(stream::Metrics &,std::span<const std::uint8_t>,const char *direction="client-receive") noexcept;
// 总桶与每Peer桶只有socket线程访问；P2也保留单Peer的375000 byte/s额度。
class Uplink final {
public:
  Uplink() noexcept {Configure(stream::RuntimeConfig{});}
  using Send = stream::ReliableSender::Send;
  void Configure(const stream::RuntimeConfig &) noexcept;
  stream::SendResult Transmit(unsigned peer, std::span<const std::uint8_t>, bool initializing,
                              std::uint64_t now, const Send &);
  struct Statistics {
    std::uint64_t packets = 0, payload_bytes = 0, wire_bytes = 0, blocked = 0, bundle_messages = 0;
    std::array<std::uint64_t, 4> lanes{},lane_packets{};
  };
  const Statistics &Status(unsigned peer) const { return stats_.at(peer); }

private:
  struct Bucket {
    std::uint64_t rate = 0, burst = 0, tokens = 0, at = 0;
    bool started = false;
    void Fill(std::uint64_t);
  };
  stream::RuntimeConfig config_;
  Bucket total_{};
  struct Peer {
    Bucket total{};
    std::array<Bucket, 4> lanes{};
  };
  std::array<Peer, 4> peers_{};
  std::array<Statistics, 4> stats_{};
};

enum class OperationStage : unsigned {
  Queued = 0,
  Accepted = 1,
  Executing,
  Complete,
  Cancelled,
  Failed,
  Receipt
};
struct OperationKey final {
  stream::Kind kind = stream::Kind::Error;
  std::uint64_t sequence = 0;
  auto operator<=>(const OperationKey &) const = default;
};
// 接纳不等于执行完成；完成原值保留在有界历史，丢确认只重发结果。
class Operations final {
public:
  struct Statistics {
    std::uint64_t retransmissions = 0, cancellations = 0, completed = 0, failed = 0;
  };
  const Statistics &Status() const noexcept { return statistics_; }
  void Configure(const stream::RuntimeConfig &c) noexcept {config_=c;rtt_.Configure(c);}
  void Bind(std::uint64_t connection, std::uint64_t route);
  void Queue(std::shared_ptr<const stream::Message>, std::uint64_t now);
  bool Admit(std::shared_ptr<const stream::Message>, std::uint64_t now);
  void Finish(OperationKey, OperationStage, const char *reason, std::uint64_t now);
  void Acknowledge(const stream::Message &, std::uint64_t now);
  void Pump(std::uint64_t now, const std::function<bool(const stream::Message &)> &);
  bool Pending() const noexcept;
  bool Settled() const noexcept;
  void CancelPending(std::uint64_t now);
  const std::string &Failure() const noexcept { return failure_; }
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  void ObserveRtt(std::uint64_t ms) noexcept { rtt_.Observe(ms); }
  static bool Reliable(stream::Kind) noexcept;

private:
  struct Record {
    memory::Lease memory;
    std::shared_ptr<const stream::Message> message;
    OperationStage stage = OperationStage::Accepted;
    std::string reason = "accepted";
    std::uint64_t started = 0, progress = 0, progress_sequence = 1, due = 0, sent = 0, retries = 0;
    bool receipt = false;
  };
  std::map<OperationKey, Record> sent_, received_;
  std::uint64_t connection_ = 0, route_ = 0, result_sequence_ = 0;
  std::map<stream::Kind, std::uint64_t> sent_floor_, received_floor_;
  stream::RttEstimator rtt_;
  std::string failure_;
  Statistics statistics_;
  stream::RuntimeConfig config_;
  void Trim(std::map<OperationKey, Record> &, std::map<stream::Kind, std::uint64_t> &floor);
};

// 编码缓存按范围原对象共享；有界LRU与三池租约限制过期值树/压缩内容。
class EncodingCache final {
public:
  EncodingCache();
  ~EncodingCache();
  EncodingCache(const EncodingCache &) = delete;
  EncodingCache &operator=(const EncodingCache &) = delete;
  void Stop() noexcept;
  stream::Metrics Statistics() const noexcept;
  // 只复制封存二进制字节／数字封套；编码与zstd沿用同一后台worker，不在socket线程运行。
  stream::SharedBlob Get(const stream::Message &,bool compressed,replica::Limits);

private:
  struct State;
  std::shared_ptr<State> state_;
};

} // namespace cs::net::live
