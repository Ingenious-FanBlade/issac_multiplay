#include "IsaacRepentance.h"
#include "game/client/scene/client_atomic_install_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <cstring>
#include <array>
#include <algorithm>

namespace cs::game {
namespace {
bool Shape(const ClientAtomicWrite &w) noexcept {
  if(!w.destination||!w.size||w.size>w.extent)return false;
  const auto begin=reinterpret_cast<std::uintptr_t>(w.destination);
  if(w.size>UINTPTR_MAX-begin)return false;
  switch(w.kind){
    case ClientAtomicWriteKind::Bytes:return w.value!=nullptr;
    case ClientAtomicWriteKind::Masked32:return w.size==4&&w.value&&w.mask;
    case ClientAtomicWriteKind::EntityReference:return w.size==sizeof(void *);
    case ClientAtomicWriteKind::FieldAttachment:return w.field!=nullptr;
  }
  return false;
}
bool Scope(bool retirement) noexcept {
  return ClientBinaryInstallationScopeActive(retirement)&&ClientNativeCpuQueuesEmpty();
}
// 原生对象只由主线程拥有；同一纯拷贝片内不改变虚拟页映射。页保护和越界仍逐区检查。
bool WritableRange(const void *source,std::size_t size,ClientAtomicReadRegion &region) noexcept {
  if(!IsMain()||!source)return false;
  const auto begin=reinterpret_cast<std::uintptr_t>(source);
  if(size>UINTPTR_MAX-begin)return false;
  for(auto cursor=begin;cursor<begin+size;){
    if(cursor<region.begin||cursor>=region.end){
      const auto end=region.recent.begin()+region.used;
      const auto at=std::upper_bound(region.recent.begin(),end,cursor,
        [](auto address,const auto &cached){return address<cached[0];});
      if(at!=region.recent.begin()&&cursor<(*(at-1))[1]){
        region.begin=(*(at-1))[0];region.end=(*(at-1))[1];
      }
    }
    if(cursor<region.begin||cursor>=region.end){
      MEMORY_BASIC_INFORMATION info{};
      ++client_native_page_counters.write_queries;
      if(region.used==region.recent.size())++client_native_page_counters.write_full;
      if(!VirtualQuery(reinterpret_cast<const void *>(cursor),&info,sizeof(info))||info.State!=MEM_COMMIT||
          (info.Protect&(PAGE_NOACCESS|PAGE_GUARD))){region.Clear();return false;}
      const auto access=info.Protect&0xff;
      if(access!=PAGE_READWRITE&&access!=PAGE_WRITECOPY&&access!=PAGE_EXECUTE_READWRITE&&access!=PAGE_EXECUTE_WRITECOPY){region.Clear();return false;}
      const auto address=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
      if(info.RegionSize>UINTPTR_MAX-address||address+info.RegionSize<=cursor){region.Clear();return false;}
      region.begin=address;region.end=address+info.RegionSize;
      // 表满不丢保护检查；当前区域仍服务连续字节，其余地址继续真实查询。
      if(region.used<region.recent.size()){
        const auto end=region.recent.begin()+region.used;
        const auto at=std::lower_bound(region.recent.begin(),end,region.begin,
          [](const auto &cached,auto address){return cached[0]<address;});
        std::move_backward(at,end,end+1);*at={region.begin,region.end};++region.used;
      }
    }
    cursor=region.end;
  }
  return true;
}
bool CopyBytes(void *destination,const void *source,std::size_t size) noexcept {
  __try {std::memcpy(destination,source,size);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool WriteComparable(const ClientAtomicWrite &w,const std::byte *original) noexcept {
  __try {
    if(w.kind==ClientAtomicWriteKind::Bytes)std::memcpy(w.destination,w.value,w.size);
    else {
      std::uint32_t old=0,next=0;
      std::memcpy(&old,original?original:w.destination,4);
      std::memcpy(&next,w.value,4);
      next=(old&~w.mask)|(next&w.mask);
      std::memcpy(w.destination,&next,4);
    }
    return true;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool CheckComparable(const ClientAtomicWrite &w) noexcept {
  __try {
    if(w.kind==ClientAtomicWriteKind::Bytes)return !std::memcmp(w.destination,w.value,w.size);
    std::uint32_t actual=0,expected=0;
    std::memcpy(&actual,w.destination,4);
    std::memcpy(&expected,w.value,4);
    return (actual&w.mask)==(expected&w.mask);
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// EntityPtr 的原生 setter 也可能触发结构异常，在 ABI 边界收住 SEH。
bool SetReference(const ClientAtomicWrite &w) {
  __try {
    static_cast<EntityPtr *>(w.destination)->SetReference(static_cast<Entity *>(w.reference));
    return true;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool CheckReference(const ClientAtomicWrite &w) noexcept {
  __try {return static_cast<EntityPtr *>(w.destination)->GetReference()==w.reference;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReferenceReady(const ClientAtomicWrite &w,ClientNativeReadWindow &reads) noexcept {
  if(w.kind!=ClientAtomicWriteKind::EntityReference||!w.reference)return true;
  // 仅检可读承载；稳定身份、真实use和引用类型仍由绑定owner在完整目标上核定。
  void *vptr=nullptr;
  return reads.Copy(w.reference,&vptr,sizeof(vptr))&&vptr;
}
}
bool ClientAtomicInstallWindow(const ClientNativeRoots &roots,void *vm) noexcept {
  return ClientPreparationWindow(roots,vm);
}
bool ClientAtomicRootScalar(const ClientNativeRoots &r,const void *p,std::size_t n) noexcept {
  if(!IsMain()||!g_Game||r.game!=g_Game||r.room!=g_Game->_room||!r.room)return false;
  const auto address=reinterpret_cast<std::uintptr_t>(p);
  const auto within=[&](const void *root,std::size_t size){
    const auto begin=reinterpret_cast<std::uintptr_t>(root);
    return address>=begin&&address-begin<=size&&n<=size-(address-begin);
  };
  return within(r.game,sizeof(Game))||within(r.room,sizeof(Room));
}
bool ClientAtomicRestoreRootScalar(const ClientNativeRoots &r,void *p,const void *bytes,std::size_t n) noexcept {
  if(!Scope(true)||!ClientAtomicRootScalar(r,p,n))return false;
  return CopyBytes(p,bytes,n);
}
bool ClientAtomicWritePreflight(const ClientAtomicWrite &w,ClientAtomicReadRegion &region) noexcept {
  if(!ClientAtomicWriteCapture(w,nullptr,region))return false;
  return w.kind!=ClientAtomicWriteKind::FieldAttachment||(w.retained_attachment?
    ClientStagedFieldAttached(w.field,w.destination):ClientInspectedFieldCanAttach(w.field,w.destination,w.size));
}
bool ClientAtomicWriteCapture(const ClientAtomicWrite &w,std::byte *original,ClientAtomicReadRegion &region) noexcept {
  return Shape(w)&&WritableRange(w.destination,w.size,region)&&ReferenceReady(w,region.references)&&
    (!original||CopyBytes(original,w.destination,w.size));
}
bool ClientAtomicCopyWindow() noexcept {return Scope(false);}
// 只拆仍指向该实例的边；较新的流可能已经更新了这个共享字段。
bool ClientAtomicClearRetiringReference(void *slot,const void *target,bool entity_ptr) noexcept {
  if(!Scope(true)||!slot)return false;
  ClientAtomicReadRegion region;
  if(!WritableRange(slot,sizeof(void *),region))return false;
  __try {
    void *current=nullptr;
    std::memcpy(&current,slot,sizeof(current));
    if(current!=target)return true;
    if(entity_ptr)static_cast<EntityPtr *>(slot)->SetReference(nullptr);
    else {current=nullptr;std::memcpy(slot,&current,sizeof(current));}
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientAtomicWriteApply(const ClientAtomicWrite &w) noexcept {
  if(!Shape(w)||!Scope(false))return false;
  try {
    switch(w.kind){
      case ClientAtomicWriteKind::Bytes:case ClientAtomicWriteKind::Masked32:
        return WriteComparable(w,nullptr);
      case ClientAtomicWriteKind::EntityReference:{
        return SetReference(w)&&CheckReference(w);
      }
      case ClientAtomicWriteKind::FieldAttachment:return w.retained_attachment?
        ClientStagedFieldBorrowAttachment(w.field,w.destination,w.size):ClientStagedFieldAttach(w.field,w.destination,w.size);
    }
    return false;
  }catch(...){return false;}
}
bool ClientAtomicWriteCheck(const ClientAtomicWrite &w) noexcept {
  if(!Shape(w)||!Scope(false))return false;
  switch(w.kind){
    case ClientAtomicWriteKind::Bytes:case ClientAtomicWriteKind::Masked32:return CheckComparable(w);
    case ClientAtomicWriteKind::EntityReference:return CheckReference(w);
    case ClientAtomicWriteKind::FieldAttachment:return ClientStagedFieldAttached(w.field,w.destination);
  }
  return false;
}
bool ClientAtomicFastWrite(void *destination,const std::byte *value,std::size_t size,std::uint32_t mask) noexcept {
  if(!destination||!value||!size||(mask&&size!=4))return false;
  __try {
    if(!mask)std::memcpy(destination,value,size);
    else {
      std::uint32_t old=0,next=0;
      std::memcpy(&old,destination,4);std::memcpy(&next,value,4);
      next=(old&~mask)|(next&mask);
      std::memcpy(destination,&next,4);
    }
    return true;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientAtomicDetachField(const ClientAtomicWrite &write) noexcept {
  return Scope(true)&&write.kind==ClientAtomicWriteKind::FieldAttachment&&write.field&&ClientStagedFieldDetach(write.field);
}
} // namespace cs::game
