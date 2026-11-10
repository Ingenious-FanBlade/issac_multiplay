#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/room/client_map_native.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cs::game {
namespace {
template<class T> T Read(const void *p,unsigned offset){T value{};std::memcpy(&value,static_cast<const unsigned char *>(p)+offset,sizeof(T));return value;}
template<class T> void Write(void *p,unsigned offset,const T &value){std::memcpy(static_cast<unsigned char *>(p)+offset,&value,sizeof(T));}
void Integer(lua_State *L,const char *name,std::int64_t n){lua_pushinteger(L,lua_Integer(n));lua_setfield(L,-2,name);}
void Real(lua_State *L,const char *name,float n){if(!std::isfinite(n))luaL_error(L,"map_nonfinite");lua_pushnumber(L,n);lua_setfield(L,-2,name);}
void VectorValue(lua_State *L,const char *name,Vector v){lua_createtable(L,0,2);Real(L,"x",v.x);Real(L,"y",v.y);lua_setfield(L,-2,name);}
constexpr unsigned camera_offsets[]={0x60,0x6c,0x74,0x7c,0x84};
// 原生仅在 map cache draw 中读这些字段；结构显式对应 0x78 stride，不复制保存对象私有指针。
struct MapSavedProjection final {
  int type=0,variant=0,subtype=0;
  unsigned unused1=0,unused2=0;
  int storage3=0;
  unsigned unused3[4]{};
  float x=0,y=0;
  unsigned unused4[18]{};
};
static_assert(sizeof(MapSavedProjection)==0x78&&offsetof(MapSavedProjection,x)==0x28);
struct VectorHeader final {void *begin=nullptr,*end=nullptr,*capacity=nullptr;};
static_assert(sizeof(VectorHeader)==12);
RoomDescriptor *Descriptor(Game &game,const ClientMapDescriptor &v){
  if(v.source_slot>=-20&&v.source_slot<0&&v.source_slot!=-19&&v.storage==-1)return &game._negativeGridRooms[-v.source_slot-1];
  return !v.source_slot&&v.storage>=0&&v.storage<507?&game._gridRooms[v.storage]:nullptr;
}
}
int ClientMapDescriptorCaptureLua(void *state){
  auto *L=static_cast<lua_State *>(state);if(!IsMain()){lua_pushnil(L);return 1;}
  auto *d=lua::GetLuabridgeUserdata<RoomDescriptor *>(L,1,lua::Metatables::ROOM_DESCRIPTOR,lua::Metatables::CONST_ROOM_DESCRIPTOR,"RoomDescriptor");
  if(d->SavedEntities.size()>4096){lua_pushnil(L);return 1;}lua_createtable(L,0,2);Integer(L,"version",1);lua_newtable(L);unsigned n=0;
  for(const auto &item:d->SavedEntities){lua_createtable(L,0,6);Integer(L,"type",item.type);Integer(L,"variant",item.variant);Integer(L,"subtype",item.subtype);
    Integer(L,"storage3",item.intStorage3);VectorValue(L,"position",item.targetPosition);lua_rawseti(L,-2,++n);}
  lua_setfield(L,-2,"icons");return 1;
}
int ClientMapStateCaptureLua(void *state){
  auto *L=static_cast<lua_State *>(state);if(!IsMain()||!g_Game||!g_Game->_room||!g_Game->_room->_Camera){lua_pushnil(L);return 1;}
  auto &room=*g_Game->_room;auto &camera=*room._Camera;lua_createtable(L,0,4);Integer(L,"version",1);
  lua_createtable(L,0,5);lua_createtable(L,5,0);
  for(unsigned i=0;i<5;++i){const auto v=Read<Vector>(&camera,camera_offsets[i]);lua_createtable(L,0,2);Real(L,"x",v.x);Real(L,"y",v.y);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"vectors");
  lua_pushboolean(L,camera._shouldOverride);lua_setfield(L,-2,"overridden");Real(L,"smoothing",Read<float>(&camera,0x90));
  const int count=Read<int>(&camera,0x58),capacity=Read<int>(&camera,0x54),head=Read<int>(&camera,0x5c);auto *values=Read<Vector *>(&camera,0x50);
  if(count<0||capacity!=120||count>capacity||head<0||head>=capacity||!values)return luaL_error(L,"camera_focus_ring");
  lua_pushboolean(L,count>0);lua_setfield(L,-2,"focus_present");VectorValue(L,"focus_tail",count?values[(head+count-1)%capacity]:Vector(0,0));lua_setfield(L,-2,"camera");
  lua_createtable(L,2,0);for(unsigned i=0;i<2;++i){auto &config=g_Game->_minimap._config[i];lua_createtable(L,0,11);
    VectorValue(L,"anchor",config._vec1);VectorValue(L,"padding",config._borderPadding);VectorValue(L,"size1",config._size1);VectorValue(L,"size2",config._size2);
    Real(L,"offset",config._posOffsetX);Real(L,"border_scale",config._borderScale);Real(L,"alpha",config._alpha);
    Integer(L,"width",config._i1);Integer(L,"height",config._i2);Integer(L,"icons",config._minimapIconNum);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"configs");
  lua_createtable(L,2,0);for(unsigned i=0;i<2;++i){lua_pushinteger(L,unsigned(g_Game->_gameStateFlags>>(32*i)));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"game_flags");return 1;
}
bool ClientCameraApply(void *pointer,const ClientCameraState &state) noexcept {
  if(!IsMain()||!pointer)return false;auto &camera=*static_cast<Camera *>(pointer);
  // 只安装已研究的数值区，保留构造器的 Room 指针与六个 ring 分配/容量。
  for(unsigned i=0;i<5;++i){const auto &v=state.vectors[i];if(!std::isfinite(v.x)||!std::isfinite(v.y))return false;}
  if(!std::isfinite(state.focus_tail.x)||!std::isfinite(state.focus_tail.y)||!std::isfinite(state.smoothing))return false;
  auto *focus=Read<Vector *>(&camera,0x50);if(!focus||Read<int>(&camera,0x54)!=120)return false;
  for(unsigned i=0;i<5;++i)Write(&camera,camera_offsets[i],Vector(state.vectors[i].x,state.vectors[i].y));
  const int head=0,count=state.focus_present?1:0;Write(&camera,0x58,count);Write(&camera,0x5c,head);if(count)focus[0]=Vector(state.focus_tail.x,state.focus_tail.y);
  camera._shouldOverride=state.overridden;Write(&camera,0x90,state.smoothing);return true;
}
bool ClientMinimapBuild(void *pointer,const std::vector<ClientMapDescriptor> &source,const ClientMapState &state,unsigned effective_curses) noexcept {
  if(!IsMain()||!g_Game||pointer!=g_Game||source.size()>526||!ClientPersistenceBlocked())return false;auto &game=*g_Game;
  // April's Fool 的原生地图 Refresh 会随机选当前标记；没有已裁决映射时显式保留该具体依赖。
  if(game._challenge==32)return false;
  // 缓冲先于借用 owner 存活，退出时先还原原生 header，再释放本方投影内存。
  std::vector<std::vector<MapSavedProjection>> values;
  struct Borrow final {
    Game &game;std::uint64_t flags;unsigned curses,temporary;std::vector<std::pair<RoomDescriptor *,VectorHeader>> old;
    ~Borrow(){for(auto &item:old)std::memcpy(&item.first->SavedEntities,&item.second,12);game._gameStateFlags=flags;game._curses=curses;Write(&game,0x25ac8,temporary);}
  } borrow{game,game._gameStateFlags,game._curses,Read<unsigned>(&game,0x25ac8),{}};
  try{
    values.resize(source.size());borrow.old.reserve(source.size());unsigned total=0;
    for(unsigned i=0;i<source.size();++i){auto *d=Descriptor(game,source[i]);if(!d||source[i].icons.size()>4096||(total+=unsigned(source[i].icons.size()))>65536)return false;
      for(const auto &s:source[i].icons){MapSavedProjection item;item.type=s.type;item.variant=s.variant;item.subtype=s.subtype;item.storage3=s.storage3;item.x=s.position.x;item.y=s.position.y;values[i].push_back(item);}}
    for(unsigned i=0;i<source.size();++i){auto *d=Descriptor(game,source[i]);VectorHeader previous;std::memcpy(&previous,&d->SavedEntities,12);borrow.old.push_back({d,previous});
      auto &v=values[i];VectorHeader next{v.empty()?nullptr:v.data(),v.empty()?nullptr:v.data()+v.size(),v.empty()?nullptr:v.data()+v.size()};std::memcpy(&d->SavedEntities,&next,12);}
    game._gameStateFlags=std::uint64_t(state.game_flags[0])|(std::uint64_t(state.game_flags[1])<<32);
    game._curses=effective_curses;Write(&game,0x25ac8,0u);
    for(unsigned i=0;i<2;++i){auto &c=game._minimap._config[i];const auto &s=state.configs[i];
      c._vec1=Vector(s.anchor.x,s.anchor.y);c._borderPadding=Vector(s.padding.x,s.padding.y);c._posOffsetX=s.offset;c._borderScale=s.border_scale;
      c._i1=s.width;c._i2=s.height;c._minimapIconNum=s.icons;c._size1=Vector(s.size1.x,s.size1.y);c._size2=Vector(s.size2.x,s.size2.y);c._alpha=s.alpha;}
    // 已主审 965D60/9635B0：仅来源 descriptor/保存图标与原生 ANM/FBO 缓存；不调用 Game/Room Update。
    // 外层 presentation RenderState 负责完整 target/shader/depth 恢复。
    game._minimap.Refresh();return true;
  }catch(...){return false;}
}
} // namespace cs::game
