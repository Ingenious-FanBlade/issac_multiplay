#include "game/join.h"
#include "binding/offsets.h"
#include "game/context.h"
#include "game/lifecycle.h"
#include "game/state.h"
#include "game/tick.h"
#include <exception>

namespace cs::game {
namespace {
bool ValidRow(void *manager, void *row) noexcept {
  const auto first =
      reinterpret_cast<uintptr_t>(manager) + offset::JoinRowCursor;
  const auto cursor = reinterpret_cast<uintptr_t>(row);
  return manager && cursor >= first &&
         cursor - first < offset::JoinRows * offset::JoinRowStride &&
         (cursor - first) % offset::JoinRowStride == 0;
}
bool CharacterAvailable(void *manager, int character) noexcept {
  uintptr_t begin = 0, end = 0;
  if (character < 0 || !ReadAt(manager, offset::CharacterBegin, begin) ||
      !ReadAt(manager, offset::CharacterEnd, end) || end < begin ||
      (end - begin) % 4 || (end - begin) / 4 > 1024)
    return false;
  for (auto cursor = begin; cursor < end; cursor += 4) {
    int role = -1;
    if (!ReadMemory(reinterpret_cast<void *>(cursor), &role, sizeof(role)))
      return false;
    if (role == character)
      return true;
  }
  return false;
}
void FailJoin(SessionRuntime &r, const char *reason) noexcept {
  r.join.state = JoinState::Failed;
  FailRuntime(reason);
}
} // namespace
const char *JoinStateName(JoinState state) noexcept {
  switch (state) {
  case JoinState::Waiting:
    return "Waiting";
  case JoinState::Request:
    return "Request";
  case JoinState::Confirm:
    return "Confirm";
  case JoinState::Joined:
    return "Joined";
  case JoinState::Failed:
    return "Failed";
  }
  return "UNKNOWN";
}
JoinScope::JoinScope(void *manager) noexcept
    : lease_(HostBusy()), exceptions_(std::uncaught_exceptions()) {
  auto *r = Runtime();
  if (!r || r->mode != core::SessionMode::Host || r->finished || r->retiring)
    return;
  if (!lease_.Ready()) {
    FailJoin(*r, "join_input_lock_unavailable");
    return;
  }
  if (r->join.scope) {
    FailJoin(*r, "join_scope_reentry");
    return;
  }
  if (!r->peer.established ||
      r->session.Status().state != core::SessionState::Warmup ||
      r->device.State() != DeviceKState::Registered || r->join.complete)
    return;
  const auto topology = ReadTopology();
  if (r->blocks || r->console_blocked || !topology.run || !topology.unique ||
      !topology.Alive() || topology.pause || topology.starting ||
      topology.room_transition || topology.stage_transition ||
      !r->players[0].BoundValid())
    return;
  // ProcessInput 的 Join 在 Manager::Update 中、Game::Update 之前，不能要求
  // Game owner。
  if (TopOwner() != Owner::Join || !HasOwner(Owner::Manager) ||
      HasOwner(Owner::Start) || HasOwner(Owner::Restore) ||
      HasOwner(Owner::Door) || HasOwner(Owner::Lifecycle))
    return;
  const auto expected = reinterpret_cast<unsigned char *>(topology.room.game) +
                        offset::PlayerManager;
  if (manager != expected || topology.player_count != 1 ||
      !(topology.pair == r->players[0].Bound())) {
    FailJoin(*r, "join_initial_topology");
    return;
  }
  auto &j = r->join;
  if (j.passes &&
      (j.manager != manager || j.host_character != topology.character ||
       !(j.host == topology.pair) || !(j.room == topology.room))) {
    FailJoin(*r, "join_identity_changed");
    return;
  }
  j.manager = manager;
  j.host = topology.pair;
  j.host_character = topology.character;
  j.room = topology.room;
  j.state = j.passes ? JoinState::Confirm : JoinState::Request;
  j.action = j.passes ? 14 : 19;
  j.scope = active_ = true;
  Trace("join", "event=begin pass=%u action=%d key=%d loop=%llu", j.passes + 1,
        j.action, r->device.Id(), r->loop);
}
// 不按查询次数消耗；原生自动确认在 P1 创建时，返回后立即收口，不再发 P2。
JoinScope::~JoinScope() noexcept {
  if (!active_)
    return;
  auto *r = Runtime();
  if (!r)
    return;
  auto &j = r->join;
  j.scope = false;
  j.action = -1;
  ++j.passes;
  if (exceptions_ != std::uncaught_exceptions()) {
    FailJoin(*r, "join_exception");
    return;
  }
  if (r->retiring)
    return;
  const auto a = ReadTopology(), b = ReadTopology(1, r->device.Object());
  if (j.creation_calls && b.unique && b.pair.key == r->device.Id())
    j.joined_player = b.pair.player;
  if (!a.unique || !a.Alive() || a.character != j.host_character ||
      !(a.pair == j.host) || !(a.room == j.room)) {
    FailJoin(*r, "join_host_changed");
    return;
  }
  if (a.player_count == 2) {
    if (j.creation_calls != 1 || !b.unique || !b.Alive() ||
        b.character != j.character || b.pair.key != r->device.Id() ||
        !r->players[1].Bind(b.pair)) {
      FailJoin(*r, "join_result_rejected");
      return;
    }
    j.complete = true;
    j.state = JoinState::Joined;
    r->topology[1] = b;
    r->controlled_player[1] = b.pair.player;
    DiscardPeerWindow(*r);
    Trace("binding",
          "ordinal=1 bound=1 generation=%llu reason=join_return loop=%llu",
          r->players[1].Generation(), r->loop);
    Trace("join", "event=complete passes=%u role=%d key=%d player=%p", j.passes,
          b.character, b.pair.key, b.pair.player);
    return;
  }
  if (a.player_count != 1 || j.passes != 1 || j.creation_calls) {
    FailJoin(*r, "join_player_count");
    return;
  }
  unsigned matches = 0;
  for (unsigned i = 0; i < offset::JoinRows; ++i) {
    int key = -1;
    if (!ReadAt(j.manager, offset::JoinRowKey + i * offset::JoinRowStride,
                key)) {
      FailJoin(*r, "join_row_unreadable");
      return;
    }
    if (key == r->device.Id())
      ++matches;
  }
  if (matches != 1) {
    FailJoin(*r, "join_row_not_unique");
    return;
  }
  j.state = JoinState::Confirm;
  Trace("join", "event=request_return row_matches=%u loop=%llu", matches,
        r->loop);
}
bool JoinQuery(void *device, int action, QueryKind query,
               float &result) noexcept {
  auto *r = Runtime();
  if (!r || !r->join.scope || device != r->device.Object())
    return false;
  ++r->join.query_count;
  result = !r->retiring && !r->blocks && query == QueryKind::Triggered &&
                   action == r->join.action
               ? 1.0f
               : 0.0f;
  return true;
}
// 原 predicate 已由裸 shim 恰好调用一次；此处只决定自有 key 的结果。
bool JoinMode(bool original, void *manager, int key, void *row, unsigned site) noexcept {
  auto *r = Runtime();
  if (!r || !r->join.scope || key != r->device.Id())
    return original;
  ++r->join.mode_calls;
  Trace("join", "event=mode site=%u original=%u result=0 native_calls=1 key=%d loop=%llu",
        site, original, key, r->loop);
  int row_key = -1;
  if (manager != r->join.manager || (row && !ValidRow(manager, row))) {
    FailJoin(*r, "join_mode_context");
    return false;
  }
  if (row && (!ReadMemory(static_cast<unsigned char *>(row) - 8, &row_key, 4) ||
              row_key != key))
    FailJoin(*r, "join_mode_row");
  if (original)
    ++r->join.baby_overrides;
  return false;
}
// CALL shim 只改唯一 role 参数；不写 selected、角色表、run flags 或存档。
bool JoinCreate(void *manager, void *row, int *role) noexcept {
  int key = -1;
  if (!IsMain()) {
    ForeignCall();
    return true;
  }
  if (!row || !ReadMemory(static_cast<unsigned char *>(row) - 8, &key, 4)) {
    if (HostBusy()) {
      FailRuntime("join_creation_row_unknown");
      return false;
    }
    return true;
  }
  auto *r = Runtime();
  if (!IsDeviceKId(key))
    return true;
  if (!r || r->mode != core::SessionMode::Host || key != r->device.Id())
    return false;
  if (!r->join.scope || r->retiring || manager != r->join.manager ||
      !ValidRow(manager, row) || !role || r->join.creation_calls ||
      !CharacterAvailable(manager, r->join.character)) {
    FailJoin(*r, "join_creation_rejected");
    return false;
  }
  ++r->join.creation_calls;
  *role = r->join.character;
  Trace("join", "event=create role=%d key=%d calls=%u loop=%llu", *role, key,
        r->join.creation_calls, r->loop);
  return true;
}
// 仅用当前 Game/manager/完整 B 的再次观察调用一次原生移除；teardown 不读旧 B。
bool RetireJoinedPlayer(SessionRuntime &r) noexcept {
  PersistenceOperation persistence("player_remove");
  if (r.run_torn_down || !r.join.joined_player)
    return true;
  if (r.join.remove_attempted)
    return false;
  InputLease lease(true);
  if (!lease.Ready())
    return false;
  const auto a = ReadTopology(), b = ReadTopology(1, r.device.Object());
  if (!a.run || a.room.game != r.join.room.game ||
      a.pair.player != r.join.host.player ||
      b.pair.player != r.join.joined_player || b.pair.key != r.device.Id() ||
      b.variant != 0 || a.player_count != 2)
    return false;
  r.players[1].Invalidate();
  r.join.remove_attempted = true;
  try {
    reinterpret_cast<void(__thiscall *)(void *, void *, bool)>(
        Base + offset::RemoveCoPlayer)(r.join.manager, r.join.joined_player,
                                       false);
  } catch (...) {
    return false;
  }
  const auto after = ReadTopology();
  if (!after.unique || after.player_count != 1 ||
      after.pair.player != r.join.host.player ||
      after.character != a.character || !(after.pair == a.pair))
    return false;
  r.join.joined_player = nullptr;
  Trace("join", "event=retired players=1 key=%d loop=%llu", r.device.Id(),
        r.loop);
  return true;
}
} // namespace cs::game
