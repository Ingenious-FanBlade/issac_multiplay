#include "game/join.h"
#include "binding/offsets.h"
#include "game/context.h"
#include "game/lifecycle.h"
#include "game/remote_life.h"
#include "game/state.h"
#include "game/tick.h"
#include "runtime.h"
#include <algorithm>
#include <cstdio>
#include <exception>

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
void FailJoin(SessionRuntime &r, const char *reason,
              unsigned ordinal) noexcept {
  r.Remote(ordinal).join.state = JoinState::Failed;
  r.Remote(ordinal).join.last = JoinState::Failed;
  r.Remote(ordinal).join.last_loop = r.loop;
  RequestRemoteRetirement(r, ordinal, core::SessionCause::PeerProtocol);
  TraceRemote(ordinal, "join", "event=failed ordinal=%u reason=%s loop=%llu",
              ordinal, reason, r.loop);
}
} // namespace
const char *JoinStateName(JoinState state) noexcept {
  switch (state) {
  case JoinState::None:
    return "None";
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
  for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
    const auto &remote = r->Remote(i);
    if (remote.peer.established && !remote.retiring && !remote.retired &&
        !remote.join.complete && remote.join.state != JoinState::Failed &&
        remote.device.State() == DeviceKState::Registered) {
      ordinal_ = i;
      break;
    }
  }
  if (!ordinal_)
    return;
  const unsigned ordinal = ordinal_;
  if (!lease_.Ready()) {
    FailJoin(*r, "join_input_lock_unavailable", ordinal);
    return;
  }
  if (r->Remote(ordinal).join.scope) {
    FailJoin(*r, "join_scope_reentry", ordinal);
    return;
  }
  if (!r->Remote(ordinal).peer.established || !r->session.AllowsBinding() ||
      r->Remote(ordinal).device.State() != DeviceKState::Registered ||
      r->Remote(ordinal).join.complete)
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
  if (manager != expected || !(topology.pair == r->players[0].Bound())) {
    FailJoin(*r, "join_initial_topology", ordinal);
    return;
  }
  auto &j = r->Remote(ordinal).join;
  uintptr_t before = 0;
  size_t before_count = 0;
  if (!VectorRange(topology.room.game, offset::PlayerManager, 64, before,
                   before_count) ||
      !ReadMemory(reinterpret_cast<void *>(before), j.before_roster.data(),
                  before_count * sizeof(void *))) {
    FailJoin(*r, "join_roster_unreadable", ordinal);
    return;
  }
  j.before_count = static_cast<unsigned>(before_count);
  if (j.passes &&
      (j.manager != manager || j.host_character != topology.character ||
       !(j.host == topology.pair) || !(j.room == topology.room))) {
    FailJoin(*r, "join_identity_changed", ordinal);
    return;
  }
  j.manager = manager;
  j.host = topology.pair;
  j.host_character = topology.character;
  j.room = topology.room;
  j.state = j.passes ? JoinState::Confirm : JoinState::Request;
  j.action = j.passes ? 14 : 19;
  r->join_ordinal = ordinal;
  j.scope = active_ = true;
  TraceRemote(ordinal, "join", "event=begin pass=%u action=%d key=%d loop=%llu",
              j.passes + 1, j.action, r->Remote(ordinal).device.Id(), r->loop);
}
// 不按查询次数消耗；原生自动确认在 P1 创建时，返回后立即收口，不再发 P2。
JoinScope::~JoinScope() noexcept {
  if (!active_)
    return;
  auto *r = Runtime();
  if (!r)
    return;
  const unsigned ordinal = ordinal_;
  r->join_ordinal = 0;
  auto &j = r->Remote(ordinal).join;
  j.scope = false;
  j.action = -1;
  ++j.passes;
  if (exceptions_ != std::uncaught_exceptions()) {
    FailJoin(*r, "join_exception", ordinal);
    return;
  }
  if (r->retiring)
    return;
  const auto a = ReadTopology();
  uintptr_t after = 0;
  size_t after_count = 0;
  std::array<void *, 64> roster{};
  if (!VectorRange(a.room.game, offset::PlayerManager, 64, after,
                   after_count) ||
      !ReadMemory(reinterpret_cast<void *>(after), roster.data(),
                  after_count * sizeof(void *))) {
    FailJoin(*r, "join_result_roster_unreadable", ordinal);
    return;
  }
  void *root = nullptr;
  for (size_t i = 0; i < after_count; ++i) {
    const auto candidate = roster[i];
    if (std::find(j.before_roster.begin(),
                  j.before_roster.begin() + j.before_count,
                  candidate) != j.before_roster.begin() + j.before_count)
      continue;
    int key = -1, character = -1;
    if (!ReadAt(candidate, offset::ControllerIndex, key) ||
        !ReadAt(candidate, offset::PlayerType, character)) {
      FailJoin(*r, "join_result_identity_unreadable", ordinal);
      return;
    }
    if (key == r->Remote(ordinal).device.Id() && character == j.character) {
      if (root) {
        FailJoin(*r, "join_multiple_roots", ordinal);
        return;
      }
      root = candidate;
    }
  }
  const auto b =
      ReadTopologyForRoot(ordinal, root, r->Remote(ordinal).device.Object());
  if (j.creation_calls && b.unique &&
      b.pair.key == r->Remote(ordinal).device.Id())
    j.joined_player = b.pair.player;
  if (!a.unique || !a.Alive() || a.character != j.host_character ||
      !(a.pair == j.host) || !(a.room == j.room)) {
    FailJoin(*r, "join_host_changed", ordinal);
    return;
  }
  if (root) {
    if (j.creation_calls != 1 || !b.unique || !b.Alive() ||
        b.character != j.character ||
        b.pair.key != r->Remote(ordinal).device.Id() ||
        !r->players[ordinal].Bind(b.pair)) {
      FailJoin(*r, "join_result_rejected", ordinal);
      return;
    }
    for (size_t i = 0; i < after_count; ++i)
      if (std::find(j.before_roster.begin(),
                    j.before_roster.begin() + j.before_count,
                    roster[i]) == j.before_roster.begin() + j.before_count &&
          !b.Contains(roster[i], false)) {
        FailJoin(*r, "join_unowned_body", ordinal);
        return;
      }
    j.complete = true;
    j.state = JoinState::Joined;
    j.last = JoinState::Joined;
    j.last_loop = r->loop;
    r->topology[ordinal] = b;
    r->controlled_player[ordinal] = b.pair.player;
    // 首次 Join 建立生命记录与当前归属；之后的重绑都走同一 ordinal。
    NoteRemoteJoined(*r, b, ordinal);
    DiscardPeerWindow(*r, ordinal);
    TraceRemote(ordinal, "binding",
                "bound=1 generation=%llu reason=join_return loop=%llu",
                r->players[ordinal].Generation(), r->loop);
    TraceRemote(ordinal, "join",
                "event=complete passes=%u role=%d key=%d player=%p", j.passes,
                b.character, b.pair.key, b.pair.player);
    return;
  }
  if (after_count != j.before_count || j.passes != 1 || j.creation_calls) {
    FailJoin(*r, "join_player_count", ordinal);
    return;
  }
  unsigned matches = 0;
  for (unsigned i = 0; i < offset::JoinRows; ++i) {
    int key = -1;
    if (!ReadAt(j.manager, offset::JoinRowKey + i * offset::JoinRowStride,
                key)) {
      FailJoin(*r, "join_row_unreadable", ordinal);
      return;
    }
    if (key == r->Remote(ordinal).device.Id())
      ++matches;
  }
  if (matches != 1) {
    FailJoin(*r, "join_row_not_unique", ordinal);
    return;
  }
  j.state = JoinState::Confirm;
  TraceRemote(ordinal, "join", "event=request_return row_matches=%u loop=%llu",
              matches, r->loop);
}
bool JoinQuery(void *device, int action, QueryKind query,
               float &result) noexcept {
  auto *r = Runtime();
  const unsigned ordinal = r ? r->join_ordinal : 0;
  if (!r || !ordinal || !r->Remote(ordinal).join.scope ||
      device != r->Remote(ordinal).device.Object())
    return false;
  ++r->Remote(ordinal).join.query_count;
  result = !r->retiring && !r->blocks && query == QueryKind::Triggered &&
                   action == r->Remote(ordinal).join.action
               ? 1.0f
               : 0.0f;
  return true;
}
// 原 predicate 已由裸 shim 恰好调用一次；此处只决定自有 key 的结果。
bool JoinMode(bool original, void *manager, int key, void *row,
              unsigned site) noexcept {
  auto *r = Runtime();
  const unsigned ordinal = r ? r->join_ordinal : 0;
  if (!r || !ordinal || !r->Remote(ordinal).join.scope ||
      key != r->Remote(ordinal).device.Id())
    return original;
  ++r->Remote(ordinal).join.mode_calls;
  TraceRemote(
      ordinal, "join",
      "event=mode site=%u original=%u result=0 native_calls=1 key=%d loop=%llu",
      site, original, key, r->loop);
  int row_key = -1;
  if (manager != r->Remote(ordinal).join.manager ||
      (row && !ValidRow(manager, row))) {
    FailJoin(*r, "join_mode_context", ordinal);
    return false;
  }
  if (row && (!ReadMemory(static_cast<unsigned char *>(row) - 8, &row_key, 4) ||
              row_key != key))
    FailJoin(*r, "join_mode_row", ordinal);
  if (original)
    ++r->Remote(ordinal).join.baby_overrides;
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
  const unsigned ordinal = r ? r->join_ordinal : 0;
  if (!IsDeviceKId(key))
    return true;
  if (!r || !ordinal || r->mode != core::SessionMode::Host ||
      key != r->Remote(ordinal).device.Id())
    return false;
  if (!r->Remote(ordinal).join.scope || r->retiring ||
      manager != r->Remote(ordinal).join.manager || !ValidRow(manager, row) ||
      !role || r->Remote(ordinal).join.creation_calls ||
      !CharacterAvailable(manager, r->Remote(ordinal).join.character)) {
    FailJoin(*r, "join_creation_rejected", ordinal);
    return false;
  }
  ++r->Remote(ordinal).join.creation_calls;
  *role = r->Remote(ordinal).join.character;
  TraceRemote(ordinal, "join", "event=create role=%d key=%d calls=%u loop=%llu",
              *role, key, r->Remote(ordinal).join.creation_calls, r->loop);
  return true;
}
// 只用“当前已确认归属”的只读快照和受限读取调用一次原生移除：
// 已确认被原生移除时跳过；身份矛盾时不删除未知 Player，保留 cleanup_failed。
bool RetireJoinedPlayer(SessionRuntime &r, unsigned ordinal) noexcept {
  PersistenceOperation persistence("player_remove");
  if (r.run_torn_down)
    return true;
  if (r.Remote(ordinal).join.remove_attempted)
    return false;
  // 未 Join 过（或从未确认自有关联）时没有需要移除的 Player。
  if (!r.Remote(ordinal).life.joined && !r.Remote(ordinal).life.retire_ready) {
    TraceRemote(ordinal, "retirement",
                "event=player_remove result=none joined=0 loop=%llu", r.loop);
    return true;
  }
  if (!r.Remote(ordinal).life.retire_ready) {
    // 没有可用快照：只有确认列表已不含 B（人数回到 1 且 A 仍可读）时才算
    // 清理完成，否则保留现场并报告残留。
    const auto host = ReadTopology();
    PlayerSlot current;
    const bool read = ReadPlayerSlot(ordinal, current);
    const bool removed =
        host.known && host.unique && !read && current.known &&
        current.fault == TopologyFault::PlayerList &&
        std::strcmp(current.reason, "player_slot_missing") == 0;
    TraceRemote(ordinal, "retirement",
                "event=player_remove result=%s joined=%u count=%u loop=%llu",
                removed ? "absent" : "unconfirmed",
                unsigned(r.Remote(ordinal).life.joined), host.player_count,
                r.loop);
    return removed;
  }
  RetirementPlayerSnapshot snapshot = r.Remote(ordinal).life.retirement;
  InputLease lease(true);
  if (!lease.Ready() || !snapshot.body_count)
    return false;
  r.players[ordinal].Invalidate();
  r.Remote(ordinal).join.remove_attempted = true;
  // 原生cleanup会递归移除较高索引twin；若先移除twin，其Index=-1而引用尚未清空，
  // 再移除根会递归重复Remove并使原生vector长度下溢。按真实索引升序让原生先处理关系。
  for (unsigned b = 0; b < snapshot.body_count; ++b)
    if (snapshot.bodies[b].index < 0)
      return false;
  std::sort(snapshot.bodies.begin(),
            snapshot.bodies.begin() + snapshot.body_count,
            [](const ControlBody &left, const ControlBody &right) noexcept {
              return left.index < right.index;
            });
  for (unsigned b = 0; b < snapshot.body_count; ++b) {
    const auto &expected = snapshot.bodies[b];
    uintptr_t begin = 0;
    size_t count = 0;
    if (GamePointer() != snapshot.game ||
        !VectorRange(snapshot.game, offset::PlayerManager, 64, begin, count))
      return false;
    bool present = false;
    for (size_t i = 0; i < count; ++i) {
      void *current = nullptr;
      if (!ReadMemory(reinterpret_cast<void *>(begin + i * sizeof(void *)),
                      &current, sizeof(current)))
        return false;
      present |= current == expected.player;
    }
    if (!present)
      continue;
    int key = -1, character = -1;
    unsigned type = 0, variant = 0;
    if (!ReadAt(expected.player, offset::ControllerIndex, key) ||
        key != snapshot.remote_key ||
        !ReadAt(expected.player, offset::PlayerType, character) ||
        character != expected.character ||
        !ReadAt(expected.player, offset::EntityType, type) || type != 1 ||
        !ReadAt(expected.player, offset::EntityVariant, variant) ||
        variant != 0)
      return false;
    try {
      reinterpret_cast<void(__thiscall *)(void *, void *, bool)>(
          Base + offset::RemoveCoPlayer)(snapshot.player_manager,
                                         expected.player, false);
    } catch (...) {
      return false;
    }
    TraceRemote(ordinal, "retirement",
                "event=body_remove player=%p key=%d loop=%llu", expected.player,
                key, r.loop);
  }
  uintptr_t after_list = 0;
  size_t after_count = 0;
  if (!VectorRange(snapshot.game, offset::PlayerManager, 64, after_list,
                   after_count))
    return false;
  for (size_t i = 0; i < after_count; ++i) {
    void *player = nullptr;
    int key = -1;
    if (!ReadMemory(reinterpret_cast<void *>(after_list + i * sizeof(void *)),
                    &player, sizeof(player)) ||
        !ReadAt(player, offset::ControllerIndex, key) ||
        key == snapshot.remote_key)
      return false;
  }
  const auto after = ReadTopology();
  if (!after.known || !after.unique || after.pair.player != snapshot.host)
    return false;
  r.Remote(ordinal).life.retire_ready = false;
  r.Remote(ordinal).join.joined_player = nullptr;
  TraceRemote(ordinal, "join", "event=retired players=1 key=%d loop=%llu",
              snapshot.remote_key, r.loop);
  return true;
}
void ResetJoinStateAfterRetirement(SessionRuntime &r,
                                   unsigned ordinal) noexcept {
  if (r.Remote(ordinal).join.state == JoinState::None)
    return;
  // 退休完成后 join_state 复位为 None，最近一次业务结果保留在 join_last。
  if (r.Remote(ordinal).join.state != JoinState::Waiting)
    r.Remote(ordinal).join.last = r.Remote(ordinal).join.state;
  r.Remote(ordinal).join.state = JoinState::None;
  TraceRemote(ordinal, "join", "event=state_reset to=None last=%s loop=%llu",
              JoinStateName(r.Remote(ordinal).join.last), r.loop);
}
} // namespace cs::game
