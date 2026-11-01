// tx-04-08 E：main 最后的关闭判断常驻桥。原 cmp 只执行一次，未请求关闭时纯透传。
#include "runtime.h"
#include "game/bridge.h"
#include "ops/world_commands.h"
#include "ASMPatcher.hpp"
#include <string>
namespace cs {
namespace {
#include "exit_patch.inc"
uintptr_t exit_resume = 0, exit_loop = 0;
bool attempted = false, installed = false;
extern "C" bool __stdcall CsWorldExitDecision() noexcept {
  const bool client_ready=game::ClientProcessExitDecision();
  const bool world_ready=WorldProcessExitDecision();
  return client_ready&&world_ready;
}

// J273 main 的 EAX 已由原指令载入窗口；不重新解释/写 GLFW 结构。
// 原 cmp 与条件跳转完整搬移。只有原 ZF=1 的退出请求才调用自有策略。
// C++ 通知前后保存 GPR、原比较 flags、x87/MXCSR/XMM，主栈保持原值。
__declspec(naked) void ExitCheckShim() {
  __asm {
    cmp byte ptr [eax+8], 1
    jnz continue_loop
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    call CsWorldExitDecision
    test al, al
    jnz decision_done
    // 延后时仅改变本次分支的 ZF；游戏的 close byte 与其余原比较 flags 原样保留。
    and dword ptr [ebx+32], 0FFFFFFBFh
  decision_done:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jnz continue_loop
    jmp dword ptr [exit_resume]
  continue_loop:
    jmp dword ptr [exit_loop]
  }
}
}
bool WorldExitPatchInstalled() noexcept { return installed; }
void InstallWorldExitPatch() noexcept {
  if (attempted || !IsMain()) return;
  attempted = true;
  if (!WorldObservationArmed() || !Capabilities[size_t(Group::World)]) return;
  if (!PatternMatches(ExitCheckRva, ExitCheckPattern)) {
    Capabilities[size_t(Group::World)] = false;
    Trace("byte_patch", "id=world_process_exit ok=0 reason=instruction_mismatch"); return;
  }
  try {
    exit_resume = Base + ExitResumeRva; exit_loop = Base + ExitLoopRva;
    ASMPatch patch; patch.AddRelativeJump(reinterpret_cast<void *>(&ExitCheckShim));
    if (patch.Length() > ExitCheckBytes) { Capabilities[size_t(Group::World)] = false; return; }
    patch.AddBytes(std::string(ExitCheckBytes - patch.Length(), '\x90'));
    auto *target = reinterpret_cast<unsigned char *>(Base + ExitCheckRva);
    sASMPatcher.FlatPatch(target, &patch);
    installed = FlushInstructionCache(GetCurrentProcess(), target, ExitCheckBytes) != FALSE;
    if (!installed) Capabilities[size_t(Group::World)] = false;
    Trace("byte_patch", "id=world_process_exit ok=%u resident=1 rva=%x", unsigned(installed), unsigned(ExitCheckRva));
  } catch (...) {
    Capabilities[size_t(Group::World)] = false;
    Trace("byte_patch", "id=world_process_exit ok=0 reason=asm_patch_exception");
  }
}
}
