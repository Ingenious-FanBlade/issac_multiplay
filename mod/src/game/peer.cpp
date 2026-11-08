#include "game/peer.h"
#include "game/lifecycle.h"
#include "game/remote_life.h"
#include "game/state.h"
#include "game/tick.h"
#include "game/world_bridge.h"
#include "net/live.h"
#include "replica/stream_e2e.h"
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
// 只分类主线程复制的网络值，不给积压命令重新赋予当前绑定代次。
DiscardReason Classify(SessionRuntime &r, unsigned ordinal,
                       const net::ReceivedInput &packet) noexcept {
  const auto &remote = r.Remote(ordinal);
  if (!r.session.AcceptsInput() || !r.session.HasParticipant(ordinal) ||
      remote.retiring)
    return DiscardReason::Inactive;
  if (remote.life.status == RemoteLifeStatus::Lost ||
      remote.life.status == RemoteLifeStatus::Revived)
    return DiscardReason::LifeLost;
  if (r.blocks)
    return DiscardReason::LoadingOrBlocked;
  if (!r.players[ordinal].BoundValid())
    return DiscardReason::Unbound;
  if (packet.received_ms <= remote.peer.receive_after_ms)
    return DiscardReason::OldWindow;
  if (!packet.ack_stamp_known ||
      packet.acknowledged_generation != r.players[ordinal].Generation())
    return DiscardReason::OldGeneration;
  if (packet.acknowledged_state != core::SessionState::Active)
    return DiscardReason::Inactive;
  return DiscardReason::Count;
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
  default:
    return "UNKNOWN";
  }
}
void DiscardPeerWindow(SessionRuntime &r, unsigned ordinal) noexcept {
  if (r.mode != core::SessionMode::Host)
    return;
  for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
    if (ordinal && ordinal != i)
      continue;
    auto &p = r.Remote(i).peer;
    p.receive_after_ms = GetTickCount64();
    p.discarded_sequence_floor =
        (std::max)(p.discarded_sequence_floor, p.observed_sequence);
  }
}
// 单 Peer 退休先关闭输入，Manager 尾部排空原生回调后才移除设备和身体。
void RequestRemoteRetirement(SessionRuntime &r, unsigned ordinal,
                             core::SessionCause cause) noexcept {
  if (!ordinal || ordinal >= core::kMaximumParticipants)
    return;
  auto &remote = r.Remote(ordinal);
  if (remote.retiring || remote.retired)
    return;
  remote.retiring = true;
  remote.retirement_cause = cause;
  InvalidateRemoteBinding(r, "peer_retiring", ordinal);
  remote.peer.stop_requested = true;
  r.network.RequestPeerStop(ordinal, ByeFor(cause));
  Trace("peer", "event=retiring ordinal=%u reason=%s loop=%llu", ordinal,
        core::SessionCauseName(cause), r.loop);
}
void PollPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host || r.finished)
    return;
  // 共享socket/线程本身失败才退休整个Host；单个Endpoint错误仍只退休所属Peer。
  if (r.network.TransportFailed()) {
    FailRuntime("network_transport_failed");
    return;
  }
  for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
    auto &remote = r.Remote(i);
    auto &p = remote.peer;
    const auto status = r.network.Status(i);
    if (status.state != p.observed.state ||
        status.terminal != p.observed.terminal)
      Trace("net",
            "event=state ordinal=%u state=%s terminal=%s error=%u rx=%llu "
            "tx=%llu",
            i, net::protocol::PeerStateName(status.state),
            net::TerminalReasonName(status.terminal),
            unsigned(status.protocol_error), status.received_packets,
            status.sent_packets);
    p.observed = status;
    if (r.retiring || remote.retired || remote.retiring)
      continue;
    if (status.terminal != net::TerminalReason::None) {
      const auto cause = status.terminal == net::TerminalReason::PeerTimeout
                             ? core::SessionCause::PeerTimeout
                         : status.terminal == net::TerminalReason::PeerBye
                             ? core::SessionCause::PeerBye
                         : status.terminal == net::TerminalReason::PeerLost
                             ? core::SessionCause::PeerLost
                             : core::SessionCause::PeerProtocol;
      RequestRemoteRetirement(r, i, cause);
      continue;
    }
    if (p.established || status.state != net::protocol::PeerState::Established)
      continue;
    if (!r.session.AddParticipant(i)) {
      RequestRemoteRetirement(r, i, core::SessionCause::PeerProtocol);
      continue;
    }
    p.established = true;
    DiscardPeerWindow(r, i);
    Trace("peer",
          "event=established ordinal=%u client=%llu participants=%zu loop=%llu",
          i, r.session.Identity().clients[i], r.session.ParticipantCount(),
          r.loop);
  }
}
// 各 Peer 独立有界 drain，轮转起点使洪水不能挤占另一人的预算。
void DrainPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host || r.retiring)
    return;
  for (unsigned step = 0; step < core::kMaximumParticipants - 1; ++step) {
    const unsigned ordinal =
        1 + static_cast<unsigned>((r.loop + step) %
                                  (core::kMaximumParticipants - 1));
    auto &p = r.Remote(ordinal).peer;
    if (!p.established || p.stop_requested)
      continue;
    net::ReceivedInput packet{};
    for (unsigned n = 0; n < 32 && r.network.TryReceive(packet, ordinal); ++n) {
      p.observed_sequence =
          (std::max)(p.observed_sequence, packet.input.command.sequence);
      const auto reason = Classify(r, ordinal, packet);
      if (reason != DiscardReason::Count) {
        const auto count = 1 + packet.input.redundancy_count;
        p.discarded += count;
        p.discards[static_cast<unsigned>(reason)] += count;
        if (!packet.ack_stamp_known)
          p.ack_unknown += count;
        p.discarded_sequence_floor = (std::max)(p.discarded_sequence_floor,
                                                packet.input.command.sequence);
        Trace("peer_discard",
              "ordinal=%u reason=%s commands=%u packet=%llu "
              "ack_generation=%llu participant_generation=%llu loop=%llu",
              ordinal, DiscardReasonName(reason), count, packet.packet_sequence,
              packet.acknowledged_generation, r.players[ordinal].Generation(),
              r.loop);
        continue;
      }
      const auto submit = [&](const core::InputCmdDraftV2 &cmd) noexcept {
        if (cmd.sequence <= p.discarded_sequence_floor) {
          ++p.discarded;
          ++p.discards[static_cast<unsigned>(DiscardReason::SequenceFloor)];
          return true;
        }
        const auto decision = r.players[ordinal].AcceptInput(
            cmd, r.players[ordinal].Generation(), r.clock.Completed());
#ifdef ISAACCS_E2E
        replica::stream::E2EInput(decision==core::Decision::Expired?"input-expired":"input-admit",{ordinal,0,0,cmd.state.edge_flags,r.players[ordinal].Generation(),cmd.sequence,cmd.target_tick,r.clock.Completed(),{cmd.state.move_x,cmd.state.move_y,cmd.state.aim_x,cmd.state.aim_y}});
#endif
        ++p.admitted;
        Trace("submit",
              "ordinal=%u generation=%llu sequence=%llu target=%llu loop=%llu "
              "packet=%llu session_generation=%llu decision=%u edge=%u "
              "move=%d,%d aim=%d,%d held=%u last_ack=%llu",
              ordinal, r.players[ordinal].Generation(), cmd.sequence,
              cmd.target_tick, r.loop, packet.packet_sequence,
              SessionGeneration(r), unsigned(decision), cmd.state.edge_flags,
              cmd.state.move_x, cmd.state.move_y, cmd.state.aim_x,
              cmd.state.aim_y, cmd.state.held_flags, packet.input.last_ack);
        if (decision == core::Decision::SequenceConflict) {
          RequestRemoteRetirement(r, ordinal, core::SessionCause::PeerProtocol);
          return false;
        }
        return true;
      };
      bool valid = true;
      for (unsigned i = 0; i < packet.input.redundancy_count && valid; ++i)
        valid = submit(packet.input.redundant[i]);
      if (!valid || !submit(packet.input.command))
        break;
    }
  }
}
void PublishPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host)
    return;
  // 世界发布失败只撤复制来源；输入scheduler与Authority提交不等待编码或网络。
  try {
    if(r.network.NeedsBaseline())RequestLiveBaseline();
    auto publication=std::make_shared<net::live::Publication>();
    publication->source=CurrentLiveSource();
    publication->baseline=CurrentLiveBaseline();publication->latest=CurrentLiveWorld();
    publication->bodies=CurrentLiveBodies();r.network.PublishWorld(std::move(publication));
  }catch(...){r.network.PublishWorld({});}
  std::uint32_t roster = 0;
  for (unsigned i = 0; i < core::kMaximumParticipants; ++i)
    if (r.session.HasParticipant(i))
      roster |= 1u << i;
  for (unsigned ordinal = 1; ordinal < core::kMaximumParticipants; ++ordinal) {
    auto &p = r.Remote(ordinal).peer;
    if (!p.established || p.stop_requested)
      continue;
    const auto progress = r.players[ordinal].Progress(r.clock.Completed());
    net::protocol::Ack ack;
    ack.roster_mask = roster;
    ack.server_tick = r.clock.Completed();
    ack.server_sample_ms = r.committed_sample_ms?r.committed_sample_ms:GetTickCount64();
    ack.authority_kind = r.committed_kind;
    ack.authority_paused = (r.blocks & Pause)!=0;
    ack.client_id = progress.client;
    ack.accepted = progress.last_accepted_sequence;
    ack.processed = progress.last_processed_sequence;
    ack.processed_tick = progress.last_processed_tick;
    ack.processed_source = progress.last_processed_source_sequence;
    if (r.session.AcceptsInput() && !r.blocks &&
        r.players[ordinal].BoundValid()) {
      p.advertised_generation = progress.generation;
      ack.input_enabled = true;
    }
    ack.generation = p.advertised_generation;
    ack.rejected_count = progress.rejected_count;
    ack.session_state = r.session.Status().state;
    for (unsigned i = 3; i < progress.rejections.size(); ++i)
      if (progress.rejections[i])
        ack.rejection_mask |= 1u << i;
    const bool queued = r.network.QueueAck(ack);
    if (!queued)
      ++p.ack_dropped;
    ++p.ack_publications;
    if (p.ack_publications == 1 || p.ack_publications % 30 == 0)
      Trace("ack",
            "event=publication ordinal=%u generation=%llu completed=%llu "
            "accepted=%llu processed=%llu source=%llu roster=%u queued=%u "
            "loop=%llu",
            ordinal, ack.generation, ack.server_tick, ack.accepted,
            ack.processed, ack.processed_source, roster, unsigned(queued),
            r.loop);
  }
}
void FinishRemoteRetirements(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host)
    return;
  for (unsigned ordinal = core::kMaximumParticipants - 1; ordinal > 0;
       --ordinal) {
    auto &remote = r.Remote(ordinal);
    if ((!r.retiring && !remote.retiring) || remote.retired ||
        remote.cleanup_failed)
      continue;
    if (remote.life.joined && !r.run_torn_down && !remote.life.retire_ready)
      CaptureRetirementSnapshot(r, ordinal);
    remote.device.Neutralize();
    const bool device_ok = remote.device.RemoveOnce();
    const bool player_ok = device_ok && RetireJoinedPlayer(r, ordinal);
    if (!device_ok || !player_ok) {
      remote.cleanup_failed = true;
      Trace("retirement_error", "ordinal=%u device=%u player=%u loop=%llu",
            ordinal, device_ok, player_ok, r.loop);
      continue;
    }
    r.players[ordinal].Retire();
    r.session.RemoveParticipant(ordinal);
    remote.retired = true;
    ResetJoinStateAfterRetirement(r, ordinal);
    Trace("peer", "event=retired ordinal=%u participants=%zu loop=%llu",
          ordinal, r.session.ParticipantCount(), r.loop);
  }
}
bool StopPeer(SessionRuntime &r) noexcept {
  if (r.mode != core::SessionMode::Host)
    return true;
  const auto result = r.network.Stop(ByeFor(r.retirement_cause), 1000);
  for (auto &remote : r.remotes) {
    remote.peer.stop_requested = true;
    remote.peer.stop_finished = result == net::StopResult::Stopped;
    remote.peer.stop_failed = !remote.peer.stop_finished;
  }
  return result == net::StopResult::Stopped;
}
} // namespace cs::game
