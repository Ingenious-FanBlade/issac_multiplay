#pragma once
#include "replica/stream_capture.h"
#include "replica/stream_transport.h"
#include <functional>

namespace cs::net::live {
namespace stream = replica::stream;
// 总桶与每Peer桶只有socket线程访问；P2也保留单Peer的312500 byte/s额度。
class Uplink final {
public:
  using Send = stream::ReliableSender::Send;
  stream::SendResult Transmit(unsigned peer, std::span<const std::uint8_t>, bool initializing,
                              std::uint64_t now, const Send &);
  struct Statistics {
    std::uint64_t packets = 0, wire_bytes = 0, blocked = 0;
    std::array<std::uint64_t, 4> lanes{};
  };
  const Statistics &Status(unsigned peer) const { return stats_.at(peer); }

private:
  struct Bucket {
    std::uint64_t rate = 0, burst = 0, tokens = 0, at = 0;
    bool started = false;
    void Fill(std::uint64_t);
  };
  Bucket total_{937500, 24576};
  struct Peer {
    Bucket total{312500, 8192};
    std::array<Bucket, 4> lanes{{{25000, 2048}, {120000, 4096}, {120000, 4096}, {47500, 8192}}};
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
  stream::SharedBlob Get(stream::Range, bool compressed, replica::Limits);

private:
  struct State;
  std::shared_ptr<State> state_;
};

// Host产品发布owner：冷依赖可靠传输，真实Applied后才追最新动态；不等待其他Peer。
class PublicationSender final {
public:
  enum class Phase {
    WaitingWorld,
    Preparing,
    Baseline,
    WaitingApplied,
    Active,
    Recovering,
    Stopped
  };
  void Offer(std::shared_ptr<const stream::Publication> baseline,
             std::shared_ptr<const stream::Publication> latest,
             std::vector<std::shared_ptr<const stream::Publication>> bodies);
  void Preparing() noexcept { phase_ = Phase::Preparing; }
  void Authorize(stream::Route, std::uint64_t now);
  bool Applied(const stream::Message &);
  bool ContextReady(const stream::Message &, std::uint64_t now);
  bool NativeProgress(const stream::Message &, std::uint64_t now);
  void FailPublication(const char *reason);
  void Stop() noexcept;
  unsigned Pump(std::uint64_t now, std::uint64_t &transfer, EncodingCache &,
                const stream::ReliableSender::Send &);
  bool Acknowledge(const stream::Message &, std::uint64_t now);
  void Cancel(std::uint64_t id) {
    reliable_.Cancel(id);
    FailPublication("remote_cancelled_transfer");
  }
  Phase State() const noexcept { return phase_; }
  bool Initializing() const noexcept { return phase_ != Phase::Active; }
  const std::shared_ptr<const stream::Publication> &Baseline() const noexcept { return baseline_; }
  std::uint64_t NextWake(std::uint64_t now) const noexcept { return reliable_.NextWake(now); }
  const std::string &Failure() const noexcept { return failure_; }

private:
  Phase phase_ = Phase::WaitingWorld;
  stream::Route route_;
  stream::ReliableSender reliable_;
  std::shared_ptr<const stream::Publication> baseline_, latest_;
  std::vector<std::shared_ptr<const stream::Publication>> bodies_;
  std::vector<stream::Range> cold_;
  std::size_t cold_cursor_ = 0;
  std::map<stream::RangeId, std::uint64_t> queued_, applied_;
  std::map<stream::Version, std::uint64_t> transfers_;
  std::map<stream::Version, std::uint64_t> pending_context_;
  std::map<stream::Version, std::uint64_t> context_since_;
  std::uint64_t started_ = 0, progress_ = 0, native_progress_ = 0;
  std::map<stream::RangeId, std::uint64_t> fully_sent_;
  std::map<stream::RangeId, std::uint64_t> sent_at_;
  memory::Lease metadata_;
  std::map<stream::RangeId, std::deque<std::uint64_t>> sent_history_;
  void Remember(const stream::Version &);
  bool WasSent(const stream::Version &) const noexcept;
  // 已发送事实只保留值元数据；远端确认不能引用当前最新Publication冒认旧版本。
  struct SentFact {
    std::vector<stream::Version> dependencies;
    std::uint64_t native=0,retained=0,watermark=0;
  };
  std::map<stream::Version,SentFact> sent_facts_;
  struct NativeFact {std::uint64_t attempt=0,instance=0,resource=0;auto operator<=>(const NativeFact &) const = default;};
  std::map<stream::Version,NativeFact> confirmed_;
  void Track(const stream::Range &);
  struct Fast {
    stream::SharedBlob blob;
    unsigned part = 0;
    std::uint64_t transfer = 0;
  };
  std::map<stream::RangeId, Fast> fast_;
  stream::RangeId fast_cursor_{};
  std::string failure_;
};
} // namespace cs::net::live
