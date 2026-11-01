#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/bridge.h"
#include "game/client_lifetime.h"
#include "runtime.h"
#include <cstddef>
#include <initializer_list>
#include <new>
#include <windows.h>

namespace cs::game {
bool ReadMemory(const void *source,void *output,std::size_t size) noexcept;
namespace {
lua_State *guard_ready_state=nullptr;
lua_State *guard_essm_state=nullptr;
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
  lua_pushcclosure(L,GuardedLuaPersistence,2);
  lua_setfield(L,table,field);
  return true;
}
// 可能触发 Lua 元方法或内存错误的安装过程放在受保护的 C thunk 内。
int InstallGuardThunk(lua_State *L) {
  bool isaac_ready=true,essm_ready=false,essm_seen=false;
  lua_getglobal(L,"Isaac");
  if(lua_istable(L,-1)) {
    for(const char *name:{"SaveModData","RemoveModData","MarkChallengeAsNotDone",
                          "UnClearChallenge","ClearChallenge","SetChallengeCompletion",
                          "SetCompletionMarks","SetCompletionMark","FillCompletionMarks",
                          "ClearCompletionMarks"})isaac_ready=GuardField(L,-1,name,0)&&isaac_ready;
    isaac_ready=GuardField(L,-1,"HasModData",1)&&isaac_ready;
    isaac_ready=GuardField(L,-1,"LoadModData",3)&&isaac_ready;
    isaac_ready=GuardField(L,-1,"LoadModDataFromFolder",2)&&isaac_ready;
  }else isaac_ready=false;
  lua_pop(L,1);
  lua_getglobal(L,"_CBindings");
  if(lua_istable(L,-1)){
    lua_getfield(L,-1,"ESSM");
    if(lua_istable(L,-1)){
      essm_seen=true;essm_ready=true;
      essm_ready=GuardField(L,-1,"SaveData",0)&&essm_ready;
      essm_ready=GuardField(L,-1,"DeleteData",0)&&essm_ready;
      essm_ready=GuardField(L,-1,"LoadData",2)&&essm_ready;
    }
    lua_pop(L,1);
  }
  lua_pop(L,1);
  lua_pushboolean(L,isaac_ready);
  lua_pushinteger(L,!essm_seen?0:essm_ready?1:2);return 2;
}
}
// RegisterClasses 外层返回后、所有 bundled script 执行前再次接线；onlinestub
// 复制 UnsafeFuncs 旧闭包时取得的已是保护闭包，无法靠后续替换 global 绕过。
void ClientGuardLuaBindings() noexcept {
  if(!IsMain()||!g_LuaEngine||!g_LuaEngine->_state)return;
  auto *L=g_LuaEngine->_state;
  guard_ready_state=nullptr;
  const int top=lua_gettop(L);
  if(!lua_checkstack(L,2))return;
  lua_pushcfunction(L,InstallGuardThunk);
  if(lua_pcall(L,0,2,0)==LUA_OK){
    const int essm=int(lua_tointeger(L,-1));
    if(essm==1)guard_essm_state=L;
    if(essm==2)guard_essm_state=nullptr;
    if(lua_toboolean(L,-2)&&guard_essm_state==L)guard_ready_state=L;
  }
  lua_settop(L,top);
}
bool ClientLuaGuardReady() noexcept {return IsMain()&&guard_ready_state&&guard_ready_state==ClientLuaStateIdentity();}
void ClientLuaGuardClosed() noexcept {if(IsMain()){guard_ready_state=nullptr;guard_essm_state=nullptr;}}
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
