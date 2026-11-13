#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/common/bridge.h"
#include "game/common/context.h"

namespace cs::game {
namespace {
// LuaInit在main_ex返回并绑定官方回调后删除_LuaBindings；本方只在删除前的同一官方
// 窗口保留owner/原闭包，运行期不扫描私有registry编号、不重新加载另一份ESSM模块。
// key对应的值由该VM registry持有，VM关闭自动释放；C++不缓存跨VM的Lua引用编号。
char essm_delete_registry_key;
enum EssmDeleteStatus : int {EssmReady,EssmBindingMissing,EssmBindingChanged,EssmCallbackFailed};
void RawField(lua_State *L,int table,const char *name) {
  table=lua_absindex(L,table);lua_pushstring(L,name);lua_rawget(L,table);
}
int SealEssmDeleteThunk(lua_State *L) {
  lua_getglobal(L,"_LuaBindings");
  if(!lua_istable(L,-1)){lua_pushboolean(L,0);return 1;}
  RawField(L,-1,"EntityManager");
  if(!lua_istable(L,-1)){lua_pushboolean(L,0);return 1;}
  const int owner=lua_absindex(L,-1);RawField(L,owner,"DeleteEntity");
  if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)){lua_pushboolean(L,0);return 1;}
  const int callback=lua_absindex(L,-1);
  RawField(L,owner,"NewEntity");if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)){lua_pushboolean(L,0);return 1;}
  const int new_callback=lua_absindex(L,-1);
  lua_rawgetp(L,LUA_REGISTRYINDEX,&essm_delete_registry_key);
  if(lua_istable(L,-1)){
    // 同VM重复调用只核对；已有未知值或不同owner/闭包绝不通过覆盖恢复资格。
    const int snapshot=lua_absindex(L,-1);RawField(L,snapshot,"owner");
    const bool same_owner=lua_rawequal(L,owner,-1);lua_pop(L,1);
    RawField(L,snapshot,"callback");const bool same_delete=lua_rawequal(L,callback,-1);lua_pop(L,1);
    RawField(L,snapshot,"new_callback");
    lua_pushboolean(L,same_owner&&same_delete&&lua_rawequal(L,new_callback,-1));return 1;
  }
  if(!lua_isnil(L,-1)){lua_pushboolean(L,0);return 1;}
  lua_pop(L,1);lua_createtable(L,0,3);const int snapshot=lua_absindex(L,-1);
  lua_pushliteral(L,"owner");lua_pushvalue(L,owner);lua_rawset(L,snapshot);
  lua_pushliteral(L,"callback");lua_pushvalue(L,callback);lua_rawset(L,snapshot);
  lua_pushliteral(L,"new_callback");lua_pushvalue(L,new_callback);lua_rawset(L,snapshot);
  lua_pushvalue(L,snapshot);lua_rawsetp(L,LUA_REGISTRYINDEX,&essm_delete_registry_key);
  lua_pushboolean(L,1);return 1;
}
int UseEssmDeleteThunk(lua_State *L) {
  const bool execute=lua_toboolean(L,2)!=0;
  const bool creating=lua_toboolean(L,3)!=0;
  lua_rawgetp(L,LUA_REGISTRYINDEX,&essm_delete_registry_key);
  if(!lua_istable(L,-1)){lua_pushinteger(L,EssmBindingMissing);return 1;}
  const int snapshot=lua_absindex(L,-1);RawField(L,snapshot,"owner");
  if(!lua_istable(L,-1)){lua_pushinteger(L,EssmBindingChanged);return 1;}
  RawField(L,-1,creating?"NewEntity":"DeleteEntity");RawField(L,snapshot,creating?"new_callback":"callback");
  if(!lua_isfunction(L,-1)||lua_iscfunction(L,-1)||!lua_rawequal(L,-1,-2)){
    lua_pushinteger(L,EssmBindingChanged);return 1;
  }
  if(execute){
    lua_pushinteger(L,lua_tointeger(L,1));
    if(lua_pcall(L,1,0,0)!=LUA_OK){lua_pushinteger(L,EssmCallbackFailed);return 1;}
  }
  lua_pushinteger(L,EssmReady);return 1;
}
// 官方闭包只处理ESSM，不执行第三方ModCallback。预检和实际执行分别受pcall保护，
// 错误区分VM/预算/绑定/闭包调用；不以userdata析构或强制GC代替一次真实Delete通知。
bool EssmDelete(Entity *entity,bool execute,const char *&reason,bool creating=false) noexcept {
  if(!IsMain()||!g_LuaEngine||!g_LuaEngine->_state||ClientLuaStateIdentity()!=g_LuaEngine->_state){
    reason="objects_essm_delete_vm_unavailable";return false;
  }
  auto *L=g_LuaEngine->_state;const int top=lua_gettop(L);
  if(!lua_checkstack(L,12)){reason="objects_essm_delete_stack_budget";return false;}
  lua_pushcfunction(L,UseEssmDeleteThunk);
  lua_pushinteger(L,g_LuaEngine->GetMaskedPointer(reinterpret_cast<uintptr_t>(entity)));
  lua_pushboolean(L,execute);
  lua_pushboolean(L,creating);
  const bool called=lua_pcall(L,3,1,0)==LUA_OK;
  const int status=called&&lua_isinteger(L,-1)?int(lua_tointeger(L,-1)):-1;
  lua_settop(L,top);
  switch(status){
    case EssmReady:return true;
    case EssmBindingMissing:reason="objects_essm_delete_binding_missing";return false;
    case EssmBindingChanged:reason="objects_essm_delete_binding_changed";return false;
    case EssmCallbackFailed:reason="objects_essm_delete_callback_failed";return false;
    default:reason="objects_essm_delete_lua_error";return false;
  }
}
}
// 仅由官方main_ex成功返回、LuaInit尚未删除_LuaBindings的接线调用。
bool ClientObjectStagedPoolNotification(void *entity,bool execute,bool creating,const char *&reason) noexcept {
  return entity&&EssmDelete(static_cast<Entity *>(entity),execute,reason,creating);
}
// 所有分配/字段读取都在pcall内，正常/错误均恢复原栈；失败由调用者撤销Client准备资格。
bool ClientObjectSealEssmDeleteBinding(void *state) noexcept {
  if(!IsMain()||ClientBusy()||!state||!g_LuaEngine||state!=g_LuaEngine->_state||ClientLuaStateIdentity()!=state)return false;
  auto *L=static_cast<lua_State *>(state);const int top=lua_gettop(L);
  if(!lua_checkstack(L,12))return false;
  lua_pushcfunction(L,SealEssmDeleteThunk);
  const bool sealed=lua_pcall(L,0,1,0)==LUA_OK&&lua_toboolean(L,-1)!=0;
  lua_settop(L,top);return sealed;
}
} // namespace cs::game
