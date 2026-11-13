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
}
