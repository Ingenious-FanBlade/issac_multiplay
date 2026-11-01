#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/bridge.h"
#include "game/client_lifetime.h"
#include "runtime.h"
#include <cstddef>
#include <cstring>
#include <cctype>
#include <initializer_list>
#include <new>
#include <windows.h>

namespace cs::game {
bool ReadMemory(const void *source,void *output,std::size_t size) noexcept;
namespace {
lua_State *guard_ready_state=nullptr;
lua_State *guard_base_state=nullptr;
lua_State *guard_essm_state=nullptr;
lua_State *guard_sealed_state=nullptr;
// 此表只由所属 VM 的 registry 持有；记录早期别名已接到保护之后的函数身份。
// C++ 不保存 Lua 引用编号，VM 销毁时由 Lua 一并释放。
char persistence_registry_key;
lua_State *callback_ready_state=nullptr;
thread_local bool menu_callback_running=false;
// VM 私有强引用只保存在本地 C++ owner，VM 换代后绝不跨 state 使用。
struct LuaRegistryOwner final {lua_State *state=nullptr;int reference=LUA_NOREF;};
// 捕获 thunk 在 pcall 内写入引用编号，异常时由外层已登记 owner 承担退休。
int RegistryCaptureThunk(lua_State *L) {
  auto *owner=static_cast<LuaRegistryOwner *>(lua_touserdata(L,1));
  lua_pushvalue(L,2);
  owner->reference=luaL_ref(L,LUA_REGISTRYINDEX);return 0;
}
int RegistryReleaseThunk(lua_State *L) {
  luaL_unref(L,LUA_REGISTRYINDEX,int(lua_tointeger(L,1)));return 0;
}
// 访问帧仅在一次受保护调用的 C++ 栈上有效，不向外发布 Lua 对象地址。
struct LuaVisitFrame final {
  LuaRegistryOwner *owner=nullptr;
  bool (*visitor)(void *,void *) noexcept=nullptr;
  void *context=nullptr;
};
// 原引用值压至 Lua 栈顶交给 visitor；返回后 pcall 收口并恢复调用前栈高。
int RegistryVisitThunk(lua_State *L) {
  auto *frame=static_cast<LuaVisitFrame *>(lua_touserdata(L,1));
  lua_rawgeti(L,LUA_REGISTRYINDEX,frame->owner->reference);
  const bool result=frame->visitor(L,frame->context);
  lua_pushboolean(L,result);return 1;
}
bool ReleaseLuaRegistryImpl(void *pointer) noexcept {
  auto *owner=static_cast<LuaRegistryOwner *>(pointer);
  if(!owner||!IsMain()||ClientLuaStateIdentity()!=owner->state)return false;
  if(owner->reference==LUA_NOREF){delete owner;return true;}
  auto *L=owner->state;
  const int top=lua_gettop(L);
  if(!lua_checkstack(L,2))return false;
  lua_pushcfunction(L,RegistryReleaseThunk);
  lua_pushinteger(L,owner->reference);
  const bool complete=lua_pcall(L,1,0,0)==LUA_OK;
  lua_settop(L,top);
  if(complete)delete owner;
  return complete;
}
template <class T> bool Field(const void *base,std::size_t offset,T &value) noexcept {
  return base&&ReadMemory(static_cast<const unsigned char *>(base)+offset,&value,sizeof(value));
}
}
bool ClientLuaRegistryReleaseRaw(void *owner) noexcept {return ReleaseLuaRegistryImpl(owner);}
// J273 GLFW window 的 HDC/HGLRC 在 +0x234/+0x238；SDK 只公开 HWND。
// 必须同时确认 renderer window 的进程归属，不能学习 ImGui 多视口 current。
bool ClientRendererHandles(void *&dc,void *&context) noexcept {
  dc=context=nullptr;
  if(!IsMain()||!__ptr_g_KAGE_Graphics_Manager)return false;
  auto *window=__ptr_g_KAGE_Graphics_Manager->_window;
  void *hwnd=nullptr;
  if(!Field(window,0x218,hwnd)||!hwnd)return false;
  DWORD pid=0;
  if(!GetWindowThreadProcessId(static_cast<HWND>(hwnd),&pid)||pid!=GetCurrentProcessId())return false;
  if(!Field(window,0x234,dc)||!Field(window,0x238,context)||!dc||!context)return false;
  return WindowFromDC(static_cast<HDC>(dc))==static_cast<HWND>(hwnd);
}
// 只观察当前 VM 的身份；借用者必须再检查对应 Lua 代次和 owner 阶段。
void *ClientLuaStateIdentity() noexcept {
  return IsMain()&&g_LuaEngine?g_LuaEngine->_state:nullptr;
}
namespace {
// Lua 闭包的原函数作为 upvalue 保存在所属 VM；state 销毁后不在 C++ 侧保留 registry 引用。
// pcall 收住 Lua longjmp，保证本 C++ 桥没有未运行的析构与跨 VM 悬空票据。
int GuardedLuaPersistence(lua_State *L) {
  if(ClientPersistenceBlocked()) {
    if(!IsMain()||ClientLuaStateIdentity()!=L)
      ClientCallbackFault("client_persistence_foreign_or_vm");
    ObserveClientPersistenceDenied(lua_tostring(L,lua_upvalueindex(3)),true);
    const int result=int(lua_tointeger(L,lua_upvalueindex(2)));
    if(result==1){lua_pushboolean(L,0);return 1;}
    if(result==2){lua_pushnil(L);return 1;}
    if(result==3){lua_pushliteral(L,"");return 1;}
    return 0;
  }
  const int arguments=lua_gettop(L);
  lua_pushvalue(L,lua_upvalueindex(1));
  lua_insert(L,1);
  if(lua_pcall(L,arguments,LUA_MULTRET,0)!=LUA_OK)return lua_error(L);
  return lua_gettop(L);
}
bool GuardField(lua_State *L,int table,const char *field,int blocked_result) {
  table=lua_absindex(L,table);
  lua_getfield(L,table,field);
  if(!lua_isfunction(L,-1)||
     (lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==GuardedLuaPersistence)){
    const bool guarded=lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==GuardedLuaPersistence;
    lua_pop(L,1);return guarded;
  }
  lua_pushinteger(L,blocked_result);
  lua_pushstring(L,field);
  lua_pushcclosure(L,GuardedLuaPersistence,3);
  lua_setfield(L,table,field);
  return true;
}
// 只包含已解释的官方持久入口；本地 Options/输入映射不借 ModData 白名单放行。
struct PersistenceLuaField final {const char *name;int blocked_result;};
constexpr PersistenceLuaField isaac_persistence_fields[]={
  {"SaveModData",0},{"RemoveModData",0},{"MarkChallengeAsNotDone",0},
  {"UnClearChallenge",0},{"ClearChallenge",0},{"SetChallengeCompletion",0},
  {"SetCompletionMarks",0},{"SetCompletionMark",0},{"FillCompletionMarks",0},
  {"ClearCompletionMarks",0},{"HasModData",1},{"LoadModData",3},
  {"LoadModDataFromFolder",2}
};
constexpr PersistenceLuaField pgd_persistence_fields[]={
  // Unlock/TryUnlock 的官方 Lua 函数在调用原生入口前会写 forceunlock/nextSkipAchiev。
  // 必须在这层之前拒绝，不能只依赖原生 TryUnlock 返回 false。
  {"Unlock",1},{"TryUnlock",1},{"IncreaseEventCounter",0},
  {"AddBestiaryKill",1},{"AddBossKilled",0}
};
constexpr PersistenceLuaField essm_persistence_fields[]={
  {"SaveData",0},{"DeleteData",0},{"LoadData",2}
};
template <std::size_t N>
bool GuardFields(lua_State *L,int table,const PersistenceLuaField (&fields)[N]) {
  bool complete=true;
  for(const auto &field:fields)complete=GuardField(L,table,field.name,field.blocked_result)&&complete;
  return complete;
}
// 可能触发 Lua 元方法或内存错误的安装过程放在受保护的 C thunk 内。
int InstallGuardThunk(lua_State *L) {
  bool isaac_ready=true,essm_ready=false,essm_seen=false;
  lua_getglobal(L,"Isaac");
  if(lua_istable(L,-1)) {
    isaac_ready=GuardFields(L,-1,isaac_persistence_fields);
  }else isaac_ready=false;
  lua_pop(L,1);
  luaL_getmetatable(L,lua::metatables::PersistentGameDataMT);
  isaac_ready=(lua_istable(L,-1)&&GuardFields(L,-1,pgd_persistence_fields))&&isaac_ready;
  lua_pop(L,1);
  lua_getglobal(L,"_CBindings");
  if(lua_istable(L,-1)){
    lua_getfield(L,-1,"ESSM");
    if(lua_istable(L,-1)){
      essm_seen=true;essm_ready=true;
      essm_ready=GuardFields(L,-1,essm_persistence_fields);
    }
    lua_pop(L,1);
  }
  lua_pop(L,1);
  lua_pushboolean(L,isaac_ready);
  lua_pushinteger(L,!essm_seen?0:essm_ready?1:2);return 2;
}
// 身份表保留真正的 owner 和每个已保护函数，不用函数指针/名称近似 Lua 闭包身份。
template <std::size_t N>
bool SnapshotGuardFields(lua_State *L,int root,int index,int owner,
                         const PersistenceLuaField (&fields)[N]) {
  root=lua_absindex(L,root);owner=lua_absindex(L,owner);
  if(!lua_istable(L,owner))return false;
  lua_createtable(L,0,int(N)+1);
  const int snapshot=lua_absindex(L,-1);
  lua_pushvalue(L,owner);lua_setfield(L,snapshot,"owner");
  for(const auto &field:fields){
    lua_getfield(L,owner,field.name);
    if(!lua_iscfunction(L,-1)||lua_tocfunction(L,-1)!=GuardedLuaPersistence){lua_pop(L,2);return false;}
    lua_setfield(L,snapshot,field.name);
  }
  lua_rawseti(L,root,index);return true;
}
// 仅 main_ex 正常返回的官方初始化窗口可建立身份表；连接时不接受晚补保护。
int SealPersistenceThunk(lua_State *L) {
  lua_createtable(L,3,0);const int root=lua_absindex(L,-1);
  lua_getglobal(L,"Isaac");
  bool complete=SnapshotGuardFields(L,root,1,-1,isaac_persistence_fields);lua_pop(L,1);
  luaL_getmetatable(L,lua::metatables::PersistentGameDataMT);
  complete=SnapshotGuardFields(L,root,2,-1,pgd_persistence_fields)&&complete;lua_pop(L,1);
  lua_getglobal(L,"_CBindings");
  if(lua_istable(L,-1))lua_getfield(L,-1,"ESSM");else lua_pushnil(L);
  complete=SnapshotGuardFields(L,root,3,-1,essm_persistence_fields)&&complete;lua_pop(L,2);
  if(complete){lua_pushvalue(L,root);lua_rawsetp(L,LUA_REGISTRYINDEX,&persistence_registry_key);}
  lua_pushboolean(L,complete);return 1;
}
template <std::size_t N>
bool CheckGuardFields(lua_State *L,int root,int index,int current_owner,
                      const PersistenceLuaField (&fields)[N]) {
  root=lua_absindex(L,root);
  if(current_owner)current_owner=lua_absindex(L,current_owner);
  lua_rawgeti(L,root,index);
  if(!lua_istable(L,-1)){lua_pop(L,1);return false;}
  const int snapshot=lua_absindex(L,-1);
  lua_getfield(L,snapshot,"owner");const int owner=lua_absindex(L,-1);
  bool complete=lua_istable(L,owner)&&(!current_owner||lua_rawequal(L,owner,current_owner));
  if(complete)for(const auto &field:fields){
    lua_getfield(L,owner,field.name);lua_getfield(L,snapshot,field.name);
    complete=lua_isfunction(L,-1)&&lua_rawequal(L,-1,-2)&&complete;lua_pop(L,2);
  }
  lua_pop(L,2);return complete;
}
// _CBindings 在 Init 尾部被官方删除；只验证保留的 ESSM owner，不重新导出内部接口。
int VerifyPersistenceThunk(lua_State *L) {
  lua_rawgetp(L,LUA_REGISTRYINDEX,&persistence_registry_key);
  if(!lua_istable(L,-1)){lua_pushboolean(L,0);return 1;}
  const int root=lua_absindex(L,-1);
  lua_getglobal(L,"Isaac");
  bool complete=CheckGuardFields(L,root,1,-1,isaac_persistence_fields);lua_pop(L,1);
  luaL_getmetatable(L,lua::metatables::PersistentGameDataMT);
  complete=CheckGuardFields(L,root,2,-1,pgd_persistence_fields)&&complete;lua_pop(L,1);
  complete=CheckGuardFields(L,root,3,0,essm_persistence_fields)&&complete;
  lua_pushboolean(L,complete);return 1;
}
// 只选本方 mod 注册的唯一菜单候选；其函数身份仍由当次官方回调表核对。
bool OwnMenuCallback(lua_State *L,int mod_index) {
  mod_index=lua_absindex(L,mod_index);
  if(!lua_istable(L,mod_index))return false;
  if(!g_LuaEngine||L!=g_LuaEngine->_state){
    ClientCallbackFault("client_callback_vm_mismatch");return false;
  }
  auto *reference=g_LuaEngine->GetModRefByTable(mod_index);
  if(!reference)return false;
  const char *path=reference->_mainLuaPath.c_str();
  static constexpr char suffix[]="isaac-cs-ui/main.lua";
  const auto length=std::strlen(path),suffix_length=sizeof(suffix)-1;
  if(length<suffix_length)return false;
  for(std::size_t i=0;i<suffix_length;++i){
    const auto a=path[length-suffix_length+i],b=suffix[i];
    if(a=='\\'&&b=='/')continue;
    if(std::tolower(static_cast<unsigned char>(a))!=
       std::tolower(static_cast<unsigned char>(b)))return false;
  }
  if(length>suffix_length&&path[length-suffix_length-1]!='/'&&
     path[length-suffix_length-1]!='\\')return false;
  lua_pushliteral(L,"Name");lua_rawget(L,mod_index);
  const bool named=lua_isstring(L,-1)&&std::strcmp(lua_tostring(L,-1),"Isaac CS UI")==0;
  lua_pop(L,1);return named;
}
// 回调列表由 main_ex.lua 官方闭包返回；先找唯一函数，再调用，避免多候选时执行部分回调。
int RunOwnMenuCallback(lua_State *L) {
  lua_pushvalue(L,1);
  lua_pushinteger(L,1023);lua_pushboolean(L,0);
  if(lua_pcall(L,2,1,0)!=LUA_OK||!lua_istable(L,-1)){
    ClientCallbackFault("client_ui_callback_list");return 0;
  }
  const int list=lua_absindex(L,-1);
  const auto count=lua_rawlen(L,list);
  if(count>256){ClientCallbackFault("client_ui_callback_budget");return 0;}
  int selected=0;
  for(std::size_t i=1;i<=count;++i){
    lua_rawgeti(L,list,int(i));
    if(lua_istable(L,-1)){
      const int item=lua_absindex(L,-1);
      lua_pushliteral(L,"Mod");lua_rawget(L,item);
      const bool own=OwnMenuCallback(L,-1);
      lua_pop(L,1);
      if(own){
        lua_pushliteral(L,"Function");lua_rawget(L,item);
        const bool callable=lua_isfunction(L,-1);lua_pop(L,1);
        if(!callable||selected){ClientCallbackFault("client_ui_callback_ambiguous");return 0;}
        selected=int(i);
      }
    }
    lua_pop(L,1);
  }
  if(!selected){ClientCallbackFault("client_ui_callback_missing");return 0;}
  lua_rawgeti(L,list,selected);
  const int item=lua_absindex(L,-1);
  lua_pushliteral(L,"Function");lua_rawget(L,item);
  lua_pushliteral(L,"Mod");lua_rawget(L,item);
  if(lua_pcall(L,1,0,0)!=LUA_OK){ClientCallbackFault("client_ui_callback_error");return 0;}
  ClientCallbackOwnExecuted();
  return 0;
}
// 官方 Lua 内部结构调用保持原链；Client ModCallback 仅菜单提示有精确白名单。
int GuardedClientCallback(lua_State *L) {
  if(ClientBusy()){
    if(!IsMain()||!g_LuaEngine||L!=g_LuaEngine->_state){
      ClientCallbackFault("client_callback_foreign_or_vm");return 0;
    }
    if(lua_type(L,1)==LUA_TNUMBER&&lua_tointeger(L,1)==1023&&ClientCallbackMenuAllowed()){
      if(menu_callback_running){
        ClientCallbackFault("client_ui_callback_phase");return 0;
      }
      // 所有栈增长及可能 longjmp 的名单遍历在受保护的零捕获 C thunk 内。
      if(!lua_checkstack(L,2)){ClientCallbackFault("client_ui_callback_stack");return 0;}
      menu_callback_running=true;
      lua_pushcfunction(L,RunOwnMenuCallback);
      lua_pushvalue(L,lua_upvalueindex(2));
      if(lua_pcall(L,1,0,0)!=LUA_OK)ClientCallbackFault("client_ui_callback_lua_error");
      menu_callback_running=false;
      return 0;
    }
    // 世界绘制中的同编号回调也属于一般 Mod 逻辑；只有安全菜单窗口允许本方唯一函数。
    ClientCallbackSkipped();
    return 0;
  }
  const int arguments=lua_gettop(L);
  lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,1);
  if(lua_pcall(L,arguments,LUA_MULTRET,0)!=LUA_OK)return lua_error(L);
  return lua_gettop(L);
}
bool GuardCallbackField(lua_State *L,int table,const char *field,int getter) {
  table=lua_absindex(L,table);getter=lua_absindex(L,getter);
  lua_getfield(L,table,field);
  if(!lua_isfunction(L,-1)) {lua_pop(L,1);return false;}
  if(lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==GuardedClientCallback){lua_pop(L,1);return true;}
  lua_pushvalue(L,getter);
  lua_pushcclosure(L,GuardedClientCallback,2);
  lua_setfield(L,table,field);return true;
}
bool GuardCallbackGlobal(lua_State *L,const char *field,int getter) {
  getter=lua_absindex(L,getter);
  lua_getglobal(L,field);
  if(!lua_isfunction(L,-1)){lua_pop(L,1);return false;}
  if(lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==GuardedClientCallback){lua_pop(L,1);return true;}
  lua_pushvalue(L,getter);
  lua_pushcclosure(L,GuardedClientCallback,2);
  lua_setglobal(L,field);return true;
}
// main_ex.lua 已建立全部公开别名，但 LuaInit 尚未把全局函数存入 native registry。
int InstallCallbackGuardThunk(lua_State *L) {
  lua_getglobal(L,"Isaac");
  if(!lua_istable(L,-1)){lua_pushboolean(L,0);return 1;}
  const int isaac=lua_absindex(L,-1);
  lua_getfield(L,isaac,"GetCallbacks");
  if(!lua_isfunction(L,-1)){lua_pushboolean(L,0);return 1;}
  const int getter=lua_absindex(L,-1);
  bool ready=true;
  for(const char *name:{"RunCallback","RunCallbackWithParam",
                         "RunCallbackWithTwoParams","RunCallbackOverIterator"})
    ready=GuardCallbackField(L,isaac,name,getter)&&ready;
  ready=GuardCallbackGlobal(L,"_RunCallback",getter)&&ready;
  ready=GuardCallbackGlobal(L,"_RunCallbackWithTwoParams",getter)&&ready;
  lua_pushboolean(L,ready);return 1;
}
}
// RegisterClasses 外层返回后、所有 bundled script 执行前再次接线；onlinestub
// 复制 UnsafeFuncs 旧闭包时取得的已是保护闭包，无法靠后续替换 global 绕过。
void ClientGuardLuaBindings(bool classes_registered) noexcept {
  if(!IsMain()||!g_LuaEngine||!g_LuaEngine->_state)return;
  auto *L=g_LuaEngine->_state;
  guard_ready_state=nullptr;
  const int top=lua_gettop(L);
  if(!lua_checkstack(L,2))return;
  if(guard_sealed_state==L){
    // 准备和后续连接只能核对；已被替换的入口不能靠再次包装恢复早期别名资格。
    lua_pushcfunction(L,VerifyPersistenceThunk);
    if(lua_pcall(L,0,1,0)==LUA_OK&&lua_toboolean(L,-1))guard_ready_state=L;
    lua_settop(L,top);
    if(!guard_ready_state&&ClientBusy())ClientCallbackFault("client_persistence_binding_changed");
    return;
  }
  lua_pushcfunction(L,InstallGuardThunk);
  if(lua_pcall(L,0,2,0)==LUA_OK){
    const int essm=int(lua_tointeger(L,-1));
    const bool base_ready=lua_toboolean(L,-2)!=0;
    if(classes_registered)guard_base_state=base_ready?L:nullptr;
    if(essm==1)guard_essm_state=L;
    if(essm==2)guard_essm_state=nullptr;
    if(base_ready&&guard_base_state==L&&guard_essm_state==L)guard_ready_state=L;
  }
  lua_settop(L,top);
}
bool ClientLuaGuardReady() noexcept {
  return IsMain()&&guard_ready_state&&guard_ready_state==guard_sealed_state&&
         guard_ready_state==ClientLuaStateIdentity();
}
void ClientLuaGuardClosed() noexcept {
  if(IsMain()){guard_ready_state=nullptr;guard_base_state=nullptr;guard_essm_state=nullptr;guard_sealed_state=nullptr;callback_ready_state=nullptr;}
}
void ClientGuardCallbackBindings(const char *path) noexcept {
  if(!IsMain()||!path||std::strcmp(path,"resources/scripts/main_ex.lua")!=0||
     !g_LuaEngine||!g_LuaEngine->_state)return;
  auto *L=g_LuaEngine->_state;
  callback_ready_state=nullptr;
  // 官方bind_lua_internals稍后会清空_LuaBindings；原生remove必须持有其原闭包。
  if(!ClientObjectSealEssmDeleteBinding(L)){guard_ready_state=nullptr;return;}
  const int top=lua_gettop(L);
  if(!lua_checkstack(L,2))return;
  // main_ex 执行前 ESSM 已捕获保护后的 Save/Load/Delete；onlinestub 的 UnsafeFuncs
  // 同样已保存保护闭包。此处再保护官方公开包装，并记录最终身份供后续只读核对。
  if(guard_ready_state!=L||guard_essm_state!=L)return;
  ClientGuardLuaBindings();
  if(guard_ready_state!=L)return;
  lua_pushcfunction(L,SealPersistenceThunk);
  const bool sealed=lua_pcall(L,0,1,0)==LUA_OK&&lua_toboolean(L,-1);
  lua_settop(L,top);
  if(!sealed){guard_ready_state=nullptr;return;}
  guard_sealed_state=L;
  lua_pushcfunction(L,InstallCallbackGuardThunk);
  if(lua_pcall(L,0,1,0)==LUA_OK&&lua_toboolean(L,-1))callback_ready_state=L;
  lua_settop(L,top);
}
bool ClientCallbackGuardReady() noexcept {
  return IsMain()&&callback_ready_state&&callback_ready_state==ClientLuaStateIdentity();
}
ClientCallbackScriptScope::ClientCallbackScriptScope(bool main,const char *path) noexcept {
  if(!main||!IsMain()){
    if(ClientBusy()){
      allowed_=false;ClientCallbackFault("client_callback_script_foreign");
    }
    return;
  }
  tracked_=path&&std::strcmp(path,"resources/scripts/main_ex.lua")==0;
  if(tracked_&&ClientBusy()){
    allowed_=false;ClientCallbackFault("client_callback_script_reload");return;
  }
  if(tracked_)callback_ready_state=nullptr;
}
void ClientCallbackScriptScope::Returned(bool success) noexcept {
  if(!tracked_||!allowed_)return;
  returned_=true;
  if(success)ClientGuardCallbackBindings("resources/scripts/main_ex.lua");
}
ClientCallbackScriptScope::~ClientCallbackScriptScope() noexcept {
  if(tracked_&&allowed_&&(!returned_||!ClientCallbackGuardReady()))
    ClientCallbackFault("client_callback_guard_reload_failed");
}
// 预留 owner 时不执行 Lua API；即使捕获失败，主线程表仍负有一次明确退休责任。
void *ClientLuaRegistryMakeOwner() noexcept {
  if(!ClientBusy()||!ClientLuaGuardReady())return nullptr;
  auto *L=static_cast<lua_State *>(ClientLuaStateIdentity());
  return L?static_cast<void *>(new(std::nothrow) LuaRegistryOwner{L,LUA_NOREF}):nullptr;
}
void ClientLuaRegistryDiscardOwner(void *pointer) noexcept {
  auto *owner=static_cast<LuaRegistryOwner *>(pointer);
  if(owner&&owner->reference==LUA_NOREF)delete owner;
}
bool ClientLuaRegistryCaptureRaw(void *pointer,int stack_index) noexcept {
  auto *owner=static_cast<LuaRegistryOwner *>(pointer);
  if(!owner||!IsMain()||ClientLuaStateIdentity()!=owner->state||owner->reference!=LUA_NOREF)return false;
  auto *L=owner->state;
  const int top=lua_gettop(L);
  stack_index=lua_absindex(L,stack_index);
  if(stack_index<1||stack_index>top||!lua_checkstack(L,3))return false;
  lua_pushcfunction(L,RegistryCaptureThunk);
  lua_pushlightuserdata(L,owner);
  lua_pushvalue(L,stack_index);
  const bool complete=lua_pcall(L,2,0,0)==LUA_OK&&owner->reference>=0;
  lua_settop(L,top);
  return complete;
}
bool ClientLuaRegistryVisitRaw(void *pointer,
                                bool (*visitor)(void *,void *) noexcept,void *context) noexcept {
  auto *owner=static_cast<LuaRegistryOwner *>(pointer);
  if(!owner||!visitor||!IsMain()||ClientLuaStateIdentity()!=owner->state||owner->reference<0)return false;
  auto *L=owner->state;
  const int top=lua_gettop(L);
  if(!lua_checkstack(L,2))return false;
  LuaVisitFrame frame{owner,visitor,context};
  lua_pushcfunction(L,RegistryVisitThunk);
  lua_pushlightuserdata(L,&frame);
  const bool complete=lua_pcall(L,1,1,0)==LUA_OK&&lua_toboolean(L,-1);
  lua_settop(L,top);
  return complete;
}
} // namespace cs::game
