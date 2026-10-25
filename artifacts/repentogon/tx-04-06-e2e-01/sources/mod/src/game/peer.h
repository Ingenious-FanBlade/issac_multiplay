#pragma once
#include "net/worker.h"

namespace cs::game {
struct SessionRuntime;
// Worker 只持网络值；本桥的身份、接收边界与进度全部由游戏主线程持有。
struct PeerBridge {
  net::Worker worker;
  net::HostIdentity identity{};
  net::WorkerStatus observed{};
  uint64_t receive_after_ms = 0, discarded = 0, admitted = 0, ack_dropped = 0;
  uint64_t session_generation = 0, advertised_generation = 0;
  uint64_t ack_publications = 0;
  uint64_t observed_sequence = 0, discarded_sequence_floor = 0;
  bool established = false, stop_requested = false, stop_finished = false;
  bool stop_failed = false;
};
void DiscardPeerWindow(SessionRuntime &runtime) noexcept;
void PollPeer(SessionRuntime &runtime) noexcept;
void DrainPeer(SessionRuntime &runtime) noexcept;
void PublishPeer(SessionRuntime &runtime) noexcept;
bool StopPeer(SessionRuntime &runtime) noexcept;
} // namespace cs::game
