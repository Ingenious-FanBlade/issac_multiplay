#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/mod_persistence.h"
#include "game/client_isolation.h"
#include "runtime.h"
#include "mologie_detours.h"
#include <array>
#include <sstream>
namespace cs::game {
namespace {
// Lua CFunction的公开cdecl ABI；在RunBundledScript入口、内部表被隐藏前获取实际函数。
struct Spec final {unsigned index;const char *path;bool optional;};
const Spec specs[]={
#include "persistence_runtime.inc"
};
struct Slot final {void *target=nullptr,*original=nullptr,*resident=nullptr;bool attempted=false,absent=false;std::atomic<std::uint64_t> blocked{0};};
std::array<Slot,4> slots{};
template<unsigned I> int __cdecl Persist(lua_State *L){
  // original可经Lua错误longjmp；此层不持有RAII对象，也不读取外部Lua/游戏对象。
  if(ClientProgressProtected()){
    slots[I].blocked.fetch_add(1,std::memory_order_relaxed);
    Trace("client_isolation","entry=mod_persist source=%u original=0",I);return 0;
  }
  return reinterpret_cast<lua_CFunction>(slots[I].original)(L);
}
lua_CFunction Resolve(lua_State *L,const char *path){
  const int top=lua_gettop(L);const std::string name=path;std::size_t begin=0,end=name.find('.');
  lua_getglobal(L,name.substr(0,end).c_str());
  while(end!=std::string::npos){begin=end+1;end=name.find('.',begin);if(!lua_istable(L,-1)){lua_settop(L,top);return nullptr;}
    lua_pushlstring(L,name.data()+begin,(end==std::string::npos?name.size():end)-begin);lua_rawget(L,-2);lua_remove(L,-2);
  }
  const auto fn=lua_tocfunction(L,-1);lua_settop(L,top);return fn;
}
}
bool ModPersistenceReady() noexcept {
  for(const auto &spec:specs){const auto &s=slots[spec.index];if(!s.resident&&!(spec.optional&&s.absent&&!s.attempted))return false;}return true;
}
void PumpModPersistenceHooks() noexcept {
  if(!IsMain()||!g_LuaEngine||!g_LuaEngine->_state)return;
  void *wrappers[]={reinterpret_cast<void *>(&Persist<0>),reinterpret_cast<void *>(&Persist<1>),reinterpret_cast<void *>(&Persist<2>),reinterpret_cast<void *>(&Persist<3>)};
  for(const auto &spec:specs){auto &s=slots[spec.index];if(s.resident||s.attempted)continue;
    try{
      auto *target=reinterpret_cast<void *>(Resolve(g_LuaEngine->_state,spec.path));
      if(!target){s.absent=true;continue;}s.absent=false;s.attempted=true;
      bool duplicate=false;for(const auto &other:slots)if(other.target==target)duplicate=true;
      if(duplicate){Trace("mod_persistence_hook","path=%s installed=0 reason=shared_cfunction",spec.path);continue;}
      MEMORY_BASIC_INFORMATION memory{};
      if(!VirtualQuery(target,&memory,sizeof(memory))||memory.State!=MEM_COMMIT||
         !(memory.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))continue;
      HMODULE pinned=nullptr;
      if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(target),&pinned))continue;
      // 与现有随机Lua入口相同的常驻Detour owner；正文拦截也覆盖提前保存的函数别名。
      auto *detour=new MologieDetours::Detour<void *>(target,wrappers[spec.index]);
      s.original=detour->GetOriginalFunction();s.target=target;s.resident=detour;
      Trace("mod_persistence_hook","path=%s installed=1 resident=1",spec.path);
    }catch(...){Trace("mod_persistence_hook","path=%s installed=0 reason=detour",spec.path);}
  }
}
std::string ModPersistenceFields(){std::ostringstream out;out<<"client.mod_persistence_ready="<<ModPersistenceReady()<<'\n';
  for(const auto &spec:specs){const auto &s=slots[spec.index];out<<"client.mod_persist."<<spec.index<<".installed="<<(s.resident!=nullptr)<<'\n'
    <<"client.mod_persist."<<spec.index<<".absent="<<s.absent<<'\n'<<"client.mod_persist."<<spec.index<<".blocked="<<s.blocked.load(std::memory_order_relaxed)<<'\n';}return out.str();}
// 仅由显式E2E命令调用实际已捕获CFunction；无参数时未拦截的original先报参数错误，不会写文件。
bool TestInternalModPersistence() noexcept {
  if(!IsMain()||!ClientProgressProtected()||!g_LuaEngine||!g_LuaEngine->_state)return false;
  auto *L=g_LuaEngine->_state;const int top=lua_gettop(L);
  for(unsigned i=2;i<4;++i){if(!slots[i].resident)return false;
    const auto before=slots[i].blocked.load(std::memory_order_relaxed);
    lua_pushcfunction(L,reinterpret_cast<lua_CFunction>(slots[i].target));
    if(lua_pcall(L,0,0,0)!=LUA_OK||slots[i].blocked.load(std::memory_order_relaxed)!=before+1){lua_settop(L,top);return false;}
  }
  lua_settop(L,top);return true;
}
} // namespace cs::game
