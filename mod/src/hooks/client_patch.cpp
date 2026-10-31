#include "runtime.h"
#include "binding/offsets.h"
#include "game/bridge.h"
#include "game/client_isolation.h"
#include "game/client.h"
#include "game/presentation.h"
#include "game/native_room_grid.h"
#include "ASMPatcher.hpp"
#include <string>

namespace cs {
namespace {
#include "client_patches.inc"
uintptr_t render_resume=0,render_client=0;
uintptr_t counters_resume=0,counters_client=0,counters_zero=0,
          flush_resume=0,flush_client=0,manager_address=0;
uintptr_t controls_character_resume=0,controls_character_client=0,controls_device_resume=0,controls_device_client=0,game_address=0;
bool attempted=false,installed=false;
extern "C" int __stdcall CsNativeControlsCharacter() noexcept {return game::NativeRoomGridControlCharacter();}
extern "C" int __stdcall CsNativeControlsDevice() noexcept {return game::NativeRoomGridContextActive()?game::ClientDeviceKey():-1;}
// 全寄存器／FP保存后只替换两个只读来源；后续角色姿态、按键查询与缓存绘制仍走原生函数。
__declspec(naked) void NativeControlsCharacterShim(){
  __asm {
    pushfd
    pushad
    mov ebx,esp
    sub esp,528
    and esp,-16
    fxsave [esp]
    call CsNativeControlsCharacter
    mov [ebx+28],eax
    fxrstor [esp]
    mov esp,ebx
    popad
    popfd
    cmp eax,0
    jl native_path
    mov [ebp-3Ch],eax
    jmp dword ptr [controls_character_client]
  native_path:
    mov eax,[ecx+4AD14h]
    jmp dword ptr [controls_character_resume]
  }
}
__declspec(naked) void NativeControlsDeviceShim(){
  __asm {
    pushfd
    pushad
    mov ebx,esp
    sub esp,528
    and esp,-16
    fxsave [esp]
    call CsNativeControlsDevice
    mov [ebx+28],eax
    fxrstor [esp]
    mov esp,ebx
    popad
    popfd
    mov esi,dword ptr [game_address]
    mov esi,[esi]
    cmp eax,0
    jl native_path
    mov edx,eax
    mov [ebp-54h],edx
    jmp dword ptr [controls_device_client]
  native_path:
    jmp dword ptr [controls_device_resume]
  }
}
// 当前线程不是主线程时只跳过未知原生场景树；不在错误context强行绘制。
extern "C" bool __stdcall CsClientRenderDispatch() noexcept {
  if(!game::ClientBusy())return false;
  if(game::NativeRoomGridOwnsDraw())game::NativeRoomGridDraw();else game::PresentationDraw();return true;
}
// J273 graphics prelude已完成，局部render-target标记保留在原栈上。
// Off/Host重放原cmp一次；Client恢复完整GPR/FP后走加载Manager指针的尾部入口，
// 不从92F046直接跳入并错误沿用临时EAX。
__declspec(naked) void ClientRenderShim(){
  __asm {
    pushfd
    pushad
    mov ebx,esp
    sub esp,528
    and esp,-16
    fxsave [esp]
    call CsClientRenderDispatch
    test al,al
    jz native_path
    or dword ptr [ebx+32],40h
    jmp decided
  native_path:
    and dword ptr [ebx+32],0FFFFFFBFh
  decided:
    fxrstor [esp]
    mov esp,ebx
    popad
    popfd
    jz client_path
    cmp byte ptr [edi+4A3D4h],0
    jmp dword ptr [render_resume]
  client_path:
    jmp dword ptr [render_client]
  }
}
extern "C" bool __stdcall CsClientCountersProtected() noexcept {
  const bool protect=game::ClientProgressProtected();
  if(protect)Trace("client_isolation","entry=shutdown_counters original=0");return protect;
}
extern "C" bool __stdcall CsClientFlushProtected() noexcept {
  const bool protect=game::ClientProgressProtected();
  if(protect)Trace("client_isolation","entry=shutdown_flush original=0");return protect;
}
// Shutdown两处策略沿用同一全寄存器／浮点保存方式；不跳过正常资源析构链。
__declspec(naked) void ClientCountersShim(){
  __asm {
    pushfd
    pushad
    mov ebx,esp
    sub esp,528
    and esp,-16
    fxsave [esp]
    call CsClientCountersProtected
    test al,al
    jz native_path
    or dword ptr [ebx+32],40h
    jmp decided
  native_path:
    and dword ptr [ebx+32],0FFFFFFBFh
  decided:
    fxrstor [esp]
    mov esp,ebx
    popad
    popfd
    jz client_path
    test eax,eax
    jz zero_path
    cmp dword ptr [eax+25BA8h],0
    jmp dword ptr [counters_resume]
  zero_path:
    jmp dword ptr [counters_zero]
  client_path:
    jmp dword ptr [counters_client]
  }
}
__declspec(naked) void ClientFlushShim(){
  __asm {
    pushfd
    pushad
    mov ebx,esp
    sub esp,528
    and esp,-16
    fxsave [esp]
    call CsClientFlushProtected
    test al,al
    jz native_path
    or dword ptr [ebx+32],40h
    jmp decided
  native_path:
    and dword ptr [ebx+32],0FFFFFFBFh
  decided:
    fxrstor [esp]
    mov esp,ebx
    popad
    popfd
    jz client_path
    mov eax,dword ptr [manager_address]
    mov eax,[eax]
    jmp dword ptr [flush_resume]
  client_path:
    jmp dword ptr [flush_client]
  }
}
}
bool ClientPatchesInstalled() noexcept {return installed;}
void InstallClientPatches() noexcept {
  if(attempted||!IsMain())return;attempted=true;
  if(!Capabilities[size_t(Group::Loop)]||!PatternMatches(ClientRenderRva,ClientRenderPattern)||
     !PatternMatches(ClientCountersRva,ClientCountersPattern)||!PatternMatches(ClientFlushRva,ClientFlushPattern)||
     !PatternMatches(NativeControlsCharacterRva,NativeControlsCharacterPattern)||!PatternMatches(NativeControlsDeviceRva,NativeControlsDevicePattern)){
    Trace("client_patch","installed=0 reason=render_binding");return;
  }
  uintptr_t operand=0;std::memcpy(&operand,reinterpret_cast<const void *>(Base+ClientFlushRva+1),sizeof(operand));
  if(operand!=Base+offset::Manager){Trace("client_patch","installed=0 reason=flush_manager_operand");return;}
  std::memcpy(&operand,reinterpret_cast<const void *>(Base+NativeControlsDeviceRva+2),sizeof(operand));
  if(operand!=Base+offset::Game){Trace("client_patch","installed=0 reason=controls_game_operand");return;}
  try{
    render_resume=Base+ClientRenderResume;render_client=Base+ClientRenderAlternate;
    counters_resume=Base+ClientCountersResume;counters_client=Base+ClientCountersAlternate;counters_zero=Base+0x582D67;
    flush_resume=Base+ClientFlushResume;flush_client=Base+ClientFlushAlternate;manager_address=Base+offset::Manager;
    controls_character_resume=Base+NativeControlsCharacterResume;controls_character_client=Base+NativeControlsCharacterAlternate;
    controls_device_resume=Base+NativeControlsDeviceResume;controls_device_client=Base+NativeControlsDeviceAlternate;game_address=Base+offset::Game;
    struct Site{uintptr_t rva;unsigned bytes;void *entry;};
    const Site sites[]={{ClientRenderRva,ClientRenderBytes,reinterpret_cast<void *>(&ClientRenderShim)},
      {ClientCountersRva,ClientCountersBytes,reinterpret_cast<void *>(&ClientCountersShim)},
      {ClientFlushRva,ClientFlushBytes,reinterpret_cast<void *>(&ClientFlushShim)},
      {NativeControlsCharacterRva,NativeControlsCharacterBytes,reinterpret_cast<void *>(&NativeControlsCharacterShim)},
      {NativeControlsDeviceRva,NativeControlsDeviceBytes,reinterpret_cast<void *>(&NativeControlsDeviceShim)}};
    bool complete=true;
    for(const auto &site:sites){ASMPatch patch;patch.AddRelativeJump(site.entry);
      if(patch.Length()>site.bytes)throw std::runtime_error("client_patch_length");
      patch.AddBytes(std::string(site.bytes-patch.Length(),'\x90'));
      auto *target=reinterpret_cast<unsigned char *>(Base+site.rva);sASMPatcher.FlatPatch(target,&patch);
      complete=FlushInstructionCache(GetCurrentProcess(),target,site.bytes)!=FALSE&&complete;
    }
    installed=complete;
    Trace("client_patch","installed=%u resident=1 rva=%x",unsigned(installed),unsigned(ClientRenderRva));
  }catch(...){Trace("client_patch","installed=0 reason=exception");}
}
} // namespace cs
