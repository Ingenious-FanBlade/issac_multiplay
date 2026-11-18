#include "game/client/display/client_display_memory.h"
#include "game/client/display/client_display_native.h"
#include "state/client_memory.h"
#include "runtime.h"
#include <array>
#include <cerrno>
#include <cstring>
#include <new>
#include <windows.h>

namespace cs::game {
void ClientDisplayRecordMemoryIssue(const ClientDisplayMemoryRef &memory,const ClientDisplayIssue &issue,const char *phase,
    std::uint64_t route,std::uint64_t scene,std::uint32_t revision) noexcept {
  if(!memory)return;
  state::ClientApplyMetrics::Issue fact;
  fact.route=route;fact.instance=scene;fact.target_version=revision;fact.expected=issue.expected;fact.actual=issue.actual;
  fact.resource_key=issue.resource;fact.field=issue.field;
  memory->ApplyMetrics().FirstIssue(1,fact,phase,issue.reason,true);
}
namespace {
#include "client_display_memory_imports.inc"
using Allocate=void *(__cdecl *)(std::size_t);
using AllocateZero=void *(__cdecl *)(std::size_t,std::size_t);
using Resize=void *(__cdecl *)(void *,std::size_t);
using Release=void (__cdecl *)(void *);
Allocate original_malloc=nullptr;
AllocateZero original_calloc=nullptr;
Resize original_realloc=nullptr;
Release original_free=nullptr;
bool installed=false,attempted=false;
thread_local ClientDisplayMemoryScope *current=nullptr;

// 原生指针及记录本体一起预付；固定桶不分配，节点与账户保留到真实释放（含 Clear 后保留的 capacity）。
// 不依赖显示 owner 或 Room 的寿命，其他线程正常释放时也按原账户销账。
struct Allocation final {
  void *pointer=nullptr;
  std::size_t bytes=0;
  ClientDisplayMemoryRef memory;
  Allocation *next=nullptr;
};
SRWLOCK allocation_lock=SRWLOCK_INIT;
std::array<Allocation *,4096> allocations{};
std::size_t Bucket(void *p) noexcept{return (reinterpret_cast<std::uintptr_t>(p)>>4)%allocations.size();}
Allocation *Detach(void *p) noexcept {
  if(!p)return nullptr;
  AcquireSRWLockExclusive(&allocation_lock);
  auto **slot=&allocations[Bucket(p)];
  while(*slot&&(*slot)->pointer!=p)slot=&(*slot)->next;
  auto *found=*slot;if(found)*slot=found->next;
  ReleaseSRWLockExclusive(&allocation_lock);
  return found;
}
void Attach(Allocation *a,void *p) noexcept {
  a->pointer=p;
  AcquireSRWLockExclusive(&allocation_lock);
  auto &head=allocations[Bucket(p)];a->next=head;head=a;
  ReleaseSRWLockExclusive(&allocation_lock);
}
void Reject() noexcept {if(current)current->Reject();errno=ENOMEM;}
// C ABI 不抛跨模块异常；malloc 返回空，由原生 operator new 使用它自己的失败/展开路径。
Allocation *Prepare(const ClientDisplayMemoryRef &memory,std::size_t bytes) noexcept {
  if(bytes>SIZE_MAX-sizeof(Allocation)){Reject();return nullptr;}
  const auto charge=bytes+sizeof(Allocation);
  try {memory->ChargeNative(charge);}catch(...){Reject();return nullptr;}
  auto *a=new(std::nothrow) Allocation;
  if(!a){memory->UnchargeNative(charge);Reject();return nullptr;}
  a->memory=memory;a->bytes=bytes;return a;
}
void Forget(Allocation *a) noexcept {
  if(!a)return;
  a->memory->UnchargeNative(a->bytes+sizeof(Allocation));delete a;
}
void *__cdecl DisplayMalloc(std::size_t bytes) noexcept {
  if(!current)return original_malloc(bytes);
  auto *a=Prepare(current->Memory(),bytes);if(!a)return nullptr;
  auto *p=original_malloc(bytes);
  if(p)Attach(a,p);else {Forget(a);Reject();}
  return p;
}
void *__cdecl DisplayCalloc(std::size_t count,std::size_t size) noexcept {
  if(!current)return original_calloc(count,size);
  if(size&&count>SIZE_MAX/size){Reject();return nullptr;}
  auto *a=Prepare(current->Memory(),count*size);if(!a)return nullptr;
  auto *p=original_calloc(count,size);
  if(p)Attach(a,p);else {Forget(a);Reject();}
  return p;
}
void __cdecl DisplayFree(void *p) noexcept {
  auto *a=Detach(p);original_free(p);Forget(a);
}
void *__cdecl DisplayRealloc(void *p,std::size_t bytes) noexcept {
  if(!p){
    if(!current)return original_realloc(nullptr,bytes);
    auto *a=Prepare(current->Memory(),bytes);if(!a)return nullptr;
    auto *result=original_realloc(nullptr,bytes);
    if(result)Attach(a,result);else {Forget(a);Reject();}
    return result;
  }
  auto *old=Detach(p);
  // UCRT realloc(p,0) 释放旧块；原生返回约定保持，不能留下悬空记账记录。
  if(!bytes){auto *result=original_realloc(p,0);Forget(old);return result;}
  if(!old&&!current)return original_realloc(p,bytes);
  // realloc 可能搬迁，先同时预留新旧块；失败原指针和原费用保持有效。
  auto *next=Prepare(old?old->memory:current->Memory(),bytes);
  if(!next){if(old)Attach(old,p);return nullptr;}
  auto *result=original_realloc(p,bytes);
  if(!result){Forget(next);if(old)Attach(old,p);Reject();return nullptr;}
  Attach(next,result);Forget(old);return result;
}
}
void ClientDisplayCharge(const ClientDisplayMemoryRef &memory,std::size_t bytes) {
  if(!memory)throw std::bad_alloc();memory->ChargeNative(bytes);
}
void ClientDisplayUncharge(const ClientDisplayMemoryRef &memory,std::size_t bytes) noexcept {
  if(memory)memory->UnchargeNative(bytes);
}
ClientDisplayMemoryScope::ClientDisplayMemoryScope(const ClientDisplayMemoryRef &memory):memory_(memory) {
  if(!memory_||!installed||!IsMain())throw std::bad_alloc();previous_=current;current=this;
}
ClientDisplayMemoryScope::~ClientDisplayMemoryScope(){current=previous_;if(rejected_&&current)current->Reject();}
void ClientDisplayMemoryScope::Complete() const {if(rejected_)throw std::bad_alloc();}
bool ClientDisplayMemoryHooksReady() noexcept{return installed;}
bool InstallClientDisplayMemoryHooks() noexcept {
  if(attempted||!IsMain()||!Base)return installed;attempted=true;
  // J273 的 new/new[]、vector 及 KAGE CPU 分配最终用这四个 UCRT heap 导入。
  // 仅改 EXE 的 IAT，避开本 DLL 记账分配的递归；原版/Host 无作用域时原样调用。
  // 在 ModInit 常驻安装，先发布所有 original，再原子改槽，不热改 CRT 指令或退出时卸载。
  auto *base=reinterpret_cast<unsigned char *>(Base);
  auto *dos=reinterpret_cast<IMAGE_DOS_HEADER *>(base);
  if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
  auto *nt=reinterpret_cast<IMAGE_NT_HEADERS32 *>(base+dos->e_lfanew);
  const auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  if(!directory.VirtualAddress)return false;
  void *wrappers[]={reinterpret_cast<void *>(&DisplayMalloc),reinterpret_cast<void *>(&DisplayCalloc),
    reinterpret_cast<void *>(&DisplayRealloc),reinterpret_cast<void *>(&DisplayFree)};
  std::array<ULONG_PTR *,4> slots{};
  for(auto *d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base+directory.VirtualAddress);d->Name;++d){
    if(!d->OriginalFirstThunk)continue;
    auto *name=reinterpret_cast<IMAGE_THUNK_DATA32 *>(base+d->OriginalFirstThunk);
    auto *iat=reinterpret_cast<IMAGE_THUNK_DATA32 *>(base+d->FirstThunk);
    for(;name->u1.AddressOfData;++name,++iat){
      if(IMAGE_SNAP_BY_ORDINAL32(name->u1.Ordinal))continue;
      const auto *entry=reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base+name->u1.AddressOfData);
      for(const auto &spec:ClientDisplayMemoryImports)if(!_stricmp(reinterpret_cast<const char *>(base+d->Name),spec.module)&&
          !std::strcmp(reinterpret_cast<const char *>(entry->Name),spec.symbol)){
        const auto i=spec.source;
        if(slots[i])return false;slots[i]=&iat->u1.Function;
      }
    }
  }
  for(auto *slot:slots)if(!slot)return false;
  original_malloc=reinterpret_cast<Allocate>(*slots[0]);original_calloc=reinterpret_cast<AllocateZero>(*slots[1]);
  original_realloc=reinterpret_cast<Resize>(*slots[2]);original_free=reinterpret_cast<Release>(*slots[3]);
  unsigned patched=0;
  for(unsigned i=0;i<4;++i){
    DWORD before=0;if(!VirtualProtect(slots[i],sizeof(*slots[i]),PAGE_READWRITE,&before))break;
    InterlockedExchangePointer(reinterpret_cast<void *volatile *>(slots[i]),wrappers[i]);
    DWORD ignored=0;VirtualProtect(slots[i],sizeof(*slots[i]),before,&ignored);++patched;
  }
  installed=patched==4;
  Trace("client_display_memory_hooks","installed=%u imports=%u",unsigned(installed),patched);
  return installed;
}
} // namespace cs::game
