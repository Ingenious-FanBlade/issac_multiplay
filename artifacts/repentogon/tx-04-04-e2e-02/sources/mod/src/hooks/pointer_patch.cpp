#include "binding/offsets.h"
#include "runtime.h"
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
  if (!IsMain())
    ++s.foreign;
}
// 设备 wrapper 同样保持异常中立，只调用一次原函数。
template <unsigned I> bool __fastcall Query(void *self, void *, int action) {
  Count<I>();
  return reinterpret_cast<bool(__thiscall *)(void *, int)>(slots[I].original)(self, action);
}
template <unsigned I> float __fastcall Value(void *self, void *, int action) {
  Count<I>();
  return reinterpret_cast<float(__thiscall *)(void *, int)>(slots[I].original)(self, action);
}
template <unsigned I> bool __fastcall Poll(void *self, void *) {
  Count<I>();
  return reinterpret_cast<bool(__thiscall *)(void *)>(slots[I].original)(self);
}
} // namespace
void InstallPointers() noexcept {
  if (attempted || !Capabilities[size_t(Group::Input)] || !IsMain())
    return;
  attempted = true;
  void *wrappers[] = {reinterpret_cast<void *>(&Query<0>), reinterpret_cast<void *>(&Query<1>),
                      reinterpret_cast<void *>(&Value<2>), reinterpret_cast<void *>(&Poll<3>),
                      reinterpret_cast<void *>(&Query<4>), reinterpret_cast<void *>(&Query<5>),
                      reinterpret_cast<void *>(&Value<6>), reinterpret_cast<void *>(&Poll<7>)};
  const unsigned indices[] = {8, 9, 10, 35};
  // 先检查全部八槽，再一次获取两张虚表共用页的权限，逐槽原子发布。
  for (unsigned i = 0; i < 8; ++i) {
    auto &s = slots[i];
    unsigned j = i % 4;
    s.address = reinterpret_cast<void **>(
        Base + (i < 4 ? offset::KeyboardVtable : offset::GamepadVtable) + indices[j] * 4);
    const uintptr_t target = j < 3 ? std::array<uintptr_t, 3>{0x5ECD80, 0x5ECDE0, 0x5ECE50}[j]
                                   : (i < 4 ? 0x633DD0 : 0x6331B0);
    s.expected = reinterpret_cast<void *>(Base + target);
    s.original = *s.address;
    s.wrapper = wrappers[i];
    if (s.original != s.expected) {
      Capabilities[size_t(Group::Input)] = false;
      Trace("pointer_patch", "ok=0 index=%u original=%p expected=%p", i, s.original, s.expected);
      return;
    }
  }
  DWORD protection = 0;
  auto *page = reinterpret_cast<void *>((Base + offset::GamepadVtable) & ~uintptr_t(0xFFF));
  if (!VirtualProtect(page, 4096, PAGE_READWRITE, &protection)) {
    Capabilities[size_t(Group::Input)] = false;
    Trace("pointer_patch", "ok=0 protect_error=%lu", GetLastError());
    return;
  }
  for (unsigned i = 0; i < 8; ++i) {
    auto &s = slots[i];
    InterlockedExchangePointer(s.address, s.wrapper);
    ++installed;
    Trace("pointer_patch", "ok=1 class=%s slot=%u original=%p expected=%p new=%p",
          i < 4 ? "keyboard" : "gamepad", indices[i % 4], s.original, s.expected, s.wrapper);
  }
  DWORD unused;
  if (!VirtualProtect(page, 4096, protection, &unused)) {
    Capabilities[size_t(Group::Input)] = false;
    Trace("pointer_patch", "ok=0 reprotect_error=%lu", GetLastError());
  }
}
unsigned PointerCount() noexcept {
  return installed.load();
}
std::string PointerCounters() {
  std::string out;
  for (unsigned i = 0; i < 8; ++i) {
    auto id = std::string(i < 4 ? "keyboard_" : "gamepad_") +
              std::to_string(std::array<unsigned, 4>{8, 9, 10, 35}[i % 4]);
    out += "hook." + id + "=" + std::to_string(slots[i].calls.load()) + "\nhook." + id +
           ".foreign=" + std::to_string(slots[i].foreign.load()) + "\n";
  }
  return out;
}
} // namespace cs
