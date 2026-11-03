#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_entity_adapters.h"
#include "game/bridge.h"
#include "game/world_sources/lua_entity.h"
#include "runtime.h"
#include <cmath>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <algorithm>

namespace cs::game {
namespace {
using Payload=ClientEntityClassState::OwnedPresentation;
static_assert(sizeof(ANM2)==0x114,"J273 embedded ANM2");
constexpr unsigned Limit=16384;
void *Allocate(std::size_t bytes){return reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(bytes);}
void Free(void *pointer){reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(pointer);}
unsigned Slot(unsigned category){return category==8?0xecc:0x4b4;}
unsigned Vtable(Payload::Kind kind){switch(kind){case Payload::MrMe:return 0x714c0c;case Payload::SpriteTrail:return 0x714be0;case Payload::PickupGhost:return 0x714bf8;case Payload::IsaacsBody:return 0x714f84;case Payload::AbyssLocust:return 0x71510c;default:return 0;}}
Payload::Kind Kind(void *data){if(!data)return Payload::None;const auto vt=*static_cast<std::uintptr_t *>(data);
  for(const auto k:{Payload::MrMe,Payload::SpriteTrail,Payload::PickupGhost,Payload::IsaacsBody,Payload::AbyssLocust})if(vt==Base+Vtable(k))return k;return Payload::None;}
void *Data(void *entity,unsigned category){return *reinterpret_cast<void **>(static_cast<unsigned char *>(entity)+Slot(category));}
// 只有已完成构造的具体类型才允许使用其真实虚析构；未完成ctor由异常前缀保留，不猜析构。
struct Delete final {void operator()(void *p)const {if(p)reinterpret_cast<void (__thiscall *)(void *,unsigned)>(*static_cast<std::uintptr_t **>(p)[0])(p,1);}};
// 游戏MSVC容器对>=4096字节的数组采用32字节对齐及前置基址；由同一CRT释放。
// 这里构造本地owner存储，不复制Authority header；小块无前缀，大块满足原生free的位移检查。
struct Allocation final {
  void *base=nullptr,*pointer=nullptr;
  explicit Allocation(std::size_t bytes){if(bytes<4096){base=pointer=Allocate(bytes);}
    else {base=Allocate(bytes+39);const auto aligned=(reinterpret_cast<std::uintptr_t>(base)+35)&~std::uintptr_t(31);pointer=reinterpret_cast<void *>(aligned);reinterpret_cast<void **>(pointer)[-1]=base;}}
  ~Allocation(){if(base)Free(base);}
  void *Release(){base=nullptr;return pointer;}
};
// Trail节点是Position(2 f32)、ColorMod(11 f32)、Scale/Rotation/Alpha(3 f32)，共64字节。
struct Trail final {void *proxy;void **map;unsigned capacity,offset,count;};
static_assert(sizeof(Trail)==20,"J273 trail deque");
Trail &TrailOf(void *data){return *reinterpret_cast<Trail *>(static_cast<unsigned char *>(data)+4);}
struct Sprites final {ANM2 *begin,*end,*capacity;};
Sprites &Group(void *data,unsigned index){return *reinterpret_cast<Sprites *>(static_cast<unsigned char *>(data)+(index?0x30:0x24));}
unsigned Size(const Sprites &v){const auto b=reinterpret_cast<std::uintptr_t>(v.begin),e=reinterpret_cast<std::uintptr_t>(v.end),c=reinterpret_cast<std::uintptr_t>(v.capacity);
  if(e<b||c<e||(e-b)%0x114||(e-b)/0x114>Limit)throw std::runtime_error("entity_owned_sprite_vector");return unsigned((e-b)/0x114);}
void ConstructSprites(Sprites &v,unsigned count){if(!count)return;Allocation storage(std::size_t(count)*0x114);v.begin=v.end=static_cast<ANM2 *>(storage.Release());v.capacity=v.begin+count;
  while(unsigned(v.end-v.begin)<count){new(v.end) ANM2;++v.end;}}
const float *Record(const Trail &t,unsigned i){return static_cast<const float *>(t.map[(t.offset+i)&(t.capacity-1)]);}
bool Valid(const Trail &t){return t.count<=Limit&&(!t.count||(t.proxy&&t.map&&t.capacity&&!(t.capacity&(t.capacity-1))&&t.capacity<=32768&&t.count<=t.capacity));}
void Assign(void *entity,unsigned category,void *data){
  // 官方shared-owner替换helper保留真实引用计数与析构，不直接改holder两字。
  reinterpret_cast<void (__thiscall *)(void *,void *)>(Base+(category==8?0x30e40:0x206310))(static_cast<unsigned char *>(entity)+Slot(category),data);
}
void *Create(Payload::Kind kind){
  if(kind==Payload::MrMe||kind==Payload::SpriteTrail){void *p=Allocate(kind==Payload::MrMe?0x128:0x18);
    // ctor异常不能析构未构造对象；上层保留原生异常前缀。正常返回后立即交给真实shared owner。
    return reinterpret_cast<void *(__thiscall *)(void *)>(Base+(kind==Payload::MrMe?0x1cce00:0x1d1720))(p);}
  if(kind==Payload::PickupGhost){auto *p=static_cast<unsigned char *>(Allocate(0x458));unsigned constructed=0;
    try {
      // 5D0FC1—5D0FF8的原生inline构造：真实ExtraData动态类型及四个独立ANM，非Entity vptr伪装。
      *reinterpret_cast<std::uintptr_t *>(p)=Base+Vtable(kind);
      for(;constructed<4;++constructed)new(p+4+constructed*0x114) ANM2;
      *reinterpret_cast<int *>(p+0x454)=0;return p;
    }catch(...){while(constructed)reinterpret_cast<ANM2 *>(p+4+(--constructed)*0x114)->destructor();Free(p);throw;}
  }
  if(kind==Payload::IsaacsBody){auto *p=static_cast<unsigned char *>(Allocate(0x578));unsigned constructed=0;
    try{*reinterpret_cast<std::uintptr_t *>(p)=Base+Vtable(kind);
      for(;constructed<5;++constructed)new(p+4+constructed*0x114) ANM2;
      std::fill_n(reinterpret_cast<int *>(p+0x568),4,-1);return p;
    }catch(...){while(constructed)reinterpret_cast<ANM2 *>(p+4+(--constructed)*0x114)->destructor();Free(p);throw;}}
  if(kind==Payload::AbyssLocust){Allocation object(0x58),sentinel(0x18);auto *p=static_cast<unsigned char *>(object.pointer);
    // 609079—60912D的具名inline构造：只建本地空树、两份ANM vector及游戏专用空关联。
    std::memset(p,0,0x58);*reinterpret_cast<std::uintptr_t *>(p)=Base+Vtable(kind);auto **head=static_cast<void **>(sentinel.pointer);
    head[0]=head;head[1]=head;head[2]=head;*reinterpret_cast<std::uint16_t *>(static_cast<unsigned char *>(sentinel.pointer)+0xc)=0x101;
    *reinterpret_cast<void **>(p+0x1c)=sentinel.Release();return object.Release();}
  return nullptr;
}
void SetKind(lua_State *L,Payload::Kind k){const char *names[]={"None","MrMe","SpriteTrail","PickupGhost","IsaacsBody","AbyssLocust"};lua_pushstring(L,names[k]);lua_setfield(L,-2,"kind");}
void Sprite(lua_State *L,void *data,unsigned offset,unsigned index){lua::luabridge::UserdataPtr::push(L,static_cast<unsigned char *>(data)+offset,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_rawseti(L,-2,index);}
}
// 正式同use来源：只复制已知具体payload的纯值和真实ANM借用，未知RTTI保持Unavailable。
int CaptureOwnedPresentationLua(lua_State *L){
  auto *e=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");const auto category=unsigned(luaL_checkinteger(L,2));
  const auto subject=ObservationSubject(e,false);lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  if(!IsMain()||!e||subject.status!=1||subject.readiness<2||(category!=8&&category!=9)){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  void *data=Data(e,category);const auto kind=Kind(data);
  // 已核主Render仅消费列出的具体payload；其他私有玩法owner只保留存在性事实，不在Client创建玩法对象。
  // 若实际需要的类别遇到未知动态类型，来源明确失败，不能用空owner代替。
  const bool required=category==8?(e->_variant==48||e->_variant==231):(e->_variant==108||e->_variant==166||e->_variant==191);
  if(data&&kind==Payload::None&&!required){lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");lua_pushliteral(L,"AuthorityOnly");lua_setfield(L,-2,"kind");lua_newtable(L);lua_setfield(L,-2,"sprites");return 1;}
  if(data&&(kind==Payload::None||(category==8)!=(kind==Payload::IsaacsBody||kind==Payload::AbyssLocust))){lua_pushliteral(L,"Unavailable");lua_setfield(L,-2,"status");return 1;}
  lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");SetKind(L,kind);
  lua_newtable(L);if(kind==Payload::MrMe)Sprite(L,data,8,1);if(kind==Payload::PickupGhost)for(unsigned i=0;i<4;++i)Sprite(L,data,4+i*0x114,i+1);
  if(kind==Payload::IsaacsBody)for(unsigned i=0;i<5;++i)Sprite(L,data,4+i*0x114,i+1);
  if(kind==Payload::AbyssLocust){unsigned index=0;for(unsigned group=0;group<2;++group){const auto &g=Group(data,group);const auto count=Size(g);
      for(unsigned i=0;i<count;++i){lua::luabridge::UserdataPtr::push(L,g.begin+i,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_rawseti(L,-2,++index);}}}
  lua_setfield(L,-2,"sprites");
  if(kind==Payload::IsaacsBody){lua_newtable(L);for(unsigned i=0;i<4;++i){lua_pushinteger(L,reinterpret_cast<int *>(static_cast<unsigned char *>(data)+0x568)[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"indices");}
  if(kind==Payload::AbyssLocust){lua_pushinteger(L,Size(Group(data,0)));lua_setfield(L,-2,"sprites_a");lua_pushinteger(L,Size(Group(data,1)));lua_setfield(L,-2,"sprites_b");
    lua_pushboolean(L,*reinterpret_cast<void **>(static_cast<unsigned char *>(data)+0x4c)!=nullptr);lua_setfield(L,-2,"authority_link_present");}
  if(kind==Payload::PickupGhost){const int count=*reinterpret_cast<int *>(static_cast<unsigned char *>(data)+0x454);if(count<0||count>4)return luaL_error(L,"entity_ghost_count");lua_pushinteger(L,count);lua_setfield(L,-2,"count");}
  if(kind==Payload::SpriteTrail){const auto &t=TrailOf(data);if(!Valid(t))return luaL_error(L,"entity_trail_container");lua_newtable(L);
    for(unsigned i=0;i<t.count;++i){const auto *record=Record(t,i);if(!record)return luaL_error(L,"entity_trail_record");lua_newtable(L);
      for(unsigned n=0;n<16;++n){std::uint32_t bits=0;std::memcpy(&bits,record+n,4);if(!std::isfinite(record[n]))return luaL_error(L,"entity_trail_nonfinite");lua_pushinteger(L,bits);lua_rawseti(L,-2,n+1);}lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"trail");}
  return 1;
}
int ClientEntityOwnedPresentationLua(lua_State *L){
  const int top=lua_gettop(L);bool failed=false;try{return CaptureOwnedPresentationLua(L);}catch(...){failed=true;}
  if(failed){lua_settop(L,top);return luaL_error(L,"entity_owned_source_container_or_allocation");}return 0;
}
bool ClientEntityOwnedPresentation(void *entity,const ClientObjectDescription &d,bool check,const char *&reason){
  if(d.category!=8&&d.category!=9)return true;const auto &expected=d.derived->owned;void *data=Data(entity,d.category);
  if(data&&Kind(data)==Payload::None){reason="entity_owned_unknown_local_owner";return false;}
  if(check){if((!data)!=(expected.kind==Payload::None)||(data&&Kind(data)!=expected.kind)){reason="entity_owned_type_postcondition";return false;}}
  else if((!data&&expected.kind!=Payload::None)||(data&&Kind(data)!=expected.kind)||
      (data&&expected.kind==Payload::AbyssLocust&&(Size(Group(data,0))!=expected.sprites_a||Size(Group(data,1))!=expected.sprites_b))){
    std::unique_ptr<void,Delete> candidate(Create(expected.kind));if(expected.kind!=Payload::None&&!candidate){reason="entity_owned_type_not_constructible";return false;}Assign(entity,d.category,candidate.get());candidate.release();data=Data(entity,d.category);
    if(expected.kind==Payload::AbyssLocust){ConstructSprites(Group(data,0),expected.sprites_a);ConstructSprites(Group(data,1),expected.sprites_b);}
  }
  if(expected.kind==Payload::IsaacsBody){auto *indices=reinterpret_cast<int *>(static_cast<unsigned char *>(data)+0x568);for(unsigned i=0;i<4;++i){if(check){if(indices[i]!=expected.body_indices[i])return false;}else indices[i]=expected.body_indices[i];}}
  if(expected.kind==Payload::AbyssLocust&&check&&(Size(Group(data,0))!=expected.sprites_a||Size(Group(data,1))!=expected.sprites_b))return false;
  if(expected.kind==Payload::PickupGhost){auto &count=*reinterpret_cast<int *>(static_cast<unsigned char *>(data)+0x454);if(check){if(count!=int(expected.count))return false;}else count=int(expected.count);}
  if(expected.kind==Payload::SpriteTrail){auto &t=TrailOf(data);if(!Valid(t)){reason="entity_trail_native_owner";return false;}
    if(check){if(t.count!=expected.trail.size())return false;for(unsigned i=0;i<t.count;++i)if(!Record(t,i)||std::memcmp(Record(t,i),expected.trail[i].data(),64))return false;}
    else {
      // 新建整个具体owner，使旧deque的块/地图由真实shared-owner析构，Source环形下标重基为0。
      std::unique_ptr<void,Delete> candidate(Create(Payload::SpriteTrail));auto &next=TrailOf(candidate.get());
      if(!expected.trail.empty()){unsigned capacity=8;while(capacity<expected.trail.size())capacity*=2;Allocation map(capacity*sizeof(void *));auto **blocks=static_cast<void **>(map.pointer);std::memset(blocks,0,capacity*sizeof(void *));
        next.map=static_cast<void **>(map.Release());next.capacity=capacity;
        for(const auto &record:expected.trail){Allocation block(64);std::memcpy(block.pointer,record.data(),64);next.map[next.count++]=block.Release();}}
      Assign(entity,d.category,candidate.get());candidate.release();
    }
  }
  reason=check?"entity_owned_payload_checked":"entity_owned_payload_installed";return true;
}
void *ClientEntityOwnedSprite(void *entity,unsigned category,unsigned index){void *data=Data(entity,category);if(!data)return nullptr;
  const auto kind=Kind(data);if(kind==Payload::MrMe&&index==0)return static_cast<unsigned char *>(data)+8;
  if(kind==Payload::PickupGhost&&index<4)return static_cast<unsigned char *>(data)+4+index*0x114;
  if(kind==Payload::IsaacsBody&&index<5)return static_cast<unsigned char *>(data)+4+index*0x114;
  if(kind==Payload::AbyssLocust){auto &a=Group(data,0);const auto size=Size(a);if(index<size)return a.begin+index;auto &b=Group(data,1);if(index-size<Size(b))return b.begin+index-size;}
  return nullptr;
}
bool ClientEntityRetireOwnedPresentation(void *entity,unsigned category,const char *&reason){
  if(category!=8&&category!=9)return true;if(void *data=Data(entity,category)){if(Kind(data)==Payload::None){reason="entity_owned_unknown_retirement";return false;}Assign(entity,category,nullptr);}return true;
}
} // namespace cs::game
