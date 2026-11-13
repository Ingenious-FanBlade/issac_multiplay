#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/player/client_player_roles.h"
#include "game/tests/client_entity_e2e.h"
#include "game/client/player/client_player_equipment.h"
#include "game/observation/sources/lua_entity.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace cs::game {
namespace {
// J273 Lua注册的只读leaf，主智能体复读；不调用Flip/InitTwin/SwapForgottenForm。
using BodyGetter=Entity_Player *(__thiscall *)(Entity_Player *);
Entity_Player *Main(Entity_Player &p){return reinterpret_cast<BodyGetter>(Base+0x13140)(&p);}
Entity_Player *Other(Entity_Player &p){return reinterpret_cast<BodyGetter>(Base+0x13160)(&p);}
Entity_Player *Sub(Entity_Player &p){return reinterpret_cast<BodyGetter>(Base+0x2d86d0)(&p);}
bool IsSub(Entity_Player &p){return reinterpret_cast<bool (__thiscall *)(Entity_Player *)>(Base+0x34d9b0)(&p);}
bool Window(){return IsMain()&&ClientBusy()&&HasOwner(Owner::Manager)&&ContextHealthy()&&ClientLuaGuardReady()&&ClientCallbackGuardReady();}
int &ItemState(Entity_Player &p){return *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(&p)+0x15f4);}
void Integer(lua_State *L,const char *key,int value){lua_pushinteger(L,value);lua_setfield(L,-2,key);}
void Boolean(lua_State *L,const char *key,bool value){lua_pushboolean(L,value);lua_setfield(L,-2,key);}
void Point(lua_State *L,const char *key,const Vector &value){
  if(!std::isfinite(value.x)||!std::isfinite(value.y))throw std::runtime_error("player_role_nonfinite_vector");
  lua_newtable(L);lua_pushnumber(L,value.x);lua_setfield(L,-2,"x");lua_pushnumber(L,value.y);lua_setfield(L,-2,"y");lua_setfield(L,-2,key);
}
}
// 与既有正式来源同一use/普通窗口；备份不借用锚点的PlayerUpdate读取资格。
int ClientPlayerRoleCaptureLua(lua_State *L){
  const auto started=ClientEntityTestClock();
  auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");
  const auto subject=ObservationSubject(p,false);const int top=lua_gettop(L);bool failed=false;
  lua_newtable(L);Integer(L,"version",1);lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  if(!IsMain()||!p||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  try{
    Integer(L,"player_index",p->_playerIndex);Boolean(L,"is_sub",IsSub(*p));Boolean(L,"hologram",p->IsHologram());
    Integer(L,"item_state",ItemState(*p));Integer(L,"suplex_state",p->_suplexState);Integer(L,"suplex_countdown",p->_suplexAimCountdown);
    Point(L,"suplex_target",p->_suplexTargetPos);Point(L,"suplex_land",p->_suplexLandPos);
    lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
  }catch(...){failed=true;}
  ClientEntityTestTime("player_source_role",ClientEntityTestClock()-started);
  if(failed){lua_settop(L,top);return luaL_error(L,"player_role_capture_failed");}return 1;
}
// 与角色本体同一合法普通窗口复制有限槽；不调用带配方/HUD副作用的公开setter。
int ClientPlayerSpecialCaptureLua(lua_State *L){
  const auto started=ClientEntityTestClock();
  auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");
  const auto subject=ObservationSubject(p,false);const int top=lua_gettop(L);bool failed=false;lua_newtable(L);Integer(L,"version",1);
  lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  if(!IsMain()||!p||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  try{
  if(p->_afterImageFrames.size()>16384)throw std::runtime_error("player_afterimage_budget");
  ClientPlayerDrawCapture(L,p);
  ClientPlayerHudCapture(L,p);
  lua_newtable(L);unsigned image_index=0;for(const auto &image:p->_afterImageFrames){lua_newtable(L);Integer(L,"duration",image._duration);Point(L,"position",image._position);lua_rawseti(L,-2,++image_index);}lua_setfield(L,-2,"afterimages");
  lua_newtable(L);for(unsigned i=0;i<16;++i){lua_pushinteger(L,p->_inventoryHistoryIdx[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"inventory_history");
  lua_newtable(L);for(unsigned i=0;i<8;++i){lua_pushinteger(L,static_cast<unsigned char>(p->_bagOfCraftingContent[i]));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"crafting");
  Integer(L,"crafting_item",p->_bagOfCraftingOutput.collectibleType);Integer(L,"crafting_pool",p->_bagOfCraftingOutput.itemPoolType);
  lua_newtable(L);for(unsigned i=0;i<6;++i){lua_pushinteger(L,static_cast<unsigned char>(p->_poopSpellQueue[i]));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"poop");
  lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
  }catch(...){failed=true;}
  ClientEntityTestTime("player_source_special_draw",ClientEntityTestClock()-started);
  if(failed){lua_settop(L,top);return luaL_error(L,"player_special_capture_failed");}return 1;
}
// 有序槽按原位最终量安装，空槽也覆盖旧值；History下标已由全身体纯值预检核对。
}
