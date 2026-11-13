#pragma once
#include "state/frame_events.h"
#include "core/replication_memory.h"
#include "state/capture_values.h"

namespace cs::state {
// 版本73起只传瞬时播放／显示请求；音乐、循环声和ItemOverlay是普通world状态。
enum class StateEventKind : std::uint8_t { Sfx=1,ItemBanner=2,Fortune=3,BossIntro=4,RoomTransition=5 };
using StateEventView=PresentationAudioEvent;
struct StateEventSegment {
  std::uint64_t first_sequence=1,watermark=0;
  std::span<const StateEventView> events;
  std::uint32_t consumed=0,field_visits=0;
};
bool StateEventCosmetic(StateEventKind) noexcept;
std::uint16_t StateEventLayout(StateEventKind) noexcept;
std::uint32_t StateEventHeaderBytes() noexcept;
std::uint32_t StateEventQueueHeaderBytes() noexcept;
// Inspect只定界头与载荷；Decode还执行typed正文校验。两者均不播放、不改ACK水位。
StateEventSegment InspectStateEvents(const PresentationBatchContext &,std::uint32_t expected_count,std::span<const std::byte>,std::span<StateEventView>);
StateEventSegment DecodeStateEvents(const PresentationBatchContext &,std::uint32_t expected_count,std::span<const std::byte>,std::span<StateEventView>,BodyCodecScratch &,const BodyCodecContext *,BodyCodecLimits);
std::span<const std::byte> EncodeStateEvents(const PresentationBatchContext &,const StateEventSegment &,std::span<std::byte>,BodyCodecScratch &,const BodyCodecContext *,BodyCodecLimits,std::uint32_t &visits);

struct StateEventQueueStats {std::uint64_t dropped_capacity=0,dropped_expired=0,replayed=0,failed=0;};
// 每个Peer／连接独占一份队列；它只拥有封存正文和精确序号，不读游戏／硬件对象。
// 段内序号空隙明确代表发送方已丢弃的纯外观事件；非外观溢出使Offer失败。
class StateEventQueue final {
public:
  explicit StateEventQueue(std::uint32_t count=512,std::uint32_t bytes=256u<<10);
  void Reset(std::uint64_t baseline_watermark=0) noexcept;
  bool Offer(const StateEventView &) noexcept;
  // 完整采集范围内未出现的序号只可能是采集端已丢弃的纯外观；不能跨过未交接范围。
  bool Cover(std::uint64_t first_sequence,std::uint64_t watermark) noexcept;
  bool CanCover(std::uint64_t first_sequence,std::uint64_t watermark) const noexcept;
  bool Covered(std::uint64_t sequence) const noexcept {return sequence<=acked_;}
  // 最大字节包含本段全部头；未选后缀留队，不能用最新源水位跳过尚未投递的事件。
  StateEventSegment Select(std::span<StateEventView>,std::uint32_t maximum_bytes,std::uint64_t through=UINT64_MAX) const noexcept;
  bool Sent(std::uint64_t watermark) noexcept;
  bool Ack(std::uint64_t watermark) noexcept;
  void Expire(std::uint32_t host_frame,std::uint32_t maximum_age=15) noexcept;
  const StateEventQueueStats &Statistics() const noexcept{return stats_;}
  std::uint64_t RetainedBytes() const noexcept{return memory_->Bytes();}
private:
  struct Entry {StateEventView value;std::uint32_t offset=0;};
  void Remove(std::uint32_t) noexcept;
  memory::Lease memory_;
  std::vector<Entry> entries_;
  std::vector<std::byte> bytes_;
  std::uint32_t count_=0,used_=0;
  std::uint64_t offered_=0,acked_=0,sent_=0;
  StateEventQueueStats stats_;
};
// worker按本Peer的成员／资源快照编码，资源号不在主线程提前分配；latest替换不清此队列。
bool QueueCapturedStateEvents(const CapturePacket &,StateEventQueue &,CaptureLinkResolver,std::span<std::byte> link_scratch,
    std::span<std::byte> encode_scratch,BodyCodecScratch &,const BodyCodecContext *,BodyCodecLimits={});
}
