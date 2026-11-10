#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/authority/tick.h"
#include "game/participants/topology.h"
#include "runtime.h"

namespace cs::game {
namespace {
thread_local unsigned lease_depth = 0;
}
void InputLeaseUnavailable() noexcept {
  FailRuntime("input_backend_lock_unavailable");
}
// 原生后端 worker 在 5EC464..5EC497 持有此 Mutex。它的 held 标记使原生
// Lock 不可递归，因此本线程用显式深度避免二次进入原生 Lock 的等待循环。
InputLease::InputLease(bool required) noexcept {
  if (!required) {
    ready_ = true;
    return;
  }
  if (!IsMain() || !Base)
    return;
  if (lease_depth) {
    ++lease_depth;
    joined_ = ready_ = true;
    return;
  }
  bool initialized = false;
  const auto mutex = reinterpret_cast<void *>(Base + offset::InputBackendMutex);
  uintptr_t vtable = 0;
  std::array<uintptr_t, 2> methods{};
  if (!ReadAt(mutex, 4, initialized) || !initialized ||
      !ReadAt(mutex, 0, vtable) ||
      !ReadAt(reinterpret_cast<void *>(vtable), 12, methods) ||
      methods[0] != Base + offset::MutexLock ||
      methods[1] != Base + offset::MutexUnlock)
    return;
  auto lock = reinterpret_cast<bool(__thiscall *)(void *, int)>(
      Base + offset::MutexLock);
  if (!lock(mutex, -1))
    return;
  lease_depth = 1;
  joined_ = ready_ = true;
}
InputLease::~InputLease() noexcept {
  if (!joined_ || --lease_depth)
    return;
  auto *mutex = reinterpret_cast<void *>(Base + offset::InputBackendMutex);
  auto **table = *reinterpret_cast<void ***>(mutex);
  reinterpret_cast<void(__thiscall *)(void *)>(table[4])(mutex);
}
} // namespace cs::game
