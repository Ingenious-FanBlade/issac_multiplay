#include "game/context.h"
#include "game/device_k.h"
#include "runtime.h"
#include <cstring>

namespace cs {
namespace {
#include "permanent_patches.inc"
uintptr_t resume = 0;
bool attempted = false, installed = false;
extern "C" void __stdcall CsOddEnter(void *player) noexcept {
  game::EnterOddNormal(player);
}
extern "C" void __stdcall CsOddLeave() noexcept { game::LeaveOddNormal(); }

// 搬移完整的 call [edx+0Ch] 与 push 0。回调前后保存 GPR/EFLAGS、x87/MXCSR/XMM；
// 原虚调用只出现一次。裸 shim 不猜测 ZHL 桩帧；异常清理由外层 Odd/Player RAII
// 承担。
__declspec(naked) void OddNormalShim() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    push dword ptr [ebx+24]
    call CsOddEnter
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    call dword ptr [edx+0Ch]
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    call CsOddLeave
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    push 0
    jmp dword ptr [resume]
  }
}
} // namespace
bool OddPatchInstalled() noexcept { return installed; }
// 所有补丁由同一个常驻安装入口协调，启动后会话只切换逻辑状态，不恢复字节。
void InstallRuntimePatches() noexcept {
  InstallPointers();
  game::InstallHostPatches();
  if (attempted || !IsMain() || !Capabilities[size_t(Group::Player)] ||
      !Capabilities[size_t(Group::Loop)])
    return;
  attempted = true;
  static_assert(sizeof(OddExpected) == 5);
  auto *target = reinterpret_cast<unsigned char *>(Base + OddCallRva);
  if (!PatternMatches(OddCallRva, OddPattern) ||
      std::memcmp(target, OddExpected, 5)) {
    Capabilities[size_t(Group::Player)] = false;
    Trace("byte_patch", "id=odd_normal ok=0 reason=instruction_mismatch");
    return;
  }
  unsigned char jump[5]{0xE9};
  const uint32_t displacement = uint32_t(
      reinterpret_cast<uintptr_t>(&OddNormalShim) - (Base + OddCallRva + 5));
  std::memcpy(jump + 1, &displacement, 4);
  resume = Base + OddResumeRva;
  DWORD protection = 0;
  if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &protection)) {
    Capabilities[size_t(Group::Player)] = false;
    Trace("byte_patch", "id=odd_normal ok=0 reason=protect error=%lu",
          GetLastError());
    return;
  }
  std::memcpy(target, jump, 5);
  const bool flushed =
      FlushInstructionCache(GetCurrentProcess(), target, 5) != FALSE;
  DWORD unused = 0;
  const bool protected_again =
      VirtualProtect(target, 5, protection, &unused) != FALSE;
  installed = flushed && protected_again;
  if (!installed)
    Capabilities[size_t(Group::Player)] = false;
  Trace("byte_patch", "id=odd_normal ok=%u resident=1 rva=%x resume=%x",
        installed, unsigned(OddCallRva), unsigned(OddResumeRva));
}
} // namespace cs
