#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/world_sources/lua_entity.h"
#include "runtime.h"
#include <cstring>
namespace cs::game {
namespace {
lua_State *State() noexcept { return IsMain()&&g_LuaEngine ? g_LuaEngine->_state : nullptr; }
void Decimal(lua_State *L,const char *key,std::uint64_t value) {
  const auto s=std::to_string(value); lua_pushlstring(L,s.data(),s.size()); lua_setfield(L,-2,key);
}
int Subject(lua_State *L,bool registering,bool grid=false) {
  if(!IsMain()) { lua_pushnil(L); return 1; }
  if(lua_isnil(L,1)) { lua_newtable(L); lua_pushstring(L,"Null"); lua_setfield(L,-2,"resolution"); return 1; }
  void *entity=grid?static_cast<void *>(lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity")):static_cast<void *>(lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity"));
  const auto subject=ObservationSubject(entity,registering,grid);
  lua_newtable(L);
  Decimal(L,"session",subject.session); Decimal(L,"epoch",subject.epoch);
  Decimal(L,"local_id",subject.local); Decimal(L,"use",subject.use); Decimal(L,"revision",subject.revision);
  lua_pushinteger(L,grid?2:1); lua_setfield(L,-2,"space");
  constexpr const char *names[]={"Null","KnownCurrent","KnownHistorical","Unresolved","NotCaptured","UnsupportedTarget","Retired","AmbiguousReuse","CrossEpoch"};
  lua_pushstring(L,subject.status<9?names[subject.status]:"Unresolved"); lua_setfield(L,-2,"resolution");
  lua_pushinteger(L,subject.origin); lua_setfield(L,-2,"origin");
  lua_pushinteger(L,subject.readiness); lua_setfield(L,-2,"native_readiness");
  if(subject.ordinal<4&&subject.participant&&subject.group) {
    lua_newtable(L); lua_pushinteger(L,subject.ordinal); lua_setfield(L,-2,"ordinal");
    Decimal(L,"participant_local_id",subject.participant); Decimal(L,"control_group_local_id",subject.group);
    Decimal(L,"control_session",subject.control_session); Decimal(L,"binding_revision",subject.binding_revision);
    lua_setfield(L,-2,"control_relation");
  }
  return 1;
}
int RegisterSubject(lua_State *L) { return Subject(L,true); }
int Reference(lua_State *L) { return Subject(L,false); }
int GridSubject(lua_State *L) { return Subject(L,true,true); }
int GridReference(lua_State *L) { return Subject(L,false,true); }
// 官方 Lua 包装每次 new Vector 未回收；此处使用已声明 SDK Getter 的栈缓冲。
int GridRenderPosition(lua_State *L) {
  auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");
  Vector buffer(0,0); grid->GetRenderPosition(&buffer);
  lua_newtable(L); lua_pushstring(L,"vector"); lua_setfield(L,-2,"type");
  lua_pushnumber(L,buffer.x); lua_setfield(L,-2,"x"); lua_pushnumber(L,buffer.y); lua_setfield(L,-2,"y"); return 1;
}
int GridSources(lua_State *L) {
  lua_newtable(L);
  struct Sink { lua_State *L; int index=0; } sink{L};
  const auto unavailable=ObservationGrids(&sink,[](void *owner,void *grid,std::uint32_t memberships) noexcept {
    auto &s=*static_cast<Sink *>(owner); lua_newtable(s.L);
    lua::luabridge::UserdataPtr::push(s.L,grid,lua::Metatables::GRID_ENTITY); lua_setfield(s.L,-2,"grid");
    lua_pushinteger(s.L,memberships); lua_setfield(s.L,-2,"membership_mask");
    lua_newtable(s.L);
    struct Members { lua_State *L; int index=0; } ms{s.L};
    ObservationGridMemberships(grid,&ms,[](void *owner,std::uint32_t source,std::uint32_t index) noexcept {
      auto &m=*static_cast<Members *>(owner); lua_newtable(m.L);
      lua_pushinteger(m.L,source); lua_setfield(m.L,-2,"source"); lua_pushinteger(m.L,index); lua_setfield(m.L,-2,"index");
      lua_rawseti(m.L,-2,++m.index);
    });
    lua_setfield(s.L,-2,"memberships"); lua_rawseti(s.L,-2,++s.index);
  });
  lua_pushinteger(L,unavailable); return 2;
}
// 不更改回调传入userdata的元表；构造经保护读取验证过的借用基类视图。
int GridCallbackView(lua_State *L) {
  if(!IsMain()||lua_type(L,1)!=LUA_TUSERDATA||lua_rawlen(L,1)!=sizeof(lua::luabridge::UserdataPtr)) { lua_pushnil(L); return 1; }
  auto *grid=lua::UserdataToData<GridEntity *>(lua_touserdata(L,1));
  if(!ObservationValidGrid(grid)) { lua_pushnil(L); return 1; }
  lua::luabridge::UserdataPtr::push(L,grid,lua::Metatables::GRID_ENTITY); return 1;
}
int GridNeeded(lua_State *L) { lua_pushboolean(L,IsMain()&&ObservationLuaNeeded(luaL_checkstring(L,1),true)); return 1; }
int Supplement(lua_State *L) {
  auto *entity=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");
  const auto v=EntitySupplement(entity);
  lua_newtable(L); lua_pushstring(L,v.class_name); lua_setfield(L,-2,"class");
  lua_pushstring(L,v.status); lua_setfield(L,-2,"status");
  if(!std::strcmp(v.class_name,"NativeText")&&(!std::strcmp(v.status,"Value")||!std::strcmp(v.status,"Truncated"))) {
    lua_pushinteger(L,v.counter); lua_setfield(L,-2,"counter");
    lua_pushinteger(L,v.duration); lua_setfield(L,-2,"duration");
    lua_pushlstring(L,v.text,v.text_size); lua_setfield(L,-2,"text");
  }
  return 1;
}
int Sources(lua_State *L) {
  lua_newtable(L);
  struct Sink { lua_State *L; int index=0; } sink{L};
  const auto unavailable=ObservationEntities(&sink,[](void *owner,void *entity,std::uint32_t readiness) noexcept {
    auto &s=*static_cast<Sink *>(owner);
    lua_newtable(s.L);
    lua::luabridge::UserdataPtr::push(s.L,entity,lua::Metatables::ENTITY); lua_setfield(s.L,-2,"entity");
    lua_pushinteger(s.L,readiness); lua_setfield(s.L,-2,"native_readiness");
    lua_rawseti(s.L,-2,++s.index);
  });
  lua_pushinteger(L,unavailable); return 2;
}
int SpriteNames(lua_State *L) {
  auto *sprite=lua::GetLuabridgeUserdata<ANM2 *>(L,1,lua::Metatables::SPRITE,"Sprite");
  lua_newtable(L); lua_newtable(L); lua_setfield(L,-2,"nulls"); lua_newtable(L); lua_setfield(L,-2,"events");
  struct Sink { lua_State *L; int nulls=0,events=0; } sink{L};
  const auto *status=ObservationSpriteNames(sprite,&sink,[](void *owner,bool event,const char *name,std::size_t length) noexcept {
    auto &s=*static_cast<Sink *>(owner); lua_getfield(s.L,-1,event?"events":"nulls");
    lua_pushlstring(s.L,name,length); lua_rawseti(s.L,-2,event?++s.events:++s.nulls); lua_pop(s.L,1);
  });
  lua_pushstring(L,status); lua_setfield(L,-2,"status"); return 1;
}
int Needed(lua_State *L) {
  lua_pushboolean(L,IsMain()&&ObservationLuaNeeded(luaL_checkstring(L,1))); return 1;
}
int Publish(lua_State *L,unsigned phase,bool grid=false) {
  std::size_t bytes=0; const auto *json=luaL_checklstring(L,1,&bytes);
  const auto complete=lua_toboolean(L,2)!=0;
  const auto objects=luaL_checkinteger(L,3),nodes=luaL_checkinteger(L,4);
  const auto *reason=luaL_optstring(L,5,"");
  const bool valid=bytes<=(16u<<20)&&objects>=0&&objects<=16384&&nodes>=0&&nodes<=262144;
  lua_pushboolean(L,valid&&ObservationPublishLua(json,bytes,complete,unsigned(objects),unsigned(nodes),reason,phase,grid));
  return 1;
}
int PublishEvent(lua_State *L) { return Publish(L,false); }
int PublishQuery(lua_State *L) { return Publish(L,true); }
int PublishRender(lua_State *L) { return Publish(L,2); }
int PublishGridEvent(lua_State *L) { return Publish(L,lua_toboolean(L,6)?2:0,true); }
int PublishGridQuery(lua_State *L) { return Publish(L,1,true); }
}
void EnsureObservationLuaBridge() noexcept {
  auto *L=State(); if(!L) return;
  const auto top=lua_gettop(L);
  lua_getglobal(L,"IsaacCSObservationNative");
  const bool exists=lua_istable(L,-1); lua_settop(L,top);
  if(exists) return;
  lua_newtable(L);
  const luaL_Reg funcs[]={{"GridCallbackView",GridCallbackView},{"GridSubject",GridSubject},{"GridReference",GridReference},{"GridSources",GridSources},{"GridRenderPosition",GridRenderPosition},{"GridNeeded",GridNeeded},{"PublishGridEvent",PublishGridEvent},{"PublishGridQuery",PublishGridQuery},{"Subject",RegisterSubject},{"Reference",Reference},{"Supplement",Supplement},{"Sources",Sources},{"SpriteNames",SpriteNames},{"Needed",Needed},{"PublishEvent",PublishEvent},{"PublishQuery",PublishQuery},{"PublishRender",PublishRender},{nullptr,nullptr}};
  luaL_setfuncs(L,funcs,0); lua_setglobal(L,"IsaacCSObservationNative"); lua_settop(L,top);
}
// Coordinator 在复制窗口内部调用一次；错误返回自有原因，不让 Lua 异常越过 Authority。
LuaEntityResult CaptureLuaEntity(std::uint32_t objects,std::uint32_t nodes,std::uint32_t depth,std::uint32_t limit,bool grid) noexcept {
  LuaEntityResult result; auto *L=State(); if(!L) { result.reason="lua_state_unavailable"; return result; }
  const auto top=lua_gettop(L);
  try {
    lua_getglobal(L,grid?"IsaacCSGridObservation":"IsaacCSObservation");
    if(!lua_istable(L,-1)) { lua_settop(L,top); result.reason="formal_observation_module_unavailable"; return result; }
    lua_getfield(L,-1,"Capture");
    lua_newtable(L);
    lua_pushinteger(L,objects); lua_setfield(L,-2,"objects");
    lua_pushinteger(L,nodes); lua_setfield(L,-2,"nodes");
    lua_pushinteger(L,depth); lua_setfield(L,-2,"depth");
    lua_pushinteger(L,limit); lua_setfield(L,-2,"bytes");
    if(!lua_isfunction(L,-2)||lua_pcall(L,1,6,0)!=LUA_OK) {
      const auto *error=lua_tostring(L,-1); result.reason=error?std::string(error).substr(0,256):"lua_capture_failed";
      lua_settop(L,top); return result;
    }
    std::size_t bytes=0; const auto *payload=lua_tolstring(L,-6,&bytes);
    if(payload&&bytes<=(16u<<20)) {
      result.payload.assign(payload,bytes); result.available=true;
      result.complete=lua_toboolean(L,-5)!=0;
      const auto objects=lua_tointeger(L,-4),nodes=lua_tointeger(L,-3);
      if(objects<0||objects>16384||nodes<0||nodes>262144) { result.complete=false; result.reason="lua_count_budget"; }
      else { result.objects=unsigned(objects); result.nodes=unsigned(nodes); }
      const auto *reason=lua_tostring(L,-2); if(reason) result.reason=reason;
    } else result.reason="lua_payload_budget";
  } catch(...) { result.complete=false; result.reason="lua_owned_value_budget"; }
  lua_settop(L,top); return result;
}
}
