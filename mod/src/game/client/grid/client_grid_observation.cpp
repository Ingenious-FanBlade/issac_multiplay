#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/common/context.h"
#include "runtime.h"
#include <cmath>
#include <cstring>

namespace cs::game {
namespace {
// E2E 对照只读 Grid 当前字段及 Sprite，不执行旧 Grid 应用或绘制。
template<class T> struct NativeArray {T *begin,*end,*capacity;};
struct Ember {float x,y,vx,vy;int age,max_life;};
struct Point {float x,y;};
template<class T> T Read(const void *base,unsigned offset) noexcept {
  T value;
  std::memcpy(&value,static_cast<const unsigned char *>(base)+offset,sizeof(value));
  return value;
}
bool Rock(int type){return (type>=2&&type<=6)||type==22||type==24||type==25||type==26||type==27;}
}
int ClientGridCaptureLua(void *state){
  auto *L=static_cast<lua_State *>(state);if(!IsMain()){lua_pushnil(L);return 1;}
  auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");if(!grid){lua_pushnil(L);return 1;}
  lua_createtable(L,0,7);lua_pushinteger(L,2);lua_setfield(L,-2,"version");lua_pushinteger(L,grid->_spawnFrame);lua_setfield(L,-2,"spawn_frame");
  const auto number=[L](const char *name,float value){if(!std::isfinite(value))luaL_error(L,"grid_runtime_nonfinite");lua_pushnumber(L,value);lua_setfield(L,-2,name);};
  lua_createtable(L,0,3);number("width",g_WIDTH);number("height",g_HEIGHT);
  number("pixel_scale",*reinterpret_cast<const float *>(Base+0x788730)* *reinterpret_cast<const float *>(Base+0x7883e8));lua_setfield(L,-2,"render_inputs");
  if(grid->GetType()==16){lua_pushboolean(L,Read<bool>(grid,0x154));lua_setfield(L,-2,"door_animation_mode");
    lua_pushinteger(L,Read<unsigned>(grid,0x28c));lua_setfield(L,-2,"door_light_flags");}
  const auto valid=[](const auto &v,unsigned stride){const auto b=reinterpret_cast<std::uintptr_t>(v.begin),e=reinterpret_cast<std::uintptr_t>(v.end),c=reinterpret_cast<std::uintptr_t>(v.capacity);
    return e>=b&&c>=e&&e-b<=4096*stride&&c-b<=65536*stride&&(e-b)%stride==0&&(c-b)%stride==0&&(b||(!e&&!c));};
  if(grid->GetType()==13){lua_pushinteger(L,Read<int>(grid,0x268));lua_setfield(L,-2,"fire_timer");lua_pushinteger(L,Read<int>(grid,0x26c));lua_setfield(L,-2,"fire_style");
    const auto v=Read<NativeArray<Ember>>(grid,0x270);if(!valid(v,24))return luaL_error(L,"grid_ember_vector");lua_newtable(L);unsigned i=0;
    for(auto *p=v.begin;p!=v.end;++p){lua_createtable(L,0,6);number("x",p->x);number("y",p->y);number("vx",p->vx);number("vy",p->vy);
      lua_pushinteger(L,p->age);lua_setfield(L,-2,"age");lua_pushinteger(L,p->max_life);lua_setfield(L,-2,"max_life");lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"embers");}
  if(Rock(grid->GetType())){const auto v=Read<NativeArray<Point>>(grid,0x184);if(!valid(v,8))return luaL_error(L,"grid_rubble_vector");lua_newtable(L);unsigned i=0;
    for(auto *p=v.begin;p!=v.end;++p){lua_createtable(L,0,2);number("x",p->x);number("y",p->y);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"rubble");}return 1;
}
int ClientGridSpritesLua(void *state){
  auto *L=static_cast<lua_State *>(state);auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");lua_newtable(L);
  // TimerPlate属性返回ANM2值副本，拷贝构造清Offset/flags；在普通主线程采集真实成员。
  // 未加载保持公开Getter的nil语义，不把未初始化或停用资源解释为有效Sprite。
  if(IsMain()&&grid&&grid->GetType()==20){auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(grid)+0x154);
    if(sprite->_loaded){lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_setfield(L,-2,"timer");}}
  if(IsMain()&&grid&&grid->GetType()==13)for(unsigned i=0;i<2;++i){auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(grid)+(i?0x27c:0x154));
    lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_rawseti(L,-2,i+1);}return 1;
}
} // namespace cs::game
