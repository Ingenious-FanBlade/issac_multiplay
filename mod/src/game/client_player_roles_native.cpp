#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_player_roles.h"
#include "game/world_sources/lua_entity.h"
#include "game/bridge.h"
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
  if(failed){lua_settop(L,top);return luaL_error(L,"player_role_capture_failed");}return 1;
}
// 只安装具名最终量；原生控制器保持中性，PlayerIndex是本机同T列表/角色语义而非网络身份。
bool ClientPlayerRoleFields(void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){
  if(!Window()||!pointer||!d.role||!ClientPlayerRole(d.player_type)){reason="player_role_fields_context_or_source";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);const auto &s=*d.role;
  if(p._type!=1||p._variant||p._playerType!=d.player_type){reason="player_role_native_type_conflict";return false;}
  if(check){
    if(p._playerIndex!=s.player_index||IsSub(p)!=s.is_sub||ItemState(p)!=s.item_state||p._suplexState!=s.suplex_state||p._suplexAimCountdown!=s.suplex_countdown||
        p._suplexTargetPos.x!=s.suplex_target.x||p._suplexTargetPos.y!=s.suplex_target.y||p._suplexLandPos.x!=s.suplex_land.x||p._suplexLandPos.y!=s.suplex_land.y){reason="player_role_final_storage_postcondition";return false;}
  }else{
    p._playerIndex=s.player_index;ItemState(p)=s.item_state;p._suplexState=s.suplex_state;p._suplexAimCountdown=s.suplex_countdown;
    p._suplexTargetPos=Vector(s.suplex_target.x,s.suplex_target.y);p._suplexLandPos=Vector(s.suplex_land.x,s.suplex_land.y);
  }
  reason=check?"player_role_final_values_checked":"player_role_final_values_installed";return true;
}
// 全部storage已由唯一Player owner绑定后，从实际native getter读取，不用期望值替代原生后态。
bool ClientPlayerRoleRelations(void *pointer,const ClientPlayerDescription &d,const ClientEntityOwners &owners,const ClientNativeRoots &roots,const char *&reason){
  if(!Window()||!pointer||!d.role||!owners.resolve){reason="player_role_relation_context";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);
  for(const auto &slot:{std::pair<const char *,Entity_Player *>{"main_twin",Main(p)},{"other_twin",Other(p)},{"sub_player",Sub(p)},{"flipped_form",p._backupPlayer}}){
    const auto i=d.body_relations.find(slot.first);if(i==d.body_relations.end()||!i->second.present){reason="player_role_named_relation_source";return false;}
    void *expected=nullptr;const auto &ref=i->second;
    if(!ref.is_null){ClientEntityBorrow use;if(!owners.resolve(owners.context,roots,ref.target,0,use,reason))return false;expected=use.object;}
    if(slot.second!=expected){reason="player_role_native_public_relation_postcondition";return false;}
  }
  if(p.IsHologram()!=d.role->hologram||IsSub(p)!=d.role->is_sub){reason="player_role_hologram_or_sub_postcondition";return false;}
  reason="player_role_public_relationships_checked";return true;
}
} // namespace cs::game
