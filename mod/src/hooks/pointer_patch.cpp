#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/participants/topology.h"
#include "runtime.h"
#include <cmath>
namespace cs {
namespace {
// 保留每个槽的原函数和计数，补丁安装后常驻到进程结束。
struct Slot {
  void **address{};
  void *expected{};
  void *original{};
  void *wrapper{};
  std::atomic<uint64_t> calls{}, foreign{};
} slots[8];
std::atomic<unsigned> installed{};
bool attempted = false;
template <unsigned I> void Count() noexcept {
  auto &s = slots[I];
  if (s.calls.fetch_add(1) == 0)
    Trace("first_call", "%s_%u", I < 4 ? "keyboard" : "gamepad",
          std::array<unsigned, 4>{8, 9, 10, 35}[I % 4]);
  if (!IsMain()) {
    ++s.foreign;
    game::ForeignCall();
  }
}
// Off/Host保持原函数恰一次；Client主动查询隔离在原调用前中性返回。
template <unsigned I> bool __fastcall Query(void *self, void *, int action) {
  Count<I>();
  // Client 的玩法查询在调用官方/原生回调前中性返回；物理采样另走保存的原函数。
  // 例外：客机本地暂停菜单（原生 PauseScreen／选项输入）只读本机设备，直接返回原函数结果，不做 Host 覆写。
  if(game::ClientBusy())
    return game::ClientPauseMenuQueryAllowed()&&
           reinterpret_cast<bool(__thiscall *)(void *, int)>(slots[I].original)(self, action);
  const bool original = reinterpret_cast<bool(__thiscall *)(void *, int)>(
      slots[I].original)(self, action);
  float result = original ? 1.0F : 0.0F;
  if (game::OverrideQuery(self, action,
                          I % 4 == 0 ? game::QueryKind::Pressed
                                     : game::QueryKind::Triggered,
                          result))
    return result != 0;
  return original;
}
template <unsigned I> float __fastcall Value(void *self, void *, int action) {
  Count<I>();
  if(game::ClientBusy())
    return game::ClientPauseMenuQueryAllowed()
               ? reinterpret_cast<float(__thiscall *)(void *, int)>(slots[I].original)(self, action)
               : 0.0F;
  const float original = reinterpret_cast<float(__thiscall *)(void *, int)>(
      slots[I].original)(self, action);
  float result = original;
  game::OverrideQuery(self, action, game::QueryKind::Scalar, result);
  return result;
}
template <unsigned I> bool __fastcall Poll(void *self, void *) {
  Count<I>();
  const bool original =
      reinterpret_cast<bool(__thiscall *)(void *)>(slots[I].original)(self);
  game::PollReturned(self, original);
  return original;
}
} // namespace
void InstallPointers() noexcept {
  if (attempted || !Capabilities[size_t(Group::Input)] || !IsMain())
    return;
  attempted = true;
  void *wrappers[] = {
      reinterpret_cast<void *>(&Query<0>), reinterpret_cast<void *>(&Query<1>),
      reinterpret_cast<void *>(&Value<2>), reinterpret_cast<void *>(&Poll<3>),
      reinterpret_cast<void *>(&Query<4>), reinterpret_cast<void *>(&Query<5>),
      reinterpret_cast<void *>(&Value<6>), reinterpret_cast<void *>(&Poll<7>)};
  const unsigned indices[] = {8, 9, 10, 35};
  // 先检查全部八槽，再一次获取两张虚表共用页的权限，逐槽原子发布。
  for (unsigned i = 0; i < 8; ++i) {
    auto &s = slots[i];
    unsigned j = i % 4;
    s.address = reinterpret_cast<void **>(
        Base + (i < 4 ? offset::KeyboardVtable : offset::GamepadVtable) +
        indices[j] * 4);
    const uintptr_t target =
        j < 3 ? std::array<uintptr_t, 3>{0x5ECD80, 0x5ECDE0, 0x5ECE50}[j]
              : (i < 4 ? 0x633DD0 : 0x6331B0);
    s.expected = reinterpret_cast<void *>(Base + target);
    s.original = *s.address;
    s.wrapper = wrappers[i];
    if (s.original != s.expected) {
      Capabilities[size_t(Group::Input)] = false;
      Trace("pointer_patch", "ok=0 index=%u original=%p expected=%p", i,
            s.original, s.expected);
      return;
    }
  }
  DWORD protection = 0;
  auto *page = reinterpret_cast<void *>((Base + offset::GamepadVtable) &
                                        ~uintptr_t(0xFFF));
  if (!VirtualProtect(page, 4096, PAGE_READWRITE, &protection)) {
    Capabilities[size_t(Group::Input)] = false;
    Trace("pointer_patch", "ok=0 protect_error=%lu", GetLastError());
    return;
  }
  for (unsigned i = 0; i < 8; ++i) {
    auto &s = slots[i];
    InterlockedExchangePointer(s.address, s.wrapper);
    ++installed;
    Trace("pointer_patch",
          "ok=1 class=%s slot=%u original=%p expected=%p new=%p",
          i < 4 ? "keyboard" : "gamepad", indices[i % 4], s.original,
          s.expected, s.wrapper);
  }
  DWORD unused;
  if (!VirtualProtect(page, 4096, protection, &unused)) {
    Capabilities[size_t(Group::Input)] = false;
    Trace("pointer_patch", "ok=0 reprotect_error=%lu", GetLastError());
  }
}
unsigned PointerCount() noexcept { return installed.load(); }
std::string PointerCounters() {
  std::string out;
  for (unsigned i = 0; i < 8; ++i) {
    auto id = std::string(i < 4 ? "keyboard_" : "gamepad_") +
              std::to_string(std::array<unsigned, 4>{8, 9, 10, 35}[i % 4]);
    out += "hook." + id + "=" + std::to_string(slots[i].calls.load()) +
           "\nhook." + id +
           ".foreign=" + std::to_string(slots[i].foreign.load()) + "\n";
  }
  return out;
}
} // namespace cs

namespace cs::game {
// 采样只调用安装时保存的三槽原函数，不重新进入覆写 wrapper，也不修改设备数组。
bool ReadPhysicalActions(void *device, PhysicalActions &actions,bool *map_pressed) {
  if (!IsMain() || PointerCount() != 8)
    return false;
  uintptr_t table = 0;
  if (!ReadAt(device, 0, table))
    return false;
  const unsigned start = table == Base + offset::KeyboardVtable ? 0 : 4;
  if (table != Base + offset::KeyboardVtable &&
      table != Base + offset::GamepadVtable)
    return false;
  for (int action = 0; action < 12; ++action) {
    actions.pressed[action] = reinterpret_cast<bool(__thiscall *)(void *, int)>(
        slots[start].original)(device, action);
    actions.triggered[action] =
        reinterpret_cast<bool(__thiscall *)(void *, int)>(
            slots[start + 1].original)(device, action);
    actions.values[action] = reinterpret_cast<float(__thiscall *)(void *, int)>(
        slots[start + 2].original)(device, action);
    if (!std::isfinite(actions.values[action]) || actions.values[action] < 0 ||
        actions.values[action] > 1)
      return false;
  }
  // ACTION_MAP 读取本机设备原生绑定，单独供本机地图使用。
  if (map_pressed)
    *map_pressed = reinterpret_cast<bool(__thiscall *)(void *, int)>(
        slots[start].original)(device, 13);
  return true;
}
} // namespace cs::game
