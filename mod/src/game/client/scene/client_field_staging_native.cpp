#include "IsaacRepentance.h"
#include "game/client/scene/client_field_staging_native.h"
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
unsigned SharedCallback(unsigned layout){return layout==18||layout==19||layout==133?0x7ebdb4:0x7ebde8;}
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
  void (__cdecl *shared_release)(void *)=nullptr;
  std::size_t object_charge=0;
  unsigned rows_constructed=0;
  bool entered=false,ready=false,uncertain=false,release_entered=false,shared_constructed=false;
  void *attached=nullptr;
  std::size_t attachment_size=0;
  std::array<std::byte,32> displaced{},published{};
  ClientStagedField *predecessor=nullptr,*successor=nullptr;
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
}
void DestroyRow(ClientStagedField &o,void *p) {
  if(Sprite(o.input.layout)||o.input.layout==152)Native(o,[&]{static_cast<ANM2 *>(p)->destructor();});
  else if(Layer(o.input.layout))Native(o,[&]{static_cast<LayerState *>(p)->~LayerState();});
  else if(o.input.layout==10)Native(o,[&]{static_cast<EntityPtr *>(p)->SetReference(nullptr);});
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
std::int32_t Int(const std::byte *p){std::int32_t n;std::memcpy(&n,p,4);return n;}
void Shared(ClientStagedField &o) {
  if(!o.input.payload_present||!o.input.count)return;
  const auto size=SharedSize(o.input.layout),vptr=SharedVptr(o.input.layout);if(!size||size!=o.input.stride||!vptr)throw std::bad_alloc();
  o.shared_release=*reinterpret_cast<void (__cdecl **)(void *)>(Base+SharedCallback(o.input.layout));
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
  Native(o,[&]{reinterpret_cast<void (__thiscall *)(void *,void *)>(Base+(o.input.layout==18||o.input.layout==19||o.input.layout==133?0x30e40:0x206310))(o.shared.data(),p);});
  if(o.shared[0]!=p||!o.shared[1]){o.uncertain=true;throw std::bad_alloc();}
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
ClientStagedField *ClientStagedFieldCreate(ClientPreparationAllocator a) noexcept {
  if(!IsMain()||!a.allocate||!a.release||!a.charge_native||!a.uncharge_native)return nullptr;void *p=nullptr;
  try{p=a.allocate(a.context,sizeof(ClientStagedField),alignof(ClientStagedField));return p?new(p)ClientStagedField(a):nullptr;}
  catch(...){if(p)a.release(a.context,p,sizeof(ClientStagedField),alignof(ClientStagedField));return nullptr;}
}
ClientPreparationResult ClientStagedFieldConstruct(ClientStagedField *o,const ClientStagedFieldInput &in) noexcept {
  if(!o||!Window()||!ClientBinaryPreparationScopeActive(false))return Bad("binary_fields_context");
  if(!ClientNativeCpuQueuesEmpty())return {ClientPreparationStatus::Waiting,"binary_fields_cpu_borrow"};
  if(o->entered)return o->ready&&ClientStagedFieldCurrent(o)?Ready():Bad("binary_fields_prefix_retained");
  if(in.kind==Kind::Shared&&in.present&&in.payload_present&&in.count&&!(*reinterpret_cast<void **>(Base+SharedCallback(in.layout))))return Bad("binary_fields_shared_release_callback");
  o->entered=true;o->input=in;o->roots=ClientPreparationRoots();o->vm=ClientLuaStateIdentity();o->manager=g_Manager;ClientRendererHandles(o->dc,o->gl);
  try {AllocationScope allocation(*o);
    if(in.present||in.kind==Kind::Records)switch(in.kind){
      case Kind::Group:break;
      case Kind::Rows:case Kind::Vector:case Kind::CString:Rows(*o);break;
      case Kind::String:{auto &s=Container<String>(*o);if(in.bytes!=in.count)throw std::bad_alloc();if(in.bytes)s.assign(reinterpret_cast<const char *>(in.data),in.bytes);break;}
      case Kind::Deque:switch(in.stride){case 4:MakeDeque<4>(*o);break;case 8:MakeDeque<8>(*o);break;case 12:MakeDeque<12>(*o);break;case 64:MakeDeque<64>(*o);break;case 120:MakeDeque<120>(*o);break;default:return Bad("binary_fields_deque_stride");}break;
      case Kind::Records:{if(in.count>1)return Bad("binary_fields_draw_record_count");auto &list=Container<List<Row<32>>>(*o);list.resize(in.count);
        o->element=[](void *p,unsigned)->void *{return reinterpret_cast<std::byte *>(&static_cast<List<Row<32>> *>(p)->front())-offsetof(RecordNode,_Myval);};break;}
      case Kind::Map:{if(in.bytes!=std::size_t(in.count)*8)return Bad("binary_fields_map_keys");auto &m=Container<Map<int,int>>(*o);o->elements.reserve(in.count);
        for(unsigned i=0;i<in.count;++i){const auto result=m.emplace(Int(in.data+i*8),Int(in.data+i*8+4));if(!result.second)return Bad("binary_fields_map_duplicate");o->elements.push_back(&*result.first);}break;}
      case Kind::Blocked:{if(in.bytes!=std::size_t(in.count)*4)return Bad("binary_fields_set_keys");auto &m=Container<Blocked>(*o);m.reserve(in.count);
        o->elements.reserve(in.count);for(unsigned i=0;i<in.count;++i){const auto result=m.insert(Int(in.data+i*4));if(!result.second)return Bad("binary_fields_set_duplicate");o->elements.push_back(const_cast<int *>(&*result.first));}break;}
      case Kind::Innate:{if(in.bytes!=std::size_t(in.count)*8)return Bad("binary_fields_innate_values");auto &m=Container<Innates>(*o);o->elements.reserve(in.count);
        if(in.count){auto &g=m["IsaacCS"];g.persistent.reserve(in.count);for(unsigned i=0;i<in.count;++i){const auto result=g.persistent.emplace(Int(in.data+i*8),Int(in.data+i*8+4));
          if(!result.second)return Bad("binary_fields_innate_duplicate");o->elements.push_back(&*result.first);}}break;}
      case Kind::Seeds:{if(in.bytes!=std::size_t(in.count)*4)return Bad("binary_fields_seed_values");auto &m=Container<Seeds>(*o);o->elements.reserve(in.count);
        for(unsigned i=0;i<in.count;++i){const auto result=m.insert(Int(in.data+i*4));if(!result.second)return Bad("binary_fields_seed_duplicate");
          o->elements.push_back(reinterpret_cast<unsigned char *>(const_cast<int *>(&*result.first))-16);}break;}
      case Kind::Shared:Shared(*o);break;
      case Kind::Weapons:Weapons(*o);break;
    }
    // 输入片段只在当前调用借用，实际承载不固定临时 vector 的地址。
    o->input.data=nullptr;o->input.bytes=0;o->ready=true;
    return ClientStagedFieldCurrent(o)?Ready():Bad("binary_fields_construct_context_changed");
  }catch(...){return Bad(o->uncertain?"binary_fields_native_prefix_retained":"binary_fields_allocation_failed");}
}
bool ClientStagedFieldCurrent(const ClientStagedField *o) noexcept {
  if(!o||!o->ready||o->uncertain||o->release_entered||!Window()||o->vm!=ClientLuaStateIdentity()||o->manager!=g_Manager)return false;
  void *dc=nullptr,*gl=nullptr;const auto roots=ClientPreparationRoots();if(roots.game!=o->roots.game||roots.room!=o->roots.room||!ClientRendererHandles(dc,gl)||dc!=o->dc||gl!=o->gl)return false;
  if(o->attached){if(o->successor){if(o->successor->predecessor!=o||o->successor->attached!=o->attached)return false;}
    else if(std::memcmp(o->attached,o->published.data(),o->attachment_size))return false;}
  if(o->input.kind==Kind::Weapons&&o->input.present&&!WeaponsCurrent(*o))return false;
  if(o->input.kind==Kind::Shared&&o->input.present&&o->input.payload_present&&o->input.count)
    return o->shared_constructed&&o->shared[0]==o->container&&o->shared[1]&&*static_cast<const std::uintptr_t *>(o->container)==Base+SharedVptr(o->input.layout)&&
      o->shared_release&&o->shared_release==*reinterpret_cast<void (__cdecl **)(void *)>(Base+SharedCallback(o->input.layout));
  return true;
}
const void *ClientStagedFieldContainer(const ClientStagedField *o) noexcept {
  if(!ClientStagedFieldCurrent(o))return nullptr;
  return o->input.kind==Kind::Weapons?static_cast<const void *>(o->weapons.data()):o->input.kind==Kind::Shared?static_cast<const void *>(o->shared.data()):o->container;
}
void *ClientStagedFieldElement(ClientStagedField *o,std::uint32_t i) noexcept {
  if(!ClientStagedFieldCurrent(o)||!o->input.present)return nullptr;
  if(o->input.kind==Kind::Weapons)return i<5?o->weapons[i]:nullptr;
  if(i>=o->input.count)return nullptr;if(o->input.kind==Kind::Shared)return o->container;
  if(i<o->elements.size())return o->elements[i];
  if(o->input.kind==Kind::String)return &(*static_cast<String *>(o->container))[i];
  if(!o->element)return nullptr;AllocationScope allocation(*o);return o->element(o->container,i);
}
std::size_t ClientStagedFieldAttachmentSize(const ClientStagedField *o) noexcept {
  if(!ClientStagedFieldCurrent(o)||(!o->input.present&&o->input.kind!=Kind::Records))return 0;
  switch(o->input.kind){
    case Kind::Vector:return 12;case Kind::String:return 24;case Kind::Deque:return 20;
    case Kind::Map:case Kind::Innate:case Kind::Seeds:case Kind::Shared:return 8;
    case Kind::Blocked:return 32;case Kind::Weapons:return 20;case Kind::Records:return 12;default:return 0;
  }
}
bool ClientStagedFieldCanAttach(const ClientStagedField *o,void *destination,std::size_t size) noexcept {
  if(!o||o->attached||o->predecessor||o->successor||!destination||!size||size!=ClientStagedFieldAttachmentSize(o)||destination==ClientStagedFieldContainer(o))return false;
  if(attachments){if(attachments->allocator.context!=o->allocator.context)return false;const auto i=attachments->entries.find(destination);
    if(i!=attachments->entries.end()){const auto *old=i->second;return old!=o&&!old->successor&&old->attachment_size==size&&
      old->input.kind==o->input.kind&&ClientStagedFieldCurrent(old);}}
  return true;
}
bool ClientStagedFieldAttach(ClientStagedField *o,void *destination,std::size_t size) noexcept {
  if(!ClientBinaryInstallationScopeActive(false)||!ClientNativeCpuQueuesEmpty()||!ClientStagedFieldCanAttach(o,destination,size))return false;
  const auto *source=ClientStagedFieldContainer(o);if(!source)return false;
  Attachments *registry=nullptr;
  try {registry=AttachmentRegistry(o->allocator);if(!registry)return false;
    const auto [at,inserted]=registry->entries.emplace(destination,o);if(!inserted){o->predecessor=at->second;o->predecessor->successor=o;at->second=o;}}
  catch(...){RetireAttachmentRegistry();return false;}
  // 同时保留新旧header：失败可归还前一租约；成功后才把最初的原生header责任向后移交。
  std::memcpy(o->displaced.data(),destination,size);
  if(o->input.kind==Kind::Records){
    // 原生绘制序列的游标跟随list头交接。空序列使用候选自身的哨兵。
    std::memcpy(o->published.data(),source,8);
    void *cursor=ClientStagedFieldElement(o,0);
    if(!cursor)std::memcpy(&cursor,source,sizeof(cursor));
    std::memcpy(o->published.data()+8,&cursor,sizeof(cursor));
  }else std::memcpy(o->published.data(),source,size);
  o->attached=destination;o->attachment_size=size;std::memcpy(destination,o->published.data(),size);DequeOwner(*o,destination);
  return ClientStagedFieldAttached(o,destination);
}
bool ClientStagedFieldAttached(const ClientStagedField *o,const void *destination) noexcept {
  return o&&!o->successor&&o->attached==destination&&destination&&ClientStagedFieldCurrent(o);
}
bool ClientStagedFieldCommitAttachment(ClientStagedField *o) noexcept {
  if(!o||!o->attached||o->successor||!ClientBinaryInstallationScopeActive(false)||!ClientStagedFieldCurrent(o)||!attachments)return false;
  const auto i=attachments->entries.find(o->attached);if(i==attachments->entries.end()||i->second!=o)return false;
  if(auto *previous=o->predecessor){
    if(previous->successor!=o||previous->attached!=o->attached||previous->attachment_size!=o->attachment_size)return false;
    o->displaced=previous->displaced;DequeOwner(*previous,previous->container);previous->attached=nullptr;previous->attachment_size=0;
    previous->successor=nullptr;o->predecessor=nullptr;
  }
  return true;
}
bool ClientStagedFieldDetach(ClientStagedField *o) noexcept {
  if(!o||!o->attached)return true;
  if(!ClientBinaryInstallationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||!ClientStagedFieldCurrent(o)||o->successor||!attachments)return false;
  const auto i=attachments->entries.find(o->attached);if(i==attachments->entries.end()||i->second!=o)return false;
  std::memcpy(o->attached,o->displaced.data(),o->attachment_size);DequeOwner(*o,o->container);
  if(auto *previous=o->predecessor){previous->successor=nullptr;i->second=previous;o->predecessor=nullptr;}
  else attachments->entries.erase(i);
  o->attached=nullptr;o->attachment_size=0;RetireAttachmentRegistry();
  return true;
}
bool ClientStagedFieldRelease(ClientStagedField *&o) noexcept {
  if(!o)return true;if(o->attached||o->predecessor||o->successor||!Window()||!ClientBinaryPreparationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||o->uncertain||o->release_entered)return false;
  if(o->entered){void *dc=nullptr,*gl=nullptr;const auto r=ClientPreparationRoots();if(r.game!=o->roots.game||r.room!=o->roots.room||o->vm!=ClientLuaStateIdentity()||o->manager!=g_Manager||!ClientRendererHandles(dc,gl)||dc!=o->dc||gl!=o->gl)return false;}
  if(o->input.kind==Kind::Shared&&o->container&&(!o->shared_constructed||o->shared[0]!=o->container||!o->shared[1]||!o->shared_release||o->shared_release!=*reinterpret_cast<void (__cdecl **)(void *)>(Base+SharedCallback(o->input.layout))))return false;
  o->release_entered=true;
  try {AllocationScope allocation(*o);
    if(o->input.kind==Kind::Shared&&o->container){
      // 替换 helper 即使传 Null 也创建新 control block，不能当作析构。按真实 strong
      // pair 的减引用／最后释放回调归还原 holder，不产生一个遗失的 Null control block。
      Native(*o,[&]{auto **table=*static_cast<void ***>(o->shared[1]);const bool last=reinterpret_cast<bool (__thiscall *)(void *)>(table[3])(o->shared[1]);
        if(!last)throw std::bad_alloc();o->shared_release(o->shared.data());});o->shared={};o->container=nullptr;
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
