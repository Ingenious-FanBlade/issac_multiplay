#include "game/common/context.h"
#include "game/participants/device_k.h"
#include "runtime.h"
#include "ops/world_commands.h"
#include <cstdio>
// SDK 补丁编码器留在 C++17 单元；Odd owner 通知经 context.h 的值接口传递。
#include "ASMPatcher.hpp"
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
  InstallWorldExitPatch();
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
  try {
    // 官方 ASMPatch 按原调用点地址编码相对跳转；FlatPatch 原位安装，不增加
    // trampoline 或返回地址，裸 shim 的寄存器、栈与完整指令重放约定保持不变。
    ASMPatch patch;
    patch.AddRelativeJump(reinterpret_cast<void *>(&OddNormalShim));
    if (patch.Length() != sizeof(OddExpected)) {
      Capabilities[size_t(Group::Player)] = false;
      Trace("byte_patch", "id=odd_normal ok=0 reason=patch_size_mismatch");
      return;
    }
    resume = Base + OddResumeRva;
    sASMPatcher.FlatPatch(target, &patch);
    // SDK 1.1.3 的 FlatPatch 对缓存刷新失败只记日志；保留显式结果检查，
    // 失败时停用能力，已经写入的常驻补丁不回滚、不自动重试。
    installed = FlushInstructionCache(GetCurrentProcess(), target,
                                      sizeof(OddExpected)) != FALSE;
    if (!installed) {
      Capabilities[size_t(Group::Player)] = false;
      Trace("byte_patch", "id=odd_normal ok=0 reason=flush error=%lu",
            GetLastError());
      return;
    }
    Trace("byte_patch", "id=odd_normal ok=1 resident=1 backend=repentogon rva=%x resume=%x",
          unsigned(OddCallRva), unsigned(OddResumeRva));
  } catch (const std::exception &error) {
    Capabilities[size_t(Group::Player)] = false;
    Trace("byte_patch", "id=odd_normal ok=0 reason=asm_patch_exception message=%s",
          error.what());
  } catch (...) {
    Capabilities[size_t(Group::Player)] = false;
    Trace("byte_patch", "id=odd_normal ok=0 reason=asm_patch_exception message=unknown");
  }
}
} // namespace cs
