#pragma once
#include "state/client_shadow.h"
#include "state/events.h"

namespace cs::state {
// 接收账本独立于latest候选；条目持有精确成员／资源快照，不能按新epoch解释旧序号。
struct ClientReceivedEvent {
  StateEventView value;
  ClientMemberSnapshotRef members;
  ClientResourceSnapshotRef resources;
  bool replayed=false,expired=false;
};
class ClientStateEvents final {
public:
  explicit ClientStateEvents(std::shared_ptr<ClientStateMemory>);
  void Open(const StateRoute &);
  void Close() noexcept;
  // 先完整检查冲突／容量，再一次接管。乱序帧可以补旧事件，不会倒退已覆盖水位。
  void Receive(const ClientStateCandidateRef &);
  // 准备只返回同目标覆盖的前缀；已经实际执行的事件不因失败重建再次播放。
  ClientStateVector<ClientReceivedEvent> Prepare(const ClientStateCandidate &,std::uint32_t host_frame);
  bool Replayed(std::span<const std::uint64_t>) noexcept;
  bool CanPromote(std::uint64_t watermark) const noexcept;
  bool Promote(std::uint64_t watermark) noexcept;
  std::uint64_t Installed() const noexcept {return installed_;}
  const StateEventQueueStats &Statistics() const noexcept {return stats_;}
private:
  struct Entry {
    ClientReceivedEvent event;
    ClientStateCandidateRef packet;
  };
  std::shared_ptr<ClientStateMemory> memory_;
  ClientStateVector<Entry> entries_;
  StateRoute route_{};
  bool active_=false;
  std::uint64_t installed_=0,covered_=0,first_=1,bytes_=0;
  StateEventQueueStats stats_{};
};
} // namespace cs::state
