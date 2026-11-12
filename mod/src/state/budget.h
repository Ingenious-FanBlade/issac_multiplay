#pragma once
#include "state/acceptance.h"

namespace cs::state {
// 所有数值来自wire.json的同源预算，单位明确，不从旧JSON握手或环境猜测额度。
struct StateBudgetPolicy {
  std::uint64_t envelope_bytes,fragment_header_bytes,fragment_payload_bytes,datagram_bytes,
      bundle_header_bytes,bundle_entry_bytes,bundle_entries,ip_udp_bytes;
  std::uint64_t link_bytes_per_second,host_utilization_percent,host_bytes_per_second,peer_bytes_per_second,control_bytes_per_second,world_bytes_per_second,
      body_bytes_per_second,reliable_bytes_per_second;
  std::uint64_t host_burst_bytes,peer_burst_bytes,control_burst_bytes,world_burst_bytes,body_burst_bytes,reliable_burst_bytes;
  std::uint64_t world_hz,body_hz,ordinary_message_bytes,reliable_message_bytes,prepare_wire_bytes,
      prepare_send_ms,prepare_total_ms,progress_ms,recovery_attempts;
  std::uint64_t world_wait_ms,joint_wait_ms,baseline_wait_ms,body_send_ms,world_send_ms,body_stale_ms,world_stale_ms,
      ack_repeat_ms,resync_repeat_ms;
  std::uint64_t sent_targets,sent_recovery_slots,sent_target_bytes,sent_age_ms,event_count,event_bytes,event_age_ms,result_cache_groups,table_snapshots;
  std::uint64_t peer_value_bytes,peer_wire_bytes,workspace_job_bytes,workspace_jobs;
  std::uint64_t pool_value_bytes,pool_wire_bytes,pool_workspace_bytes,shared_value_bytes,shared_wire_bytes;
  std::uint64_t world_body_zstd,zstd_level,zstd_window_bytes;
};
const StateBudgetPolicy &StateBudgets() noexcept;
struct StatePathOverhead { std::uint64_t ip_udp=48,relay=0; };
struct StateWireCost {
  std::uint64_t decoded=0,encoded=0,fragments=0,datagrams=0,charged=0;
};
// encoded是实际压缩后整个ICSR消息大小；未知时按不压缩，不给予预估压缩收益。
// datagrams包含ICRF，charged再含IP／UDP与实际中继头；重传每次另算。
StateWireCost MeasureStateMessage(std::uint64_t body,std::optional<std::uint64_t> encoded={},StatePathOverhead={});
std::uint64_t MeasureStateDatagram(std::uint64_t udp_payload,StatePathOverhead={});
std::uint64_t MeasureStateBundle(std::span<const std::uint64_t> bodies,StatePathOverhead={});
std::uint64_t StateMessageBodyLimit(StateLane) noexcept;
struct StateLaneRates { std::uint64_t control=0,world=0,body=0,reliable=0; };
// 只借用本Peer因初建／恢复而停用的流；不跨Peer，未恢复的独立流仍保留其配额。
StateLaneRates StateTransferRates(std::uint8_t rebuilding_streams=0);
std::uint64_t StateSendWaitLowerBound(std::uint64_t cost,std::uint64_t available,std::uint64_t rate);
// false表示连无竞争下界都超过期限；true仍须实际分片／公平调度／三级桶许可。
bool StateOrdinarySendLowerBoundFits(StateLane,const StateWireCost &,std::uint64_t age_ms,
    std::uint64_t lane_credit,std::uint64_t peer_credit,std::uint64_t total_credit);

// owner自行持有正文租约，本队列仅保存两个有界借用描述。替换不延长首次等待期限，
// 丢候选不丢Host未确认音频事件。普通单body直接应用或丢弃，不进入等待队列。
class StatePendingCandidates {
public:
  bool Offer(const StateTarget &,std::uint64_t now_ms);
  std::optional<StateTarget> At(unsigned slot) const;
  void Remove(unsigned slot) noexcept;
  std::uint8_t Expire(std::uint64_t now_ms);
  void Clear() noexcept;
private:
  std::array<std::optional<StateTarget>,2> targets_{};
  std::array<std::uint64_t,2> started_{},observed_{};
};
enum class StateEpisodeStatus : std::uint8_t { Active, Inactive, ByteLimit, SendDeadline, TotalDeadline, NoProgress, Attempts, Clock };
// 一个准备／恢复episode的总起点固定；重复Resync／重传／同候选替换不刷新。
// Charge在本端发包前／对端有效反馈收齐时累计各方向成本，包含重传，不能跨端再算两次。
// Host出站速率桶只扣本端发送；episode双向总额与三级速率桶分别满足。
class StateTransferEpisode {
public:
  // Begin已计第一次完整尝试；Attempt仅用于同episode内的下一次全量重建。
  void Begin(std::uint64_t now_ms);
  bool Attempt(std::uint64_t now_ms);
  bool Charge(std::uint64_t packet_cost,std::uint64_t now_ms);
  void Progress(std::uint64_t now_ms);
  void Received(std::uint64_t now_ms);
  // 只在完整成功或明确退休后结束；不得为同一故障反复Finish／Begin续命。
  void Finish() noexcept { active_=false; }
  StateEpisodeStatus Status(std::uint64_t now_ms) const noexcept;
  std::uint64_t Charged() const noexcept { return charged_; }
private:
  bool active_=false,received_=false;
  StateEpisodeStatus failure_=StateEpisodeStatus::Active;
  std::uint64_t started_=0,progress_=0,observed_=0,charged_=0,attempts_=0;
};
// 仅统计完整事件头／正文的保留成本；容器capacity、索引与租约元数据另外记实际内存。
bool StateEventsWithinBudget(std::uint64_t count,std::uint64_t bytes,std::uint64_t oldest_age_ms) noexcept;
} // namespace cs::state
