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
// 读取失败的原因分类：先区分“暂时不可读”和“已读到但身份不符”。
// 前者只让远端保持中性，不得冒充被原生移除；后者才允许进入 PeerLost 防御。
enum class TopologyFault : unsigned {
  None,
  NotRun,     // 可读且明确不在 run 内
  PlayerList, // Player vector 不可读或不足以覆盖该 ordinal
  Unreadable, // 已定位对象但字段读取失败
  Identity,   // 可读但该 slot 不是本 ordinal 的完整普通 Player
  Ambiguous,  // 同一 ordinal 出现多个候选或设备归属重复
  Room,       // 房间身份未知
  Device,     // 设备列表不可读或不是唯一可用的自有 K
  Ownership   // 可读但 controller 不再指向自有 K（归属矛盾）
};
const char *TopologyFaultName(TopologyFault fault) noexcept;
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
  TopologyFault fault = TopologyFault::None;
  const char *reason = "unknown";
  bool Alive() const noexcept {
    return valid && exists && !dead && !removed && !ghost;
  }
  // 暂时不可读：只表示“不能绑定”，不能据此判定远端已被移除。
  bool Unreadable() const noexcept {
    return !known || fault == TopologyFault::Unreadable ||
           fault == TopologyFault::PlayerList ||
           fault == TopologyFault::Device;
  }
};
Topology ReadTopology(unsigned ordinal = 0,
                      void *owned_device = nullptr) noexcept;
// 受限的 Player slot 读取：只确认当前 Player 列表中的 ordinal 位置，
// 不要求设备仍在 InputManager vector 中。用于 K 已移出设备 vector 之后的
// 退休确认与复活后身份复核（tx-04-07 §2.4）。
struct PlayerSlot {
  void *player = nullptr;
  unsigned count = 0;
  int index = -1, controller = -1, character = -1;
  unsigned type = 0, variant = 0;
  bool valid = false, exists = false, dead = false, removed = false,
       ghost = false;
  bool known = false, identity = false;
  TopologyFault fault = TopologyFault::None;
  const char *reason = "unknown";
  bool Alive() const noexcept {
    return valid && exists && !dead && !removed && !ghost;
  }
};
bool ReadPlayerSlot(unsigned ordinal, PlayerSlot &out) noexcept;
// 已知 vector 活动范围的只读复制；仅在主线程且持有 InputLease 时枚举设备。
bool VectorRange(const void *object, size_t offset, size_t maximum,
                 uintptr_t &begin, size_t &count) noexcept;
int DeviceOccurrences(const void *device) noexcept;
int DeviceIdOccurrences(int id) noexcept;
bool RunActive() noexcept;
bool SameLife(const Topology &a, const Topology &b) noexcept;
} // namespace cs::game
