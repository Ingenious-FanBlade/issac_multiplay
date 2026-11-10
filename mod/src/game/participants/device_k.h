#pragma once
#include <cstdint>

namespace cs::game {
// 设备记录在首次原生构造之前发布为进程常驻 root，绝不复用或释放。
enum class DeviceKState : unsigned {
  Empty,
  RawAllocated,
  BaseConstructed,
  Initialized,
  Registered,
  Retiring,
  Removed
};
const char *DeviceKStateName(DeviceKState state) noexcept;
class DeviceK final {
public:
  bool CreateOnce() noexcept;
  void Neutralize() noexcept;
  bool RemoveOnce() noexcept;
  void *Object() const noexcept;
  int Id() const noexcept;
  DeviceKState State() const noexcept;

private:
  void *record_ = nullptr;
  bool attempted_ = false, removal_attempted_ = false,
       registration_attempted_ = false;
};
// 常驻身份集合只作地址比较；物理断连线程可读，不访问 Session 或 Lua。
bool IsDeviceK(const void *device) noexcept;
bool IsDeviceKId(int id) noexcept;
bool HostPatchesInstalled() noexcept;
void InstallHostPatches() noexcept;
} // namespace cs::game
