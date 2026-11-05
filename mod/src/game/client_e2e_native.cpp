#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/bridge.h"
#include "game/context.h"
#include "game/world_sources/global_random.h"
#include "runtime.h"
#include "game/client_entity_e2e.h"
#include "game/client_entity_adapters.h"
#include "game/client_objects_native.h"
#include "game/client_inspect_value.h"
#include "game/client_grid_native.h"
#include "game/client_entity_adapters.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>

namespace cs::game {
// 读真实已提交实例；逐类字段/容器/资源使用正式只读后态函数，不运行Update。
std::string ClientTestInspectNative(const ClientNativeObjects *objects) noexcept {
  if(!IsMain()||!ClientBusy())return "ok=0\nerror=inspect_native_context\n";
  try {
    const char *relations="inspect_relations";if(!ClientNativeObjectsInspectRelations(objects,relations))return "ok=0\nerror="+std::string(relations)+"\n";
    // SDK边界维持C++17，只构造自有JSON文本，不引入C++20的复制协议／参与者类型。
    struct State {rapidjson::Document rows{rapidjson::kArrayType};std::set<void *> unique;std::vector<ClientEntityClassAdapter> adapters=ClientEntityBuiltinAdapters();const char *reason="inspect_native_failed";} state;
    const auto visit=[](void *context,void *pointer,const ClientObjectKey &key,std::uint64_t instance,unsigned type) noexcept ->bool {
      auto &s=*static_cast<State *>(context);
      try {
        const auto *d=ClientNativeObjectDrawDescription(pointer);
        if(!d||!instance||!s.unique.insert(pointer).second)return false;
        auto &e=*static_cast<Entity *>(pointer);
        // 正式Player的dead/exists/非roster资格由其唯一owner核对；不能用旧单Isaac条件拒绝合法身体。
        if(e._type!=int(type)||(type!=1&&!e._valid)||(!d->full_entity&&!e._exists))return false;
        if(type!=1&&d->full_entity&&d->category){
          const ClientEntityClassAdapter *adapter=nullptr;for(const auto &a:s.adapters)if(a.category==d->category)adapter=&a;
          if(!adapter||!ClientEntityCommonFields(pointer,d->common_fields,true,s.reason)||
             !adapter->fields(adapter->context,pointer,*d,true,s.reason)||!adapter->resources(adapter->context,pointer,*d,true,s.reason))return false;
        }
        auto &a=s.rows.GetAllocator();rapidjson::Value row(rapidjson::kObjectType);
        const auto decimal=[&](std::uint64_t value){const auto text=std::to_string(value);return rapidjson::Value(text.c_str(),rapidjson::SizeType(text.size()),a);};
        row.AddMember("run",decimal(key.run),a);row.AddMember("id",decimal(key.id),a);
        row.AddMember("instance",decimal(instance),a);row.AddMember("shape",decimal(d->shape_revision),a);
        row.AddMember("category",d->category,a);row.AddMember("type",type,a);
        row.AddMember("variant",e._variant,a);row.AddMember("exists",e._exists,a);
        row.AddMember("space",key.space,a);
        rapidjson::Value fields(rapidjson::kObjectType);
        for(const auto &f:d->common_fields){
#define CS_ENTITY_FIELD(id,field,kind) if(f.member==id)ClientInspectField(fields,id,e.field,a);
#define CS_ENTITY_RAW_FIELD(id,offset,type,kind) if(f.member==id)ClientInspectField(fields,id,*reinterpret_cast<const type *>(static_cast<const unsigned char *>(pointer)+offset),a);
#define CS_ENTITY_SPECIAL_FIELD(id,kind)
#include "game/client_entity_fields.inc"
#undef CS_ENTITY_FIELD
#undef CS_ENTITY_RAW_FIELD
#undef CS_ENTITY_SPECIAL_FIELD
        }
        if(d->derived)for(const auto &f:d->derived->fields){
#define CS_CLASS_FIELD(cat,id,offset,type,kind) if(d->category==cat&&f.member==id)ClientInspectField(fields,id,*reinterpret_cast<const type *>(static_cast<const unsigned char *>(pointer)+offset),a);
#include "game/client_entity_class_fields.inc"
#undef CS_CLASS_FIELD
        }
        row.AddMember("fields",std::move(fields),a);
        s.rows.PushBack(std::move(row),a);return true;
      }catch(...){return false;}
    };
    if(!ClientNativeObjectsVisit(objects,&state,visit))return "ok=0\nerror="+std::string(state.reason)+"\n";
    rapidjson::StringBuffer text;rapidjson::Writer<rapidjson::StringBuffer> writer(text);state.rows.Accept(writer);
    return "ok=1\nnative_checked=1\nobjects="+std::string(text.GetString(),text.GetSize())+"\n";
  }catch(...){return "ok=0\nerror=inspect_allocation\n";}
}
// Grid存在性和描述从真实实例读取；完整专用字段/资源另经同owner只读后态核对。
std::string ClientTestInspectGrids(const ClientNativeGrids *owner) noexcept {
 try{rapidjson::Document rows(rapidjson::kArrayType);
  const auto visitor=[](void *context,void *pointer,const ClientObjectKey &key,std::uint64_t instance,unsigned) noexcept ->bool {
   try{auto &rows=*static_cast<rapidjson::Document *>(context);auto &a=rows.GetAllocator();const auto &g=*static_cast<GridEntity *>(pointer);rapidjson::Value row(rapidjson::kObjectType);
    ClientInspectField(row,"run",key.run,a);ClientInspectField(row,"id",key.id,a);ClientInspectField(row,"space",key.space,a);ClientInspectField(row,"instance",instance,a);
    ClientInspectField(row,"index",g._gridIndex,a);ClientInspectField(row,"type",g._desc._type,a);ClientInspectField(row,"variant",g._desc._variant,a);ClientInspectField(row,"state",g._desc._state,a);ClientInspectField(row,"var_data",g._desc._varData,a);ClientInspectField(row,"spawn_seed",g._desc._spawnSeed,a);ClientInspectField(row,"collision",g._collisionClass,a);rows.PushBack(std::move(row),a);return true;
   }catch(...){return false;}};
  if(!ClientNativeGridsVisit(owner,&rows,visitor))return "ok=0\nerror=grid_inspect_owner\n";
  rapidjson::StringBuffer text;rapidjson::Writer<rapidjson::StringBuffer> writer(text);rows.Accept(writer);return "ok=1\ngrid_native_checked=1\ngrids="+std::string(text.GetString(),text.GetSize())+"\n";
 }catch(...){return "ok=0\nerror=grid_inspect_allocation\n";}
}
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
