#include "IsaacRepentance.h"
#include "game/client/scene/client_field_staging_native.h"
#include "game/client/scene/client_atomic_bindings_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/player/client_player_spoof_layout.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <deque>
#include <map>
#include <list>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <windows.h>

namespace cs::game {
namespace {
using Kind=ClientStagedFieldKind;
ClientPreparationResult Bad(const char *why){return {ClientPreparationStatus::Failed,why};}
ClientPreparationResult Ready(){return {ClientPreparationStatus::Ready,"binary_fields_native_staged"};}
bool Window() noexcept {return ClientPreparationWindow(ClientPreparationRoots(),ClientLuaStateIdentity());}
// 只允许当前字段的同步构造／退休栈分配，空 allocator 保持 J273 Release STL 的实际头布局。
// 分配指针使用游戏 CRT，账目留在字段 owner；后续安装必须显式接管，不能裸 swap 后遗失账目。
thread_local ClientStagedField *allocating=nullptr;
void *FieldAllocate(std::size_t);
void FieldFree(void *) noexcept;
template<class T>struct FieldAllocator {
  using value_type=T;
  FieldAllocator()=default;
  template<class U>FieldAllocator(const FieldAllocator<U> &)noexcept{}
  T *allocate(std::size_t n){if(n>SIZE_MAX/sizeof(T))throw std::bad_alloc();return static_cast<T *>(FieldAllocate(n*sizeof(T)));}
  void deallocate(T *p,std::size_t)noexcept{FieldFree(p);}
  template<class U>bool operator==(const FieldAllocator<U> &)const noexcept{return true;}
  template<class U>bool operator!=(const FieldAllocator<U> &)const noexcept{return false;}
};
template<class T>struct AccountAllocator {
  using value_type=T;ClientPreparationAllocator account;
  explicit AccountAllocator(ClientPreparationAllocator a):account(a){}
  template<class U>AccountAllocator(const AccountAllocator<U> &a):account(a.account){}
  T *allocate(std::size_t n){if(n>SIZE_MAX/sizeof(T))throw std::bad_alloc();return static_cast<T *>(account.allocate(account.context,n*sizeof(T),alignof(T)));}
  void deallocate(T *p,std::size_t n)noexcept{account.release(account.context,p,n*sizeof(T),alignof(T));}
  template<class U>bool operator==(const AccountAllocator<U> &a)const noexcept{return account.context==a.account.context;}
  template<class U>bool operator!=(const AccountAllocator<U> &a)const noexcept{return !(*this==a);}
};
template<class T>using Vector=std::vector<T,FieldAllocator<T>>;
template<class T>using Deque=std::deque<T,FieldAllocator<T>>;
template<class T>using List=std::list<T,FieldAllocator<T>>;
template<class K,class V>using Map=std::map<K,V,std::less<K>,FieldAllocator<std::pair<const K,V>>>;
template<class K,class V>using Hash=std::unordered_map<K,V,std::hash<K>,std::equal_to<K>,FieldAllocator<std::pair<const K,V>>>;
using String=std::basic_string<char,std::char_traits<char>,FieldAllocator<char>>;
using Blocked=std::unordered_set<int,std::hash<int>,std::equal_to<int>,FieldAllocator<int>>;
using Seeds=std::set<int,std::less<int>,FieldAllocator<int>>;
struct Innate {Hash<int,int> persistent;Vector<player_sdk::Timed> temporary;Hash<int,int> totals;};
using Innates=Map<std::string,Innate>;
template<unsigned N>struct Row {alignas(4) std::byte bytes[N]{};};
using SeedNode=std::_Tree_node<int,void *>;
using RecordNode=std::_List_node<Row<32>,void *>;
static_assert(_ITERATOR_DEBUG_LEVEL==0&&sizeof(Vector<std::byte>)==12&&sizeof(String)==24&&sizeof(Deque<Row<12>>)==20&&
  sizeof(Map<int,int>)==8&&sizeof(Blocked)==32&&sizeof(Innate)==sizeof(player_sdk::Innate)&&sizeof(Innates)==8&&sizeof(List<Row<32>>)==8&&
  sizeof(SeedNode)==20&&offsetof(SeedNode,_Myval)==16&&sizeof(RecordNode)==40&&offsetof(RecordNode,_Myval)==8,"J273 字段容器 ABI");
struct Allocation {void *base=nullptr;std::size_t bytes=0;};
void *NativeNew(std::size_t n){return reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(n);}
void NativeFree(void *p){reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(p);}
bool Sprite(std::uint16_t id){switch(id){case 7:case 134:case 144:case 201:case 202:case 204:case 215:case 226:case 281:case 282:case 355:case 434:return true;default:return false;}}
bool Layer(std::uint16_t id){return id==6||id==200||id==214;}
// 真实动态 Weapon 合同；不能按 SDK 的 0x40 基类大小自行分配派生类。
struct WeaponContract {unsigned vptr,size;std::array<unsigned,7> refs;};
constexpr WeaponContract weapon_contracts[]={
  {},{0x731ea4,0x50,{8}},{0x731db4,0x4c,{8,0x48}},{0x731aac,0x50,{8,0x44}},
  {0x731d64,0x54,{8,0x44,0x48,0x4c,0x50}},{0x731a64,0x48,{8,0x44}},{0x731e5c,0x48,{8,0x44}},
  {0x731dfc,0x44,{8}},{0x731a1c,0x48,{8,0x44}},{0x731cd4,0x44,{8}},
  {0x731d1c,0x60,{8,0x44,0x48,0x4c,0x50,0x54,0x58}},{0x731d1c,0x60,{8,0x44,0x48,0x4c,0x50,0x54,0x58}},
  {0x731b6c,0x54,{8,0x4c,0x50}},{0x731eec,0x50,{8,0x44}},{0x731c1c,0x44,{8}},{0x731bc4,0x48,{8,0x44}}
};
unsigned WeaponLayout(unsigned type){return type>=1&&type<=10?180+type:type==11?190:type>=12&&type<=15?179+type:0;}
unsigned SharedVptr(unsigned layout){switch(layout){case 18:return 0x714f84;case 19:return 0x71510c;case 25:return 0x714c0c;case 27:return 0x714be0;case 28:return 0x714bf8;case 133:return 0x70f2e8;default:return 0;}}
unsigned SharedSize(unsigned layout){switch(layout){case 18:return 0x578;case 19:return 0x58;case 25:return 0x128;case 27:return 0x18;case 28:return 0x458;case 133:return 0x158;default:return 0;}}
bool FamiliarShared(unsigned layout){return layout==18||layout==19||layout==133;}

// J273 的两字 holder 保存 payload 和控制块，控制块 +0x14 记录实际析构的对象。
bool SharedPairCurrent(const std::array<void *,2> &holder,unsigned layout) noexcept {
  const auto vptr=SharedVptr(layout);
  if(!vptr)return false;
  const auto *control=static_cast<const unsigned char *>(holder[1]);
  if(!control)return !holder[0];
  const auto control_vptr=FamiliarShared(layout)?0x70f2cc:0x714ba0;
  return *reinterpret_cast<const std::uintptr_t *>(control)==Base+control_vptr&&
    *reinterpret_cast<void *const *>(control+0x14)==holder[0]&&
    (!holder[0]||*static_cast<const std::uintptr_t *>(holder[0])==Base+vptr);
}

// 原生虚槽 +0x0C 负责减强引用及必要的析构。返回值只决定是否尝试调用可空的通知回调。
void ReleaseShared(std::array<void *,2> &holder,unsigned layout) {
  if(auto *control=holder[1]){
    auto **table=*static_cast<void ***>(control);
    if(reinterpret_cast<bool (__thiscall *)(void *)>(table[3])(control)){
      const auto callback=FamiliarShared(layout)?0x7ebdb4:0x7ebde8;
      auto notify=*reinterpret_cast<void (__cdecl **)(void *)>(Base+callback);
      if(notify)notify(holder.data());
    }
  }
  holder={};
}

// 活容器由游戏本身释放。分配和大块对齐须与 J273 的 Release STL 完全一致。
template<class T>struct FlatGameAllocator {
  using value_type=T;
  FlatGameAllocator()=default;
  template<class U> FlatGameAllocator(const FlatGameAllocator<U> &) noexcept {}
  T *allocate(std::size_t n){
    if(n>(SIZE_MAX-39)/sizeof(T))throw std::bad_alloc();
    const auto bytes=n*sizeof(T);
    auto *base=NativeNew(bytes+(bytes>=4096?39:0));
    if(!base)throw std::bad_alloc();
    if(bytes<4096)return static_cast<T *>(base);
    auto *aligned=reinterpret_cast<T *>((reinterpret_cast<std::uintptr_t>(base)+35)&~std::uintptr_t(31));
    reinterpret_cast<void **>(aligned)[-1]=base;
    return aligned;
  }
  void deallocate(T *p,std::size_t n) noexcept {
    if(p)NativeFree(n*sizeof(T)>=4096?reinterpret_cast<void **>(p)[-1]:p);
  }
  template<class U>bool operator==(const FlatGameAllocator<U> &)const noexcept{return true;}
  template<class U>bool operator!=(const FlatGameAllocator<U> &)const noexcept{return false;}
};
template<class T>using FlatDeque=std::deque<T,FlatGameAllocator<T>>;
using FlatRecords=std::list<Row<32>,FlatGameAllocator<Row<32>>>;

struct FlatVectorHeader {std::byte *first,*last,*end;};
bool FlatVectorShape(const FlatVectorHeader &v,unsigned stride,std::size_t &count,std::size_t &capacity) noexcept {
  const auto first=reinterpret_cast<std::uintptr_t>(v.first),last=reinterpret_cast<std::uintptr_t>(v.last),
    end=reinterpret_cast<std::uintptr_t>(v.end);
  if(!stride||first>last||last>end||(!first&&end)||(last-first)%stride||(end-first)%stride)return false;
  count=(last-first)/stride;capacity=(end-first)/stride;
  return capacity<=16384*16;
}
void ConstructFlatRow(std::byte *row,unsigned layout,unsigned stride) {
  std::memset(row,0,stride);
  if(Sprite(std::uint16_t(layout))||layout==152)reinterpret_cast<ANM2 *>(row)->constructor();
}
void DestroyFlatRow(std::byte *row,unsigned layout) {
  if(Sprite(std::uint16_t(layout))||layout==152)reinterpret_cast<ANM2 *>(row)->destructor();
}
bool ResizeFlatVector(void *slot,unsigned layout,unsigned stride,unsigned count) {
  if((Sprite(std::uint16_t(layout))&&stride!=sizeof(ANM2))||(layout==152&&stride!=304))return false;
  auto &v=*static_cast<FlatVectorHeader *>(slot);
  std::size_t old=0,capacity=0;
  if(!FlatVectorShape(v,stride,old,capacity))return false;
  if(old==count)return true;
  if(count<=capacity){
    while(old>count){DestroyFlatRow(v.first+(old-1)*stride,layout);--old;v.last-=stride;}
    while(old<count){ConstructFlatRow(v.last,layout,stride);++old;v.last+=stride;}
    return true;
  }
  FlatGameAllocator<std::byte> allocator;
  const auto bytes=std::size_t(count)*stride;
  auto *next=allocator.allocate(bytes);
  unsigned built=0;
  try {
    while(built<count){ConstructFlatRow(next+std::size_t(built)*stride,layout,stride);++built;}
  }catch(...){
    while(built){--built;DestroyFlatRow(next+std::size_t(built)*stride,layout);}
    allocator.deallocate(next,bytes);throw;
  }
  const auto prior=v;
  v={next,next+bytes,next+bytes};
  while(old){--old;DestroyFlatRow(prior.first+old*stride,layout);}
  allocator.deallocate(prior.first,capacity*stride);
  return true;
}
template<class T>bool ResizeFlatDeque(void *slot,unsigned count) {
  auto &list=*static_cast<FlatDeque<T> *>(slot);
  if(list.size()>16384)return false;
  if(list.size()!=count)list.resize(count);
  return true;
}
template<class T>void *FlatDequeElement(void *slot,unsigned count,unsigned index) noexcept {
  auto &list=*static_cast<FlatDeque<T> *>(slot);
  return list.size()==count?&list[index]:nullptr;
}
bool ResizeFlatSequence(void *slot,ClientBinaryFlatSequence kind,unsigned layout,unsigned stride,unsigned count) {
  if(!slot||!stride||count>65535)return false;
  using Seq=ClientBinaryFlatSequence;
  if(kind==Seq::Vector)return ResizeFlatVector(slot,layout,stride,count);
  if(kind==Seq::Inline)return true; // 固定数组的有效前缀随另一个生成字段联写。
  if(kind==Seq::Records){
    if(count>1)return false;
    auto &list=*static_cast<FlatRecords *>(slot);
    if(list.size()>16384)return false;
    if(list.size()!=count)list.resize(count);
    auto *cursor=count?reinterpret_cast<std::byte *>(&list.front())-8:*static_cast<std::byte **>(slot);
    std::memcpy(static_cast<std::byte *>(slot)+sizeof(FlatRecords),&cursor,sizeof(cursor));
    return true;
  }
  switch(stride){
    case 4:return layout==10&&ResizeFlatDeque<EntityPtr>(slot,count);
    case 8:return ResizeFlatDeque<Row<8>>(slot,count);
    case 12:return ResizeFlatDeque<Row<12>>(slot,count);
    case 64:return ResizeFlatDeque<Row<64>>(slot,count);
    case 120:return ResizeFlatDeque<Row<120>>(slot,count);
    default:return false;
  }
}
bool TryResizeFlatSequence(void *slot,ClientBinaryFlatSequence kind,unsigned layout,unsigned stride,unsigned count) noexcept {
  try{return ResizeFlatSequence(slot,kind,layout,stride,count);}catch(...){return false;}
}
void *FlatSequenceElement(void *slot,ClientBinaryFlatSequence kind,unsigned layout,unsigned stride,
                          unsigned count,unsigned index) noexcept {
  if(!slot||index>=count)return nullptr;
  using Seq=ClientBinaryFlatSequence;
  if(kind==Seq::Inline)return static_cast<std::byte *>(slot)+std::size_t(index)*stride;
  if(kind==Seq::Vector){
    const auto &v=*static_cast<const FlatVectorHeader *>(slot);
    std::size_t actual=0,capacity=0;
    return FlatVectorShape(v,stride,actual,capacity)&&actual==count?v.first+std::size_t(index)*stride:nullptr;
  }
  if(kind==Seq::Records){
    auto &list=*static_cast<FlatRecords *>(slot);
    return count==1&&list.size()==1?reinterpret_cast<std::byte *>(&list.front())-8:nullptr;
  }
  switch(stride){
    case 4:return layout==10?FlatDequeElement<EntityPtr>(slot,count,index):nullptr;
    case 8:return FlatDequeElement<Row<8>>(slot,count,index);
    case 12:return FlatDequeElement<Row<12>>(slot,count,index);
    case 64:return FlatDequeElement<Row<64>>(slot,count,index);
    case 120:return FlatDequeElement<Row<120>>(slot,count,index);
    default:return nullptr;
  }
}
}
bool ClientBinaryResizeFlat(void *slot,ClientBinaryFlatSequence kind,unsigned layout,unsigned stride,unsigned count) noexcept {
  if(!ClientBinaryInstallationScopeActive(false)||!ClientNativeCpuQueuesEmpty())return false;
  __try {return TryResizeFlatSequence(slot,kind,layout,stride,count);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void *ClientBinaryFlatElement(void *slot,ClientBinaryFlatSequence kind,unsigned layout,unsigned stride,
                             unsigned count,unsigned index) noexcept {
  __try {return FlatSequenceElement(slot,kind,layout,stride,count,index);}
  __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
// 每个字段独立持有 detached 容器或 payload；异常后记录真实前缀，绝不让后台析构。
struct ClientStagedField {
  explicit ClientStagedField(ClientPreparationAllocator a):allocator(a),allocations(std::less<void *>(),AccountAllocator<std::pair<void *const,Allocation>>(a)),elements(AccountAllocator<void *>(a)){}
  ClientPreparationAllocator allocator;
  std::map<void *,Allocation,std::less<void *>,AccountAllocator<std::pair<void *const,Allocation>>> allocations;
  std::vector<void *,AccountAllocator<void *>> elements;
  ClientStagedFieldInput input;
  ClientNativeRoots roots{};void *vm=nullptr,*dc=nullptr,*gl=nullptr,*manager=nullptr;
  void *container=nullptr;std::size_t container_bytes=0,container_alignment=0;
  void (*destroy)(void *)=nullptr;
  void *(*element)(void *,unsigned)=nullptr;
  std::array<Weapon *,5> weapons{};
  std::array<void *,2> shared{};
  std::size_t object_charge=0;
  unsigned rows_constructed=0;
  bool entered=false,ready=false,uncertain=false,release_entered=false,shared_constructed=false,deferred=false;
  void *attached=nullptr;
  std::size_t attachment_size=0;
  std::array<std::byte,32> displaced{},published{};
  ClientStagedField *predecessor=nullptr,*successor=nullptr;
  // 暂存槽持有承载，已安装写入器持有 header 用途。两者都在主线程退休。
  unsigned owners=1,attachment_uses=0;
};
namespace {
// 注册表节点直接使用Peer分配器；不归某个字段的native分配集合，因而可跨批次移交头部责任。
struct Attachments {
  ClientPreparationAllocator allocator;
  std::map<void *,ClientStagedField *,std::less<void *>,AccountAllocator<std::pair<void *const,ClientStagedField *>>> entries;
  explicit Attachments(ClientPreparationAllocator a):allocator(a),entries(std::less<void *>(),AccountAllocator<std::pair<void *const,ClientStagedField *>>(a)){}
};
thread_local Attachments *attachments=nullptr;
Attachments *AttachmentRegistry(ClientPreparationAllocator a) {
  if(attachments)return attachments->allocator.context==a.context?attachments:nullptr;
  void *raw=a.allocate(a.context,sizeof(Attachments),alignof(Attachments));if(!raw)throw std::bad_alloc();
  try {attachments=new(raw)Attachments(a);return attachments;}
  catch(...){a.release(a.context,raw,sizeof(Attachments),alignof(Attachments));throw;}
}
void RetireAttachmentRegistry() noexcept {
  if(attachments&&attachments->entries.empty()){const auto a=attachments->allocator;auto *p=attachments;attachments=nullptr;p->~Attachments();a.release(a.context,p,sizeof(Attachments),alignof(Attachments));}
}
void DequeOwner(ClientStagedField &o,void *owner) {
  if(o.input.kind==Kind::Deque&&o.container){auto *proxy=*static_cast<void **>(o.container);if(proxy)*static_cast<void **>(proxy)=owner;}
}
struct AllocationScope {
  ClientStagedField *previous=allocating;
  explicit AllocationScope(ClientStagedField &o){allocating=&o;}
  ~AllocationScope(){allocating=previous;}
};
void *FieldAllocate(std::size_t bytes) {
  if(!allocating)throw std::bad_alloc();auto &o=*allocating;
  if(bytes>SIZE_MAX-39)throw std::bad_alloc();const auto charged=bytes>=4096?bytes+39:bytes;
  o.allocator.charge_native(o.allocator.context,charged);void *base=nullptr;
  try {base=NativeNew(charged);if(!base)throw std::bad_alloc();void *p=base;
    if(bytes>=4096){p=reinterpret_cast<void *>((reinterpret_cast<std::uintptr_t>(base)+35)&~std::uintptr_t(31));reinterpret_cast<void **>(p)[-1]=base;}
    if(!o.allocations.emplace(p,Allocation{base,charged}).second)throw std::bad_alloc();return p;
  }catch(...){if(base)NativeFree(base);o.allocator.uncharge_native(o.allocator.context,charged);throw;}
}
void FieldFree(void *p) noexcept {
  if(!p)return;if(!allocating)return;auto &o=*allocating;const auto i=o.allocations.find(p);
  if(i==o.allocations.end()){o.uncertain=true;return;}
  const auto allocation=i->second;NativeFree(allocation.base);o.allocator.uncharge_native(o.allocator.context,allocation.bytes);o.allocations.erase(i);
}
template<class F>void Native(ClientStagedField &o,F &&f){o.uncertain=true;f();o.uncertain=false;}
template<class T>T &Container(ClientStagedField &o) {
  void *p=o.allocator.allocate(o.allocator.context,sizeof(T),alignof(T));if(!p)throw std::bad_alloc();
  try {new(p)T();}catch(...){o.allocator.release(o.allocator.context,p,sizeof(T),alignof(T));throw;}
  o.container=p;o.container_bytes=sizeof(T);o.container_alignment=alignof(T);o.destroy=[](void *q){static_cast<T *>(q)->~T();};return *static_cast<T *>(p);
}
void ConstructRow(ClientStagedField &o,void *p) {
  if(Sprite(o.input.layout)||o.input.layout==152){if(o.input.stride<sizeof(ANM2))throw std::bad_alloc();Native(o,[&]{static_cast<ANM2 *>(p)->constructor();});}
  else if(Layer(o.input.layout)){if(o.input.stride!=sizeof(LayerState))throw std::bad_alloc();new(p)LayerState{};}
  else if(o.input.layout==246){
    // Ray 尾部持有图像强引用，元素更换时须走原生引用计数。
    static_assert(sizeof(KAGE_SmartPointer_ImageBase)==8);
    if(o.input.stride!=20)throw std::bad_alloc();
    new(static_cast<std::byte *>(p)+12)KAGE_SmartPointer_ImageBase();
  }
}
void DestroyRow(ClientStagedField &o,void *p) {
  if(Sprite(o.input.layout)||o.input.layout==152)Native(o,[&]{static_cast<ANM2 *>(p)->destructor();});
  else if(Layer(o.input.layout))Native(o,[&]{static_cast<LayerState *>(p)->~LayerState();});
  else if(o.input.layout==10)Native(o,[&]{static_cast<EntityPtr *>(p)->SetReference(nullptr);});
  else if(o.input.layout==246)Native(o,[&]{
    reinterpret_cast<KAGE_SmartPointer_ImageBase *>(static_cast<std::byte *>(p)+12)->~KAGE_SmartPointer_ImageBase();
  });
}
void Rows(ClientStagedField &o) {
  if(!o.input.stride||o.input.count>SIZE_MAX/o.input.stride)throw std::bad_alloc();auto &v=Container<Vector<std::byte>>(o);
  v.resize(std::size_t(o.input.count)*o.input.stride+(o.input.kind==Kind::CString?1u:0u));o.element=[](void *p,unsigned i){return static_cast<void *>(static_cast<Vector<std::byte> *>(p)->data()+i*allocating->input.stride);};
  if(o.input.kind==Kind::CString){if(o.input.bytes!=o.input.count)throw std::bad_alloc();if(o.input.bytes)std::memcpy(v.data(),o.input.data,o.input.bytes);}
  while(o.rows_constructed<o.input.count){ConstructRow(o,v.data()+std::size_t(o.rows_constructed)*o.input.stride);++o.rows_constructed;}
}
template<unsigned N>void MakeDeque(ClientStagedField &o) {
  auto &v=Container<Deque<Row<N>>>(o);v.resize(o.input.count);o.element=[](void *p,unsigned i){return static_cast<void *>(&(*static_cast<Deque<Row<N>> *>(p))[i]);};
  o.rows_constructed=o.input.count;
}
void MakeReferenceDeque(ClientStagedField &o) {
  static_assert(sizeof(EntityPtr)==4&&sizeof(Deque<EntityPtr>)==20);
  // deque 扩缩不搬移仍存活的引用槽。收缩时由 EntityPtr 析构向原生反向树摘边。
  auto &v=Container<Deque<EntityPtr>>(o);
  v.resize(o.input.count);
  o.element=[](void *p,unsigned i)->void *{return &(*static_cast<Deque<EntityPtr> *>(p))[i];};
}
std::int32_t Int(const std::byte *p){std::int32_t n;std::memcpy(&n,p,4);return n;}
void Shared(ClientStagedField &o) {
  if(!o.input.payload_present||!o.input.count)return;
  const auto size=SharedSize(o.input.layout),vptr=SharedVptr(o.input.layout);if(!size||size!=o.input.stride||!vptr)throw std::bad_alloc();
  const auto internal=o.input.layout==19?0x18u:o.input.layout==27||o.input.layout==133?8u:0u;
  // 全部已知固定分配先收费，避免 payload 构造前后再因 quota 留下可回滚的 raw allocation。
  o.allocator.charge_native(o.allocator.context,size+internal+0x18);o.object_charge=size+internal+0x18;
  Native(o,[&]{o.container=NativeNew(size);if(!o.container)throw std::bad_alloc();});auto *p=static_cast<unsigned char *>(o.container);
  // 使用具名 ctor 或已核 inline ctor；不调用 Entity Init，不安装到任何 Entity holder。
  Native(o,[&]{
    if(o.input.layout==25||o.input.layout==27)reinterpret_cast<void *(__thiscall *)(void *)>(Base+(o.input.layout==25?0x1cce00:0x1d1720))(p);
    else {std::memset(p,0,size);*reinterpret_cast<std::uintptr_t *>(p)=Base+vptr;
      if(o.input.layout==18||o.input.layout==28){const unsigned n=o.input.layout==18?5:4;for(unsigned i=0;i<n;++i)reinterpret_cast<ANM2 *>(p+4+i*0x114)->constructor();
        if(o.input.layout==18)std::fill_n(reinterpret_cast<int *>(p+0x568),4,-1);}
      else if(o.input.layout==19){auto **head=static_cast<void **>(NativeNew(0x18));if(!head)throw std::bad_alloc();head[0]=head;head[1]=head;head[2]=head;
        *reinterpret_cast<std::uint16_t *>(reinterpret_cast<unsigned char *>(head)+0xc)=0x101;*reinterpret_cast<void **>(p+0x1c)=head;}
      else if(o.input.layout==133){auto **proxy=static_cast<void **>(NativeNew(8));if(!proxy)throw std::bad_alloc();proxy[0]=p+4;proxy[1]=nullptr;
        *reinterpret_cast<void **>(p+4)=proxy;reinterpret_cast<ANM2 *>(p+0x18)->constructor();}
    }
  });o.shared_constructed=true;
  Native(o,[&]{
    const auto assign=FamiliarShared(o.input.layout)?0x30e40:0x206310;
    reinterpret_cast<void (__thiscall *)(void *,void *)>(Base+assign)(o.shared.data(),p);
    if(o.shared[0]!=p||!SharedPairCurrent(o.shared,o.input.layout))throw std::bad_alloc();
  });
}
bool WeaponsCurrent(const ClientStagedField &o) {
  for(unsigned i=0;i<5;++i){auto *w=o.weapons[i];const auto &in=o.input.weapons[i];
    if(bool(w)!=bool(o.input.slot_mask&(1u<<i)))return false;
    if(w&&(!in.type||in.type>15||*reinterpret_cast<const std::uintptr_t *>(w)!=Base+weapon_contracts[in.type].vptr||w->_weaponType!=int(in.type)||w->_owner))return false;
    if(w&&!o.attached)for(const auto offset:weapon_contracts[in.type].refs)if(offset&&*reinterpret_cast<void *const *>(reinterpret_cast<const unsigned char *>(w)+offset))return false;}
  return true;
}
void Weapons(ClientStagedField &o) {
  for(unsigned i=0;i<5;++i)if(o.input.slot_mask&(1u<<i)){const auto &in=o.input.weapons[i];
    if(!in.type||in.type>15||in.layout!=WeaponLayout(in.type)||in.size!=weapon_contracts[in.type].size||
      !*reinterpret_cast<void **>(Base+0x7e65a8+in.type*4))throw std::bad_alloc();
    o.allocator.charge_native(o.allocator.context,in.size);o.object_charge+=in.size;
    // 9CADB0 -> fresh factory -> Init(Reset,type,nullptr)。没有现有身体或槽位可被回写。
    Native(o,[&]{o.weapons[i]=Isaac::CreateWeapon(WeaponType(in.type),nullptr);});
    auto *w=o.weapons[i];if(!w||*reinterpret_cast<std::uintptr_t *>(w)!=Base+weapon_contracts[in.type].vptr||w->_weaponType!=int(in.type)||w->_owner){o.uncertain=true;throw std::bad_alloc();}
    for(const auto offset:weapon_contracts[in.type].refs)if(offset&&*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(w)+offset)){o.uncertain=true;throw std::bad_alloc();}
  }
}
}

namespace {
// 共享 payload 沿用已核构造和控制块 ABI。活 holder 接管新引用后才释放被替换的引用。
bool FlatShared(void *slot,unsigned layout,bool present,bool install,void *&payload) noexcept {
  try {
    if(!slot)return false;
    auto &holder=*static_cast<std::array<void *,2> *>(slot);
    if(!SharedPairCurrent(holder,layout))return false;
    payload=holder[0];
    if(bool(payload)==present)return true;
    if(!install||!ClientNativeCpuQueuesEmpty())return false;
    std::array<void *,2> next{};
    if(present){
      ClientPreparationAllocator accounting{};
      // Shared 不分配暂存元数据。分配进入游戏 owner，镜像字节由状态预算单独计量。
      accounting.charge_native=[](void *,std::size_t){};
      ClientStagedField build(accounting);
      build.input.layout=std::uint16_t(layout);
      build.input.stride=SharedSize(layout);
      build.input.payload_present=true;
      build.input.count=1;
      Shared(build);
      next=build.shared;
    }
    auto old=holder;
    holder=next;
    payload=holder[0];
    ReleaseShared(old,layout);
    return true;
  }catch(...){return false;}
}

// Weapon 槽接收真实工厂产出的派生对象。旧对象先离开槽，再按实际类型拆 EntityPtr 和释放。
bool FlatWeapon(void *slot,unsigned layout,unsigned type,bool install,void *&payload) noexcept {
  if(!slot||type>15||(type&&WeaponLayout(type)!=layout))return false;
  auto &current=*static_cast<Weapon **>(slot);
  const auto old=current?unsigned(current->_weaponType):0u;
  if(current&&(!old||old>15||*reinterpret_cast<std::uintptr_t *>(current)!=Base+weapon_contracts[old].vptr))return false;
  payload=current;
  if(old==type)return true;
  if(!install||!ClientNativeCpuQueuesEmpty())return false;
  auto *next=type?Isaac::CreateWeapon(WeaponType(type),nullptr):nullptr;
  if(type&&(!next||next->_weaponType!=int(type)||next->_owner||
      *reinterpret_cast<std::uintptr_t *>(next)!=Base+weapon_contracts[type].vptr))return false;
  auto *prior=current;
  current=next;payload=next;
  if(prior){
    for(const auto offset:weapon_contracts[old].refs)if(offset)
      reinterpret_cast<EntityPtr *>(reinterpret_cast<std::byte *>(prior)+offset)->SetReference(nullptr);
    prior->_owner=nullptr;
    auto **table=*reinterpret_cast<void ***>(prior);
    reinterpret_cast<void (__thiscall *)(void *,unsigned)>(table[0])(prior,1);
  }
  return true;
}

using FlatString=std::basic_string<char,std::char_traits<char>,FlatGameAllocator<char>>;
template<class K,class V>using FlatMap=std::map<K,V,std::less<K>,FlatGameAllocator<std::pair<const K,V>>>;
template<class T>using FlatVector=std::vector<T,FlatGameAllocator<T>>;
using FlatCounts=std::unordered_map<int,int,std::hash<int>,std::equal_to<int>,FlatGameAllocator<std::pair<const int,int>>>;
using FlatBlocked=std::unordered_set<int,std::hash<int>,std::equal_to<int>,FlatGameAllocator<int>>;
struct FlatInnate {FlatCounts persistent;FlatVector<player_sdk::Timed> temporary;FlatCounts totals;};
using FlatInnates=FlatMap<FlatString,FlatInnate>;
static_assert(sizeof(FlatInnate)==sizeof(player_sdk::Innate)&&sizeof(FlatInnates)==sizeof(Innates));

// 关联容器的键通过游戏 STL 插入。新集合构造完成之后才替换活头。
bool AssignFlatItems(void *slot,ClientBinaryFlatAssociative kind,const std::byte *entries,unsigned count) noexcept {
  try {
    if(!slot||count>16384||(count&&!entries))return false;
    if(kind==ClientBinaryFlatAssociative::Map){
      FlatMap<int,int> next;
      for(unsigned i=0;i<count;++i)if(!next.emplace(Int(entries+i*8),Int(entries+i*8+4)).second)return false;
      next.swap(*static_cast<FlatMap<int,int> *>(slot));
    }else if(kind==ClientBinaryFlatAssociative::Blocked){
      FlatBlocked next;
      next.reserve(count);
      for(unsigned i=0;i<count;++i)if(!next.insert(Int(entries+i*4)).second)return false;
      next.swap(*static_cast<FlatBlocked *>(slot));
    }else{
      FlatInnates next;
      if(count){
        auto &group=next[FlatString("IsaacCS")].persistent;
        group.reserve(count);
        for(unsigned i=0;i<count;++i)if(!group.emplace(Int(entries+i*8),Int(entries+i*8+4)).second)return false;
      }
      next.swap(*static_cast<FlatInnates *>(slot));
    }
    return true;
  }catch(...){return false;}
}
}
bool ClientBinaryFlatShared(void *slot,unsigned layout,bool present,bool install,void *&payload) noexcept {
  __try{return FlatShared(slot,layout,present,install,payload);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientBinaryFlatWeapon(void *slot,unsigned layout,unsigned type,bool install,void *&payload) noexcept {
  __try{return FlatWeapon(slot,layout,type,install,payload);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientBinaryFlatItems(void *slot,ClientBinaryFlatAssociative kind,const std::byte *entries,unsigned count) noexcept {
  __try{return ClientBinaryInstallationScopeActive(false)&&AssignFlatItems(slot,kind,entries,count);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// 退休使用当前活头和实际动态类型；上一张镜像不能证明 deque 长度或 Weapon 选择臂。
bool ClientBinaryFlatOwnedReferences(void *slot,bool weapon,void *context,ClientBinaryFlatReferenceVisitor visit) noexcept {
  __try {
    if(!ClientBinaryInstallationScopeActive(true)||!slot||!visit)return false;
    if(weapon){
      const auto *slots=static_cast<Weapon *const *>(slot);
      for(unsigned i=0;i<5;++i)if(auto *current=slots[i]){
        const auto type=unsigned(current->_weaponType);
        if(!type||type>15||*reinterpret_cast<std::uintptr_t *>(current)!=Base+weapon_contracts[type].vptr)return false;
        for(const auto offset:weapon_contracts[type].refs)
          if(offset&&!visit(context,reinterpret_cast<std::byte *>(current)+offset))return false;
      }
    }else{
      auto &deque=*static_cast<FlatDeque<EntityPtr> *>(slot);
      if(deque.size()>16384)return false;
      for(unsigned i=0;i<deque.size();++i)if(!visit(context,&deque[i]))return false;
    }
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
ClientStagedField *ClientStagedFieldCreate(ClientPreparationAllocator a) noexcept {
  if(!IsMain()||!a.allocate||!a.release||!a.charge_native||!a.uncharge_native)return nullptr;void *p=nullptr;
  try{p=a.allocate(a.context,sizeof(ClientStagedField),alignof(ClientStagedField));return p?new(p)ClientStagedField(a):nullptr;}
  catch(...){if(p)a.release(a.context,p,sizeof(ClientStagedField),alignof(ClientStagedField));return nullptr;}
}
ClientPreparationResult ClientStagedFieldConstruct(ClientStagedField *o,const ClientStagedFieldInput &in) noexcept {
  if(!o||!Window()||!ClientBinaryPreparationScopeActive(false))return Bad("binary_fields_context");
  if(!ClientNativeCpuQueuesEmpty())return {ClientPreparationStatus::Waiting,"binary_fields_cpu_borrow"};
  if(o->entered)return o->ready&&ClientStagedFieldCurrent(o)?Ready():Bad("binary_fields_prefix_retained");
  o->entered=true;o->input=in;o->roots=ClientPreparationRoots();o->vm=ClientLuaStateIdentity();o->manager=g_Manager;ClientRendererHandles(o->dc,o->gl);
  o->deferred=true;o->ready=true;o->input.data=nullptr;o->input.bytes=0;
  return Ready();
}
bool ClientStagedFieldCurrent(const ClientStagedField *o) noexcept {
  if(!o||!o->ready||o->uncertain||o->release_entered||!Window()||o->vm!=ClientLuaStateIdentity()||o->manager!=g_Manager)return false;
  void *dc=nullptr,*gl=nullptr;const auto roots=ClientPreparationRoots();if(roots.game!=o->roots.game||roots.room!=o->roots.room||!ClientRendererHandles(dc,gl)||dc!=o->dc||gl!=o->gl)return false;
  if(o->attached){if(o->successor){if(o->successor->predecessor!=o||o->successor->attached!=o->attached)return false;}
    else if(std::memcmp(o->attached,o->published.data(),o->attachment_size))return false;}
  if(o->deferred)return true;
  if(o->input.kind==Kind::Weapons&&o->input.present&&!WeaponsCurrent(*o))return false;
  if(o->input.kind==Kind::Shared&&o->input.present&&o->input.payload_present&&o->input.count)
    return o->shared_constructed&&o->container&&o->shared[0]==o->container&&SharedPairCurrent(o->shared,o->input.layout);
  return true;
}
namespace {
bool MaterializeField(ClientStagedField *o,const ClientStagedFieldInput &in) noexcept {
  if(!o||!o->deferred||o->input.kind!=in.kind||
      !ClientBinaryInstallationScopeActive(false)||!ClientNativeCpuQueuesEmpty()||!ClientStagedFieldCurrent(o))return false;
  try {
    AllocationScope allocation(*o);
    // 在实际写段构造接入本地字段的承载，正文只在这次调用内借用。
    if(in.present||in.kind==Kind::Records)switch(in.kind){
      case Kind::Group:break;
      case Kind::Rows:case Kind::Vector:case Kind::CString:Rows(*o);break;
      case Kind::String:{auto &s=Container<String>(*o);if(in.bytes!=in.count)throw std::bad_alloc();if(in.bytes)s.assign(reinterpret_cast<const char *>(in.data),in.bytes);break;}
      case Kind::Deque:switch(in.stride){case 4:if(in.layout==10)MakeReferenceDeque(*o);else MakeDeque<4>(*o);break;
        case 8:MakeDeque<8>(*o);break;case 12:MakeDeque<12>(*o);break;case 64:MakeDeque<64>(*o);break;
        case 120:MakeDeque<120>(*o);break;default:return false;}break;
      case Kind::Records:{if(in.count>1)return false;auto &list=Container<List<Row<32>>>(*o);list.resize(in.count);
        o->element=[](void *p,unsigned)->void *{return reinterpret_cast<std::byte *>(&static_cast<List<Row<32>> *>(p)->front())-offsetof(RecordNode,_Myval);};break;}
      case Kind::Map:{if(in.bytes!=std::size_t(in.count)*8)return false;auto &m=Container<Map<int,int>>(*o);o->elements.reserve(in.count);
        for(unsigned i=0;i<in.count;++i){const auto result=m.emplace(Int(in.data+i*8),Int(in.data+i*8+4));if(!result.second)return false;o->elements.push_back(&*result.first);}break;}
      case Kind::Blocked:{if(in.bytes!=std::size_t(in.count)*4)return false;auto &m=Container<Blocked>(*o);m.reserve(in.count);
        o->elements.reserve(in.count);for(unsigned i=0;i<in.count;++i){const auto result=m.insert(Int(in.data+i*4));if(!result.second)return false;o->elements.push_back(const_cast<int *>(&*result.first));}break;}
      case Kind::Innate:{if(in.bytes!=std::size_t(in.count)*8)return false;auto &m=Container<Innates>(*o);o->elements.reserve(in.count);
        if(in.count){auto &g=m["IsaacCS"];g.persistent.reserve(in.count);for(unsigned i=0;i<in.count;++i){const auto result=g.persistent.emplace(Int(in.data+i*8),Int(in.data+i*8+4));
          if(!result.second)return false;o->elements.push_back(&*result.first);}}break;}
      case Kind::Seeds:{if(in.bytes!=std::size_t(in.count)*4)return false;auto &m=Container<Seeds>(*o);o->elements.reserve(in.count);
        for(unsigned i=0;i<in.count;++i){const auto result=m.insert(Int(in.data+i*4));if(!result.second)return false;
          o->elements.push_back(reinterpret_cast<unsigned char *>(const_cast<int *>(&*result.first))-16);}break;}
      case Kind::Shared:Shared(*o);break;
      case Kind::Weapons:Weapons(*o);break;
    }
    o->deferred=false;
    return ClientStagedFieldCurrent(o);
  }catch(...){return false;}
}
}
bool ClientStagedFieldMaterialize(ClientStagedField *o,const ClientStagedFieldInput &in) noexcept {
  __try {return MaterializeField(o,in);}
  __except(EXCEPTION_EXECUTE_HANDLER){if(o)o->uncertain=true;return false;}
}
namespace {
// 只在已验证此 owner 的纯访问片中使用；别名和 attachment 变动之后重新验证。
const void *ContainerAddress(const ClientStagedField &o) noexcept {
  if(o.input.kind==Kind::Weapons)return o.weapons.data();
  if(o.input.kind==Kind::Shared)return o.shared.data();
  return o.container;
}
void *ElementAddress(ClientStagedField *o,std::uint32_t i) noexcept {
  if(o->deferred||!o->input.present)return nullptr;
  if(o->input.kind==Kind::Weapons)return i<5?o->weapons[i]:nullptr;
  if(i>=o->input.count)return nullptr;if(o->input.kind==Kind::Shared)return o->container;
  if(i<o->elements.size())return o->elements[i];
  if(o->input.kind==Kind::String)return &(*static_cast<String *>(o->container))[i];
  if(!o->element)return nullptr;AllocationScope allocation(*o);return o->element(o->container,i);
}
std::size_t AttachmentBytes(const ClientStagedField *o) noexcept {
  if(!o->input.present&&o->input.kind!=Kind::Records)return 0;
  switch(o->input.kind){
    case Kind::Vector:return 12;case Kind::String:return 24;case Kind::Deque:return 20;
    case Kind::Map:case Kind::Innate:case Kind::Seeds:case Kind::Shared:return 8;
    case Kind::Blocked:return 32;case Kind::Weapons:return 20;case Kind::Records:return 12;default:return 0;
  }
}
}
void *ClientStagedFieldElement(ClientStagedField *o,std::uint32_t i) noexcept {
  return ClientStagedFieldCurrent(o)?ElementAddress(o,i):nullptr;
}
void *ClientInspectedFieldElement(ClientStagedField *o,std::uint32_t i) noexcept {
  return o?ElementAddress(o,i):nullptr;
}
std::size_t ClientInspectedFieldAttachmentSize(const ClientStagedField *o) noexcept {return o?AttachmentBytes(o):0;}
bool ClientInspectedFieldCanAttach(const ClientStagedField *o,void *destination,std::size_t size) noexcept {
  if(!o||o->attached||o->predecessor||o->successor||!destination||!size||
      size!=AttachmentBytes(o)||destination==ContainerAddress(*o))return false;
  if(attachments){if(attachments->allocator.context!=o->allocator.context)return false;const auto i=attachments->entries.find(destination);
    if(i!=attachments->entries.end()){const auto *old=i->second;return old!=o&&!old->successor&&old->attachment_size==size&&
      old->input.kind==o->input.kind&&ClientStagedFieldCurrent(old);}}
  return true;
}
// 下列向量元素只有值或不拥有资源的配置指针，没有移动时需维护的引用槽和子承载。
bool ClientLiveVectorLayout(std::uint16_t layout,std::uint32_t stride) noexcept {
  switch(layout){
    case 0:return stride!=0; // 无子布局的扁平标量序列，调用者另核对 linked。
    case 1:return stride==8; // Vector
    case 14:case 180:return stride==16; // TemporaryEffect / PlayerTemporaryEffect
    case 155:return stride==4; // SmeltedTrinketDesc
    case 205:return stride==24; // GridFireEmber
    case 244:return stride==20; // FXPose
    default:return false;
  }
}
namespace {
// 记录投影最多一条。链表哨兵继续归原容器，cursor 跟随 resize 后的当前节点。
bool ResizeDrawRecords(ClientStagedField &o,const ClientStagedFieldInput &in) {
  static_assert(sizeof(List<Row<32>>)==8);
  if(!in.resize_live_sequence||in.count>1)return false;
  auto &live=*static_cast<List<Row<32>> *>(o.attached);
  live.resize(in.count);
  std::memcpy(o.container,o.attached,sizeof(live));
  o.input.count=in.count;o.input.present=in.present;o.input.payload_present=in.payload_present;
  void *cursor=ElementAddress(&o,0);
  if(!cursor)std::memcpy(&cursor,o.attached,sizeof(cursor));
  std::memcpy(static_cast<std::byte *>(o.attached)+sizeof(live),&cursor,sizeof(cursor));
  std::memcpy(o.published.data(),o.attached,o.attachment_size);
  return true;
}
// 原生容器的头部在活对象内，旧 owner 仅记录实际分配和最终析构责任。
bool InstallLiveContainer(ClientStagedField &o,const ClientStagedFieldInput &in) noexcept {
  const auto width=in.kind==Kind::String?1u:(in.kind==Kind::Map||in.kind==Kind::Innate)?8u:
    (in.kind==Kind::Blocked||in.kind==Kind::Seeds)?4u:0u;
  if(in.bytes!=std::uint64_t(in.count)*width)return false;
  if((in.kind==Kind::Map||in.kind==Kind::Innate)&&o.elements.size()==in.count){
    bool same_keys=true;
    for(unsigned i=0;i<in.count;++i){
      const auto *entry=static_cast<const std::pair<const int,int> *>(o.elements[i]);
      if(entry->first!=Int(in.data+i*8)){same_keys=false;break;}
    }
    if(same_keys){
      // 键集合相同的更新保留节点和迭代器，只改映射值。
      for(unsigned i=0;i<in.count;++i){
        auto &mapped=static_cast<std::pair<const int,int> *>(o.elements[i])->second;
        const auto value=Int(in.data+i*8+4);
        if(mapped!=value)mapped=value;
      }
      return true;
    }
  }
  AllocationScope allocation(o);
  bool complete=false;
  try {
    if(in.kind==Kind::Records)return ResizeDrawRecords(o,in);
    // 元素索引先预留。若分配失败，现有容器仍可安全析构。
    if(in.kind==Kind::Map||in.kind==Kind::Innate||in.kind==Kind::Blocked||in.kind==Kind::Seeds)
      o.elements.reserve(in.count);
    switch(in.kind){
      case Kind::String:{
        auto &live=*static_cast<String *>(o.attached);
        if(in.bytes)live.assign(reinterpret_cast<const char *>(in.data),in.bytes);
        else live.clear();
        break;
      }
      case Kind::Vector:{
        if(!in.resize_live_sequence||!ClientLiveVectorLayout(in.layout,in.stride)||in.count>SIZE_MAX/in.stride)return false;
        static_cast<Vector<std::byte> *>(o.attached)->resize(std::size_t(in.count)*in.stride);
        o.rows_constructed=in.count;
        break;
      }
      case Kind::Deque:{
        if(!in.resize_live_sequence)return false;
        switch(in.stride){
          case 4:if(in.layout!=10)return false;static_cast<Deque<EntityPtr> *>(o.attached)->resize(in.count);break;
          case 8:static_cast<Deque<Row<8>> *>(o.attached)->resize(in.count);break;
          case 12:static_cast<Deque<Row<12>> *>(o.attached)->resize(in.count);break;
          case 64:static_cast<Deque<Row<64>> *>(o.attached)->resize(in.count);break;
          default:return false;
        }
        if(in.layout!=10)o.rows_constructed=in.count;
        break;
      }
      case Kind::Map:{
        auto &live=*static_cast<Map<int,int> *>(o.attached);
        live.clear();o.elements.clear();
        for(unsigned i=0;i<in.count;++i){
          const auto inserted=live.emplace(Int(in.data+i*8),Int(in.data+i*8+4));
          if(!inserted.second)throw std::invalid_argument("binary_live_map_duplicate");
          o.elements.push_back(&*inserted.first);
        }
        break;
      }
      case Kind::Innate:{
        auto &live=*static_cast<Innates *>(o.attached);
        live.clear();o.elements.clear();
        if(in.count){
          auto &entries=live["IsaacCS"].persistent;
          entries.reserve(in.count);
          for(unsigned i=0;i<in.count;++i){
            const auto inserted=entries.emplace(Int(in.data+i*8),Int(in.data+i*8+4));
            if(!inserted.second)throw std::invalid_argument("binary_live_innate_duplicate");
            o.elements.push_back(&*inserted.first);
          }
        }
        break;
      }
      case Kind::Blocked:{
        auto &live=*static_cast<Blocked *>(o.attached);
        live.clear();o.elements.clear();live.reserve(in.count);
        for(unsigned i=0;i<in.count;++i){
          const auto inserted=live.insert(Int(in.data+i*4));
          if(!inserted.second)throw std::invalid_argument("binary_live_blocked_duplicate");
          o.elements.push_back(const_cast<int *>(&*inserted.first));
        }
        break;
      }
      case Kind::Seeds:{
        auto &live=*static_cast<Seeds *>(o.attached);
        live.clear();o.elements.clear();
        for(unsigned i=0;i<in.count;++i){
          const auto inserted=live.insert(Int(in.data+i*4));
          if(!inserted.second)throw std::invalid_argument("binary_live_seed_duplicate");
          o.elements.push_back(reinterpret_cast<std::byte *>(const_cast<int *>(&*inserted.first))-16);
        }
        break;
      }
      default:return false;
    }
    complete=true;
  }catch(...){}
  // 正常 C++ 异常后，仍保留实际 header，供既有退休路径释放本次已取得的分配。
  std::memcpy(o.container,o.attached,o.attachment_size);
  std::memcpy(o.published.data(),o.attached,o.attachment_size);
  if(in.kind==Kind::Deque)DequeOwner(o,o.attached);
  if(complete)o.input.count=in.count;
  return complete&&!o.uncertain;
}
}
bool ClientStagedFieldInstallValues(ClientStagedField *o,const ClientStagedFieldInput &in) noexcept {
  __try {
    return in.update_live&&o&&ClientBinaryInstallationScopeActive(false)&&o->attached&&
      ClientStagedFieldAttached(o,o->attached)&&in.kind==o->input.kind&&InstallLiveContainer(*o,in);
  } __except(EXCEPTION_EXECUTE_HANDLER){
    if(o)o->uncertain=true;
    return false;
  }
}
bool ClientStagedFieldAttach(ClientStagedField *o,void *destination,std::size_t size) noexcept {
  // 成功路径仅以Peer分配器新增注册表节点并改写已保管的header／Deque代理。
  // 不执行原生回调或销毁旧承载；安装器据此保留页见证。失败后禁止继续使用本批。
  if(!o||o->deferred||!ClientBinaryInstallationScopeActive(false)||!ClientNativeCpuQueuesEmpty()||!ClientStagedFieldCurrent(o)||
      !ClientInspectedFieldCanAttach(o,destination,size))return false;
  const auto *source=ContainerAddress(*o);if(!source)return false;
  Attachments *registry=nullptr;
  try {registry=AttachmentRegistry(o->allocator);if(!registry)return false;
    const auto [at,inserted]=registry->entries.emplace(destination,o);if(!inserted){o->predecessor=at->second;o->predecessor->successor=o;at->second=o;}}
  catch(...){RetireAttachmentRegistry();return false;}
  // 同时保留新旧header：失败可归还前一租约；成功后才把最初的原生header责任向后移交。
  std::memcpy(o->displaced.data(),destination,size);
  if(o->input.kind==Kind::Records){
    // 原生绘制序列的游标跟随list头交接。空序列使用候选自身的哨兵。
    std::memcpy(o->published.data(),source,8);
    void *cursor=ElementAddress(o,0);
    if(!cursor)std::memcpy(&cursor,source,sizeof(cursor));
    std::memcpy(o->published.data()+8,&cursor,sizeof(cursor));
  }else std::memcpy(o->published.data(),source,size);
  o->attached=destination;o->attachment_size=size;o->attachment_uses=1;
  std::memcpy(destination,o->published.data(),size);DequeOwner(*o,destination);
  return ClientStagedFieldAttached(o,destination);
}
bool ClientStagedFieldAttached(const ClientStagedField *o,const void *destination) noexcept {
  return o&&!o->successor&&o->attached==destination&&destination&&ClientStagedFieldCurrent(o);
}
bool ClientStagedFieldRetain(ClientStagedField *o) noexcept {
  if(!o||o->owners==UINT_MAX||!o->attachment_uses||o->predecessor||
      !ClientStagedFieldAttached(o,o->attached)||!attachments)return false;
  const auto i=attachments->entries.find(o->attached);
  if(i==attachments->entries.end()||i->second!=o)return false;
  ++o->owners;return true;
}
bool ClientStagedFieldBorrowAttachment(ClientStagedField *o,void *destination,std::size_t size) noexcept {
  // 安装器在此纯借用路径保留页见证：只能检查当前承载和修改自有计数，
  // 不得加入原生调用、header替换或分配释放；若扩展这些行为须恢复调用边界。
  if(!ClientBinaryInstallationScopeActive(false)||!ClientNativeCpuQueuesEmpty()||!o||o->owners<2||
      !o->attachment_uses||o->attachment_uses==UINT_MAX||size!=o->attachment_size||!ClientStagedFieldAttached(o,destination))return false;
  ++o->attachment_uses;return true;
}
bool ClientStagedFieldCommitCheckedAttachment(ClientStagedField *o,const void *destination) noexcept {
  // 发布已读回此 header。同一主线程段内只有保管转移，仍核对登记及前一租约关系。
  if(!o||!destination||o->attached!=destination||o->successor||!ClientBinaryInstallationScopeActive(false)||!attachments)return false;
  const auto i=attachments->entries.find(o->attached);if(i==attachments->entries.end()||i->second!=o)return false;
  if(auto *previous=o->predecessor){
    if(previous->successor!=o||previous->attached!=o->attached||previous->attachment_size!=o->attachment_size)return false;
    o->displaced=previous->displaced;DequeOwner(*previous,previous->container);previous->attached=nullptr;previous->attachment_size=0;
    previous->successor=nullptr;o->predecessor=nullptr;
  }
  return true;
}
bool ClientStagedFieldDetach(ClientStagedField *o) noexcept {
  if(!o||!o->attachment_uses)return true;
  if(!ClientBinaryInstallationScopeActive(true)||!ClientNativeCpuQueuesEmpty())return false;
  // 新批次仍借用同一 header 时只退本批用途。真实替换已折叠的 header 同样无需再写。
  if(o->attachment_uses>1||!o->attached){--o->attachment_uses;return true;}
  if(!ClientBinaryInstallationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||!ClientStagedFieldCurrent(o)||o->successor||!attachments)return false;
  const auto i=attachments->entries.find(o->attached);if(i==attachments->entries.end()||i->second!=o)return false;
  std::memcpy(o->attached,o->displaced.data(),o->attachment_size);DequeOwner(*o,o->container);
  if(auto *previous=o->predecessor){previous->successor=nullptr;i->second=previous;o->predecessor=nullptr;}
  else attachments->entries.erase(i);
  o->attached=nullptr;o->attachment_size=0;o->attachment_uses=0;RetireAttachmentRegistry();
  return true;
}
bool ClientStagedFieldRelease(ClientStagedField *&o) noexcept {
  if(o&&o->owners>1){
    if(!Window()||!ClientBinaryPreparationScopeActive(true)||!ClientNativeCpuQueuesEmpty())return false;
    --o->owners;o=nullptr;return true;
  }
  if(o&&o->attachment_uses)return false;
  if(!o)return true;if(o->attached||o->predecessor||o->successor||!Window()||!ClientBinaryPreparationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||o->uncertain||o->release_entered)return false;
  if(o->entered){void *dc=nullptr,*gl=nullptr;const auto r=ClientPreparationRoots();if(r.game!=o->roots.game||r.room!=o->roots.room||o->vm!=ClientLuaStateIdentity()||o->manager!=g_Manager||!ClientRendererHandles(dc,gl)||dc!=o->dc||gl!=o->gl)return false;}
  if(o->input.kind==Kind::Shared&&o->container&&
      (!o->shared_constructed||o->shared[0]!=o->container||!SharedPairCurrent(o->shared,o->input.layout)))return false;
  o->release_entered=true;
  try {AllocationScope allocation(*o);
    if(o->input.kind==Kind::Shared&&o->container){
      // 直接归还暂存 holder 的强引用，避免替换 helper 为 Null 另建控制块。
      Native(*o,[&]{ReleaseShared(o->shared,o->input.layout);});o->container=nullptr;
    }else if(o->input.kind==Kind::Weapons){for(unsigned i=5;i>0;--i)if(auto *w=o->weapons[i-1]){const auto type=o->input.weapons[i-1].type;
      if(!type||type>15||*reinterpret_cast<std::uintptr_t *>(w)!=Base+weapon_contracts[type].vptr||w->_owner)return false;
      Native(*o,[&]{for(const auto offset:weapon_contracts[type].refs)if(offset)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(w)+offset)->SetReference(nullptr);
        w->_owner=nullptr;auto **table=*reinterpret_cast<void ***>(w);reinterpret_cast<void (__thiscall *)(void *,unsigned)>(table[0])(w,1);});o->weapons[i-1]=nullptr;}
    }else if(o->container){while(o->rows_constructed){const auto i=o->rows_constructed-1;DestroyRow(*o,o->element(o->container,i));--o->rows_constructed;}
      o->destroy(o->container);o->allocator.release(o->allocator.context,o->container,o->container_bytes,o->container_alignment);o->container=nullptr;}
    if(!o->allocations.empty()||o->uncertain)return false;
    if(o->object_charge)o->allocator.uncharge_native(o->allocator.context,o->object_charge);
    const auto a=o->allocator;o->~ClientStagedField();a.release(a.context,o,sizeof(ClientStagedField),alignof(ClientStagedField));o=nullptr;return true;
  }catch(...){o->uncertain=true;return false;}
}
} // namespace cs::game
