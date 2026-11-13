#include "game/client/display/client_display_bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include "ASMPatcher.hpp"

namespace cs {
namespace {
#include "display_patch.inc"
std::uintptr_t angel_original=0;
bool attempted=false,installed=false;
extern "C" bool __stdcall CsClientAngelShare(float *value) noexcept {
  return game::ClientDisplayAngelShare(*value);
}
// 仅替换两个 HUD 调用点。保留调用者寄存器及 SIMD 状态，只按原 ABI 返回 XMM0。
__declspec(naked) void AngelShare() {
  __asm {
    pushfd
    pushad
    mov ebx,esp
    sub esp,544
    and esp,-16
    fxsave [esp]
    lea eax,[esp+512]
    push eax
    call CsClientAngelShare
    test al,al
    jz passthrough
    fxrstor [esp]
    movss xmm0,dword ptr [esp+512]
    mov esp,ebx
    popad
    popfd
    ret
  passthrough:
    fxrstor [esp]
    mov esp,ebx
    popad
    popfd
    jmp dword ptr [angel_original]
  }
}
}
void InstallClientDisplayPatch() noexcept {
  if(attempted||!IsMain()||!Capabilities[size_t(Group::Client)])return;
  attempted=true;
  for(unsigned i=0;i<2;++i)if(!PatternMatches(AngelCallRva[i],AngelCallPattern[i])){
    Trace("byte_patch","id=client_stat_angel_share ok=0 reason=instruction_mismatch");return;
  }
  angel_original=Base+AngelOriginalRva;
  try{
    for(const auto rva:AngelCallRva){
      ASMPatch patch;patch.AddInternalCall(reinterpret_cast<void *>(&AngelShare));
      if(patch.Length()!=5)return;
      auto *target=reinterpret_cast<void *>(Base+rva);
      sASMPatcher.FlatPatch(target,&patch);
      if(!FlushInstructionCache(GetCurrentProcess(),target,5))return;
    }
    installed=true;
  }catch(...){Trace("byte_patch","id=client_stat_angel_share ok=0 reason=asm_patch_exception");}
}
bool ClientDisplayPatchInstalled() noexcept {return installed;}
}
