#include "runtime.h"
#include "game/bridge.h"
#include "ASMPatcher.hpp"
#include <string>
namespace cs {
namespace {
#include "backdrop_patch.inc"
uintptr_t backdrop_resume[2]{},backdrop_skip[2]{},backdrop_random=0;
bool backdrop_attempted=false,backdrop_installed=false;
extern "C" bool __stdcall CsBackdropSkipEffect(void *value) noexcept {return game::ClientBackdropSkipDirectEffect(value);}
// GPR/flags及x87/MXCSR/XMM全保存；site无临时参数，Source/Off重放原call后回原链。
#define BACKDROP_SHIM(name,index) \
__declspec(naked) void name(){ \
  __asm { pushfd } \
  __asm { pushad } \
  __asm { mov ebx,esp } \
  __asm { sub esp,528 } \
  __asm { and esp,-16 } \
  __asm { fxsave [esp] } \
  __asm { push dword ptr [ebx] } \
  __asm { call CsBackdropSkipEffect } \
  __asm { test al,al } \
  __asm { jnz skipped } \
  __asm { fxrstor [esp] } \
  __asm { mov esp,ebx } \
  __asm { popad } \
  __asm { popfd } \
  __asm { call dword ptr [backdrop_random] } \
  __asm { jmp dword ptr [backdrop_resume+index*4] } \
  __asm { skipped: fxrstor [esp] } \
  __asm { mov esp,ebx } \
  __asm { popad } \
  __asm { popfd } \
  __asm { jmp dword ptr [backdrop_skip+index*4] } \
}
BACKDROP_SHIM(BackdropEffect1,0)
BACKDROP_SHIM(BackdropEffect2,1)
#undef BACKDROP_SHIM
}
bool ClientBackdropPatchInstalled() noexcept {return backdrop_installed;}
void InstallClientBackdropPatch() noexcept {
  if(backdrop_attempted||!IsMain())return;backdrop_attempted=true;
  if(!Capabilities[size_t(Group::Client)])return;
  for(unsigned i=0;i<2;++i)if(!PatternMatches(BackdropEffectRva[i],BackdropEffectPattern[i])){
    Capabilities[size_t(Group::Client)]=false;Trace("byte_patch","id=client_backdrop_effect ok=0 reason=instruction_mismatch");return;
  }
  try{
    backdrop_random=Base+BackdropRandomRva;
    const void *entries[]={reinterpret_cast<void *>(&BackdropEffect1),reinterpret_cast<void *>(&BackdropEffect2)};
    bool flushed=true;
    for(unsigned i=0;i<2;++i){
      backdrop_resume[i]=Base+BackdropEffectResume[i];backdrop_skip[i]=Base+BackdropEffectSkip[i];
      ASMPatch patch;patch.AddRelativeJump(const_cast<void *>(entries[i]));
      if(patch.Length()>5){Capabilities[size_t(Group::Client)]=false;return;}
      patch.AddBytes(std::string(5-patch.Length(),'\x90'));
      auto *target=reinterpret_cast<unsigned char *>(Base+BackdropEffectRva[i]);sASMPatcher.FlatPatch(target,&patch);
      flushed=FlushInstructionCache(GetCurrentProcess(),target,5)!=FALSE&&flushed;
    }
    backdrop_installed=flushed;if(!flushed)Capabilities[size_t(Group::Client)]=false;
    Trace("byte_patch","id=client_backdrop_effect ok=%u resident=1",unsigned(flushed));
  }catch(...){Capabilities[size_t(Group::Client)]=false;Trace("byte_patch","id=client_backdrop_effect ok=0 reason=asm_patch_exception");}
}
}
