// tx-04-08 E：main 最后的关闭判断常驻桥。原 cmp 只执行一次，未请求关闭时纯透传。
#include "runtime.h"
#include "game/bridge.h"
#include "ops/world_commands.h"
#include "ASMPatcher.hpp"
#include "binding/offsets.h"
#include <string>
namespace cs {
namespace {
#include "exit_patch.inc"
#include "shutdown_patch.inc"
uintptr_t exit_resume = 0, exit_loop = 0;
uintptr_t shutdown_counter_resume=0, shutdown_counter_skip=0;
uintptr_t shutdown_save_resume=0, shutdown_save_skip=0, shutdown_manager_slot=0;
bool attempted = false, installed = false;
bool shutdown_attempted=false, shutdown_installed=false;
extern "C" bool __stdcall CsClientShutdownProgressBlocked() noexcept {
  return game::ClientShutdownProgressBlocked();
}
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
// 原始 test/jz/cmp 完整重放。Client 只跨过 PGD counter 直接 store，绝不跨 Options。
__declspec(naked) void ShutdownCounterShim() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    call CsClientShutdownProgressBlocked
    test al, al
    jz counter_normal
    or dword ptr [ebx+32], 40h
    jmp counter_restored
  counter_normal:
    and dword ptr [ebx+32], 0FFFFFFBFh
  counter_restored:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jz counter_skip
    test eax, eax
    jz counter_skip
    cmp dword ptr [eax+25BA8h], 0
    jmp dword ptr [shutdown_counter_resume]
  counter_skip:
    jmp dword ptr [shutdown_counter_skip]
  }
}
// 原始 Manager 载入在两条路径均执行；Client 仅跳过 PGD dirty/backup/save。
__declspec(naked) void ShutdownSaveShim() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    call CsClientShutdownProgressBlocked
    test al, al
    jz save_normal
    or dword ptr [ebx+32], 40h
    jmp save_restored
  save_normal:
    and dword ptr [ebx+32], 0FFFFFFBFh
  save_restored:
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    mov eax, dword ptr [shutdown_manager_slot]
    mov eax, dword ptr [eax]
    jz save_skip
    jmp dword ptr [shutdown_save_resume]
  save_skip:
    jmp dword ptr [shutdown_save_skip]
  }
}
}
bool WorldExitPatchInstalled() noexcept { return installed; }
bool ClientShutdownPatchInstalled() noexcept {return shutdown_installed;}
void InstallClientShutdownPatch() noexcept {
  if(shutdown_attempted||!IsMain())return;
  shutdown_attempted=true;
  if(!Capabilities[size_t(Group::Client)])return;
  if(!PatternMatches(ShutdownCounterRva,ShutdownCounterPattern)||
     !PatternMatches(ShutdownSaveRva,ShutdownSavePattern)){
    Capabilities[size_t(Group::Client)]=false;
    Trace("byte_patch","id=client_shutdown ok=0 reason=instruction_mismatch");return;
  }
  try {
    shutdown_counter_resume=Base+ShutdownCounterResumeRva;
    shutdown_counter_skip=Base+ShutdownCounterSkipRva;
    shutdown_save_resume=Base+ShutdownSaveResumeRva;
    shutdown_save_skip=Base+ShutdownSaveSkipRva;
    shutdown_manager_slot=Base+offset::Manager;
    ASMPatch counter;counter.AddRelativeJump(reinterpret_cast<void *>(&ShutdownCounterShim));
    ASMPatch save;save.AddRelativeJump(reinterpret_cast<void *>(&ShutdownSaveShim));
    if(counter.Length()>ShutdownCounterBytes||save.Length()>ShutdownSaveBytes){
      Capabilities[size_t(Group::Client)]=false;return;
    }
    counter.AddBytes(std::string(ShutdownCounterBytes-counter.Length(),'\x90'));
    save.AddBytes(std::string(ShutdownSaveBytes-save.Length(),'\x90'));
    auto *counter_target=reinterpret_cast<unsigned char *>(Base+ShutdownCounterRva);
    auto *save_target=reinterpret_cast<unsigned char *>(Base+ShutdownSaveRva);
    sASMPatcher.FlatPatch(counter_target,&counter);
    sASMPatcher.FlatPatch(save_target,&save);
    shutdown_installed=FlushInstructionCache(GetCurrentProcess(),counter_target,ShutdownCounterBytes)!=FALSE&&
                       FlushInstructionCache(GetCurrentProcess(),save_target,ShutdownSaveBytes)!=FALSE;
    if(!shutdown_installed)Capabilities[size_t(Group::Client)]=false;
    Trace("byte_patch","id=client_shutdown ok=%u resident=1",unsigned(shutdown_installed));
  }catch(...){Capabilities[size_t(Group::Client)]=false;Trace("byte_patch","id=client_shutdown ok=0 reason=asm_patch_exception");}
}
void InstallWorldExitPatch() noexcept {
  if (attempted || !IsMain()) return;
  attempted = true;
  // Client 进程关闭退休独立于 world 录制/观察能力。
  if (!Capabilities[size_t(Group::World)] && !Capabilities[size_t(Group::Client)]) return;
  if (!PatternMatches(ExitCheckRva, ExitCheckPattern)) {
    Capabilities[size_t(Group::World)] = false;
    Capabilities[size_t(Group::Client)] = false;
    Trace("byte_patch", "id=world_process_exit ok=0 reason=instruction_mismatch"); return;
  }
  try {
    exit_resume = Base + ExitResumeRva; exit_loop = Base + ExitLoopRva;
    ASMPatch patch; patch.AddRelativeJump(reinterpret_cast<void *>(&ExitCheckShim));
    if (patch.Length() > ExitCheckBytes) { Capabilities[size_t(Group::World)] = false; Capabilities[size_t(Group::Client)] = false; return; }
    patch.AddBytes(std::string(ExitCheckBytes - patch.Length(), '\x90'));
    auto *target = reinterpret_cast<unsigned char *>(Base + ExitCheckRva);
    sASMPatcher.FlatPatch(target, &patch);
    installed = FlushInstructionCache(GetCurrentProcess(), target, ExitCheckBytes) != FALSE;
    if (!installed) {Capabilities[size_t(Group::World)] = false; Capabilities[size_t(Group::Client)] = false;}
    Trace("byte_patch", "id=world_process_exit ok=%u resident=1 rva=%x", unsigned(installed), unsigned(ExitCheckRva));
  } catch (...) {
    Capabilities[size_t(Group::World)] = false;
    Capabilities[size_t(Group::Client)] = false;
    Trace("byte_patch", "id=world_process_exit ok=0 reason=asm_patch_exception");
  }
}
}
