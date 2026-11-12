#pragma once
#include "replica/stream_codec.h"
#include "replica/stream_config.h"
#include <array>
#include <functional>
#include <deque>
#include <algorithm>

namespace cs::replica::stream {
Limits HandshakeLimits() noexcept;
Limits WorldLimits(bool fast=false) noexcept;
Limits ConfiguredLimits(const RuntimeConfig &,bool fast=false) noexcept;
bool Reliable(Lane) noexcept;
// endpoint先由socket owner核对；每连接实例只接纳显式授权的route，报文不能自行扩权。
struct Route final {
  std::uint64_t connection=0,epoch=0;
  bool negotiated=false,world=false,zstd=false;
  Direction direction=Direction::ToClient;
  Limits world_limits=WorldLimits();
  WorldScope scope{};
  RuntimeConfig config;
  Limits fast_limits=WorldLimits(true);
  bool world_body_zstd=false;
};
// 仅从严格协商结果取得本地上限；不接受报文Header作为预算来源。
Limits NegotiatedWorldLimits(const Value &);
Limits NegotiatedFastLimits(const Value &);
struct TransferKey final {std::uint64_t connection=0,route=0,transfer=0;auto operator<=>(const TransferKey &) const = default;};
// 共享blob只有逻辑正文；连接/route/transfer在发送单片时写入，不为三个Peer复制大消息。
struct Blob final {
  memory::Lease memory;
  std::vector<std::uint8_t> bytes;
  std::uint32_t decoded_bytes=0;
  Kind kind=Kind::Error;
  Lane lane=Lane::Control;
  Codec codec=Codec::None;
  std::uint64_t sequence=0,encode_us=0,compression_us=0,encode_cpu_us=UINT64_MAX,compression_cpu_us=UINT64_MAX;
  WorldScope world{};
  RangeId range_id{};
  Sample sample;
};
using SharedBlob=std::shared_ptr<const Blob>;
SharedBlob EncodeBlob(const Message &,bool zstd,Limits=WorldLimits());
// 自有完整ICSR字节接口，供二进制Frame接线复用；不解析／构建JSON或访问游戏。
// 每条独立zstd帧，无字典；不更小时返回原文。owner持有输入／输出及其实际capacity租约。
struct WorldBodyCompressionView {
  Codec codec=Codec::None;
  std::uint32_t decoded_bytes=0;
  std::span<const std::uint8_t> bytes;
};
WorldBodyCompressionView CompressWorldBodyInto(std::span<const std::uint8_t> logical,Lane,std::span<std::uint8_t> scratch);
std::span<const std::uint8_t> DecompressWorldBodyInto(Codec,std::span<const std::uint8_t> encoded,
    std::uint32_t expected,Lane,std::span<std::uint8_t> output);
std::vector<std::uint8_t> Datagram(const Blob &,TransferKey,std::uint32_t index,Limits=WorldLimits());
// 小包容器保持逐条kind/lane/sequence和各自完整分片，不跨连接、路由；最大等待2ms。
bool IsBundle(std::span<const std::uint8_t>) noexcept;
std::vector<std::vector<std::uint8_t>> Unbundle(std::span<const std::uint8_t>);
std::vector<std::vector<std::uint8_t>> Bundle(const std::vector<std::vector<std::uint8_t>> &);

struct ReceiveBitmap final {
  std::uint64_t transfer=0;
  std::uint32_t part_count=0,window=0;
  std::array<std::uint64_t,4> bits{};
  bool complete=false;
};
// Karn规则由调用者排除重传样本；本地RTT估计不比较两台机器的绝对时钟。
class RttEstimator final {
public:
  void Configure(const RuntimeConfig &c) noexcept {minimum_=c.rto_min_ms;maximum_=c.rto_max_ms;rto_=c.rto_initial_ms;}
  void Observe(std::uint64_t ms) noexcept;
  std::uint64_t Rto() const noexcept {return rto_;}
private:
  std::uint64_t mean_=0,variation_=0,rto_=RuntimeConfig{}.rto_initial_ms;
  bool initialized_=false;
  std::uint64_t minimum_=RuntimeConfig{}.rto_min_ms,maximum_=RuntimeConfig{}.rto_max_ms;
};
// 截止同时受授予速率、剩余量和起始120秒约束；重复数据不刷新进度。
std::uint64_t TransferDeadline(std::uint64_t started,std::uint64_t now,
                               std::uint64_t remaining,std::uint64_t rate,const RuntimeConfig &config={}) noexcept;
struct TransferStatistics final {
  std::uint64_t fragments=0,duplicates=0,conflicts=0,decoded=0,ack_windows=0,retransmitted=0;
  std::uint64_t payload_bytes=0,decoded_bytes=0,decode_us=0,expired=0;
};
Message AckMessage(const ReceiveBitmap &,std::uint64_t sequence,Route);
ReceiveBitmap ReadAck(const Message &);
// 收齐记录保留原始压缩字节，冲突副本可比较；不计算摘要，不重复解压/应用。
class Reassembler final {
public:
  explicit Reassembler(Limits l=HandshakeLimits()){Configure(l);}
  void Configure(Limits) noexcept;
  void Authorize(Route) noexcept;
  void Cancel(std::uint64_t transfer) noexcept;
  bool Receive(std::span<const std::uint8_t>,std::uint64_t now,Message &);
  std::vector<ReceiveBitmap> Acks(std::uint64_t now,unsigned max_windows=8);
  void Expire(std::uint64_t now);
  void GrantRate(std::uint64_t bytes_per_second) noexcept {rate_=(std::max)(1ull,bytes_per_second);}
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  const TransferStatistics &Statistics() const noexcept {return statistics_;}
  std::size_t Pending() const noexcept {return pending_.size();}
private:
  struct Candidate final {
    Header header;
    memory::Lease memory;
    std::uint64_t started=0,progress=0,ack_due=0,deadline=0;
    std::uint32_t received=0,cursor=0;
    bool complete=false;
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint64_t> bits;
    std::vector<bool> dirty;
  };
  Limits limits_;
  Route route_;
  bool routed_=false;
  std::map<TransferKey,Candidate> pending_;
  std::deque<std::uint64_t> retired_;
  std::uint64_t retired_before_=0;
  TransferStatistics statistics_;
  std::uint64_t rate_=RuntimeConfig{}.reliable_bytes_per_second;
  void Retire(std::uint64_t) noexcept;
};
enum class SendResult { Sent, WouldBlock, Failed };
// 网络线程唯一owner；发送成功后才标sent，ACK只能确认已经实际发送的位。
class ReliableSender final {
public:
  using Send=std::function<SendResult(std::span<const std::uint8_t>)>;
  void Authorize(Route) noexcept;
  void Start(SharedBlob,std::uint64_t transfer,std::uint64_t now);
  bool Acknowledge(const Message &,std::uint64_t now);
  void Cancel(std::uint64_t transfer) noexcept;
  void GrantRate(std::uint64_t bytes_per_second) noexcept {rate_=(std::max)(1ull,bytes_per_second);}
  std::size_t Active() const noexcept {return transfers_.size();}
  // 超时不是成功；上层按真实transfer独立恢复或停止该Peer。
  struct Result {std::uint64_t transfer=0;bool complete=false;const char *reason="";};
  bool Poll(Result &);
  bool AllSent(std::uint64_t transfer) const noexcept;
  void Confirm(std::uint64_t transfer);
  unsigned Pump(std::uint64_t now,unsigned packet_budget,const Send &);
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  bool Empty() const noexcept {return transfers_.empty();}
  const TransferStatistics &Statistics() const noexcept {return statistics_;}
private:
  struct Transfer final {
    SharedBlob blob;
    memory::Lease memory;
    std::uint64_t id=0,started=0,progress=0,deadline=0;
    std::uint32_t count=0,cursor=0;
    std::vector<std::uint64_t> sent_at;
    std::vector<bool> sent,acked;
    std::vector<bool> retransmitted;
  };
  Route route_;
  std::map<std::uint64_t,Transfer> transfers_;
  std::uint64_t last_transfer_=0;
  std::uint64_t round_robin_=0;
  TransferStatistics statistics_;
  RttEstimator rtt_;
  std::uint64_t rate_=RuntimeConfig{}.reliable_bytes_per_second;
  std::deque<Result> results_;
};
} // namespace cs::replica::stream
