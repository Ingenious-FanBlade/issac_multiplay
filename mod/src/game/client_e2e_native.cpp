#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/bridge.h"
#include "game/context.h"
#include "game/world_sources/global_random.h"
#include "runtime.h"

namespace cs::game {
void *ClientTestCreateVm() noexcept {return IsMain()&&!Settings.world_e2e_token.empty()?luaL_newstate():nullptr;}
void ClientTestDestroyVm(void *state) noexcept {if(IsMain()&&state)lua_close(static_cast<lua_State *>(state));}
// 失败方式：Hook漏接、一般Lua仍执行、PGD写前未隔离、玩法推进。只在专用E2E token开放。
// 调用真实已采用入口并比较具名值，既不清dirty也不恢复进度来制造通过。
std::string ClientTestExerciseNative(const std::string &token) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||!ClientBusy()||!ClientPersistenceBlocked()||
     Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!g_Game||!g_Manager||
     !g_Game->_room||g_Game->_playerManager._playerList.size()!=1)return "ok=0\nerror=client_exercise_context\n";
  try {
    auto *player=g_Game->_playerManager._playerList[0];const auto pos=player->_pos;
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
    return "ok=1\nisolation_exercise=1\nloading_exercise=1\n";
  }catch(...){return "ok=0\nerror=client_exercise_exception\n";}
}
}
