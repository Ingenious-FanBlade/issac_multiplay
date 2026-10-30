#pragma once
#include "net/worker.h"
#include <array>
#include <cstddef>

namespace cs::game {
struct SessionRuntime;
// 远端命令的丢弃原因。与 Accepted 分开统计；admitted 仍只是尝试数。
enum class DiscardReason : unsigned {
  LifeLost,         // B 处于 Lost，或 Revived 尚未重绑
  LoadingOrBlocked, // 加载/暂停等阻断窗口
  Unbound,          // B 未绑定
  OldGeneration,    // Ack 记录的代次与当前绑定不同（含记录未命中）
  OldWindow,        // 早于接收边界的积压包
  SequenceFloor,    // 低于已丢弃的序列下界
  Inactive,         // 会话未处于可接收输入的 Active
  Count
};
const char *DiscardReasonName(DiscardReason reason) noexcept;
// Worker 只持网络值；本桥的身份、接收边界与进度全部由游戏主线程持有。
struct PeerBridge {
  net::HostIdentity identity{};
  net::WorkerStatus observed{};
  uint64_t receive_after_ms = 0, discarded = 0, admitted = 0, ack_dropped = 0;
  std::array<uint64_t, static_cast<std::size_t>(DiscardReason::Count)> discards{};
  // Ack 记录缺失（stamp 未命中）的次数；旧代次仍拒绝，但不能冒充已证明旧代次。
  uint64_t ack_unknown = 0;
  uint64_t session_generation = 0, advertised_generation = 0;
  uint64_t ack_publications = 0;
  uint64_t observed_sequence = 0, discarded_sequence_floor = 0;
  bool established = false, stop_requested = false, stop_finished = false;
  bool stop_failed = false;
};
void DiscardPeerWindow(SessionRuntime &runtime, unsigned ordinal = 0) noexcept;
void PollPeer(SessionRuntime &runtime) noexcept;
void DrainPeer(SessionRuntime &runtime) noexcept;
void PublishPeer(SessionRuntime &runtime) noexcept;
bool StopPeer(SessionRuntime &runtime) noexcept;
void RequestRemoteRetirement(SessionRuntime &runtime, unsigned ordinal,
                             core::SessionCause cause) noexcept;
void FinishRemoteRetirements(SessionRuntime &runtime) noexcept;
} // namespace cs::game
