#include "game/participants/remote_life.h"
#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "game/participants/device_k.h"
#include "game/authority/lifecycle.h"
#include "game/participants/peer.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/observation/world_bridge.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

// 每条原生归属诊断明确参与者，不再把所有远端标成旧的B。
namespace cs::game {
namespace {
// 只追加参与者标签，不改变原事件字段或格式参数；不在查询路径动态分配。
template <class... Args>
void TraceRemote(unsigned ordinal, const char *topic, const char *format,
                 Args... args) noexcept {
  char tagged[3500]{};
  _snprintf_s(tagged, sizeof(tagged), _TRUNCATE, "ordinal=%u %s", ordinal,
              format);
  ::cs::Trace(topic, tagged, args...);
}
} // namespace

// 重绑原因粗分类（room/pause/stage/revive 等）：原始 reason 仍完整保留，
// 这里只给 status/trace 一个稳定的分类值。
const char *EligibilityReasonClass(const char *reason) noexcept {
  if (!reason || !reason[0])
    return "stable";
  if (std::strstr(reason, "revive"))
    return "revive";
  if (std::strstr(reason, "room"))
    return "room";
  if (std::strstr(reason, "stage"))
    return "stage";
  if (std::strstr(reason, "pause"))
    return "pause";
  if (std::strstr(reason, "start"))
    return "start";
  if (std::strstr(reason, "life"))
    return "life";
  return "other";
}
namespace {
// 状态迁移只在实际事件时输出一条归属快照。指针仅作本地诊断值，不进入网络，
// simulation_tick 区分 tick 中途观测与 completed 之后的稳定观测。
void TraceLifeOwnership(const SessionRuntime &r, const char *event,
                        const Topology &remote, unsigned ordinal) noexcept {
  const auto &life = r.Remote(ordinal).life;
  const auto owner = TopOwner();
  TraceRemote(
      ordinal, "life_ownership",
      "event=%s status=%s session_generation=%llu participant_generation=%llu "
      "bound=%u death=%llu deaths=%llu revives=%llu rebinds=%llu "
      "player=%p device=%p key=%d character=%d known=%u unique=%u "
      "valid=%u exists=%u dead=%u ghost=%u removed=%u player_count=%u fault=%s "
      "current_player=%p current_device=%p current_key=%d "
      "ownership_generation=%llu ownership_confirmed=%u "
      "room=%p stage=%u index=%u dimension=%u loop=%llu completed=%llu "
      "simulation_tick=%llu epoch=%llu owner=%u owner_token=%llu",
      event, RemoteLifeStatusName(life.status), SessionGeneration(r),
      r.players[ordinal].Generation(),
      unsigned(r.players[ordinal].BoundValid()), life.death_event, life.deaths,
      life.revives, life.rebinds, remote.pair.player,
      remote.pair.device, remote.pair.key, remote.character,
      unsigned(remote.known), unsigned(remote.unique), unsigned(remote.valid),
      unsigned(remote.exists), unsigned(remote.dead), unsigned(remote.ghost),
      unsigned(remote.removed), remote.player_count,
      TopologyFaultName(remote.fault), life.current.player, life.current.device,
      life.current.key, life.current.generation,
      unsigned(life.current.confirmed), remote.room.room, remote.room.stage,
      remote.room.index, remote.room.dimension, r.loop, r.clock.Completed(),
      r.pipeline.SimulationTick(), r.query_epoch, unsigned(owner),
      OwnerToken(owner));
}
// 只在主线程调用；把当前确认归属复制成值，不跨原生调用保留可解引用指针。
RemoteOwnership MakeOwnership(SessionRuntime &r, const Topology &remote,
                              unsigned ordinal) noexcept {
  RemoteOwnership value;
  value.player = remote.pair.player;
  value.device = remote.pair.device
                     ? remote.pair.device
                     : r.Remote(ordinal).device.Object();
  value.key = remote.pair.key;
  value.character = remote.character;
  value.room = remote.room;
  value.confirmed = remote.unique;
  return value;
}
} // namespace
const char *RemoteLifeStatusName(RemoteLifeStatus status) noexcept {
  switch (status) {
  case RemoteLifeStatus::None:
    return "None";
  case RemoteLifeStatus::Alive:
    return "Alive";
  case RemoteLifeStatus::Lost:
    return "Lost";
  case RemoteLifeStatus::Revived:
    return "Revived";
  }
  return "UNKNOWN";
}
// 健康条件：Type/Variant/index/controller/唯一设备已由 ReadTopology 校验；
// 这里再要求生命可用与角色与本轮指定角色一致。
bool RemoteHealthEligible(const SessionRuntime &r, const Topology &remote,
                          unsigned ordinal) noexcept {
  if (r.mode != core::SessionMode::Host)
    return false;
  if (!remote.known || !remote.unique || !remote.Alive())
    return false;
  if (remote.fault != TopologyFault::None)
    return false;
  return remote.pair.key >= 0;
}
// 撤销 B 的绑定一次：Participant::Invalidate 已递增 generation，这里不再加一。
// 重复调用在未绑定时不产生第二次失效，也不清空当前归属。
void InvalidateRemoteBinding(SessionRuntime &r, const char *reason,
                             unsigned ordinal) noexcept {
  WorldRevokePlayer(ordinal);
  // 先推进观测 epoch：失效前后的查询不进入同一比较段。
  ++r.query_epoch;
  if (r.loading.active)
    ++r.loading.invalidations;
  if (r.players[ordinal].BoundValid()) {
    r.controlled_player[ordinal] = r.players[ordinal].Bound().player;
    r.players[ordinal].Invalidate();
    TraceRemote(
        ordinal, "binding",
        "bound=0 generation=%llu reason=%s epoch=%llu loop=%llu "
        "reason_class=%s session_generation=%llu participant_generation=%llu",
        r.players[ordinal].Generation(), reason, r.query_epoch, r.loop,
        EligibilityReasonClass(reason), SessionGeneration(r),
        r.players[ordinal].Generation());
  }
  r.Remote(ordinal).life.pending_reason = reason;
  DiscardPeerWindow(r, ordinal);
}
// Join 正常返回并确认完整 B 之后建立首次生命记录与当前归属。
void NoteRemoteJoined(SessionRuntime &r, const Topology &remote,
                      unsigned ordinal) noexcept {
  auto &life = r.Remote(ordinal).life;
  life.status = RemoteLifeStatus::Alive;
  life.joined = true;
  life.awaiting_rebind = false;
  life.death_event = 0;
  life.current = MakeOwnership(r, remote, ordinal);
  life.current.generation = r.players[ordinal].Generation();
  life.conflict_reason = "";
  ++r.query_epoch;
  TraceRemote(
      ordinal, "life",
      "event=join status=%s character=%d key=%d player=%p generation=%llu "
      "loop=%llu",
      RemoteLifeStatusName(life.status), remote.character, remote.pair.key,
      remote.pair.player, life.current.generation, r.loop);
  TraceLifeOwnership(r, "join", remote, ordinal);
}
// 稳定且未阻断的 Authority 入口：重新读取唯一 exact pair 后才绑定。
// 不放宽身份条件，不重建 K，不再走 Join，不改变 ClientId->ordinal。
bool BindRevivedRemote(SessionRuntime &r, bool after_revival,
                       unsigned ordinal) noexcept {
  if (r.mode != core::SessionMode::Host || !r.Remote(ordinal).join.complete ||
      r.finished || r.retiring)
    return false;
  if (r.blocks || r.loop <= r.binding_defer_through[ordinal] ||
      r.control_quarantined[ordinal] || r.players[ordinal].BoundValid())
    return false;
  // 复活动作仍在原生栈内时不得绑定/推进代次（ST2：ghost 先于下游被清零）。
  if (PersistenceWindowActive(PersistenceWindowKind::BRevive, ordinal) &&
      !PersistenceWindowCallReturned(PersistenceWindowKind::BRevive, ordinal))
    return false;
  const auto remote = ReadTopology(ordinal, r.Remote(ordinal).device.Object());
  if (!RemoteHealthEligible(r, remote, ordinal))
    return false;
  if (!r.players[ordinal].Bind(remote.pair))
    return false;
  r.controlled_player[ordinal] = remote.pair.player;
  auto &life = r.Remote(ordinal).life;
  const RemoteOwnership previous = life.current;
  life.current = MakeOwnership(r, remote, ordinal);
  life.current.generation = r.players[ordinal].Generation();
  life.awaiting_rebind = false;
  if (after_revival && life.status == RemoteLifeStatus::Revived)
    life.status = RemoteLifeStatus::Alive;
  ++life.rebinds;
  ++r.query_epoch;
  DiscardPeerWindow(r, ordinal);
  const char *reason =
      after_revival
          ? "revive"
          : (life.pending_reason[0] ? life.pending_reason : "stable_authority");
  TraceRemote(
      ordinal, "binding",
      "bound=1 generation=%llu loop=%llu reason=%s reason_class=%s "
      "revived=%u rebinds=%llu previous_player=%p player=%p character=%d "
      "key=%d session_generation=%llu participant_generation=%llu epoch=%llu",
      r.players[ordinal].Generation(), r.loop, reason,
      EligibilityReasonClass(reason), after_revival, life.rebinds,
      previous.player, remote.pair.player, remote.character, remote.pair.key,
      SessionGeneration(r), r.players[ordinal].Generation(), r.query_epoch);
  TraceRemote(ordinal, "life",
              "event=rebind status=%s reason=%s reason_class=%s deaths=%llu "
              "revives=%llu rebinds=%llu generation=%llu loop=%llu",
              RemoteLifeStatusName(life.status), reason,
              EligibilityReasonClass(reason), life.deaths, life.revives,
              life.rebinds, life.current.generation, r.loop);
  TraceLifeOwnership(r, "rebind", remote, ordinal);
  // 本次失效已被消费；下一次重绑若没有新的失效原因则记 stable。
  life.pending_reason = "";
  return true;
}
RemoteLifeAction ObserveRemoteLife(SessionRuntime &r, const Topology &host,
                                   const Topology &remote,
                                   unsigned ordinal) noexcept {
  auto &life = r.Remote(ordinal).life;
  if (r.mode != core::SessionMode::Host || !r.Remote(ordinal).join.complete ||
      r.finished)
    return RemoteLifeAction::None;
  const bool host_ok = host.known && host.run && host.unique && host.Alive();
  // 1. 暂时不可读：只保持中性，绝不冒充“已被移除”。
  if (!remote.known || !remote.run) {
    ++life.unreadable;
    return RemoteLifeAction::Invalidate;
  }
  // 2. 已确认的移除/归属矛盾（只在 A 自身可读且健康时判断）。
  if (host_ok) {
    const bool count_anomaly =
        remote.reason && std::strcmp(remote.reason, "group_root_missing") == 0;
    const bool identity_conflict = remote.fault == TopologyFault::Ambiguous ||
                                   remote.fault == TopologyFault::Ownership;
    if (count_anomaly || identity_conflict) {
      const char *reason = count_anomaly ? "control_group_missing"
                                         : "control_group_identity_conflict";
      life.conflict_reason = reason;
      ++life.conflicts;
      TraceLifeOwnership(r, "conflict", remote, ordinal);
      TraceRemote(
          ordinal, "life",
          "event=conflict status=%s reason=%s count=%u fault=%s character=%d "
          "expected=%d loop=%llu",
          RemoteLifeStatusName(life.status), reason, remote.player_count,
          TopologyFaultName(remote.fault), remote.character,
          r.Remote(ordinal).join.character, r.loop);
      return RemoteLifeAction::PeerLost;
    }
  }
  const bool alive = remote.Alive();
  switch (life.status) {
  case RemoteLifeStatus::None:
    // Join 已完成却还没有生命记录：按当前读取建立一条，不伪造死亡历史。
    life.status = RemoteLifeStatus::Alive;
    life.joined = true;
    life.current = MakeOwnership(r, remote, ordinal);
    life.current.generation = r.players[ordinal].Generation();
    return RemoteLifeAction::None;
  case RemoteLifeStatus::Alive:
    if (!remote.unique)
      return RemoteLifeAction::Invalidate;
    if (!alive) {
      // 首次读到 dead || ghost：进入 Lost，记录一次死亡事件并撤销 B 的绑定。
      life.status = RemoteLifeStatus::Lost;
      ++life.deaths;
      ++life.death_event;
      life.lost_loop = r.loop;
      life.lost_tick = r.clock.Completed();
      life.awaiting_rebind = false;
      TraceRemote(
          ordinal, "life",
          "event=lost status=Lost death=%llu dead=%u ghost=%u removed=%u "
          "player=%p generation=%llu loop=%llu tick=%llu",
          life.death_event, unsigned(remote.dead), unsigned(remote.ghost),
          unsigned(remote.removed), remote.pair.player,
          r.players[ordinal].Generation(), r.loop, r.clock.Completed());
      InvalidateRemoteBinding(r, "remote_life_lost", ordinal);
      TraceLifeOwnership(r, "lost", remote, ordinal);
      // b_death 窗口通常由 Kill 观察打开；没有 Kill 记录时在这里补开，
      // 保持“先请求、后结果”的真实顺序，不伪造已发生的原生调用。
      if (!PersistenceWindowActive(PersistenceWindowKind::BDeath, ordinal))
        OpenPersistenceWindow(PersistenceWindowKind::BDeath, "observed_dead",
                              life.death_event, ordinal);
      if (remote.ghost) {
        // 只有相关原生调用已正常返回，“ghost 已观察”才算正常收口。
        const bool returned =
            PersistenceWindowCallNormal(PersistenceWindowKind::BDeath, ordinal);
        ClosePersistenceWindow(PersistenceWindowKind::BDeath,
                               returned ? "ghost_observed"
                                        : "ghost_before_call_return",
                               returned, ordinal);
      }
      return RemoteLifeAction::Invalidate;
    }
    // 健康时若地址/设备关联变化，撤销并在稳定入口重绑，不把旧地址当身份。
    if (life.current.confirmed && r.players[ordinal].BoundValid() &&
        !SameControlGroup(r.topology[ordinal], remote)) {
      InvalidateRemoteBinding(r, "remote_pair_changed", ordinal);
      return RemoteLifeAction::Invalidate;
    }
    return RemoteLifeAction::None;
  case RemoteLifeStatus::Lost:
    if (!remote.unique || !alive) {
      // 仍 dead/ghost：不再次 Invalidate，也不再次计死亡。
      if (life.death_event && remote.ghost &&
          PersistenceWindowActive(PersistenceWindowKind::BDeath, ordinal)) {
        const bool returned =
            PersistenceWindowCallNormal(PersistenceWindowKind::BDeath, ordinal);
        ClosePersistenceWindow(PersistenceWindowKind::BDeath,
                               returned ? "ghost_observed"
                                        : "ghost_before_call_return",
                               returned, ordinal);
      }
      return RemoteLifeAction::Invalidate;
    }
    // ST2：RevivePlayerGhost 在下游调用之前就清了 ghost，因此动作正常返回前
    // 不能仅凭 ghost=false 进入 Revived，更不能借此推进代次；B 保持中性。
    if (PersistenceWindowActive(PersistenceWindowKind::BRevive, ordinal) &&
        !PersistenceWindowCallReturned(PersistenceWindowKind::BRevive,
                                       ordinal)) {
      TraceRemote(
          ordinal, "life",
          "event=revive_pending status=Lost ghost=0 player=%p loop=%llu "
          "tick=%llu",
          remote.pair.player, r.loop, r.clock.Completed());
      return RemoteLifeAction::Invalidate;
    }
    if (!RemoteHealthEligible(r, remote, ordinal))
      return RemoteLifeAction::Invalidate;
    // 健康与身份全部满足：进入 Revived，但此时仍不接收 B 输入。
    life.status = RemoteLifeStatus::Revived;
    life.awaiting_rebind = true;
    life.revived_loop = r.loop;
    life.revived_tick = r.clock.Completed();
    ++life.revives;
    TraceLifeOwnership(r, "revived", remote, ordinal);
    // 复活动作已返回且观察到健康：按 §5.1 收口 b_revive 窗口；随后的 stable
    // rebind 由 life/rebind 事件与 revived_loop 关联，不以窗口长期占用代替。
    if (PersistenceWindowActive(PersistenceWindowKind::BRevive, ordinal)) {
      const bool returned =
          PersistenceWindowCallNormal(PersistenceWindowKind::BRevive, ordinal);
      ClosePersistenceWindow(PersistenceWindowKind::BRevive,
                             returned ? "health_observed"
                                      : "health_observed_call_abnormal",
                             returned, ordinal);
    }
    TraceRemote(
        ordinal, "life",
        "event=revived status=Revived death=%llu revives=%llu player=%p "
        "character=%d key=%d loop=%llu tick=%llu",
        life.death_event, life.revives, remote.pair.player, remote.character,
        remote.pair.key, r.loop, r.clock.Completed());
    return RemoteLifeAction::None;
  case RemoteLifeStatus::Revived:
    if (!alive) {
      // 尚未重绑又再次 dead/ghost：回 Lost，保留同一死亡事件，不重复计数。
      life.status = RemoteLifeStatus::Lost;
      life.awaiting_rebind = false;
      TraceLifeOwnership(r, "lost_again", remote, ordinal);
      TraceRemote(ordinal, "life",
                  "event=lost_again status=Lost death=%llu dead=%u ghost=%u "
                  "loop=%llu",
                  life.death_event, unsigned(remote.dead),
                  unsigned(remote.ghost), r.loop);
      return RemoteLifeAction::Invalidate;
    }
    return RemoteLifeAction::None;
  }
  return RemoteLifeAction::None;
}
// 移出 K 前保存只读关联：Game/PlayerManager/A/B/controller
// 都是值，不跨原生调用解引用。
bool CaptureRetirementSnapshot(SessionRuntime &r, unsigned ordinal) noexcept {
  auto &snap = r.Remote(ordinal).life.retirement;
  snap = {};
  r.Remote(ordinal).life.retire_ready = false;
  if (r.mode != core::SessionMode::Host || !r.Remote(ordinal).join.complete)
    return false;
  const auto host = ReadTopology(0);
  const auto remote = ReadTopology(ordinal, r.Remote(ordinal).device.Object());
  if (!host.known || !host.run || !host.unique || !host.pair.player)
    return false;
  // RemoveCoPlayer 的 this 是 Game+PlayerManager
  // 内嵌对象，不能传管理菜单/输入的 全局 Manager。先复核其活动 vector
  // 恰好对应当前 A/B，避免把其它对象当列表。
  const auto game_address = reinterpret_cast<uintptr_t>(host.room.game);
  PlayerSlot current;
  if (host.room.game != GamePointer() || !ReadPlayerSlot(ordinal, current) ||
      current.player != remote.pair.player ||
      current.controller != r.Remote(ordinal).device.Id())
    return false;
  for (unsigned i = 0; i < remote.body_count; ++i)
    if (remote.bodies[i].active && remote.bodies[i].roster)
      snap.bodies[snap.body_count++] = remote.bodies[i];
  snap.game = host.room.game;
  snap.player_manager =
      reinterpret_cast<void *>(game_address + offset::PlayerManager);
  snap.host = host.pair.player;
  snap.remote = remote.pair.player;
  snap.device = r.Remote(ordinal).device.Object();
  snap.host_key = host.pair.key;
  snap.remote_key = remote.pair.key;
  snap.host_character = host.character;
  snap.remote_character = remote.character;
  snap.room = host.room;
  snap.generation = r.players[ordinal].Generation();
  snap.confirmed = remote.unique && remote.pair.player &&
                   remote.pair.key == r.Remote(ordinal).device.Id();
  r.Remote(ordinal).life.retire_ready = snap.confirmed;
  TraceRemote(
      ordinal, "retirement",
      "event=snapshot ok=%u player=%p host=%p key=%d character=%d count=%u "
      "game=%p player_manager=%p "
      "loop=%llu",
      snap.confirmed, snap.remote, snap.host, snap.remote_key,
      snap.remote_character, remote.player_count, snap.game,
      snap.player_manager, r.loop);
  return snap.confirmed;
}
// K 已离开设备 vector：用受限读取重新确认当前列表中的 B 与快照一致。
RetirementConfirm ConfirmRetirementPlayer(SessionRuntime &r, void *&player,
                                          unsigned ordinal) noexcept {
  player = nullptr;
  const auto &snap = r.Remote(ordinal).life.retirement;
  if (!snap.confirmed || !r.Remote(ordinal).life.retire_ready)
    return RetirementConfirm::Unknown;
  PlayerSlot slot;
  const bool read = ReadPlayerSlot(ordinal, slot);
  if (slot.known && slot.fault == TopologyFault::NotRun) {
    TraceRemote(ordinal, "retirement",
                "event=confirm result=absent reason=not_in_run");
    return RetirementConfirm::Absent;
  }
  if (!read) {
    // vector 不可读也使用 PlayerList 分类；只有明确的 slot 缺席才算已移除。
    if (slot.fault == TopologyFault::PlayerList &&
        std::strcmp(slot.reason, "player_slot_missing") == 0) {
      TraceRemote(
          ordinal, "retirement",
          "event=confirm result=absent reason=player_slot_missing count=%u",
          slot.count);
      return RetirementConfirm::Absent;
    }
    TraceRemote(ordinal, "retirement",
                "event=confirm result=unknown reason=slot_unreadable fault=%s",
                TopologyFaultName(slot.fault));
    return RetirementConfirm::Unknown;
  }
  // 已确认被原生移除时跳过移除调用，不把缺席当成可以移除。
  if (slot.removed) {
    TraceRemote(ordinal, "retirement",
                "event=confirm result=absent reason=already_removed count=%u "
                "removed=%u",
                slot.count, unsigned(slot.removed));
    return RetirementConfirm::Absent;
  }
  if (!slot.identity || slot.player != snap.remote ||
      slot.controller != snap.remote_key ||
      slot.character != snap.remote_character) {
    TraceRemote(
        ordinal, "retirement",
        "event=confirm result=conflict player=%p expected=%p controller=%d "
        "expected=%d identity=%u",
        slot.player, snap.remote, slot.controller, snap.remote_key,
        unsigned(slot.identity));
    return RetirementConfirm::Conflict;
  }
  // K 已移除也不要求其仍在设备列表；复读当前 Game、A 和同一内嵌 manager，
  // 身份变化时停止清理，不能对快照里的旧 Game/PlayerManager 发出原生调用。
  PlayerSlot host;
  if (GamePointer() != snap.game ||
      reinterpret_cast<uintptr_t>(snap.player_manager) !=
          reinterpret_cast<uintptr_t>(snap.game) + offset::PlayerManager ||
      !ReadPlayerSlot(0, host) || !host.identity || host.player != snap.host ||
      host.controller != snap.host_key ||
      host.character != snap.host_character) {
    TraceRemote(ordinal, "retirement",
                "event=confirm result=conflict reason=player_manager_changed");
    return RetirementConfirm::Conflict;
  }
  player = slot.player;
  TraceRemote(ordinal, "retirement",
              "event=confirm result=ready player=%p key=%d "
              "game=%p player_manager=%p loop=%llu",
              player, slot.controller, snap.game, snap.player_manager, r.loop);
  return RetirementConfirm::Ready;
}
} // namespace cs::game
