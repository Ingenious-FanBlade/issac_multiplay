#pragma once
#include "state/acceptance.h"
#include "state/events.h"
#include <atomic>
#include <memory>
#include <vector>

namespace cs::state {
// 来源快照只拥有规范字节；成员号在一个run内只追加，不把地址、来源修订当成Peer目标号。
struct HostStateBytes {
  memory::Lease memory;
  std::vector<std::byte> bytes;
  explicit HostStateBytes(std::span<const std::byte>);
};
using HostStateBytesRef=std::shared_ptr<const HostStateBytes>;
struct HostStateResource {std::uint64_t handle=0;ResourceKey key;std::uint8_t kind=255;HostStateBytesRef payload;};
struct HostStateBlock {std::uint8_t block=0;std::uint64_t changed=0;HostStateBytesRef payload;};
struct HostStateRecord {
  memory::Lease memory;
  StateMemberEntry member;
  BodyStream stream=BodyStream::World;
  std::uint64_t topology=0,binding=0;
  std::vector<HostStateBlock> blocks;
  std::vector<ProjectionFact> projections;
};
struct HostStateEvent {StateEventView value;HostStateBytesRef payload;};
struct HostStateSnapshot {
  memory::Lease memory;
  CaptureWitness witness,ordinary;
  std::uint32_t member_epoch=0,resource_epoch=0;
  std::uint64_t structure_revision=0;
  std::vector<StateMemberEntry> members;
  std::vector<HostStateResource> resources;
  std::vector<std::shared_ptr<const HostStateRecord>> records;
  std::uint64_t event_first=0,event_watermark=0;
  std::vector<HostStateEvent> events;
  // 网络owner向全部当前Peer交接成功后置位；仅表示接管，不代表安装／播放／ACK。
  mutable std::atomic<std::uint32_t> events_transferred{0};
};
using HostStateSnapshotRef=std::shared_ptr<const HostStateSnapshot>;

// 唯一采集consumer调用；失败不替换已封存状态或污染下一次memcmp基线。
class HostStateCapture final {
public:
  HostStateCapture();
  ~HostStateCapture();
  void Reset() noexcept;
  HostStateSnapshotRef Capture(const CapturePacket &,std::span<const HostStateResource>,void *qualification=nullptr,
      bool (*current)(void *) noexcept=nullptr);
  HostStateSnapshotRef Latest() const noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};

struct HostStateMessage {
  StateMessageKind kind=StateMessageKind::Frame;
  StateLane lane=StateLane::World;
  HostStateBytesRef payload;
  StateTarget target;
  // 对正文所用成员／资源快照的强引用，ACK之前不可因latest替换释放。
  HostStateSnapshotRef source;
};
// 每个网络Peer单独持有；只在网络线程访问，所有ACK都核对真实完整发送凭据。
class HostStatePublisher final {
public:
  HostStatePublisher();~HostStatePublisher();
  void Open(const StateRoute &);
  void Stop() noexcept;
  bool Offer(HostStateSnapshotRef);
  // 表先发、ContextReady后组帧；返回值由发送owner保存到实际发完／确认。
  std::shared_ptr<const HostStateMessage> Next();
  void AbandonPending(const std::shared_ptr<const HostStateMessage> &) noexcept;
  bool Sent(const std::shared_ptr<const HostStateMessage> &,std::uint64_t now_ms);
  bool ContextReady(const StateRoute &,const StateContextReady &);
  StateAckResult Applied(const StateRoute &,const StateAppliedAck &);
  bool Recover(const StateRoute &,const StateResync &);
  bool RequireRecovery(StateResyncReason);
  // 已发目标超龄时使用保留的恢复槽重建，不清除尚未确认的事件／旧目标凭据。
  bool RecoverExpired(std::uint64_t now_ms);
  bool Active() const noexcept;
  bool Preparing() const noexcept;
  std::uint64_t EventWatermark() const noexcept;
  StateEventQueueStats EventStatistics() const noexcept;
  std::array<StateAckEntry,2> Confirmed() const noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
}
