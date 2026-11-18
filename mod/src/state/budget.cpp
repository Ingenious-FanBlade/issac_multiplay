#include "state/budget.h"
#include "core/replication_memory.h"
#include <algorithm>

namespace cs::state {
namespace {
#include "state_budget.inc"
// 子额度来自协议，实际分配仍共用既有进程池；编译时阻止两份上限悄悄分叉。
static_assert(kStateBudgetPolicy.pool_value_bytes==memory::kReplicationPoolCeilings[0] &&
    kStateBudgetPolicy.pool_wire_bytes==memory::kReplicationPoolCeilings[1] &&
    kStateBudgetPolicy.pool_workspace_bytes==memory::kReplicationPoolCeilings[2]);
// 收到的差分基线至少覆盖一整流的在途窗口。
static_assert(kStateBudgetPolicy.decode_snapshots>=kStateBudgetPolicy.world_send_window);
static_assert(kStateBudgetPolicy.native_object_failure_limit&&kStateBudgetPolicy.pool_object_missing_limit);
[[noreturn]] void Fail(const char *reason) { throw Gap("state_budget",reason); }
std::uint64_t Add(std::uint64_t a,std::uint64_t b) { if (b>UINT64_MAX-a) Fail("size_overflow");return a+b; }
std::uint64_t Multiply(std::uint64_t a,std::uint64_t b) { if (a && b>UINT64_MAX/a) Fail("size_overflow");return a*b; }
std::uint64_t Ceil(std::uint64_t n,std::uint64_t d) { if (!d) Fail("zero_rate");return n/d+bool(n%d); }
unsigned Slot(const StateTarget &target) {
  const auto mask=StateTargetStreams(target);
  if (target.count==1 && target.sections[0].base_version && mask==2) Fail("ordinary_body_cannot_wait");
  return target.count==1 && target.sections[0].base_version && mask==1?0:1;
}
} // namespace
const StateBudgetPolicy &StateBudgets() noexcept { return kStateBudgetPolicy; }
std::uint64_t MeasureStateDatagram(std::uint64_t bytes,StatePathOverhead path) {
  if (!bytes || bytes>kStateBudgetPolicy.datagram_bytes) Fail("invalid_datagram_size");
  return Add(Add(bytes,path.ip_udp),path.relay);
}
StateWireCost MeasureStateMessage(std::uint64_t body,std::optional<std::uint64_t> encoded,StatePathOverhead path) {
  const auto &p=kStateBudgetPolicy;StateWireCost cost;cost.decoded=Add(body,p.envelope_bytes);cost.encoded=encoded.value_or(cost.decoded);
  if (!cost.encoded) Fail("empty_encoded_message");cost.fragments=Ceil(cost.encoded,p.fragment_payload_bytes);
  cost.datagrams=Add(cost.encoded,Multiply(cost.fragments,p.fragment_header_bytes));
  cost.charged=Add(cost.datagrams,Multiply(cost.fragments,Add(path.ip_udp,path.relay)));return cost;
}
std::uint64_t MeasureStateBundle(std::span<const std::uint64_t> bodies,StatePathOverhead path) {
  const auto &p=kStateBudgetPolicy;if (bodies.empty() || bodies.size()>p.bundle_entries) Fail("bundle_count");
  auto bytes=p.bundle_header_bytes;
  for (const auto body:bodies) {if (body<2 || body>p.fragment_payload_bytes-p.envelope_bytes) Fail("bundle_body");bytes=Add(bytes,Add(p.bundle_entry_bytes,body));}
  return MeasureStateDatagram(bytes,path);
}
std::uint64_t StateMessageBodyLimit(StateLane lane) noexcept {
  const auto &p=kStateBudgetPolicy;
  return (lane==StateLane::Baseline || lane==StateLane::Context?p.reliable_message_bytes:p.ordinary_message_bytes)-p.envelope_bytes;
}
StateLaneRates StateTransferRates(std::uint8_t mask) {
  if (mask>3) Fail("invalid_rebuilding_mask");const auto &p=kStateBudgetPolicy;
  StateLaneRates rates{p.control_bytes_per_second,p.world_bytes_per_second,p.body_bytes_per_second,p.reliable_bytes_per_second};
  if (mask&1) {rates.reliable+=rates.world;rates.world=0;}
  if (mask&2) {rates.reliable+=rates.body;rates.body=0;}
  return rates;
}
std::uint64_t StateSendWaitLowerBound(std::uint64_t cost,std::uint64_t available,std::uint64_t rate) {
  return cost<=available?0:Ceil(Multiply(cost-available,1000),rate);
}
bool StateOrdinarySendLowerBoundFits(StateLane lane,const StateWireCost &cost,std::uint64_t age,
    std::uint64_t lane_credit,std::uint64_t peer_credit,std::uint64_t total_credit) {
  const auto &p=kStateBudgetPolicy;
  if ((lane!=StateLane::World && lane!=StateLane::Body) || cost.decoded>p.ordinary_message_bytes ||
      !cost.encoded || cost.encoded>p.ordinary_message_bytes || cost.encoded>cost.decoded) return false;
  const auto rate=lane==StateLane::World?p.world_bytes_per_second:p.body_bytes_per_second;
  const auto burst=lane==StateLane::World?p.world_burst_bytes:p.body_burst_bytes;
  const auto deadline=lane==StateLane::World?p.world_send_ms:p.body_send_ms;
  const auto wait=(std::max)({StateSendWaitLowerBound(cost.charged,(std::min)(lane_credit,burst),rate),
      StateSendWaitLowerBound(cost.charged,(std::min)(peer_credit,p.peer_burst_bytes),p.peer_bytes_per_second),
      StateSendWaitLowerBound(cost.charged,(std::min)(total_credit,p.host_burst_bytes),p.host_bytes_per_second)});
  return age<=deadline && wait<=deadline-age;
}
bool StatePendingCandidates::Offer(const StateTarget &target,std::uint64_t now) {
  const auto slot=Slot(target);const auto bytes=target.encoded.size();
  if (!bytes || bytes>StateMessageBodyLimit(slot==0?StateLane::World:StateLane::Baseline)) Fail("pending_candidate_bytes");
  if (targets_[slot]) {
    const auto &old=*targets_[slot];if (now<observed_[slot] || old.route!=target.route) Fail("pending_clock_or_route");
    if (StateTargetsConflict(old,target)) Fail("pending_target_conflict");
    if (StateTargetStreams(old)!=StateTargetStreams(target)) return false;
    for (unsigned i=0;i<target.count;++i) if (target.sections[i].target_version<=old.sections[i].target_version) return false;
  } else started_[slot]=now;
  targets_[slot]=target;observed_[slot]=now;return true;
}
std::optional<StateTarget> StatePendingCandidates::At(unsigned slot) const { if (slot>1) Fail("pending_slot");return targets_[slot]; }
void StatePendingCandidates::UseDecodedBytes(unsigned slot,std::span<const std::byte> bytes) noexcept {
  targets_[slot]->encoded=bytes;
  targets_[slot]->decoded=true;
}
void StatePendingCandidates::Remove(unsigned slot) noexcept { if (slot<2) targets_[slot].reset(); }
void StatePendingCandidates::Clear() noexcept { for (auto &target:targets_) target.reset(); }
std::uint8_t StatePendingCandidates::Expire(std::uint64_t now) {
  std::uint8_t mask=0;
  for (unsigned i=0;i<2;++i) if (targets_[i]) {
    if (now<observed_[i]) Fail("pending_clock_regressed");observed_[i]=now;
    const auto limit=i==0?kStateBudgetPolicy.world_wait_ms:kStateBudgetPolicy.baseline_wait_ms;
    if (now-started_[i]>=limit) {mask|=StateTargetStreams(*targets_[i]);targets_[i].reset();}
  }
  return mask;
}
void StateTransferEpisode::Begin(std::uint64_t now) {
  if (active_) {if (now<observed_) Fail("episode_clock_regressed");return;}
  active_=true;received_=false;failure_=StateEpisodeStatus::Active;
  started_=progress_=observed_=now;charged_=0;attempts_=1;
}
StateEpisodeStatus StateTransferEpisode::Status(std::uint64_t now) const noexcept {
  if (!active_) return StateEpisodeStatus::Inactive;if (now<observed_) return StateEpisodeStatus::Clock;
  if (failure_!=StateEpisodeStatus::Active) return failure_;
  const auto &p=kStateBudgetPolicy;
  if (now-started_>=p.prepare_total_ms) return StateEpisodeStatus::TotalDeadline;
  if (!received_ && now-started_>=p.prepare_send_ms) return StateEpisodeStatus::SendDeadline;
  // 传输完成前按分片进展计时。完整基线收齐后原生安装没有中间网络回执，
  // 继续使用从同一起点计算的 prepare_total_ms 等待 AppliedAck。
  if (!received_ && now-progress_>=p.progress_ms) return StateEpisodeStatus::NoProgress;
  if (charged_>p.prepare_wire_bytes) return StateEpisodeStatus::ByteLimit;
  return StateEpisodeStatus::Active;
}
bool StateTransferEpisode::Attempt(std::uint64_t now) {
  if (Status(now)!=StateEpisodeStatus::Active) return false;
  if (attempts_==kStateBudgetPolicy.recovery_attempts) {failure_=StateEpisodeStatus::Attempts;return false;}
  observed_=now;++attempts_;return true;
}
bool StateTransferEpisode::Charge(std::uint64_t bytes,std::uint64_t now) {
  if (Status(now)!=StateEpisodeStatus::Active) return false;
  if (!bytes || bytes>kStateBudgetPolicy.prepare_wire_bytes-charged_) {failure_=StateEpisodeStatus::ByteLimit;return false;}
  observed_=now;charged_+=bytes;return true;
}
void StateTransferEpisode::Progress(std::uint64_t now) {
  if (Status(now)!=StateEpisodeStatus::Active) Fail("late_transfer_progress");progress_=observed_=now;
}
void StateTransferEpisode::Received(std::uint64_t now) {
  Progress(now);received_=true;
}
bool StateEventsWithinBudget(std::uint64_t count,std::uint64_t bytes,std::uint64_t age) noexcept {
  const auto &p=kStateBudgetPolicy;return count<=p.event_count && bytes<=p.event_bytes && (!count || age<p.event_age_ms);
}
} // namespace cs::state
