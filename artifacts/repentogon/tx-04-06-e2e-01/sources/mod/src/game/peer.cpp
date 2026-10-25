#include "game/peer.h"
#include "game/lifecycle.h"
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
  default:
    return B::InternalFailure;
  }
}
} // namespace
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
  net::ReceivedInput packet{};
  for (unsigned n = 0; n < 32 && r.peer.worker.TryReceive(packet); ++n) {
    r.peer.observed_sequence =
        (std::max)(r.peer.observed_sequence, packet.input.command.sequence);
    if (!r.session.AcceptsInput() || r.blocks || !r.players[1].BoundValid() ||
        packet.received_ms <= r.peer.receive_after_ms ||
        packet.acknowledged_generation != r.players[1].Generation() ||
        packet.acknowledged_state != core::SessionState::Active) {
      r.peer.discarded += 1 + packet.input.redundancy_count;
      r.peer.discarded_sequence_floor =
          (std::max)(r.peer.discarded_sequence_floor,
                     packet.input.command.sequence);
      continue;
    }
    const auto submit = [&](const core::InputCmdDraftV2 &cmd) noexcept {
      if (cmd.sequence <= r.peer.discarded_sequence_floor) {
        ++r.peer.discarded;
        return true;
      }
      const auto decision = r.players[1].AcceptInput(
          cmd, r.players[1].Generation(), r.clock.Completed());
      ++r.peer.admitted;
      Trace("submit",
            "ordinal=1 generation=%llu sequence=%llu target=%llu loop=%llu "
            "packet=%llu session_generation=%llu decision=%u edge=%u move=%d,%d aim=%d,%d held=%u",
            r.players[1].Generation(), cmd.sequence, cmd.target_tick, r.loop,
            packet.packet_sequence, r.peer.session_generation, unsigned(decision), cmd.state.edge_flags,
            cmd.state.move_x, cmd.state.move_y, cmd.state.aim_x,
            cmd.state.aim_y, cmd.state.held_flags);
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
  // 只在新绑定已可接收输入时公布其代次；暂停/未绑定期间保留上次公布值。
  // 否则远端会在暂停中提前清一次历史，恢复时把暂停边沿混进同代次冗余。
  if (r.session.AcceptsInput() && !r.blocks && r.players[1].BoundValid())
    r.peer.advertised_generation = progress.generation;
  ack.generation = r.peer.advertised_generation;
  ack.rejected_count = progress.rejected_count;
  ack.session_state = r.session.Status().state;
  for (unsigned i = 3; i < progress.rejections.size(); ++i)
    if (progress.rejections[i])
      ack.rejection_mask |= 1u << i;
  if (!r.peer.worker.QueueAck(ack))
    ++r.peer.ack_dropped;
  // 每 30 次 publication 留一条进度摘要；网络层心跳没有冒充模拟 tick。
  if (++r.peer.ack_publications % 30 == 0)
    Trace("ack", "generation=%llu completed=%llu b_generation=%llu accepted=%llu processed=%llu processed_tick=%llu source=%llu rejected=%llu mask=%u state=%u dropped=%llu",
          r.peer.session_generation, ack.server_tick, ack.generation, ack.accepted,
          ack.processed, ack.processed_tick, ack.processed_source, ack.rejected_count,
          ack.rejection_mask, unsigned(ack.session_state), r.peer.ack_dropped);
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
  Trace("net", "event=stop generation=%llu stopped=%u terminal=%s rx=%llu tx=%llu duplicate=%llu stale=%llu foreign=%llu rx_drop=%llu tx_drop=%llu bye_attempts=%llu bye_sent=%llu",
        r.peer.session_generation, r.peer.stop_finished,
        net::TerminalReasonName(status.terminal), status.received_packets,
        status.sent_packets, status.duplicate_packets, status.stale_packets,
        status.foreign_packets, status.receive_queue_dropped, status.send_queue_dropped,
        status.bye_send_attempts, status.bye_sent_packets);
  return r.peer.stop_finished;
}
} // namespace cs::game
