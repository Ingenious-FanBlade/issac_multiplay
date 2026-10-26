#include "game/peer.h"
#include "game/lifecycle.h"
#include "game/remote_life.h"
#include "game/state.h"
#include "game/tick.h"
#include "runtime.h"
#include <algorithm>

namespace cs::game {
namespace {
net::protocol::ByeReason ByeFor(core::SessionCause cause) noexcept {
  using B = net::protocol::ByeReason;
  switch (cause) {
  case core::SessionCause::Stop:
    return B::Stop;
  case core::SessionCause::RunTeardown:
    return B::RunTeardown;
  case core::SessionCause::PeerBye:
    return B::PeerBye;
  case core::SessionCause::PeerTimeout:
    return B::PeerTimeout;
  case core::SessionCause::PeerProtocol:
    return B::PeerProtocol;
  case core::SessionCause::PeerLost:
    return B::PeerLost;
  default:
    return B::InternalFailure;
  }
}
// 每次 Manager drain 按原因合并，最多七条事件；优先保留首条 Ack 已知的样本、
// 新包/命令序号与代次，旧 Ack 测试不能用 stale 包或未知 stamp 冒充。
// 析构覆盖提前返回路径，不改变接收、丢弃与 admission 的原有顺序。
class DiscardBatch final {
public:
  explicit DiscardBatch(SessionRuntime &runtime) noexcept : r(runtime) {}
  void Count(DiscardReason reason, const net::ReceivedInput &packet,
             const core::InputCmdDraftV2 &command, unsigned commands) noexcept {
    const auto index = static_cast<std::size_t>(reason);
    auto &sample = samples[index];
    if (!sample.commands || (!sample.ack_known && packet.ack_stamp_known)) {
      sample.packet = packet.packet_sequence;
      sample.sequence = command.sequence;
      sample.last_ack = packet.input.last_ack;
      sample.generation = packet.acknowledged_generation;
      sample.ack_state = packet.acknowledged_state;
      sample.ack_known = packet.ack_stamp_known;
    }
    sample.commands += commands;
    sample.unknown_commands += packet.ack_stamp_known ? 0 : commands;
    if (reason == DiscardReason::OldGeneration && packet.ack_stamp_known)
      sample.known_old_generation_commands += commands;
    r.peer.discards[index] += commands;
    r.peer.discarded += commands;
  }
  ~DiscardBatch() noexcept {
    for (std::size_t i = 0; i < samples.size(); ++i) {
      const auto &sample = samples[i];
      if (!sample.commands)
        continue;
      Trace("peer_discard",
            "reason=%s commands=%llu total=%llu loop=%llu tick=%llu "
            "session_generation=%llu participant_generation=%llu "
            "advertised_generation=%llu b_life=%s blocks=%u "
            "sample_packet=%llu sample_sequence=%llu sample_last_ack=%llu "
            "sample_ack_known=%u sample_ack_generation=%llu sample_ack_state=%u "
            "ack_unknown_commands=%llu known_old_generation_commands=%llu "
            "receive_after_ms=%llu sequence_floor=%llu",
            DiscardReasonName(static_cast<DiscardReason>(i)), sample.commands,
            r.peer.discards[i], r.loop, r.clock.Completed(), SessionGeneration(r),
            r.players[1].Generation(), r.peer.advertised_generation,
            RemoteLifeStatusName(r.life.status), r.blocks, sample.packet,
            sample.sequence, sample.last_ack, unsigned(sample.ack_known),
            sample.generation, unsigned(sample.ack_state), sample.unknown_commands,
            sample.known_old_generation_commands,
            r.peer.receive_after_ms, r.peer.discarded_sequence_floor);
    }
  }
private:
  // 只复制网络值，不保留 packet 引用、不从后台读取 Player 或 Lua。
  struct Sample {
    uint64_t commands = 0, unknown_commands = 0, known_old_generation_commands = 0,
             packet = 0, sequence = 0,
             last_ack = 0, generation = 0;
    core::SessionState ack_state = core::SessionState::Warmup;
    bool ack_known = false;
  };
  SessionRuntime &r;
  std::array<Sample, static_cast<std::size_t>(DiscardReason::Count)> samples{};
};
// 依次区分丢弃原因：先会话/生命/阻断，再绑定与接收窗口，最后命令级下界。
// 这里只读主线程已复制的值，不解码 payload 决定归属。
bool Classify(SessionRuntime &r, const net::ReceivedInput &packet,
              DiscardReason &reason) noexcept {
  if (!r.session.AcceptsInput()) {
    reason = DiscardReason::Inactive;
    return true;
  }
  if (r.life.status == RemoteLifeStatus::Lost ||
      r.life.status == RemoteLifeStatus::Revived) {
    reason = DiscardReason::LifeLost;
    return true;
  }
  if (r.blocks) {
    reason = DiscardReason::LoadingOrBlocked;
    return true;
  }
  if (!r.players[1].BoundValid()) {
    reason = DiscardReason::Unbound;
    return true;
  }
  if (packet.received_ms <= r.peer.receive_after_ms) {
    reason = DiscardReason::OldWindow;
    return true;
  }
  if (!packet.ack_stamp_known) {
    // 记录缺失时仍拒绝，但单独计数：不能声称已经证明旧代次分支。
    ++r.peer.ack_unknown;
    reason = DiscardReason::OldGeneration;
    return true;
  }
  if (packet.acknowledged_generation != r.players[1].Generation()) {
    reason = DiscardReason::OldGeneration;
    return true;
  }
  if (packet.acknowledged_state != core::SessionState::Active) {
    reason = DiscardReason::Inactive;
    return true;
  }
  return false;
}
} // namespace
const char *DiscardReasonName(DiscardReason reason) noexcept {
  switch (reason) {
  case DiscardReason::LifeLost:
    return "life_lost";
  case DiscardReason::LoadingOrBlocked:
    return "loading_or_blocked";
  case DiscardReason::Unbound:
    return "unbound";
  case DiscardReason::OldGeneration:
    return "old_generation";
  case DiscardReason::OldWindow:
    return "old_window";
  case DiscardReason::SequenceFloor:
    return "sequence_floor";
  case DiscardReason::Inactive:
    return "inactive";
  case DiscardReason::Count:
    break;
  }
  return "UNKNOWN";
}
// 丢弃边界使用接收墙钟，仅作窗口分类，不推进 Authority tick，也不改远端
// target。
void DiscardPeerWindow(SessionRuntime &r) noexcept {
  if (r.mode == core::SessionMode::Host) {
    r.peer.receive_after_ms = GetTickCount64();
    r.peer.discarded_sequence_floor =
        (std::max)(r.peer.discarded_sequence_floor, r.peer.observed_sequence);
  }
}
void PollPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host || r.finished)
    return;
  auto &p = r.peer;
  const auto status = p.worker.Status();
  if (status.state != p.observed.state ||
      status.terminal != p.observed.terminal)
    Trace("net", "event=state state=%s terminal=%s error=%u native=%u generation=%llu rx=%llu tx=%llu duplicate=%llu stale=%llu",
          net::protocol::PeerStateName(status.state),
          net::TerminalReasonName(status.terminal),
          unsigned(status.protocol_error), status.native_error,
          p.session_generation, status.received_packets, status.sent_packets,
          status.duplicate_packets, status.stale_packets);
  p.observed = status;
  // 粘性 terminal 优先于任何输入或激活；队列满也不会丢失断线事实。
  switch (status.terminal) {
  case net::TerminalReason::PeerBye:
    StopSession(core::SessionCause::PeerBye);
    return;
  case net::TerminalReason::PeerTimeout:
    StopSession(core::SessionCause::PeerTimeout);
    return;
  case net::TerminalReason::PeerProtocol:
    StopSession(core::SessionCause::PeerProtocol);
    return;
  case net::TerminalReason::RunTeardown:
    StopSession(core::SessionCause::RunTeardown);
    return;
  case net::TerminalReason::InternalFailure:
    FailRuntime("network_worker_failed");
    return;
  case net::TerminalReason::PeerLost:
    // 本地请求的 PeerLost 已经退休；远端若以该原因结束也按同一有序退休处理。
    if (!r.retiring)
      StopSession(core::SessionCause::PeerLost);
    return;
  case net::TerminalReason::Stop:
    if (!r.retiring)
      StopSession(core::SessionCause::Stop);
    return;
  default:
    break;
  }
  if (r.retiring || p.established ||
      status.state != net::protocol::PeerState::Established)
    return;
  core::SessionIdentity identity{p.identity.session, p.identity.clients,
                                 p.session_generation};
  if (!r.session.Start(identity, core::SessionMode::Host)) {
    FailRuntime("host_identity_rejected");
    return;
  }
  for (unsigned i = 0; i < 2; ++i)
    r.players[i].Initialize(identity.session, identity.clients[i], i,
                            &r.session);
  p.established = true;
  DiscardPeerWindow(r);
  Trace("session", "from=Hosting to=Warmup mode=Host generation=%llu loop=%llu",
        identity.generation, r.loop);
}
// 每个 Manager 最多 32 包、128 条命令；冗余从旧到新入同一个 Participant
// admission。 当前绑定代次来自主线程 owner，接收前已积压的旧窗口绝不被赋予新
// generation。
void DrainPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host || !r.peer.established || r.retiring)
    return;
  DiscardBatch discarded(r);
  net::ReceivedInput packet{};
  for (unsigned n = 0; n < 32 && r.peer.worker.TryReceive(packet); ++n) {
    r.peer.observed_sequence =
        (std::max)(r.peer.observed_sequence, packet.input.command.sequence);
    DiscardReason reason = DiscardReason::Count;
    if (Classify(r, packet, reason)) {
      const unsigned commands = 1 + packet.input.redundancy_count;
      discarded.Count(reason, packet, packet.input.command, commands);
      r.peer.discarded_sequence_floor =
          (std::max)(r.peer.discarded_sequence_floor,
                     packet.input.command.sequence);
      continue;
    }
    const auto submit = [&](const core::InputCmdDraftV2 &cmd) noexcept {
      if (cmd.sequence <= r.peer.discarded_sequence_floor) {
        discarded.Count(DiscardReason::SequenceFloor, packet, cmd, 1);
        return true;
      }
      const auto decision = r.players[1].AcceptInput(
          cmd, r.players[1].Generation(), r.clock.Completed());
      ++r.peer.admitted;
      Trace("submit",
            "ordinal=1 generation=%llu sequence=%llu target=%llu loop=%llu "
            "packet=%llu session_generation=%llu decision=%u edge=%u move=%d,%d aim=%d,%d held=%u "
            "participant_generation=%llu last_ack=%llu ack_stamp_known=%u ack_generation=%llu",
            r.players[1].Generation(), cmd.sequence, cmd.target_tick, r.loop,
            packet.packet_sequence, r.peer.session_generation, unsigned(decision), cmd.state.edge_flags,
            cmd.state.move_x, cmd.state.move_y, cmd.state.aim_x,
            cmd.state.aim_y, cmd.state.held_flags, r.players[1].Generation(),
            packet.input.last_ack, unsigned(packet.ack_stamp_known),
            packet.acknowledged_generation);
      if (decision == core::Decision::SequenceConflict) {
        FailRuntime("remote_sequence_conflict");
        return false;
      }
      return true;
    };
    for (unsigned i = 0; i < packet.input.redundancy_count; ++i)
      if (!submit(packet.input.redundant[i]))
        return;
    if (!submit(packet.input.command))
      return;
  }
}
// TopExit 在 completed 发布之后复制值；无 tick
// 时也发布最新绑定/会话状态供心跳使用。
void PublishPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host || !r.peer.established ||
      r.peer.stop_requested)
    return;
  const auto progress = r.players[1].Progress(r.clock.Completed());
  net::protocol::Ack ack;
  ack.server_tick = r.clock.Completed();
  ack.client_id = progress.client;
  ack.accepted = progress.last_accepted_sequence;
  ack.processed = progress.last_processed_sequence;
  ack.processed_tick = progress.last_processed_tick;
  ack.processed_source = progress.last_processed_source_sequence;
  // 只在新绑定已可接收输入时公布其代次；暂停/未绑定/死亡期间保留上次公布值。
  // 否则远端会在暂停中提前清一次历史，恢复时把暂停边沿混进同代次冗余。
  const auto previous_generation = r.peer.advertised_generation;
  if (r.session.AcceptsInput() && !r.blocks && r.players[1].BoundValid())
    r.peer.advertised_generation = progress.generation;
  ack.generation = r.peer.advertised_generation;
  ack.rejected_count = progress.rejected_count;
  ack.session_state = r.session.Status().state;
  for (unsigned i = 3; i < progress.rejections.size(); ++i)
    if (progress.rejections[i])
      ack.rejection_mask |= 1u << i;
  const bool queued = r.peer.worker.QueueAck(ack);
  if (!queued)
    ++r.peer.ack_dropped;
  // 首次与代次变化立即记录，其余每 30 次汇总。queued 仅表示交给 worker，
  // 不冒充成功发包；真正的成功发送 stamp 仍由 worker 建立。
  ++r.peer.ack_publications;
  if (previous_generation != ack.generation || r.peer.ack_publications == 1 ||
      r.peer.ack_publications % 30 == 0)
    Trace("ack", "event=publication generation=%llu completed=%llu b_generation=%llu accepted=%llu processed=%llu processed_tick=%llu source=%llu rejected=%llu mask=%u state=%u dropped=%llu "
          "session_generation=%llu participant_generation=%llu previous_generation=%llu queued=%u publication=%llu loop=%llu",
          r.peer.session_generation, ack.server_tick, ack.generation, ack.accepted,
          ack.processed, ack.processed_tick, ack.processed_source, ack.rejected_count,
          ack.rejection_mask, unsigned(ack.session_state), r.peer.ack_dropped,
          SessionGeneration(r), progress.generation, previous_generation,
          unsigned(queued), r.peer.ack_publications, r.loop);
}
bool StopPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host || r.peer.stop_finished)
    return true;
  if (r.peer.stop_failed)
    return false;
  r.peer.stop_requested = true;
  const auto result = r.peer.worker.Stop(ByeFor(r.retirement_cause), 1000);
  r.peer.stop_finished = result == net::StopResult::Stopped;
  r.peer.stop_failed = !r.peer.stop_finished;
  Trace("peer_stop", "result=%u reason=%s generation=%llu", unsigned(result),
        core::SessionCauseName(r.retirement_cause), r.peer.session_generation);
  const auto status = r.peer.worker.Status();
  Trace("net", "event=stop generation=%llu stopped=%u terminal=%s rx=%llu tx=%llu duplicate=%llu stale=%llu foreign=%llu rx_drop=%llu tx_drop=%llu bye_attempts=%llu bye_sent=%llu terminal_at_ms=%llu last_receive_ms=%llu",
        r.peer.session_generation, r.peer.stop_finished,
        net::TerminalReasonName(status.terminal), status.received_packets,
        status.sent_packets, status.duplicate_packets, status.stale_packets,
        status.foreign_packets, status.receive_queue_dropped, status.send_queue_dropped,
          status.bye_send_attempts, status.bye_sent_packets, status.terminal_at_ms,
          status.last_receive_ms);
  return r.peer.stop_finished;
}
} // namespace cs::game
