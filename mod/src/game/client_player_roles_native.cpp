#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_player_roles.h"
#include "game/client_entity_e2e.h"
#include "game/client_player_equipment.h"
#include "game/world_sources/lua_entity.h"
#include "game/bridge.h"
#include "game/context.h"
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
bool ClientPlayerSpecialFields(void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){
  if(!Window()||!pointer||!d.special){reason="player_special_context_or_source";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);const auto &s=*d.special;
  if(!ClientPlayerDrawRecords(pointer,d,check,reason))return false;
  decltype(p._afterImageFrames) images;for(const auto &image:s.afterimages){AfterImageFrame row{};row._duration=image.duration;row._position=Vector(image.position.x,image.position.y);images.push_back(row);}
  if(check){if(p._afterImageFrames.size()!=images.size()){reason="player_afterimage_count_postcondition";return false;}
    for(unsigned i=0;i<images.size();++i)if(p._afterImageFrames[i]._duration!=images[i]._duration||p._afterImageFrames[i]._position.x!=images[i]._position.x||p._afterImageFrames[i]._position.y!=images[i]._position.y){reason="player_afterimage_value_postcondition";return false;}}
  else p._afterImageFrames.swap(images);
  if(s.crafting_item&&(!g_Manager||!g_Manager->GetItemConfig()->GetCollectible(s.crafting_item))){reason="player_crafting_local_config";return false;}
  for(unsigned i=0;i<16;++i){if(check){if(p._inventoryHistoryIdx[i]!=s.inventory_history[i]){reason="player_inventory_history_postcondition";return false;}}else p._inventoryHistoryIdx[i]=s.inventory_history[i];}
  for(unsigned i=0;i<8;++i){if(check){if(static_cast<unsigned char>(p._bagOfCraftingContent[i])!=s.crafting[i]){reason="player_crafting_content_postcondition";return false;}}else p._bagOfCraftingContent[i]=char(s.crafting[i]);}
  for(unsigned i=0;i<6;++i){if(check){if(static_cast<unsigned char>(p._poopSpellQueue[i])!=s.poop[i]){reason="player_poop_queue_postcondition";return false;}}else p._poopSpellQueue[i]=char(s.poop[i]);}
  if(check){if(p._bagOfCraftingOutput.collectibleType!=s.crafting_item||p._bagOfCraftingOutput.itemPoolType!=s.crafting_pool){reason="player_crafting_result_postcondition";return false;}}
  else {p._bagOfCraftingOutput.collectibleType=s.crafting_item;p._bagOfCraftingOutput.itemPoolType=s.crafting_pool;}
  reason=check?"player_special_final_slots_checked":"player_special_final_slots_installed";return true;
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
