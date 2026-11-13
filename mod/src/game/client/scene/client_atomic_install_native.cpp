#include "IsaacRepentance.h"
#include "game/client/scene/client_atomic_install_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <cstring>
#include <array>

namespace cs::game {
bool ReadMemory(const void *,void *,std::size_t) noexcept;
namespace {
bool Shape(const ClientAtomicWrite &w) noexcept {
  if(!w.destination||!w.size||w.size>w.extent)return false;
  const auto begin=reinterpret_cast<std::uintptr_t>(w.destination);
  if(w.size>UINTPTR_MAX-begin)return false;
  switch(w.kind){
    case ClientAtomicWriteKind::Bytes:return w.value!=nullptr;
    case ClientAtomicWriteKind::Masked32:return w.size==4&&w.value&&w.mask;
    case ClientAtomicWriteKind::EntityReference:return w.size==sizeof(void *);
    case ClientAtomicWriteKind::FieldAttachment:return w.field&&w.size==ClientStagedFieldAttachmentSize(w.field);
  }
  return false;
}
bool Scope(bool retirement) noexcept {
  return ClientBinaryInstallationScopeActive(retirement)&&ClientNativeCpuQueuesEmpty();
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
  std::memcpy(p,bytes,n);return true;
}
bool ClientAtomicWritePreflight(const ClientAtomicWrite &w,std::byte *original,std::size_t size) noexcept {
  if(!Shape(w)||size!=w.size||!original||!ReadMemory(w.destination,original,size))return false;
  if(w.kind==ClientAtomicWriteKind::FieldAttachment)return ClientStagedFieldCanAttach(w.field,w.destination,w.size);
  if(w.kind==ClientAtomicWriteKind::EntityReference&&w.reference){
    // 仅检可读承载；稳定身份、真实use和引用类型仍由绑定owner在完整目标上核定。
    void *vptr=nullptr;if(!ReadMemory(w.reference,&vptr,sizeof(vptr))||!vptr)return false;
  }
  return true;
}
bool ClientAtomicWriteApply(const ClientAtomicWrite &w) noexcept {
  if(!Shape(w)||!Scope(false))return false;
  try {
    switch(w.kind){
      case ClientAtomicWriteKind::Bytes:std::memcpy(w.destination,w.value,w.size);break;
      case ClientAtomicWriteKind::Masked32:{std::uint32_t old=0,next=0;std::memcpy(&old,w.destination,4);std::memcpy(&next,w.value,4);
        next=(old&~w.mask)|(next&w.mask);std::memcpy(w.destination,&next,4);break;}
      case ClientAtomicWriteKind::EntityReference:static_cast<EntityPtr *>(w.destination)->SetReference(static_cast<Entity *>(w.reference));break;
      case ClientAtomicWriteKind::FieldAttachment:if(!ClientStagedFieldAttach(w.field,w.destination,w.size))return false;break;
    }
    return ClientAtomicWriteCheck(w);
  }catch(...){return false;}
}
bool ClientAtomicWriteCheck(const ClientAtomicWrite &w) noexcept {
  if(!Shape(w)||!Scope(false))return false;
  switch(w.kind){
    case ClientAtomicWriteKind::Bytes:return !std::memcmp(w.destination,w.value,w.size);
    case ClientAtomicWriteKind::Masked32:{std::uint32_t old=0,next=0;std::memcpy(&old,w.destination,4);std::memcpy(&next,w.value,4);return (old&w.mask)==(next&w.mask);}
    case ClientAtomicWriteKind::EntityReference:return static_cast<EntityPtr *>(w.destination)->GetReference()==w.reference;
    case ClientAtomicWriteKind::FieldAttachment:return ClientStagedFieldAttached(w.field,w.destination);
  }
  return false;
}
bool ClientAtomicWriteRestore(const ClientAtomicWrite &w,const std::byte *original,std::size_t size) noexcept {
  if(!Scope(true))return false;
  // 后继发布可能已接管header；此时旧字段已经拆除，只需保留其私有存储直到真正退休。
  // 不能再按“未安装Weapon引用必须为空”等暂存资格拒绝这个已完成的拆除步骤。
  if(w.kind==ClientAtomicWriteKind::FieldAttachment)return w.field&&ClientStagedFieldDetach(w.field);
  if(!Shape(w)||!original||size!=w.size)return false;
  try {
    switch(w.kind){
      case ClientAtomicWriteKind::Bytes:std::memcpy(w.destination,original,size);return !std::memcmp(w.destination,original,size);
      case ClientAtomicWriteKind::Masked32:{std::uint32_t current=0,old=0;std::memcpy(&current,w.destination,4);std::memcpy(&old,original,4);
        old=(current&~w.mask)|(old&w.mask);std::memcpy(w.destination,&old,4);return true;}
      case ClientAtomicWriteKind::EntityReference:{Entity *old=nullptr;std::memcpy(&old,original,sizeof(old));auto &ref=*static_cast<EntityPtr *>(w.destination);
        ref.SetReference(old);return ref.GetReference()==old;}
      case ClientAtomicWriteKind::FieldAttachment:return ClientStagedFieldDetach(w.field);
    }
  }catch(...){return false;}
  return false;
}
} // namespace cs::game
