#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/resources/client_resources.h"
#include "game/common/bridge.h"
#include <cstring>

namespace cs::game {
namespace {
// 路径集合属于VM registry，不保存ANM/Layer地址。即使池复用，也必须通过当前getter匹配。
char shader_paths;
constexpr unsigned kShaderPaths=256;
void Paths(lua_State *L){lua_rawgetp(L,LUA_REGISTRYINDEX,&shader_paths);}
int TrackShaderSetter(lua_State *L){
  const int count=lua_gettop(L);const bool path=lua_type(L,2)==LUA_TSTRING;
  // 路径副本放在Lua栈中，避免Lua longjmp跨越C++字符串析构。
  if(path)lua_pushvalue(L,2);else lua_pushnil(L);const int saved=lua_absindex(L,-1);
  lua_pushvalue(L,lua_upvalueindex(1));for(int i=1;i<=count;++i)lua_pushvalue(L,i);
  if(lua_pcall(L,count,LUA_MULTRET,0)!=LUA_OK)return lua_error(L);
  const int results=lua_gettop(L)-saved;
  if(path&&results&&lua_toboolean(L,saved+1)){
    Paths(L);const int paths=lua_absindex(L,-1);const auto size=lua_rawlen(L,paths);bool found=false;
    for(unsigned i=1;i<=size;++i){lua_rawgeti(L,paths,i);found=lua_rawequal(L,-1,saved)!=0;lua_pop(L,1);if(found)break;}
    if(!found&&size<kShaderPaths){lua_pushvalue(L,saved);lua_rawseti(L,paths,lua_Integer(size+1));}lua_pop(L,1);
  }
  // 原参数/副本位于返回值之前，Lua只返回栈顶的原setter结果。
  return results;
}
int Install(lua_State *L){
  Paths(L);if(lua_isnil(L,-1)){lua_pop(L,1);lua_newtable(L);lua_pushvalue(L,-1);lua_rawsetp(L,LUA_REGISTRYINDEX,&shader_paths);}lua_pop(L,1);
  for(unsigned mt=0;mt<2;++mt){
    if(mt)luaL_getmetatable(L,lua::metatables::LayerStateMT);else lua::PushMetatable(L,lua::Metatables::SPRITE);if(!lua_istable(L,-1))return 0;
    for(const char *name:{"SetCustomShader","SetCustomChampionShader"}){
      lua_getfield(L,-1,name);if(!lua_isfunction(L,-1))return 0;
      if(lua_iscfunction(L,-1)&&lua_tocfunction(L,-1)==TrackShaderSetter)lua_pop(L,1);
      else {lua_pushcclosure(L,TrackShaderSetter,1);lua_setfield(L,-2,name);}
    }lua_pop(L,1);
  }lua_pushboolean(L,1);return 1;
}
bool Query(lua_State *L,int object,const char *method,const char *path){
  object=lua_absindex(L,object);lua_getfield(L,object,method);lua_pushvalue(L,object);lua_pushstring(L,path);lua_call(L,2,1);
  if(!lua_isboolean(L,-1))luaL_error(L,"resource_shader_query_type");const bool out=lua_toboolean(L,-1)!=0;lua_pop(L,1);return out;
}
void Capture(lua_State *L,int object){
  object=lua_absindex(L,object);lua_newtable(L);const int pair=lua_absindex(L,-1);
  for(unsigned k=0;k<2;++k){const char *method=k?"HasCustomChampionShader":"HasCustomShader";
    if(!Query(L,object,method,"")){lua_pushliteral(L,"");lua_rawseti(L,pair,k+1);continue;}
    Paths(L);const int paths=lua_absindex(L,-1);if(!lua_istable(L,paths))luaL_error(L,"resource_shader_tracker_unavailable");bool found=false;
    for(unsigned i=1;i<=lua_rawlen(L,paths);++i){lua_rawgeti(L,paths,i);const char *path=lua_tostring(L,-1);
      if(path&&Query(L,object,method,path)){lua_rawseti(L,pair,k+1);found=true;break;}lua_pop(L,1);
    }lua_pop(L,1);if(!found)luaL_error(L,"resource_shader_path_unobserved");
  }
}
void PushLayer(lua_State *L,LayerState *layer){*static_cast<LayerState **>(lua_newuserdata(L,sizeof(LayerState *)))=layer;luaL_setmetatable(L,lua::metatables::LayerStateMT);}
}
bool ClientResourceInstallShaderBindings(void *state) noexcept {
  auto *L=static_cast<lua_State *>(state);if(!L)return false;const int top=lua_gettop(L);try{lua_pushcfunction(L,Install);
    const bool ok=lua_pcall(L,0,1,0)==LUA_OK&&lua_toboolean(L,-1);lua_settop(L,top);return ok;}catch(...){lua_settop(L,top);return false;}
}
bool ClientResourceCaptureShaderPaths(void *state,void *pointer) noexcept {
  auto *L=static_cast<lua_State *>(state);auto *sprite=static_cast<ANM2 *>(pointer);if(!L||!sprite||sprite->_layerCount>64||(sprite->_layerCount&&!sprite->_layerState))return false;
  const int top=lua_gettop(L);try{
  lua_newtable(L);const int shaders=lua_absindex(L,-1);
  lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));Capture(L,-1);lua_setfield(L,shaders,"sprite");lua_pop(L,1);
  lua_newtable(L);for(unsigned i=0;i<sprite->_layerCount;++i){PushLayer(L,&sprite->_layerState[i]);Capture(L,-1);lua_rawseti(L,-3,i+1);lua_pop(L,1);}lua_setfield(L,shaders,"layers");return true;
  }catch(...){lua_settop(L,top);return false;}
}
} // namespace cs::game
