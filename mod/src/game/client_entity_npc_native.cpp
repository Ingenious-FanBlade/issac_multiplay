#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_entity_adapters.h"
#include "game/bridge.h"
#include "game/world_sources/lua_entity.h"
#include "runtime.h"
#include <charconv>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <utility>

namespace cs::game {
namespace {
constexpr unsigned Limit=16384;
// J273 MSVC deque：保留原ctor建立的proxy，map/block各自由游戏allocator拥有。
struct Deque final {void *proxy;void **map;unsigned capacity,offset,count;};
static_assert(sizeof(Deque)==20);
Deque &Refs(void *p){return *reinterpret_cast<Deque *>(static_cast<unsigned char *>(p)+0xbf0);}
Deque &Path(void *p){return *reinterpret_cast<Deque *>(static_cast<unsigned char *>(p)+0xc18);}
Deque &SecondaryPath(void *p){return *reinterpret_cast<Deque *>(static_cast<unsigned char *>(p)+0xc04);}
bool Valid(const Deque &d,unsigned block){return d.count<=Limit&&d.proxy&&(!d.capacity?(!d.map&&!d.offset&&!d.count):
  (d.map&&d.capacity>=8&&d.capacity<=32768&&!(d.capacity&(d.capacity-1))&&d.count<=d.capacity*block));}
template<class T> T *At(Deque &d,unsigned i){const unsigned index=d.offset+i,block=16/sizeof(T);auto *p=d.map[(index/block)&(d.capacity-1)];if(!p)throw std::runtime_error("entity_npc_deque_block");return static_cast<T *>(p)+index%block;}
void *Allocate(std::size_t n){void *p=reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(n);if(!p)throw std::bad_alloc();return p;}
void Free(void *p){if(p)reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(p);}
void **Map(unsigned capacity){const auto bytes=std::size_t(capacity)*4;void *base=Allocate(bytes>=4096?bytes+39:bytes);void *pointer=base;
  if(bytes>=4096){pointer=reinterpret_cast<void *>((reinterpret_cast<std::uintptr_t>(base)+35)&~std::uintptr_t(31));reinterpret_cast<void **>(pointer)[-1]=base;}
  std::memset(pointer,0,bytes);return static_cast<void **>(pointer);}
// 调用前关系槽已撤登记；空map也保留其真实proxy，原生Clear/dtor不会双重释放。
void Clear(Deque &d,bool references){if(!Valid(d,references?4:2))throw std::runtime_error("entity_npc_deque_owner");
  if(references)for(unsigned i=0;i<d.count;++i)reinterpret_cast<EntityPtr *>(At<Entity *>(d,i))->SetReference(nullptr);
  for(unsigned i=0;i<d.capacity;++i)Free(d.map[i]);if(d.map)Free(d.capacity*4>=4096?reinterpret_cast<void **>(d.map)[-1]:d.map);
  d.map=nullptr;d.capacity=d.offset=d.count=0;
}
void Resize(Deque &d,unsigned count,bool references){
  if(count>Limit)throw std::runtime_error("entity_npc_deque_budget");Clear(d,references);if(!count)return;
  unsigned capacity=8;while(capacity*(references?4u:2u)<count)capacity*=2;d.map=Map(capacity);d.capacity=capacity;
  // 先安装空map的真实责任；分配异常保留可解释前缀，不丢失已分配block。
  const unsigned blocks=(count+(references?3u:1u))/(references?4u:2u);
  for(unsigned i=0;i<blocks;++i){d.map[i]=Allocate(16);std::memset(d.map[i],0,16);}d.count=count;
}
}
// 只在同use已完成创建owner后读两份容器；Source不运行路径更新、AI或Render补算。
static int NpcPresentation(lua_State *L){
  auto *e=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");const auto subject=ObservationSubject(e,false);
  lua_newtable(L);if(!IsMain()||!e||!e->ToNPC()||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  auto &refs=Refs(e),&path=Path(e),&secondary=SecondaryPath(e);if(!Valid(refs,4)||!Valid(path,2)||!Valid(secondary,2))return luaL_error(L,"entity_npc_source_deque");
  lua_pushinteger(L,1);lua_setfield(L,-2,"version");lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
  lua_pushinteger(L,refs.count);lua_setfield(L,-2,"reference_count");lua_newtable(L);
  for(unsigned i=0;i<refs.count;++i){lua::luabridge::UserdataPtr::push(L,*At<Entity *>(refs,i),lua::GetMetatableKey(lua::Metatables::ENTITY));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"references");
  for(const auto &stream:{std::pair<const char *,Deque *>{"path",&path},{"secondary_path",&secondary}}){lua_newtable(L);for(unsigned i=0;i<stream.second->count;++i){const auto &p=*At<Vector>(*stream.second,i);if(!std::isfinite(p.x)||!std::isfinite(p.y))return luaL_error(L,"entity_npc_source_path_nonfinite");
    lua_newtable(L);lua_pushnumber(L,p.x);lua_setfield(L,-2,"x");lua_pushnumber(L,p.y);lua_setfield(L,-2,"y");lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,stream.first);}return 1;
}
int ClientEntityNpcPresentationLua(lua_State *L){const int top=lua_gettop(L);bool failed=false;int result=0;
  try{result=NpcPresentation(L);}catch(...){failed=true;}
  if(failed){lua_settop(L,top);return luaL_error(L,"entity_npc_source_deque_copy_failed");}return result;
}
bool ClientEntityNpcCollections(void *pointer,const ClientObjectDescription &d,bool check,const char *&reason){
  if(d.category!=1)return true;auto &refs=Refs(pointer);const auto &s=*d.derived;
  if(!Valid(refs,4)){reason="entity_npc_local_deque_owner";return false;}
  if(check){if(refs.count!=s.npc_composite_count){reason="entity_npc_deque_postcondition_count";return false;}}
  else if(refs.count!=s.npc_composite_count)Resize(refs,s.npc_composite_count,true);
  for(const auto &stream:{std::pair<Deque *,const std::vector<ClientPoseVector> *>{&Path(pointer),&s.npc_path},{&SecondaryPath(pointer),&s.npc_secondary_path}}){auto &path=*stream.first;const auto &points=*stream.second;
    if(!Valid(path,2)){reason="entity_npc_local_path_owner";return false;}
    if(check){if(path.count!=points.size()){reason="entity_npc_path_postcondition_count";return false;}
      for(unsigned i=0;i<path.count;++i){const auto &p=*At<Vector>(path,i);if(p.x!=points[i].x||p.y!=points[i].y){reason="entity_npc_path_postcondition";return false;}}}
    else {if(path.count!=points.size())Resize(path,unsigned(points.size()),false);for(unsigned i=0;i<path.count;++i)*At<Vector>(path,i)=Vector(points[i].x,points[i].y);}}
  return true;
}
bool ClientEntityNpcRelation(void *pointer,const ClientEntityRelation &r,void *target,bool check,bool clear,const char *&reason){
  constexpr const char prefix[]="NativeNPC.Composite:";if(r.member.rfind(prefix,0))return false;unsigned index=0;
  const auto result=std::from_chars(r.member.data()+sizeof(prefix)-1,r.member.data()+r.member.size(),index);auto &refs=Refs(pointer);
  if(result.ec!=std::errc{}||result.ptr!=r.member.data()+r.member.size()||!Valid(refs,4)||index>=refs.count){reason="entity_npc_composite_slot";return false;}
  auto *expected=clear?nullptr:static_cast<Entity *>(target);auto &slot=*reinterpret_cast<EntityPtr *>(At<Entity *>(refs,index));
  if(check){if(slot.GetReference()!=expected){reason="entity_npc_composite_postcondition";return false;}}
  else slot.SetReference(expected);return true;
}
bool ClientEntityNpcRetire(void *pointer,const char *&reason){
  if(!pointer){reason="entity_npc_retire_null";return false;}Clear(Refs(pointer),true);Clear(Path(pointer),false);Clear(SecondaryPath(pointer),false);return true;
}
} // namespace cs::game
