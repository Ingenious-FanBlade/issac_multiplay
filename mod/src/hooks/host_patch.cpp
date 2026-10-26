#include "game/device_k.h"
#include "game/bridge.h"
#include "game/join.h"
#include "runtime.h"
#include <cstdio>
#include "ASMPatcher.hpp"

namespace cs::game {
namespace {
#include "host_patches.inc"
bool attempted = false, installed = false;
uintptr_t mode_native = 0, create_native = 0, create_reject = 0;
uintptr_t resumes[11]{}, alternatives[11]{};
extern "C" bool __stdcall CsOwnedDevice(void *device) noexcept { return IsDeviceK(device); }
extern "C" bool __stdcall CsJoinMode(bool original, void *manager, int key, void *row, unsigned site) noexcept {
  return JoinMode(original, manager, key, row, site);
}
extern "C" bool __stdcall CsJoinCreate(void *manager, void *row, int *role) noexcept {
  return JoinCreate(manager, row, role);
}
// 三处 mode 都恰好调用原 predicate；helper 的 GP/EFLAGS/x87/MXCSR/XMM 副作用被隔离。
// 第三处捕获并恢复 ECX，因为它在原生返回后仍被用作 key。
__declspec(naked) void Mode1() {
  __asm {
    call dword ptr [mode_native]
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push 1
    push 0
    push dword ptr [ebx+4]
    push dword ptr [ebx+16]
    movzx eax, byte ptr [ebx+28]
    push eax
    call CsJoinMode
    mov byte ptr [ebx+28], al
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    ret
  }
}
__declspec(naked) void Mode2() {
  __asm {
    call dword ptr [mode_native]
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push 2
    push 0
    push dword ptr [ebx+4]
    push dword ptr [ebx+16]
    movzx eax, byte ptr [ebx+28]
    push eax
    call CsJoinMode
    mov byte ptr [ebx+28], al
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    ret
  }
}
__declspec(naked) void Mode3() {
  __asm {
    call dword ptr [mode_native]
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push 3
    push dword ptr [ebx]
    push dword ptr [ebx+24]
    push dword ptr [ebx+16]
    movzx eax, byte ptr [ebx+28]
    push eax
    call CsJoinMode
    mov byte ptr [ebx+28], al
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    ret
  }
}
// CALL shim 入口 role 在 [esp+4]；保存区后是 [ebx+40]。
// 拒绝时清 shim 返回地址和唯一 role 共 8 字节，再进入原生行收束分支。
__declspec(naked) void Create() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    lea eax, [ebx+40]
    push eax
    push dword ptr [ebx]
    push dword ptr [ebx+24]
    call CsJoinCreate
    test al, al
    jz refused
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [create_native]
  refused:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    add esp, 8
    jmp dword ptr [create_reject]
  }
}
// 所有 K 分支只按常驻自有对象地址判断，物理设备重放完整被覆盖指令。
__declspec(naked) void RegisterNotify() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+16]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    mov esi, [edi+14h]
    mov ebx, [ebx+8]
    jmp dword ptr [resumes+16]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [alternatives+16]
  }
}
__declspec(naked) void RemoveNotify() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+0]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    mov esi, [ebx+20h]
    cmp esi, [ebx+24h]
    jmp dword ptr [resumes+20]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [alternatives+20]
  }
}
__declspec(naked) void PollNotify() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+24]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    mov esi, [ebx+20h]
    mov eax, [ecx+8]
    jmp dword ptr [resumes+24]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    mov eax, [ecx+8]
    mov [ebp-1Ch], eax
    jmp dword ptr [alternatives+24]
  }
}
__declspec(naked) void PhysicalDisconnect() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+0]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    mov ecx, [ebp+8]
    mov eax, [edi+3F8h]
    jmp dword ptr [resumes+28]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [alternatives+28]
  }
}
__declspec(naked) void ControllerList() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+28]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    test eax, eax
    jz owned
    jmp dword ptr [resumes+32]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
  owned:
    jmp dword ptr [alternatives+32]
  }
}
__declspec(naked) void KeyConfigDraw() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+28]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    cmp dword ptr [eax+48Ch], 0
    jmp dword ptr [resumes+36]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [alternatives+36]
  }
}
__declspec(naked) void PauseDisconnect() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+28]
    call CsOwnedDevice
    test al, al
    jnz owned_saved
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    cmp dword ptr [eax+48Ch], 0
    jmp dword ptr [resumes+40]
  owned_saved:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [alternatives+40]
  }
}
} // namespace
bool HostPatchesInstalled() noexcept { return installed; }
// 统一清单、首次 Manager 主线程安装、常驻到进程结束。半安装失败仅关闭 Host 能力，
// 不创建 K、不猜测被其他插件改写的指令，也不恢复已安装补丁。
void InstallHostPatches() noexcept {
  if (attempted || !IsMain()) return;
  attempted = true;
  if (!Capabilities[size_t(Group::Join)] || !Capabilities[size_t(Group::Device)]) return;
  void *shims[] = {reinterpret_cast<void *>(&Mode1), reinterpret_cast<void *>(&Mode2),
    reinterpret_cast<void *>(&Mode3), reinterpret_cast<void *>(&Create),
    reinterpret_cast<void *>(&RegisterNotify), reinterpret_cast<void *>(&RemoveNotify),
    reinterpret_cast<void *>(&PollNotify), reinterpret_cast<void *>(&PhysicalDisconnect),
    reinterpret_cast<void *>(&ControllerList), reinterpret_cast<void *>(&KeyConfigDraw),
    reinterpret_cast<void *>(&PauseDisconnect)};
  static_assert(sizeof(shims) / sizeof(shims[0]) == sizeof(HostPatches) / sizeof(HostPatches[0]));
  for (const auto &dependency : HostDependencies) {
    if (!PatternMatches(dependency.rva, dependency.expected)) {
      Capabilities[size_t(Group::Join)] = false;
      Trace("byte_patch", "id=%s ok=0 reason=native_dependency_mismatch", dependency.id);
      return;
    }
  }
  // 物理断连补丁可能由 worker 执行；持其已有后端锁后才写入口，避免并发取指。
  InputLease lease(true);
  if (!lease.Ready()) {
    Capabilities[size_t(Group::Join)] = false;
    Trace("byte_patch", "id=host_batch ok=0 reason=input_backend_lock_unavailable");
    return;
  }
  for (const auto &patch : HostPatches) {
    if (!PatternMatches(patch.rva, patch.expected)) {
      Capabilities[size_t(Group::Join)] = false;
      CapabilityReasons[size_t(Group::Join)] += std::string(patch.id) + ":instruction_mismatch;";
      Trace("byte_patch", "id=%s ok=0 reason=instruction_mismatch", patch.id);
      return;
    }
  }
  mode_native = Base + 0x5910A0;
  create_native = Base + 0x590250;
  create_reject = Base + 0x5925E4;
  for (size_t i = 0; i < sizeof(shims) / sizeof(shims[0]); ++i) {
    const auto &patch = HostPatches[i];
    resumes[i] = Base + patch.resume;
    alternatives[i] = Base + patch.alternate;
    auto *target = reinterpret_cast<unsigned char *>(Base + patch.rva);
    try {
      // 由官方编码器区分 CALL/JMP，再用 FlatPatch 原位写入。尤其不能把
      // Join 的 CALL 换成经 PatchAt 建立的 JMP 桥，否则 Create 的返回地址
      // 与拒绝路径 add esp,8 的栈约定会改变。
      ASMPatch assembly;
      if (patch.call)
        assembly.AddInternalCall(shims[i]);
      else
        assembly.AddRelativeJump(shims[i]);
      if (assembly.Length() > patch.size) {
        Capabilities[size_t(Group::Join)] = false;
        Trace("byte_patch", "id=%s ok=0 reason=patch_size_mismatch", patch.id);
        return;
      }
      // 清单已经给出完整指令长度；显式补齐 NOP，不让安装器扩大覆盖范围。
      if (assembly.Length() < patch.size)
        assembly.AddBytes(std::string(patch.size - assembly.Length(), '\x90'));
      sASMPatcher.FlatPatch(target, &assembly);
      // SDK 对刷新失败只记日志；显式检查后才报告安装成功，失败不回滚。
      if (!FlushInstructionCache(GetCurrentProcess(), target, patch.size)) {
        Capabilities[size_t(Group::Join)] = false;
        Trace("byte_patch", "id=%s ok=0 reason=flush error=%lu", patch.id,
              GetLastError());
        return;
      }
      Trace("byte_patch", "id=%s ok=1 resident=1 backend=repentogon rva=%x",
            patch.id, unsigned(patch.rva));
    } catch (const std::exception &error) {
      Capabilities[size_t(Group::Join)] = false;
      Trace("byte_patch", "id=%s ok=0 reason=asm_patch_exception message=%s",
            patch.id, error.what());
      return;
    } catch (...) {
      Capabilities[size_t(Group::Join)] = false;
      Trace("byte_patch", "id=%s ok=0 reason=asm_patch_exception message=unknown",
            patch.id);
      return;
    }
  }
  installed = true;
}
} // namespace cs::game
