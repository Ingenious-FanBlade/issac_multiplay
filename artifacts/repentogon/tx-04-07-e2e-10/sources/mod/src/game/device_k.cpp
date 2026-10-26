#include "game/device_k.h"
#include "binding/offsets.h"
#include "game/bridge.h"
#include "game/join.h"
#include "game/topology.h"
#include "runtime.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

namespace cs::game {
namespace {
// +3FC 是官方 Initialize 所读名称；私有状态放在外部记录，不冒充 Gamepad 尾部。
struct alignas(16) OwnedRecord {
  alignas(16) unsigned char object[0x400]{};
  std::atomic<DeviceKState> state{DeviceKState::RawAllocated};
  std::atomic<int> id{-1};
  OwnedRecord *next = nullptr;
};
std::atomic<OwnedRecord *> roots{nullptr};
// MSVC x86 RTTI 描述（与原型 BuildTables 相同）。原生 6 处
// dynamic_cast<NetInputDevice*> 会读 vtable[-1] 的 COL，缺失即抛
// __non_rtti_object（tx-04-06 第一次 E2E 崩溃点 0x52F880）。
struct Hierarchy;
struct TypeDescriptor {
  const void *vtable;
  void *spare;
  char name[48];
};
struct BaseDescriptor {
  const void *type;
  std::uint32_t contained;
  std::int32_t mdisp, pdisp, vdisp;
  std::uint32_t attributes;
  const Hierarchy *hierarchy;
};
struct Hierarchy {
  std::uint32_t signature, attributes, count;
  const BaseDescriptor *const *bases;
};
struct Locator {
  std::uint32_t signature, offset, constructor_offset;
  const void *type;
  const Hierarchy *hierarchy;
};
// K 的完整表：表头是自有 COL，对象的 vtable 指针指向 slots，使 slots[-1] 为 COL。
struct DeviceTable {
  const Locator *locator = nullptr;
  uintptr_t slots[38]{};
};
static_assert(offsetof(DeviceTable, slots) == sizeof(void *));
TypeDescriptor rtti_type{};
BaseDescriptor rtti_self{};
const BaseDescriptor *rtti_bases[2]{};
Hierarchy rtti_hierarchy{};
Locator rtti_locator{};
DeviceTable table{};
const char device_name[] = "Isaac C/S Remote";
OwnedRecord *Find(const void *device) noexcept {
  for (auto *p = roots.load(std::memory_order_acquire); p; p = p->next)
    if (p->object == device)
      return p;
  return nullptr;
}
// __fastcall 的 ECX=this、EDX 占位与 thiscall 的栈清理一致，float 用 x87 返回。
void *__fastcall Delete(void *self, void *, unsigned) noexcept {
  if (auto *p = Find(self))
    p->state.store(DeviceKState::Removed);
  Trace("device_k", "event=delete object=%p never=1", self);
  return self;
}
void __fastcall Cleanup(void *self, void *) noexcept {
  if (auto *p = Find(self); p && p->state.load() != DeviceKState::Removed)
    p->state.store(DeviceKState::Retiring);
}
const char *__fastcall Name(void *, void *) noexcept { return device_name; }
bool __fastcall FalseAction(void *, void *, int) noexcept { return false; }
float __fastcall ZeroAction(void *, void *, int) noexcept { return 0; }
bool __fastcall False(void *, void *) noexcept { return false; }
int __fastcall NoButton(void *, void *) noexcept { return -1; }
float Query(void *self, int action, QueryKind kind) noexcept {
  auto *p = Find(self);
  bool enabled = false;
  if (!p || !IsMain() || p->state.load() != DeviceKState::Registered ||
      !ReadAt(self, offset::DeviceEnabled, enabled) || !enabled)
    return 0;
  float result = 0;
  if (JoinQuery(self, action, kind, result))
    return result;
  return OverrideQuery(self, action, kind, result) ? result : 0;
}
bool __fastcall Pressed(void *self, void *, int action) noexcept {
  return Query(self, action, QueryKind::Pressed) != 0;
}
bool __fastcall Triggered(void *self, void *, int action) noexcept {
  return Query(self, action, QueryKind::Triggered) != 0;
}
float __fastcall Scalar(void *self, void *, int action) noexcept {
  return Query(self, action, QueryKind::Scalar);
}
bool __fastcall Poll(void *self, void *) noexcept {
  auto *p = Find(self);
  return p && p->state.load() == DeviceKState::Registered;
}
// 自有类型 RemoteInputDevice 单继承 InputDeviceBase：层次为“自身 + 基类 COL 的
// bases[0]”。到 InputDeviceBase 的转换成功，到 NetInputDevice/Keyboard/Gamepad
// 等派生类的转换返回 null，与物理设备走同一非联机分支。基类 COL 与静态核对
// （COL 0x745F4C、单继承 1 个基类）不符时失败关闭，不注册 K。
bool PrepareRtti() noexcept {
  const Locator *base = nullptr;
  Locator base_locator{};
  Hierarchy base_hierarchy{};
  const BaseDescriptor *base_self = nullptr;
  TypeDescriptor base_type{};
  constexpr char base_name[] = ".?AVInputDeviceBase@Input@KAGE@@";
  if (!ReadMemory(reinterpret_cast<void *>(Base + offset::DeviceBaseVtable -
                                           sizeof(void *)),
                  &base, sizeof(base)) ||
      !ReadMemory(base, &base_locator, sizeof(base_locator)) ||
      base_locator.signature != 0 || base_locator.offset != 0 ||
      !ReadMemory(base_locator.hierarchy, &base_hierarchy,
                  sizeof(base_hierarchy)) ||
      base_hierarchy.count != 1 ||
      !ReadMemory(base_hierarchy.bases, &base_self, sizeof(base_self)) ||
      !ReadMemory(base_locator.type, &base_type, sizeof(base_type)) ||
      std::memcmp(base_type.name, base_name, sizeof(base_name)) != 0)
    return false;
  constexpr char type_name[] = ".?AVRemoteInputDevice@IsaacCs@@";
  rtti_type.vtable = base_type.vtable; // type_info 的 vtable，供 typeid/name 使用
  std::memcpy(rtti_type.name, type_name, sizeof(type_name));
  // contained=1：其后紧跟 1 个基类项；attributes=0x40 为 BCD_HASPCHD。
  rtti_self = {&rtti_type, 1, 0, -1, 0, 0x40, &rtti_hierarchy};
  rtti_bases[0] = &rtti_self;
  rtti_bases[1] = base_self;
  rtti_hierarchy = {0, 0, 2, rtti_bases};
  rtti_locator = {0, 0, 0, &rtti_type, &rtti_hierarchy};
  table.locator = &rtti_locator;
  return true;
}
// 在已有完整表上仅覆写指定 13 槽；不调用物理派生类实现，不修改原表。
bool PrepareTable() noexcept {
  if (table.locator)
    return true;
  if (!ReadMemory(reinterpret_cast<void *>(Base + offset::DeviceBaseVtable),
                  table.slots, sizeof(table.slots)))
    return false;
  auto *slots = table.slots;
  slots[0] = reinterpret_cast<uintptr_t>(&Delete);
  slots[2] = reinterpret_cast<uintptr_t>(&Cleanup);
  slots[3] = reinterpret_cast<uintptr_t>(&Name);
  slots[5] = slots[6] = reinterpret_cast<uintptr_t>(&FalseAction);
  slots[7] = reinterpret_cast<uintptr_t>(&ZeroAction);
  slots[8] = reinterpret_cast<uintptr_t>(&Pressed);
  slots[9] = reinterpret_cast<uintptr_t>(&Triggered);
  slots[10] = reinterpret_cast<uintptr_t>(&Scalar);
  slots[11] = reinterpret_cast<uintptr_t>(&NoButton);
  slots[26] = slots[27] = reinterpret_cast<uintptr_t>(&False);
  slots[35] = reinterpret_cast<uintptr_t>(&Poll);
  if (!PrepareRtti()) {
    Trace("device_k", "event=rtti_invalid");
    return false;
  }
  return true;
}
} // namespace
bool IsDeviceK(const void *device) noexcept { return Find(device) != nullptr; }
bool IsDeviceKId(int id) noexcept {
  if (id < 0)
    return false;
  for (auto *p = roots.load(std::memory_order_acquire); p; p = p->next)
    if (p->id.load(std::memory_order_acquire) == id)
      return true;
  return false;
}
const char *DeviceKStateName(DeviceKState s) noexcept {
  switch (s) {
  case DeviceKState::Empty:
    return "Empty";
  case DeviceKState::RawAllocated:
    return "RawAllocated";
  case DeviceKState::BaseConstructed:
    return "BaseConstructed";
  case DeviceKState::Initialized:
    return "Initialized";
  case DeviceKState::Registered:
    return "Registered";
  case DeviceKState::Retiring:
    return "Retiring";
  case DeviceKState::Removed:
    return "Removed";
  }
  return "UNKNOWN";
}
void *DeviceK::Object() const noexcept {
  auto *p = static_cast<OwnedRecord *>(record_);
  return p ? p->object : nullptr;
}
DeviceKState DeviceK::State() const noexcept {
  auto *p = static_cast<OwnedRecord *>(record_);
  return p ? p->state.load() : DeviceKState::Empty;
}
int DeviceK::Id() const noexcept {
  int id = -1;
  ReadAt(Object(), offset::DeviceId, id);
  return id;
}
bool DeviceK::CreateOnce() noexcept {
  PersistenceOperation persistence("device_register");
  if (!IsMain() || attempted_)
    return false;
  attempted_ = true;
  if (!HostPatchesInstalled() || !PrepareTable())
    return false;
  InputLease lease(true);
  if (!lease.Ready())
    return false;
  HMODULE pinned = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&Query), &pinned))
    return false;
  auto *p = new (std::nothrow) OwnedRecord;
  if (!p)
    return false;
  record_ = p;
  Trace("device_k", "event=allocated object=%p never=1", Object());
  p->next = roots.load(std::memory_order_relaxed);
  roots.store(p, std::memory_order_release);
  // 从进入构造起即 NEVER；异常时不析构、不重试，不声称部分 Init 已恢复。
  p->state.store(DeviceKState::BaseConstructed);
  try {
    reinterpret_cast<void *(__thiscall *)(void *)>(Base + offset::DeviceCtor)(
        p->object);
    p->id.store(Id(), std::memory_order_release);
    Trace("device_k", "event=constructed id=%d object=%p never=1", Id(), Object());
    if (Id() < 0 || DeviceIdOccurrences(Id()) != 0)
      return false;
    const char *name = device_name;
    std::memcpy(p->object + offset::DeviceName, &name, sizeof(name));
    auto **vtable = *reinterpret_cast<void ***>(p->object);
    if (!reinterpret_cast<bool(__thiscall *)(void *, void *)>(vtable[1])(
            p->object, nullptr))
      return false;
    p->state.store(DeviceKState::Initialized);
    Trace("device_k", "event=initialized id=%d object=%p never=1", Id(), Object());
    *reinterpret_cast<uintptr_t **>(p->object) = table.slots;
    const auto manager = reinterpret_cast<void *>(Base + offset::InputManager);
    auto **slots = *reinterpret_cast<void ***>(manager);
    registration_attempted_ = true;
    reinterpret_cast<void(__thiscall *)(void *, void *)>(slots[45])(manager,
                                                                    p->object);
    if (DeviceOccurrences(p->object) != 1 || DeviceIdOccurrences(Id()) != 1)
      return false;
    p->state.store(DeviceKState::Registered);
    Trace("device_k", "event=registered id=%d object=%p never=1", Id(),
          Object());
    return true;
  } catch (...) {
    Trace("device_k", "event=create_exception object=%p never=1", Object());
    return false;
  }
}
void DeviceK::Neutralize() noexcept {
  auto *p = static_cast<OwnedRecord *>(record_);
  if (p && p->state.load() != DeviceKState::Removed && p->state.load() != DeviceKState::Retiring) {
    p->state.store(DeviceKState::Retiring);
    Trace("device_k", "event=retiring object=%p neutral=1 never=1", Object());
  }
}
// 主线程持物理后端锁执行唯一移除；槽 0 通知与 vector 缺席必须同时成立。
bool DeviceK::RemoveOnce() noexcept {
  PersistenceOperation persistence("device_remove");
  if (!IsMain())
    return false;
  if (!record_)
    return true;
  InputLease lease(true);
  if (!lease.Ready())
    return false;
  const auto count = DeviceOccurrences(Object());
  if (State() == DeviceKState::Removed)
    return count == 0;
  if (count == 0)
    return !registration_attempted_;
  if (count != 1 || removal_attempted_)
    return false;
  removal_attempted_ = true;
  Neutralize();
  try {
    auto *manager = reinterpret_cast<void *>(Base + offset::InputManager);
    auto **slots = *reinterpret_cast<void ***>(manager);
    reinterpret_cast<void(__thiscall *)(void *, void *)>(slots[46])(manager,
                                                                    Object());
  } catch (...) {
    return false;
  }
  return State() == DeviceKState::Removed && DeviceOccurrences(Object()) == 0;
}
} // namespace cs::game
