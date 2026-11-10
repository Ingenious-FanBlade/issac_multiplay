#include "game/participants/topology.h"
#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/participants/device_k.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "runtime.h"
#include <algorithm>
#include <limits>

namespace cs::game {
bool ReadMemory(const void *source, void *output, std::size_t size) noexcept {
  if (!IsMain() || !source || !output)
    return false;
  const auto begin = reinterpret_cast<uintptr_t>(source);
  if (size > (std::numeric_limits<uintptr_t>::max)() - begin)
    return false;
  const auto end = begin + size;
  for (auto cursor = begin; cursor < end;) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void *>(cursor), &info, sizeof(info)) ||
        info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
      return false;
    const auto next =
        reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    if (next <= cursor)
      return false;
    cursor = next;
  }
  std::memcpy(output, source, size);
  return true;
}
const char *TopologyFaultName(TopologyFault fault) noexcept {
  switch (fault) {
  case TopologyFault::None:
    return "none";
  case TopologyFault::NotRun:
    return "not_run";
  case TopologyFault::PlayerList:
    return "player_list";
  case TopologyFault::Unreadable:
    return "unreadable";
  case TopologyFault::Identity:
    return "identity";
  case TopologyFault::Ambiguous:
    return "ambiguous";
  case TopologyFault::Room:
    return "room";
  case TopologyFault::Device:
    return "device";
  case TopologyFault::Ownership:
    return "ownership";
  }
  return "UNKNOWN";
}
// 只枚举已知 vector 的活动区间，容量终点不作为额外设备读取。
bool VectorRange(const void *object, size_t offset, size_t maximum,
                 uintptr_t &begin, size_t &count) noexcept {
  std::array<uintptr_t, 3> range{};
  if (!ReadAt(object, offset, range) || range[1] < range[0] ||
      range[2] < range[1] || (range[0] % alignof(void *)) ||
      (range[1] - range[0]) % sizeof(void *))
    return false;
  count = (range[1] - range[0]) / sizeof(void *);
  begin = range[0];
  return count <= maximum && (!count || begin);
}
bool RunActive() noexcept {
  bool run = false;
  return ReadAt(GamePointer(), offset::RunActive, run) && run;
}
// 受限读取：不检查设备注册，只确认 Player vector 与该 slot 的实体身份。
// 读取失败时区分不可读与身份不符，调用方据此决定中性或 PeerLost。
bool ReadPlayerSlot(unsigned ordinal, PlayerSlot &out) noexcept {
  out = {};
  if (!IsMain() || !Base || ordinal >= core::kMaximumParticipants)
    return false;
  InputLease lease(HostBusy());
  if (!lease.Ready())
    return false;
  auto *game = GamePointer();
  bool run = false;
  if (!ReadAt(game, offset::RunActive, run)) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "run_unreadable";
    return false;
  }
  out.known = true;
  if (!run) {
    out.fault = TopologyFault::NotRun;
    out.reason = "not_in_run";
    return true;
  }
  uintptr_t players = 0;
  size_t count = 0;
  if (!VectorRange(game, offset::PlayerManager, 64, players, count)) {
    out.fault = TopologyFault::PlayerList;
    out.reason = "player_list_unknown";
    return false;
  }
  out.count = static_cast<unsigned>(count);
  // ordinal 只选择已确认控制组的根；原生列表重排不改变参与者身份。
  auto *runtime = Runtime();
  void *expected = runtime ? runtime->controlled_player[ordinal] : nullptr;
  if (!expected && ordinal == 0 && count)
    ReadMemory(reinterpret_cast<void *>(players), &expected, sizeof(expected));
  for (size_t i = 0; i < count; ++i) {
    void *candidate = nullptr;
    if (!ReadMemory(reinterpret_cast<void *>(players + i * sizeof(void *)),
                    &candidate, sizeof(candidate)))
      return false;
    if (candidate == expected)
      out.player = candidate;
  }
  if (!out.player) {
    out.fault = TopologyFault::PlayerList;
    out.reason = "player_slot_missing";
    return false;
  }
  if (!ReadAt(out.player, offset::PlayerIndex, out.index) ||
      !ReadAt(out.player, offset::EntityType, out.type) ||
      !ReadAt(out.player, offset::EntityVariant, out.variant) ||
      !ReadAt(out.player, offset::PlayerType, out.character) ||
      !ReadAt(out.player, offset::ControllerIndex, out.controller) ||
      !ReadAt(out.player, offset::EntityValid, out.valid) ||
      !ReadAt(out.player, offset::EntityExists, out.exists) ||
      !ReadAt(out.player, offset::EntityDead, out.dead) ||
      !ReadAt(out.player, offset::EntityRemoved, out.removed) ||
      !ReadAt(out.player, offset::CoopGhost, out.ghost)) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "player_fields_unreadable";
    return false;
  }
  if (out.type != 1 || out.variant != 0) {
    out.fault = TopologyFault::Identity;
    out.reason = "player_slot_identity";
    return false;
  }
  out.identity = true;
  out.fault = TopologyFault::None;
  out.reason = "slot_known";
  return true;
}
Topology ReadTopology(unsigned ordinal, void *owned_device) noexcept {
  auto *r = Runtime();
  void *root = r && ordinal < core::kMaximumParticipants
                   ? r->controlled_player[ordinal]
                   : nullptr;
  return ReadTopologyForRoot(ordinal, root, owned_device);
}
Topology ReadTopologyForRoot(unsigned ordinal, void *root,
                             void *owned_device) noexcept {
  Topology out;
  if (!IsMain() || !Base)
    return out;
  InputLease lease(HostBusy());
  if (!lease.Ready() || ordinal >= core::kMaximumParticipants)
    return out;
  out.room.game = GamePointer();
  if (!ReadAt(out.room.game, offset::RunActive, out.run))
    return out;
  if (!out.run) {
    out.known = true;
    out.fault = TopologyFault::NotRun;
    out.reason = "not_in_run";
    return out;
  }
  void *manager = nullptr;
  if (!ReadMemory(reinterpret_cast<void *>(Base + offset::Manager), &manager,
                  sizeof(manager)) ||
      !ReadAt(manager, offset::Focus, out.focused) ||
      !ReadAt(manager, offset::PauseOnFocusLost, out.pause_on_focus_lost) ||
      !ReadAt(manager, offset::Starting, out.starting) ||
      !ReadAt(out.room.game, offset::CurrentRoom, out.room.room) ||
      !ReadAt(out.room.game, 0, out.room.stage) ||
      !ReadAt(out.room.game, 4, out.room.stage_type) ||
      !ReadAt(out.room.game, offset::RoomIndex, out.room.index) ||
      !ReadAt(out.room.game, offset::Dimension, out.room.dimension) ||
      !ReadAt(out.room.game, offset::PauseScreen, out.pause) ||
      !ReadAt(out.room.game, offset::RoomTransition, out.room_transition) ||
      !ReadAt(out.room.game, offset::StageTransition, out.stage_transition)) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "game_fields_unreadable";
    return out;
  }
  out.known = true;
  uintptr_t players = 0;
  size_t player_count = 0;
  if (!VectorRange(out.room.game, offset::PlayerManager, 64, players,
                   player_count) ||
      !player_count) {
    out.player_count = static_cast<unsigned>(player_count);
    out.fault = TopologyFault::PlayerList;
    out.reason = "player_list_unknown";
    return out;
  }
  out.player_count = static_cast<unsigned>(player_count);
  std::array<void *, 64> roster{};
  if (!ReadMemory(reinterpret_cast<void *>(players), roster.data(),
                  player_count * sizeof(void *))) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "player_pointer_unreadable";
    return out;
  }
  if (!root && ordinal == 0)
    root = roster[0];
  // 只从当前 roster 的已知关系寻找替换根，不解引用已经缺席的历史地址。
  bool in_roster = std::find(roster.begin(), roster.begin() + player_count,
                             root) != roster.begin() + player_count;
  if (!in_roster && root) {
    void *replacement = nullptr;
    for (size_t i = 0; i < player_count; ++i) {
      for (const size_t relation :
           {size_t(0x1D84), size_t(0x1E4C), size_t(0x1E50), size_t(0x267C)}) {
        void *target = nullptr;
        if (!ReadAt(roster[i], relation, target)) {
          out.fault = TopologyFault::Unreadable;
          return out;
        }
        if (target == root) {
          if (replacement && replacement != roster[i]) {
            out.fault = TopologyFault::Ambiguous;
            return out;
          }
          replacement = roster[i];
        }
      }
    }
    root = replacement;
  }
  if (!root) {
    out.fault = TopologyFault::Identity;
    out.reason = "group_root_missing";
    return out;
  }
  out.pair.player = root;
  int index = -1;
  unsigned type = 0;
  if (!ReadAt(out.pair.player, offset::PlayerIndex, index) ||
      !ReadAt(out.pair.player, offset::EntityType, type) || type != 1 ||
      !ReadAt(out.pair.player, offset::EntityVariant, out.variant) ||
      out.variant != 0 ||
      !ReadAt(out.pair.player, offset::PlayerType, out.character) ||
      !ReadAt(out.pair.player, offset::ControllerIndex, out.pair.key) ||
      !ReadAt(out.pair.player, offset::EntityValid, out.valid) ||
      !ReadAt(out.pair.player, offset::EntityExists, out.exists) ||
      !ReadAt(out.pair.player, offset::EntityDead, out.dead) ||
      !ReadAt(out.pair.player, offset::EntityRemoved, out.removed) ||
      !ReadAt(out.pair.player, offset::CoopGhost, out.ghost)) {
    out.pair.player = nullptr;
    // 已读到对象但字段/身份不成立：属于身份矛盾，而不是暂时不可读。
    out.fault = TopologyFault::Identity;
    out.reason = "player_identity_unknown";
    return out;
  }
  // 关系偏移复用 J273 world_layout 已核定的公开 Lua
  // Getter；关系资格不传赠读取历史。
  const auto append = [&](void *player, BodyRole role) noexcept {
    if (!player)
      return true;
    for (unsigned i = 0; i < out.body_count; ++i)
      if (out.bodies[i].player == player)
        return true;
    if (out.body_count == out.bodies.size())
      return false;
    auto &body = out.bodies[out.body_count++];
    body.player = player;
    body.role = role;
    body.roster = std::find(roster.begin(), roster.begin() + player_count,
                            player) != roster.begin() + player_count;
    return true;
  };
  append(root, BodyRole::Primary);
  for (unsigned i = 0; i < out.body_count; ++i) {
    auto &body = out.bodies[i];
    unsigned type = 0, variant = 0;
    bool valid = false, exists = false, removed = false;
    if (!ReadAt(body.player, offset::EntityType, type) || type != 1 ||
        !ReadAt(body.player, offset::EntityVariant, variant) || variant != 0 ||
        !ReadAt(body.player, offset::PlayerIndex, body.index) ||
        !ReadAt(body.player, offset::ControllerIndex, body.controller) ||
        !ReadAt(body.player, offset::PlayerType, body.character) ||
        !ReadAt(body.player, offset::EntityValid, valid) ||
        !ReadAt(body.player, offset::EntityExists, exists) ||
        !ReadAt(body.player, offset::EntityRemoved, removed) ||
        !ReadAt(body.player, offset::EntityDead, body.dead) ||
        !ReadAt(body.player, offset::CoopGhost, body.ghost)) {
      out.fault = TopologyFault::Unreadable;
      out.reason = "group_body_unreadable";
      return out;
    }
    body.valid = valid;
    body.exists = exists;
    body.removed = removed;
    body.hologram =
        valid && exists && !removed && PlayerIsHologram(body.player);
    const bool control_role = body.role == BodyRole::Primary ||
                              body.role == BodyRole::Twin ||
                              body.role == BodyRole::SubPlayer;
    body.active = control_role && valid && exists && !removed &&
                  body.controller == out.pair.key;
    // 普通16/17的+1D84是形态状态交换对象；非roster时不预设它独立参与模拟。
    body.native_activity_required =
        body.role == BodyRole::SubPlayer && !body.roster &&
        (body.character == 16 || body.character == 17);
    if (body.native_activity_required) {
      if (auto *r = Runtime(); r && r->players[ordinal].BoundValid())
        for (const auto &witness : r->body_activity[ordinal])
          if (witness.player == body.player &&
              witness.generation == r->players[ordinal].Generation() &&
              witness.tick && witness.tick <= r->clock.Completed())
            body.native_activity_seen = true;
    }
    out.active_bodies += body.CanConsume();
    // Esau Jr槽由当前实体各active
    // slot的703元数据定位，绝不按ordinal/PlayerIndex猜测。
    if (control_role)
      for (unsigned slot = 0; slot < 4; ++slot) {
        int item = 0, association = 0;
        if (!ReadAt(body.player,
                    offset::ActiveItemSlotBase +
                        slot * offset::ActiveItemSlotStride,
                    item)) {
          out.fault = TopologyFault::Unreadable;
          out.reason = "active_item_unreadable";
          return out;
        }
        if (item != 703)
          continue;
        if (!ReadAt(body.player,
                    offset::EsauJrAssociation +
                        slot * offset::ActiveItemSlotStride,
                    association) ||
            association < 0 || association > 4) {
          out.fault = TopologyFault::Identity;
          out.reason = "esau_jr_slot_invalid";
          return out;
        }
        if (!association)
          continue;
        void *backup = nullptr;
        if (!ReadAt(out.room.game,
                    offset::PlayerManager + offset::EsauJrStates +
                        (association - 1) * sizeof(void *),
                    backup) ||
            !append(backup, BodyRole::EsauJr)) {
          out.fault = TopologyFault::Unreadable;
          out.reason = "esau_jr_backup_unknown";
          return out;
        }
      }
    const size_t relations[] = {0x1D84, 0x1E4C, 0x1E50, 0x267C};
    const BodyRole roles[] = {BodyRole::SubPlayer, BodyRole::Twin,
                              BodyRole::Backup, BodyRole::Replaced};
    for (unsigned j = 0; j < 4; ++j) {
      void *target = nullptr;
      if (!ReadAt(body.player, relations[j], target) ||
          !append(target, control_role ? roles[j] : BodyRole::Inactive)) {
        out.fault = TopologyFault::Ambiguous;
        out.reason = "group_relation_unknown_or_overflow";
        return out;
      }
      body.relations[j] = target;
    }
  }
  // 同 key 的另一个 roster 根必须由已确认关系解释，否则整组关闭，不能默默串控。
  for (size_t i = 0; i < player_count; ++i) {
    int key = -1;
    if (!ReadAt(roster[i], offset::ControllerIndex, key)) {
      out.fault = TopologyFault::Unreadable;
      return out;
    }
    if (key == out.pair.key && !out.Contains(roster[i], false)) {
      out.fault = TopologyFault::Ambiguous;
      out.reason = "unowned_shared_controller";
      return out;
    }
  }
  uintptr_t devices = 0;
  size_t device_count = 0;
  if (out.pair.key < 0 ||
      !VectorRange(reinterpret_cast<void *>(Base + offset::InputManager),
                   offset::DeviceVector, 256, devices, device_count)) {
    out.pair.player = nullptr;
    out.fault = TopologyFault::Device;
    out.reason = "device_list_unknown";
    return out;
  }
  unsigned matches = 0;
  for (size_t i = 0; i < device_count; ++i) {
    void *device = nullptr;
    int id = -1;
    if (!ReadMemory(reinterpret_cast<void *>(devices + i * sizeof(void *)),
                    &device, sizeof(device)) ||
        !ReadAt(device, offset::DeviceId, id)) {
      out.pair.player = nullptr;
      out.fault = TopologyFault::Device;
      out.reason = "device_identity_unknown";
      return out;
    }
    if (id == out.pair.key) {
      ++matches;
      out.pair.device = device;
    }
  }
  uintptr_t vtable = 0;
  bool enabled = false;
  const bool readable = matches == 1 && ReadAt(out.pair.device, 0, vtable) &&
                        ReadAt(out.pair.device, offset::DeviceEnabled, enabled);
  // A 只接受键盘/手柄后端；B 只接受构造时记录的自有 K。归属不符与
  // 设备暂时不可用必须分开：前者允许进入 PeerLost 防御，后者只表示不可绑定。
  const bool ownership_ok =
      readable &&
      (ordinal == 0 ? (vtable == Base + offset::KeyboardVtable ||
                       vtable == Base + offset::GamepadVtable)
                    : (owned_device && out.pair.device == owned_device &&
                       IsDeviceK(out.pair.device)));
  if (!ownership_ok || !enabled) {
    out.pair.player = nullptr;
    if (matches > 1) {
      out.fault = TopologyFault::Ambiguous;
      out.reason = "ambiguous_device";
    } else if (ordinal != 0 && matches != 1) {
      out.fault = TopologyFault::Ownership;
      out.reason = "device_missing";
    } else if (ordinal != 0 && readable && !ownership_ok) {
      out.fault = TopologyFault::Ownership;
      out.reason = "device_ownership_mismatch";
    } else {
      out.fault = TopologyFault::Device;
      out.reason = "device_unavailable";
    }
    return out;
  }
  bool structural = false;
  for (unsigned i = 0; i < out.body_count; ++i)
    structural |= out.bodies[i].active;
  out.unique = out.room.room != nullptr && structural;
  out.fault = out.unique ? TopologyFault::None : TopologyFault::Room;
  out.reason = out.unique ? "unique" : "room_unknown";
  return out;
}
int DeviceOccurrences(const void *device) noexcept {
  if (!IsMain() || !device)
    return -1;
  InputLease lease(true);
  uintptr_t begin = 0;
  size_t count = 0;
  if (!lease.Ready() ||
      !VectorRange(reinterpret_cast<void *>(Base + offset::InputManager),
                   offset::DeviceVector, 256, begin, count))
    return -1;
  int matches = 0;
  for (size_t i = 0; i < count; ++i) {
    void *p = nullptr;
    if (!ReadMemory(reinterpret_cast<void *>(begin + i * 4), &p, sizeof(p)))
      return -1;
    if (p == device)
      ++matches;
  }
  return matches;
}
bool SameLife(const Topology &a, const Topology &b) noexcept {
  return a.pair.player == b.pair.player && a.valid == b.valid &&
         a.exists == b.exists && a.dead == b.dead && a.removed == b.removed &&
         a.ghost == b.ghost;
}
// 构造取得的原生 id 必须在注册前空闲，注册后恰好出现一次；不是 vector 下标。
int DeviceIdOccurrences(int id) noexcept {
  if (!IsMain() || id < 0)
    return -1;
  InputLease lease(true);
  uintptr_t begin = 0;
  size_t count = 0;
  if (!lease.Ready() ||
      !VectorRange(reinterpret_cast<void *>(Base + offset::InputManager),
                   offset::DeviceVector, 256, begin, count))
    return -1;
  int matches = 0;
  for (size_t i = 0; i < count; ++i) {
    void *device = nullptr;
    int current = -1;
    if (!ReadMemory(reinterpret_cast<void *>(begin + i * 4), &device, 4) ||
        !ReadAt(device, offset::DeviceId, current))
      return -1;
    if (current == id)
      ++matches;
  }
  return matches;
}
} // namespace cs::game

namespace cs::game {
bool SameControlGroup(const Topology &a, const Topology &b) noexcept {
  if (!(a.pair == b.pair) || a.body_count != b.body_count)
    return false;
  for (unsigned i = 0; i < a.body_count; ++i) {
    auto left = a.bodies[i], right = b.bodies[i];
    left.native_activity_seen = right.native_activity_seen = false;
    left.index = right.index =
        -1; // 原生RemoveCoPlayer重编号不改变其余参与者归属。
    if (!(left == right))
      return false;
  }
  return true;
}
} // namespace cs::game
