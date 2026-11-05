#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace cs::replica::stream {
// 单位随字段名固定；连接启动时复制，远端协商只能收紧保护上限。
#define CS_STREAM_SETTINGS(X) \
  X(player_hz,60,1,60) X(world_hz,30,1,30) X(roster_hz,20,1,20) X(timing_hz,60,1,60) \
  X(total_bytes_per_second,937500,1228,937500) X(peer_bytes_per_second,312500,1228,937500) \
  X(control_bytes_per_second,25000,1228,937500) X(player_bytes_per_second,120000,1228,937500) \
  X(world_bytes_per_second,120000,1228,937500) X(reliable_bytes_per_second,47500,1228,937500) \
  X(total_burst_bytes,24576,1228,24576) X(peer_burst_bytes,8192,1228,8192) \
  X(control_burst_bytes,2048,1228,8192) X(player_burst_bytes,4096,1228,8192) \
  X(world_burst_bytes,4096,1228,8192) X(reliable_burst_bytes,8192,1228,8192) \
  X(bundle_wait_ms,2,1,2) X(heartbeat_ms,250,10,1000) X(peer_timeout_ms,3000,500,60000) \
  X(rto_initial_ms,200,100,1000) X(rto_min_ms,100,100,1000) X(rto_max_ms,1000,100,1000) \
  X(progress_timeout_ms,2000,500,2000) X(absolute_timeout_ms,120000,10000,120000) \
  X(final_retry_ms,100,10,100) X(stale_ms,500,100,500) X(recovery_ms,2000,500,2000) \
  X(applied_ack_ms,50,1,50) X(receive_ack_ms,50,1,50) \
  X(datagram_bytes,1200,1200,1200) X(fragment_payload_bytes,1136,1136,1136) \
  X(world_message_bytes,33554432,65536,33554432) X(fast_message_bytes,65536,1200,65536) \
  X(world_nodes,67108864,32768,67108864) X(fast_nodes,32768,1024,32768) \
  X(fragments,32768,64,32768) X(reliable_candidates,2,1,2) X(dynamic_candidates,65,1,65) \
  X(value_pool_bytes,268435456,1048576,268435456) X(wire_pool_bytes,67108864,1048576,67108864) \
  X(workspace_pool_bytes,67108864,1048576,67108864)
struct RuntimeConfig final {
#define CS_STREAM_FIELD(name,value,min,max) std::uint64_t name=value;
  CS_STREAM_SETTINGS(CS_STREAM_FIELD)
#undef CS_STREAM_FIELD
  // P2/P4验收均禁止跨Peer借用；初建仅可借用自己未用于展示的动态份额。
  bool acceptance_profile=true, borrow_initial_lanes=true;
  const char *Validate() const noexcept;
  std::string Fields(const std::string &prefix) const;
};
// 失败类别描述可执行动作；具体首因、key和原生前缀由各owner另外保留。
enum class FailureClass { Dependency, Budget, Resource, Prewrite, Native, Owner, Stale, Recovery, Capability };
struct FailurePolicy final {FailureClass category;const char *name,*message;bool recoverable,waitable;};
FailurePolicy ClassifyFailure(const char *reason,bool written,bool unsafe=false) noexcept;
} // namespace cs::replica::stream
