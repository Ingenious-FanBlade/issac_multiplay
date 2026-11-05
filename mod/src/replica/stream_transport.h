#pragma once
#include "replica/stream_codec.h"
#include <array>
#include <functional>
#include <deque>

namespace cs::replica::stream {
Limits HandshakeLimits() noexcept;
Limits WorldLimits(bool fast=false) noexcept;
bool Reliable(Lane) noexcept;
// endpoint先由socket owner核对；每连接实例只接纳显式授权的route，报文不能自行扩权。
struct Route final {
  std::uint64_t connection=0,epoch=0;
  bool negotiated=false,world=false,zstd=false;
  Direction direction=Direction::ToClient;
  Limits world_limits=WorldLimits();
  WorldScope scope{};
};
// 仅从严格协商结果取得本地上限；不接受报文Header作为预算来源。
Limits NegotiatedWorldLimits(const Value &);
struct TransferKey final {std::uint64_t connection=0,route=0,transfer=0;auto operator<=>(const TransferKey &) const = default;};
// 共享blob只有逻辑正文；连接/route/transfer在发送单片时写入，不为三个Peer复制大消息。
struct Blob final {
  memory::Lease memory;
  std::vector<std::uint8_t> bytes;
  std::uint32_t decoded_bytes=0;
  Kind kind=Kind::Error;
  Lane lane=Lane::Control;
  Codec codec=Codec::None;
  std::uint64_t sequence=0,encode_us=0,compression_us=0;
  WorldScope world{};
};
using SharedBlob=std::shared_ptr<const Blob>;
SharedBlob EncodeBlob(const Message &,bool zstd,Limits=WorldLimits());
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
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  const TransferStatistics &Statistics() const noexcept {return statistics_;}
private:
  struct Candidate final {
    Header header;
    memory::Lease memory;
    std::uint64_t started=0,progress=0,ack_due=0;
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
  unsigned Pump(std::uint64_t now,unsigned packet_budget,const Send &);
  std::uint64_t NextWake(std::uint64_t now) const noexcept;
  bool Empty() const noexcept {return transfers_.empty();}
  const TransferStatistics &Statistics() const noexcept {return statistics_;}
private:
  struct Transfer final {
    SharedBlob blob;
    memory::Lease memory;
    std::uint64_t id=0,started=0,progress=0;
    std::uint32_t count=0,cursor=0;
    std::vector<std::uint64_t> sent_at;
    std::vector<bool> sent,acked;
  };
  Route route_;
  std::map<std::uint64_t,Transfer> transfers_;
  std::uint64_t last_transfer_=0;
  std::uint64_t round_robin_=0;
  TransferStatistics statistics_;
};
} // namespace cs::replica::stream
