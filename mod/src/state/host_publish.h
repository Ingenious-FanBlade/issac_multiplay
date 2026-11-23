#pragma once
#include "state/acceptance.h"
#include "state/events.h"
#include "state/flat_encode.h"
#include "net/mod_intent.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace cs::state {
// 来源快照只拥有规范字节。成员号由实际在途或已收到的快照和事件保留。
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
  HostFlatImageRef flat;
  std::shared_ptr<const HostStateRecord> flat_receipt; // 共享同一记录的无镜像元数据。
  bool flat_suspended=false; // 沿用镜像的成员引用已失效，本帧只登记沿用。
  // 编码真正读到的成员号及所需资源前缀，用于跨代表的 Body 闭包判定。
  std::vector<std::uint16_t> member_refs;
  std::uint32_t resource_prefix=0;
};
struct HostStateEvent {StateEventView value;HostStateBytesRef payload;};
class HostStateMemberIndices;
// 首次采集便失败的根可能尚无线上编号，仍按物理身份保留首因和连续次数。
struct HostRetainedRoot {
  std::uint16_t member=kNullIndex;
  std::uint32_t count=0;
  CaptureFailure first=CaptureFailure::None;
  std::uint16_t space=0;
  std::uint64_t local=0,use=0;
};
struct HostStateSnapshot {
  memory::Lease memory;
  CaptureWitness witness,ordinary;
  std::uint32_t member_epoch=0,resource_epoch=0;
  std::uint64_t structure_revision=0;
  FlatVersions flat_versions{}; // 与 Peer 的线上目标号独立，由 ACK 的真实发送凭据关联。
  std::vector<StateMemberEntry> members;
  std::vector<HostRetainedRoot> retained_members; // 对象仍在，保留连续失败次数及首因。
  // 普通快照中值与引用均齐全的可复用根，按完整成员键排序。
  std::vector<PresentationMember> reusable_roots;
  std::uint64_t baseline_request_revision=0;
  // 普通快照持有目录租约，Body 共享它。后台和网络仍可能使用的成员号不会回到空闲池。
  std::shared_ptr<const HostStateMemberIndices> member_indices;
  std::vector<HostStateResource> resources;
  std::vector<std::shared_ptr<const HostStateRecord>> records;
  std::uint64_t event_first=0,event_watermark=0;
  std::vector<HostStateEvent> events;
  ModShadowRef mods;
  std::uint64_t mod_missing_references=0,mod_encode_failures=0;
  // 发送账本与 ACK 只借用目录、旧域值及平铺根元数据，不持有平铺镜像版本。
  std::shared_ptr<const HostStateSnapshot> flat_receipt;
  // 网络owner向全部当前Peer交接成功后置位；仅表示接管，不代表安装／播放／ACK。
  mutable std::atomic<std::uint32_t> events_transferred{0};
};
using HostStateSnapshotRef=std::shared_ptr<const HostStateSnapshot>;

// 采集后台直接向网络线程交接已经封存的纯值。主线程只授权当前来源及撤销水位。
class HostStatePublication final {
public:
  struct View { HostStateSnapshotRef latest,ordinary; };
  HostStatePublication();
  View Read() const noexcept;
  void Publish(const HostStateSnapshotRef &,bool ordinary) noexcept;
  void InvalidateBefore(std::uint64_t revision) noexcept;
  void DiscardBody() noexcept;
  void Clear() noexcept;
private:
  memory::Lease memory_;
  mutable std::mutex mutex_;
  View published_;
  std::uint64_t floor_=0;
};

struct HostMemberIndexStats {std::uint32_t high_water=0,reused=0;};

// 工作线程提供计量回调，状态库只报告纯值索引和编码的边界。
enum class HostCapturePhase : std::uint8_t { Index, Encode };
struct HostCaptureTiming {
  void *owner=nullptr;
  void (*sample)(void *,HostCapturePhase,bool entering) noexcept=nullptr;
};
// 唯一采集consumer调用；失败不替换已封存状态或污染下一次memcmp基线。
class HostStateCapture final {
public:
  HostStateCapture();
  ~HostStateCapture();
  void Reset() noexcept;
  void DiscardBody() noexcept;
  HostStateSnapshotRef Capture(const CapturePacket &,std::span<const HostStateResource>,void *qualification=nullptr,
      bool (*current)(void *) noexcept=nullptr,HostCaptureTiming={});
  HostStateSnapshotRef Latest() const noexcept;
  HostMemberIndexStats MemberIndexStats() const noexcept;
  std::uint32_t FlatPaddingFailures() const noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};

struct HostStateMessage {
  StateMessageKind kind=StateMessageKind::Frame;
  StateLane lane=StateLane::World;
  HostStateBytesRef payload;
  StateTarget target;
  // Body 的值可以比实际使用的目录更新，两份见证都保留到真实 ACK。
  HostStateSnapshotRef source,tables;
  ModShadowRef mods;
  std::uint64_t mod_result_through=0;
};
// 普通 World 的每 Peer 聚合读数，峰值包含暂时越过窗口的重试。
struct HostWorldProgress {
  std::uint64_t window_skips=0,inflight_max=0,retries=0;
};
// 交给网络节拍 owner 消费当前 World 机会，Body 仍可独立发送。
enum class HostWorldDeferral : std::uint8_t {None, WindowFull, EventBudget};
// 每个网络Peer单独持有；只在网络线程访问，所有ACK都核对真实完整发送凭据。
class HostStatePublisher final {
public:
  HostStatePublisher();~HostStatePublisher();
  void Open(const StateRoute &,unsigned ordinal=kModPublic);
  void Stop() noexcept;
  bool Offer(HostStateSnapshotRef);
  bool QueueModResult(const net::ModIntentResult &) noexcept;
  std::uint64_t ModResultReceipt() const noexcept;
  // 两条普通流独立组帧；可靠资源目录和 base=0 不受普通流时钟阻断。
  std::shared_ptr<const HostStateMessage> Next(std::uint64_t now_ms,std::uint64_t now_us,
      std::uint8_t dynamic_streams,HostWorldDeferral &);
  // 可靠资源目录使用独立发送槽。
  bool StartTable(const std::shared_ptr<const HostStateMessage> &) noexcept;
  void AbandonPending(const std::shared_ptr<const HostStateMessage> &) noexcept;
  bool Sent(const std::shared_ptr<const HostStateMessage> &,std::uint64_t now_ms,std::uint64_t now_us);
  // 只有可靠传输完整回执能把表变成下一次增量的基准。
  void TableReceived(const HostStateMessage &);
  StateAckResult Received(const StateRoute &,const StateAppliedAck &);
  bool Recover(const StateRoute &,const StateResync &);
  bool RequireRecovery(StateResyncReason);
  // 回收超龄发送凭据；连续全局采集失败后，有新来源时发完整基线。
  bool RecoverExpired(std::uint64_t now_ms);
  bool Active() const noexcept;
  bool Preparing() const noexcept;
  std::uint64_t BodyDependencyBlocks() const noexcept;
  HostWorldProgress WorldProgress() const noexcept;
  std::uint64_t EventWatermark() const noexcept;
  StateEventQueueStats EventStatistics() const noexcept;
  std::array<StateAckEntry,2> Confirmed() const noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
}
