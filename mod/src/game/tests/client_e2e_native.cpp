#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "game/observation/sources/global_random.h"
#include "runtime.h"
#include "game/tests/client_entity_e2e.h"

namespace cs::game {
namespace {void *dummy_source_factory=nullptr;bool dummy_source_used=false;}
// 仅在一个显式Source Lua调用栈内替换通用Factory选择；保留外层官方New/Init/Publish。
bool ClientTestDummyAcquire(void *factory,unsigned type,bool,void *&result) noexcept {
  if(!IsMain()||ClientBusy()||factory!=dummy_source_factory||!factory||type!=964)return false;
  dummy_source_used=true;result=nullptr;
  auto *pool=static_cast<unsigned char *>(factory)+0x675650;
  for(unsigned i=0;i<128;++i){auto *e=reinterpret_cast<Entity *>(pool+i*0x410);
    if(*reinterpret_cast<std::uintptr_t *>(e)!=Base+0x7181b8)return true;
    if(!e->_valid&&!e->_exists){result=e;break;}
  }
  return true;
}
int ClientTestEntitySourceLua(lua_State *L){
  const auto *token=luaL_checkstring(L,1),*kind=luaL_checkstring(L,2);
  if(!IsMain()||ClientBusy()||!HostBusy()||Settings.world_e2e_token.empty()||Settings.world_e2e_token!=token||
     std::strcmp(kind,"native-dummy")||!g_Game||!g_Game->_room||!g_Game->_entityFactory||
     g_Game->_playerManager._playerList.empty()||!g_Game->_playerManager._playerList[0]||dummy_source_factory)
    return luaL_error(L,"entity_source_context_or_token");
  Entity *value=nullptr;
  try {
    struct Scope {Scope(){dummy_source_factory=g_Game->_entityFactory;dummy_source_used=false;}~Scope(){dummy_source_factory=nullptr;}} scope;
    // SDK没有Lua的Room:GetCenterPos包装；专用Source桥在真实当前Player位置生成，不猜原生入口。
    const Vector position=g_Game->_playerManager._playerList[0]->_pos,velocity(0,0);
    // 正常Game::Spawn负责所有后续owner；该记录明确是E2E专用池桥，不冒充自然Dummy caller。
    value=g_Game->Spawn(964,0,position,velocity,nullptr,0,191913,0);
  }catch(...){return luaL_error(L,"entity_source_unknown_native_prefix;no_retry");}
  if(!dummy_source_used||!value||*reinterpret_cast<std::uintptr_t *>(value)!=Base+0x7181b8||!value->_valid||!value->_exists)
    return luaL_error(L,"entity_source_dummy_not_published");
  lua_pushboolean(L,true);return 1;
}
void *ClientTestCreateVm() noexcept {return IsMain()&&!Settings.world_e2e_token.empty()?luaL_newstate():nullptr;}
void ClientTestDestroyVm(void *state) noexcept {if(IsMain()&&state)lua_close(static_cast<lua_State *>(state));}
// 失败方式：Hook漏接、一般Lua仍执行、PGD写前未隔离、玩法推进。只在专用E2E token开放。
// 调用真实已采用入口并比较具名值，既不清dirty也不恢复进度来制造通过。
std::string ClientTestExerciseNative(const std::string &token) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||!ClientBusy()||!ClientPersistenceBlocked()||
     Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!g_Game||!g_Manager||
     !g_Game->_room||g_Game->_playerManager._playerList.empty())return "ok=0\nerror=client_exercise_context\n";
  try {
    auto *player=g_Game->_playerManager._playerList[0];const auto pos=player->_pos;
    const auto roster=g_Game->_playerManager._playerList;std::vector<std::pair<Vector,int>> body_values;for(auto *p:roster)body_values.push_back({p->_pos,p->_redHearts});
    const auto hearts=player->_redHearts;auto *pgd=g_Manager->GetPersistentGameData();
    const auto counter=pgd->GetEventCounter(522);const auto random=ObservationGlobalRandomState(true);
    g_Game->Update();player->Update();g_Game->_roomTransition.Update();
    reinterpret_cast<void (__thiscall *)(Room *)>(Base+0x3f7f60)(g_Game->_room);
    reinterpret_cast<int (__cdecl *)()>(Base+0x2f9340)();
    // 实际加载入口也必须在任何初始化/Restore副作用前拒绝；参数取真实现有owner。
    g_Manager->ExecuteStartGame();g_Game->RestoreState(&g_Manager->_gamestate,false);
    g_Game->_room->Init(g_Game->_room->_roomType,g_Game->_room->_descriptor);
    // 这些入口必须在官方前置写入之前返回；E2E同时检查对应拒绝计数增长。
    pgd->IncreaseEventCounter(522,1);pgd->SaveLocally();pgd->SaveToSteamCloud();
    Manager::RecordPlayerCompletion(1);
    auto *L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L)return "ok=0\nerror=client_exercise_lua\n";
    const auto top=lua_gettop(L);
    static constexpr char script[]=
      "local p=Isaac.GetPersistentGameData(); p:IncreaseEventCounter(522,1); p:TryUnlock(1); "
      "local m=assert(IsaacCSClientTestMod); Isaac.SaveModData(m,'tx17-must-not-write'); "
      "Isaac.RemoveModData(m); m:SaveData('tx17-method-must-not-write'); m:RemoveData(); "
      "collectgarbage('collect'); _RunCallback(0,nil)";
    const bool lua_ok=luaL_loadbuffer(L,script,sizeof(script)-1,"tx17-isolation-exercise")==LUA_OK&&lua_pcall(L,0,0,0)==LUA_OK;
    lua_settop(L,top);
    if(!lua_ok||pgd->GetEventCounter(522)!=counter||player->_pos.x!=pos.x||player->_pos.y!=pos.y||
       player->_redHearts!=hearts||ObservationGlobalRandomState(true)!=random)return "ok=0\nerror=client_isolation_changed\n";
    if(roster!=g_Game->_playerManager._playerList)return "ok=0\nerror=client_isolation_roster_changed\n";
    for(unsigned i=0;i<roster.size();++i)if(roster[i]->_pos.x!=body_values[i].first.x||roster[i]->_pos.y!=body_values[i].first.y||roster[i]->_redHearts!=body_values[i].second)return "ok=0\nerror=client_isolation_body_changed\n";
    return "ok=1\nisolation_exercise=1\nloading_exercise=1\n";
  }catch(...){return "ok=0\nerror=client_exercise_exception\n";}
}
}
