#include "game/topology.h"
#include "binding/offsets.h"
#include "runtime.h"
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
namespace {
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
} // namespace
bool RunActive() noexcept {
  bool run = false;
  return ReadAt(GamePointer(), offset::RunActive, run) && run;
}
Topology ReadTopology() noexcept {
  Topology out;
  if (!IsMain() || !Base)
    return out;
  out.room.game = GamePointer();
  if (!ReadAt(out.room.game, offset::RunActive, out.run))
    return out;
  if (!out.run) {
    out.known = true;
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
      !ReadAt(out.room.game, offset::StageTransition, out.stage_transition))
    return out;
  out.known = true;
  uintptr_t players = 0;
  size_t player_count = 0;
  if (!VectorRange(out.room.game, offset::PlayerManager, 16, players,
                   player_count) ||
      !player_count) {
    out.reason = "player_list_unknown";
    return out;
  }
  if (!ReadMemory(reinterpret_cast<void *>(players), &out.pair.player,
                  sizeof(void *)))
    return out;
  int index = -1;
  unsigned type = 0;
  if (!ReadAt(out.pair.player, offset::PlayerIndex, index) || index != 0 ||
      !ReadAt(out.pair.player, offset::EntityType, type) || type != 1 ||
      !ReadAt(out.pair.player, offset::ControllerIndex, out.pair.key) ||
      !ReadAt(out.pair.player, offset::EntityValid, out.valid) ||
      !ReadAt(out.pair.player, offset::EntityExists, out.exists) ||
      !ReadAt(out.pair.player, offset::EntityDead, out.dead) ||
      !ReadAt(out.pair.player, offset::EntityRemoved, out.removed) ||
      !ReadAt(out.pair.player, offset::CoopGhost, out.ghost)) {
    out.pair.player = nullptr;
    out.reason = "player_identity_unknown";
    return out;
  }
  for (size_t i = 1; i < player_count; ++i) {
    void *other = nullptr;
    int other_index = -1;
    if (!ReadMemory(reinterpret_cast<void *>(players + i * sizeof(void *)),
                    &other, sizeof(other)) ||
        !ReadAt(other, offset::PlayerIndex, other_index) || other_index == 0 ||
        other == out.pair.player) {
      out.reason = "ambiguous_player";
      return out;
    }
  }
  uintptr_t devices = 0;
  size_t device_count = 0;
  if (out.pair.key < 0 ||
      !VectorRange(reinterpret_cast<void *>(Base + offset::InputManager),
                   offset::DeviceVector, 256, devices, device_count)) {
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
  if (matches != 1 || !ReadAt(out.pair.device, 0, vtable) ||
      (vtable != Base + offset::KeyboardVtable &&
       vtable != Base + offset::GamepadVtable) ||
      !ReadAt(out.pair.device, offset::DeviceEnabled, enabled) || !enabled) {
    out.reason = matches > 1 ? "ambiguous_device" : "device_unavailable";
    return out;
  }
  out.unique = out.room.room != nullptr;
  out.reason = out.unique ? "unique" : "room_unknown";
  return out;
}
bool SameLife(const Topology &a, const Topology &b) noexcept {
  return a.pair.player == b.pair.player && a.valid == b.valid &&
         a.exists == b.exists && a.dead == b.dead && a.removed == b.removed &&
         a.ghost == b.ghost;
}
} // namespace cs::game
