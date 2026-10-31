#include "hooks/random_runtime.h"
#include "game/world_sources/random_sources.h"
#include "runtime.h"
#include "mologie_detours.h"
#include <array>
namespace cs {
namespace {
// source 0/1为CRT，2为REPENTOGON全局RandomFloat，3/4为Lua math随机函数。
struct Slot final { void *target=nullptr,*original=nullptr; void *detour=nullptr; unsigned import_mask=0; bool attempted=false,installed=false; };
std::array<Slot,5> slots{};
std::array<bool,5> declared{};
// CRT按已装载模块的实际IAT接入；先发布original再原子改槽，不在其他线程可能执行的CRT正文上热改指令。
unsigned PatchCrtImports(HMODULE module,void *target,void *wrapper) noexcept {
  if(!module)return 0;auto *base=reinterpret_cast<unsigned char *>(module);
  auto *dos=reinterpret_cast<IMAGE_DOS_HEADER *>(base);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;
  auto *nt=reinterpret_cast<IMAGE_NT_HEADERS32 *>(base+dos->e_lfanew);
  const auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  if(!directory.VirtualAddress)return 0;unsigned patched=0;
  for(auto *d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base+directory.VirtualAddress);d->Name;++d){
    for(auto *slot=reinterpret_cast<IMAGE_THUNK_DATA32 *>(base+d->FirstThunk);slot->u1.Function;++slot){
      if(reinterpret_cast<void *>(slot->u1.Function)==wrapper){++patched;continue;}
      if(reinterpret_cast<void *>(slot->u1.Function)!=target)continue;
      DWORD old=0;if(!VirtualProtect(&slot->u1.Function,4,PAGE_READWRITE,&old))continue;
      InterlockedExchangePointer(reinterpret_cast<void *volatile *>(&slot->u1.Function),wrapper);
      DWORD ignored=0;VirtualProtect(&slot->u1.Function,4,old,&ignored);++patched;
    }
  }return patched;
}
int __cdecl CrtRand() {
  game::NativeRandomCall call(nullptr,game::RandomAlgorithm::CrtRand,"CRT.rand");
  const auto value=reinterpret_cast<int (__cdecl *)()>(slots[0].original)();
  call.Returned(std::uint32_t(value));return value;
}
void __cdecl CrtSeed(unsigned value) {
  game::NativeRandomCall call(nullptr,game::RandomAlgorithm::CrtRand,"CRT.srand",value,true);
  reinterpret_cast<void (__cdecl *)(unsigned)>(slots[1].original)(value);call.Returned(0);
}
template<unsigned I> int __cdecl LuaRandom(void *lua) {
  // Lua错误会longjmp；跨original只保存平凡整数，不留下C++析构/服务pending scope。
  const auto ticket=game::NativeRandomLuaEnter(I,lua);(void)ticket;
  const auto result=reinterpret_cast<int (__cdecl *)(void *)>(slots[I].original)(lua);
  game::NativeRandomLuaReturned(I,lua,result);return result;
}
}
void *RuntimeRandomOriginal(unsigned source) noexcept { return source<slots.size()?slots[source].original:nullptr; }
bool RuntimeRandomHookInstalled(unsigned source) noexcept { return source<slots.size()&&slots[source].installed; }
unsigned RuntimeRandomImportMask(unsigned source) noexcept { return source<slots.size()?slots[source].import_mask:0; }
void InstallRuntimeRandomHook(unsigned source,void *target) noexcept {
  if(!IsMain()||source>=slots.size()||!target||!declared[source])return;
  auto &s=slots[source];if(source>=2&&(s.installed||s.attempted))return;
  if(source<=1&&s.import_mask==7)return;
  s.attempted=true;
  MEMORY_BASIC_INFORMATION memory{};
  if(!VirtualQuery(target,&memory,sizeof(memory))||memory.State!=MEM_COMMIT||
     !(memory.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return;
  void *wrappers[]={reinterpret_cast<void *>(&CrtRand),reinterpret_cast<void *>(&CrtSeed),
    reinterpret_cast<void *>(&LuaRandom<2>),reinterpret_cast<void *>(&LuaRandom<3>),reinterpret_cast<void *>(&LuaRandom<4>)};
  try {
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(target),&pinned))return;
    if(!s.original)game::PrepareRuntimeRandomHook(source,target);
    if(source<=1){
      s.original=target;s.target=target;
      HMODULE self=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&InitializeRuntimeRandomHooks),&self);
      // 官方REPENTOGON可晚于本DLL的ModInit加载；Lua注册边界重新接入缺失模块，既有槽不卸载。
      HMODULE modules[]={GetModuleHandleW(nullptr),GetModuleHandleW(L"zhlREPENTOGON.dll"),self};
      const auto before=s.import_mask;unsigned count=0;
      for(unsigned i=0;i<3;++i){
        const auto matched=PatchCrtImports(modules[i],target,wrappers[source]);count+=matched;
        if(matched)s.import_mask|=1u<<i;
        if(!(before&(1u<<i)))Trace("random_runtime_import","source=%u module=%u loaded=%u matched=%u",source,i,unsigned(modules[i]!=nullptr),matched);
      }
      s.installed=s.import_mask==7;Trace("random_runtime_hook","source=%u installed=%u iat_slots=%u module_mask=%u",source,unsigned(s.installed),count,s.import_mask);return;
    }
    // 与ZHL相同的Detour实现；只从中央生成清单安装，永不析构/恢复目标字节。
    auto *d=new MologieDetours::Detour<void *>(target,wrappers[source]);
    s.detour=d;s.target=target;s.original=d->GetOriginalFunction();s.installed=true;
    Trace("random_runtime_hook","source=%u installed=1",source);
  }catch(...){Trace("random_runtime_hook","source=%u installed=0 reason=detour",source);}
}
void InitializeRuntimeRandomHooks() noexcept {
  if(!IsMain())return;
  const RuntimeRandomHookSpec entries[]={
#include "random_runtime.inc"
  };
  for(const auto &e:entries){
    if(e.source<declared.size())declared[e.source]=true;
    if(e.module&&*e.module&&e.symbol&&*e.symbol){
      const auto m=GetModuleHandleA(e.module);
      if(m)InstallRuntimeRandomHook(e.source,reinterpret_cast<void *>(GetProcAddress(m,e.symbol)));
    }
  }
}
}
