#pragma once
#include "core/participant.h"
#include <cstddef>
#include <cstring>
#include <limits>

namespace cs::game {
// 只在主线程、已知生命周期边界读取；失败返回未知，不把零值解释为有效布局。
bool ReadMemory(const void *source, void *output, std::size_t size) noexcept;
template <class T>
bool ReadAt(const void *object, std::size_t offset, T &value) noexcept {
  const auto address = reinterpret_cast<uintptr_t>(object);
  return object &&
         offset <= (std::numeric_limits<uintptr_t>::max)() - address &&
         ReadMemory(reinterpret_cast<const void *>(address + offset), &value,
                    sizeof(value));
}
struct RoomIdentity {
  void *game = nullptr;
  void *room = nullptr;
  unsigned stage = 0, stage_type = 0, index = 0, dimension = 0;
  bool operator==(const RoomIdentity &) const = default;
};
// exact pair 与生命状态分开；同一 Player 地址上的死亡/复活也必须失效。
struct Topology {
  RoomIdentity room{};
  core::Binding pair{};
  bool run = false, known = false, unique = false;
  bool valid = false, exists = false, dead = false, removed = false,
       ghost = false;
  bool focused = true, pause_on_focus_lost = false, starting = false;
  int pause = 0, room_transition = 0, stage_transition = 0;
  unsigned player_count = 0, variant = 0;
  int character = -1;
  const char *reason = "unknown";
  bool Alive() const noexcept {
    return valid && exists && !dead && !removed && !ghost;
  }
};
Topology ReadTopology(unsigned ordinal = 0,
                      void *owned_device = nullptr) noexcept;
// 已知 vector 活动范围的只读复制；仅在主线程且持有 InputLease 时枚举设备。
bool VectorRange(const void *object, size_t offset, size_t maximum,
                 uintptr_t &begin, size_t &count) noexcept;
int DeviceOccurrences(const void *device) noexcept;
int DeviceIdOccurrences(int id) noexcept;
bool RunActive() noexcept;
bool SameLife(const Topology &a, const Topology &b) noexcept;
} // namespace cs::game
