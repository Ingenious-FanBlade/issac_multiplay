#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/world_sources/lua_entity.h"
#include "game/native_room_grid.h"
#include "game/world_sources/global_random.h"
#include "game/world_sources/random_sources.h"
#include "runtime.h"
#include "game/native_entity_weapon.h"
#include <cstring>
#include <cmath>
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
    Decimal(L,"device_relation_revision",subject.device_revision);
    if(subject.control_active) { lua_pushinteger(L,subject.body_slot); lua_setfield(L,-2,"body_slot"); }
    if(subject.group_known) {
      lua_pushinteger(L,subject.body_role); lua_setfield(L,-2,"body_role");
      lua_pushboolean(L,subject.body_active); lua_setfield(L,-2,"active");
      lua_pushboolean(L,subject.can_consume); lua_setfield(L,-2,"can_consume");
      lua_pushvalue(L,-1); lua_setfield(L,-3,"control_group_membership");
    }
    if(subject.control_active) lua_setfield(L,-2,"control_relation"); else lua_pop(L,1);
  }
  return 1;
}
int RegisterSubject(lua_State *L) { return Subject(L,true); }
int Reference(lua_State *L) { return Subject(L,false); }
int GridSubject(lua_State *L) { return Subject(L,true,true); }
int GridReference(lua_State *L) { return Subject(L,false,true); }
// Statue显示完全由Room中的Entity绘制；Grid只借用该主体，不能按格号/seed猜对应对象。
int GridDisplayEntity(lua_State *L){if(!IsMain())return luaL_error(L,"grid_display_thread");
  auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");Entity *display=nullptr;
  if(grid->_desc._type==21)std::memcpy(&display,reinterpret_cast<char *>(grid)+0x154,4);
  if(display)lua::luabridge::UserdataPtr::push(L,display,lua::Metatables::ENTITY);else lua_pushnil(L);return 1;
}
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
// 只复制当前PlayerManager的真实身体关系；Lua随后转换为既有规范key，不按参与者号猜roster。
int PlayerContext(lua_State *L){
  if(!IsMain()||!g_Game)return luaL_error(L,"entity_player_context");
  if(g_Game->_playerManager._playerList.size()>8)return luaL_error(L,"entity_player_context_budget");
  lua_newtable(L);lua_newtable(L);int i=0;
  for(auto *player:g_Game->_playerManager._playerList){if(!player)return luaL_error(L,"entity_player_context_null");lua::luabridge::UserdataPtr::push(L,player,lua::Metatables::ENTITY_PLAYER);lua_rawseti(L,-2,++i);}
  lua_setfield(L,-2,"roster");lua_newtable(L);
  for(unsigned slot=0;slot<4;++slot)if(auto *player=g_Game->_playerManager._esauJrState[slot]){lua::luabridge::UserdataPtr::push(L,player,lua::Metatables::ENTITY_PLAYER);lua_rawseti(L,-2,slot+1);}
  lua_setfield(L,-2,"esau");return 1;
}
// 没有公开Getter的附属Sprite仍按原生资源/姿态采集；不导出对象地址或绘制数据。
int EntityRenderState(lua_State *L){
  if(!IsMain())return luaL_error(L,"entity_render_state_thread");
  auto *e=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");
  lua_newtable(L);lua_pushinteger(L,e->_index&0x3ffu);lua_setfield(L,-2,"order_fragment");
  if(e->_type==8){float layer=0;std::memcpy(&layer,reinterpret_cast<char *>(e)+0x434,4);if(!std::isfinite(layer))return luaL_error(L,"knife_sort_layer");lua_pushnumber(L,layer);lua_setfield(L,-2,"knife_sort_z");}
  lua_newtable(L);
  if(e->_type==5){auto *pickup=static_cast<Entity_Pickup *>(e);
    lua::luabridge::UserdataPtr::push(L,&pickup->_altPedestalANM2,lua::Metatables::SPRITE);lua_setfield(L,-2,"alternate_pedestal");}
  void *body=nullptr;unsigned abyss_counts[2]{};
  if(e->_type==3&&e->_variant==48){std::memcpy(&body,reinterpret_cast<char *>(e)+0xecc,4);
    if(body&&*static_cast<std::uintptr_t *>(body)!=Base+0x714f84)return luaL_error(L,"familiar_body_class");
    if(body){for(unsigned i=0;i<5;++i){const auto name=std::string("isaacs_body_")+std::to_string(i);
      auto *sprite=reinterpret_cast<ANM2 *>(static_cast<char *>(body)+(i?0x118+(i-1)*0x114:4));
      lua::luabridge::UserdataPtr::push(L,sprite,lua::Metatables::SPRITE);lua_setfield(L,-2,name.c_str());}}}
  if(e->_type==3&&e->_variant==231){void *data=nullptr;std::memcpy(&data,reinterpret_cast<char *>(e)+0xecc,4);
    if(!data||*static_cast<std::uintptr_t *>(data)!=Base+0x71510c)return luaL_error(L,"familiar_abyss_class");
    for(unsigned side=0;side<2;++side){ANM2 *header[3]{};std::memcpy(header,static_cast<char *>(data)+0x24+side*12,sizeof(header));
      const auto begin=reinterpret_cast<std::uintptr_t>(header[0]),end=reinterpret_cast<std::uintptr_t>(header[1]),cap=reinterpret_cast<std::uintptr_t>(header[2]);
      if(end<begin||cap<end||(end-begin)%0x114||(cap-begin)%0x114||(cap-begin)/0x114>256)return luaL_error(L,"familiar_abyss_vector");
      abyss_counts[side]=unsigned((end-begin)/0x114);
      for(unsigned i=0;i<abyss_counts[side];++i){const auto name=std::string(side?"abyss_after_":"abyss_before_")+std::to_string(i);
        lua::luabridge::UserdataPtr::push(L,reinterpret_cast<ANM2 *>(begin+i*0x114),lua::Metatables::SPRITE);lua_setfield(L,-2,name.c_str());}}}
  lua_setfield(L,-2,"sprites");lua_pushboolean(L,body!=nullptr);lua_setfield(L,-2,"isaacs_body");lua_newtable(L);
  if(body)for(unsigned i=0;i<4;++i){int id=0;std::memcpy(&id,static_cast<char *>(body)+0x568+i*4,4);lua_pushinteger(L,id);lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"body_ids");lua_pushinteger(L,abyss_counts[0]);lua_setfield(L,-2,"abyss_before");lua_pushinteger(L,abyss_counts[1]);lua_setfield(L,-2,"abyss_after");
  Entity *child=nullptr;if(e->_type==3&&e->_variant==240)std::memcpy(&child,reinterpret_cast<char *>(e)+0xdac,4);
  if(child)lua::luabridge::UserdataPtr::push(L,child,lua::Metatables::ENTITY);else lua_pushnil(L);lua_setfield(L,-2,"familiar_child");
  Weapon *weapon=e->_type==3?static_cast<Entity_Familiar *>(e)->_weapon:nullptr;NativeWeaponLayout layout;
  if(weapon){try{layout=EntityWeaponLayout(weapon);}catch(...){return luaL_error(L,"entity_weapon_reference_layout");}}
  lua_newtable(L);lua_pushboolean(L,weapon!=nullptr);lua_setfield(L,-2,"present");lua_pushstring(L,layout.name);lua_setfield(L,-2,"layout");lua_pushinteger(L,layout.count);lua_setfield(L,-2,"count");lua_newtable(L);
  if(weapon)for(unsigned i=0;i<layout.count;++i){Entity *ref=nullptr;std::memcpy(&ref,reinterpret_cast<char *>(weapon)+layout.offsets[i],4);
    if(ref)lua::luabridge::UserdataPtr::push(L,ref,lua::Metatables::ENTITY);else lua_pushnil(L);lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"refs");lua_setfield(L,-2,"weapon");
  return 1;
}
// 保存原生sort的输入次序为规范关系；不把源地址、完整Index或池槽序号传给Client。
int EntityRenderOrder(lua_State *L){if(!IsMain()||!g_Game||!g_Game->_room)return luaL_error(L,"entity_render_order_context");
  const auto &list=g_Game->_room->_entityList._renderEL;
  if(list._size>list._capacity||list._size>4096||(list._size&&!list._data))return luaL_error(L,"entity_render_order_budget");
  lua_newtable(L);for(unsigned i=0;i<list._size;++i){if(!list._data[i])return luaL_error(L,"entity_render_order_null");lua::luabridge::UserdataPtr::push(L,list._data[i],lua::Metatables::ENTITY);lua_rawseti(L,-2,i+1);}return 1;
}
// 公开GetFrame只返回整数，补充当前连续时钟和逐层缓存；不推进动画、不读取像素或绘制几何。
int SpritePose(lua_State *L) {
  if(!IsMain())return luaL_error(L,"sprite_pose_thread");
  auto *s=lua::GetLuabridgeUserdata<ANM2 *>(L,1,lua::Metatables::SPRITE,"Sprite");
  if(s->_layerCount>64)return luaL_error(L,"sprite_pose_layer_budget");
  lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  auto state=[&](const char *name,const AnimationState &state){
    if(!std::isfinite(state._animFrame))luaL_error(L,"sprite_pose_nonfinite");
    lua_newtable(L);lua_pushnumber(L,state._animFrame);lua_setfield(L,-2,"frame");
    lua_pushboolean(L,state._isPlaying);lua_setfield(L,-2,"playing");
    lua_pushinteger(L,state._currentlyTriggeredEvents);lua_setfield(L,-2,"events_current");lua_pushinteger(L,state._previouslyTriggeredEvents);lua_setfield(L,-2,"events_previous");
    lua_newtable(L);if(state._animData){const int count=state._animData->_layerCount;
      if(count<0||count>64||(count&&(!state._layerFrames||!state._animData->_animLayers)))luaL_error(L,"sprite_pose_layer_storage");
      for(int i=0;i<count;++i){lua_newtable(L);lua_pushinteger(L,state._animData->_animLayers[i]._layerID);lua_setfield(L,-2,"id");lua_pushinteger(L,state._layerFrames[i]);lua_setfield(L,-2,"frame");lua_rawseti(L,-2,i+1);}}
    lua_setfield(L,-2,"layers");
    lua_newtable(L);if(state._animData){const auto count=state._animData->_nullLayerCount;
      if(count<0||count>256)luaL_error(L,"sprite_pose_null_budget");
      if(count&&!state._nullLayerFrames)luaL_error(L,"sprite_pose_null_storage");
      const int *records=nullptr;std::memcpy(&records,state._animData->_nullLayersPtr,sizeof(records));
      if(count&&!records)luaL_error(L,"sprite_pose_null_records");
      for(int i=0;i<count;++i){lua_newtable(L);lua_pushinteger(L,records[i*4]);lua_setfield(L,-2,"id");lua_pushinteger(L,state._nullLayerFrames[i]);lua_setfield(L,-2,"frame");lua_rawseti(L,-2,i+1);}}
    lua_setfield(L,-2,"nulls");lua_setfield(L,-2,name);
  };
  state("main",s->_animState);state("overlay",s->_overlayAnimState);return 1;
}
int Supplement(lua_State *L) {
  auto *entity=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");
  const auto v=EntitySupplement(entity);
  lua_newtable(L); lua_pushstring(L,v.class_name); lua_setfield(L,-2,"class");
  lua_pushstring(L,v.status); lua_setfield(L,-2,"status");
  if(!std::strcmp(v.class_name,"NativeText")&&(!std::strcmp(v.status,"Value")||!std::strcmp(v.status,"Truncated"))) {
    lua_pushinteger(L,v.counter); lua_setfield(L,-2,"counter");
    lua_pushinteger(L,v.duration); lua_setfield(L,-2,"duration");
    lua_pushlstring(L,v.text,v.text_size); lua_setfield(L,-2,"text");
    const char *channels[]={"red","green","blue","alpha"};
    for(unsigned i=0;i<4;++i){lua_pushnumber(L,v.text_color[i]);lua_setfield(L,-2,channels[i]);}
    lua_pushinteger(L,v.color_word);lua_setfield(L,-2,"color_word");
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
// Lua只取得十进制纯值身份，不持有后台可访问的Room/Level userdata。
int Location(lua_State *L) {
  const auto v=ObservationLocation(int(luaL_checkinteger(L,1)),int(luaL_checkinteger(L,2)),std::uint32_t(luaL_checkinteger(L,3)),int(luaL_checkinteger(L,4)),int(luaL_checkinteger(L,5)),int(luaL_checkinteger(L,6)),luaL_checkstring(L,7));
  lua_newtable(L); Decimal(L,"session",v.session); Decimal(L,"epoch",v.epoch); Decimal(L,"run",v.run); Decimal(L,"floor",v.floor); Decimal(L,"dimension",v.dimension); Decimal(L,"descriptor",v.descriptor); Decimal(L,"room_instance",v.room); return 1;
}
int Descriptor(lua_State *L) { const auto v=ObservationDescriptor(int(luaL_checkinteger(L,1)),int(luaL_checkinteger(L,2)),int(luaL_checkinteger(L,3)),luaL_checkstring(L,4)); const auto s=std::to_string(v); lua_pushlstring(L,s.data(),s.size()); const auto rev=std::to_string(ObservationDescriptorRevision(v)); lua_pushlstring(L,rev.data(),rev.size()); return 2; }
int Dimension(lua_State *L) { const auto s=std::to_string(ObservationDimension(int(luaL_checkinteger(L,1)))); lua_pushlstring(L,s.data(),s.size()); return 1; }
int LocationBoundary(lua_State *L) { ObservationLocationBoundary(luaL_checkstring(L,1),lua_toboolean(L,2)!=0); return 0; }
int RelatedSubject(lua_State *L) { const auto s=std::to_string(ObservationRelatedSubject(unsigned(luaL_checkinteger(L,1)))); lua_pushlstring(L,s.data(),s.size()); return 1; }
int LocationNeeded(lua_State *L) { lua_pushboolean(L,ObservationLocationNeeded()); return 1; }
int PublishLocation(lua_State *L) {
  std::size_t bytes=0; const auto *p=luaL_checklstring(L,1,&bytes);
  lua_pushboolean(L,ObservationPublishLocation(p,bytes,lua_toboolean(L,2)!=0,std::uint32_t(luaL_checkinteger(L,4)),luaL_optstring(L,5,""),unsigned(luaL_checkinteger(L,6)),unsigned(luaL_checkinteger(L,7)))); return 1;
}
int CaptureContext(lua_State *L) { const auto s=ObservationCaptureContext(); lua_pushlstring(L,s.data(),s.size()); return 1; }
int LuaGlobalRandomState(lua_State *L) { const auto s=ObservationGlobalRandomState();lua_pushlstring(L,s.data(),s.size());return 1; }
int LuaNativeRandomSources(lua_State *L){const auto s=RandomSourcesSummary();lua_pushlstring(L,s.data(),s.size());return 1;}
int LuaTestNativeRandomSources(lua_State *L){
  try{const auto s=TestNativeRandomSources(luaL_checkstring(L,1),luaL_checkstring(L,2),unsigned(luaL_optinteger(L,3,0)),std::uint32_t(luaL_optinteger(L,4,1)));lua_pushlstring(L,s.data(),s.size());return 1;}
  catch(const std::exception &e){return luaL_error(L,"%s",e.what());}
}
int LuaRegisterRandomSource(lua_State *L){
  auto *rng=lua::GetLuabridgeUserdata<RNG *>(L,1,lua::Metatables::RNG,"RNG");
  std::size_t n=0;const auto *owner=luaL_checklstring(L,2,&n);
  const auto *purpose=luaL_checkstring(L,3);const auto *life=luaL_checkstring(L,4);
  if(n>4096||std::strlen(purpose)>256||std::strlen(life)>256)return luaL_error(L,"random_owner_metadata_budget");
  BindRandomSource({rng,RandomAlgorithm::XorShift32,std::string(owner,n),purpose,life});
  const auto s=RandomSourceReference(rng);lua_pushlstring(L,s.data(),s.size());return 1;
}
// 测试入口只在专用token配置中开放；动作由E2E夹具请求，正式采集器不推进随机源。
int TestGlobalRandom(lua_State *L) {
  const auto *token=luaL_checkstring(L,1);const auto count=luaL_checkinteger(L,2);
  if(!IsMain()||Settings.world_e2e_token.empty()||Settings.world_e2e_token!=token||count<0||count>1400)return luaL_error(L,"global_random_test_disabled_or_bounds");
  lua_newtable(L);const auto before=ObservationGlobalRandomState(true);lua_pushlstring(L,before.data(),before.size());lua_setfield(L,-2,"before");
  if(!lua_isnoneornil(L,3)){
    const auto value=std::uint32_t(luaL_checkinteger(L,3));
    // 已核定ECX传种子、无栈参数；该函数不把ECX当对象。
    const auto seed=reinterpret_cast<int (__thiscall *)(void *)>(Base+0x2eb470);
    const auto result=seed(reinterpret_cast<void *>(uintptr_t(value)));
    lua_pushinteger(L,result);lua_setfield(L,-2,"seed_result");
  }
  lua_newtable(L);for(lua_Integer i=1;i<=count;++i){lua_pushinteger(L,::Random());lua_rawseti(L,-2,i);}lua_setfield(L,-2,"results");
  const auto after=ObservationGlobalRandomState(true);lua_pushlstring(L,after.data(),after.size());lua_setfield(L,-2,"after");return 1;
}
// 官方GetGenericPrompt对未构造userdata赋值复制ANM2；只借用活根，复制公开纯值，不复制C++对象。
int PromptState(lua_State *L) {
  if(!IsMain()||!g_Game) { lua_pushnil(L); return 1; }
  auto *p=g_Game->GetGenericPrompt(); lua_newtable(L);
  lua_pushinteger(L,p->_currentSelection); lua_setfield(L,-2,"selection");
  lua_pushinteger(L,p->_submittedSelection); lua_setfield(L,-2,"submitted");
  lua_pushboolean(L,p->IsActive()); lua_setfield(L,-2,"active");
  lua::luabridge::UserdataPtr::push(L,&p->_anm2,lua::Metatables::SPRITE); lua_setfield(L,-2,"sprite");
  return 1;
}
int SaveEntry(lua_State *L) { const auto s=std::to_string(ObservationSaveEntry()); lua_pushlstring(L,s.data(),s.size()); return 1; }
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
  RegisterRuntimeRandomFunctions();
  lua_newtable(L);
  const luaL_Reg funcs[]={{"TestNativeRandomSources",LuaTestNativeRandomSources},{"NativeRandomSources",LuaNativeRandomSources},{"RegisterRandomSource",LuaRegisterRandomSource},{"GlobalRandomState",LuaGlobalRandomState},{"TestGlobalRandom",TestGlobalRandom},{"PromptState",PromptState},{"RelatedSubject",RelatedSubject},{"CaptureContext",CaptureContext},{"SaveEntry",SaveEntry},{"Dimension",Dimension},{"Location",Location},{"Descriptor",Descriptor},{"LocationBoundary",LocationBoundary},{"LocationNeeded",LocationNeeded},{"PublishLocation",PublishLocation},{"GridCallbackView",GridCallbackView},{"GridSubject",GridSubject},{"GridReference",GridReference},{"GridSources",GridSources},{"GridRenderPosition",GridRenderPosition},{"GridNeeded",GridNeeded},{"PublishGridEvent",PublishGridEvent},{"PublishGridQuery",PublishGridQuery},{"Subject",RegisterSubject},{"Reference",Reference},{"Supplement",Supplement},{"Sources",Sources},{"SpriteNames",SpriteNames},{"Needed",Needed},{"PublishEvent",PublishEvent},{"PublishQuery",PublishQuery},{"PublishRender",PublishRender},{nullptr,nullptr}};
  luaL_setfuncs(L,funcs,0);lua_pushcfunction(L,SpritePose);lua_setfield(L,-2,"SpritePose");lua_pushcfunction(L,PlayerContext);lua_setfield(L,-2,"PlayerContext");
  lua_pushcfunction(L,EntityRenderState);lua_setfield(L,-2,"EntityRenderState");lua_pushcfunction(L,EntityRenderOrder);lua_setfield(L,-2,"EntityRenderOrder");
  lua_pushcfunction(L,GridDisplayEntity);lua_setfield(L,-2,"GridDisplayEntity");lua_setglobal(L,"IsaacCSObservationNative"); lua_settop(L,top);
}
// Coordinator 在复制窗口内部调用一次；错误返回自有原因，不让 Lua 异常越过 Authority。
LuaEntityResult CaptureLuaEntity(std::uint32_t objects,std::uint32_t nodes,std::uint32_t depth,std::uint32_t limit,unsigned domain) noexcept {
  LuaEntityResult result; auto *L=State(); if(!L) { result.reason="lua_state_unavailable"; return result; }
  const auto top=lua_gettop(L);
  try {
    const char *modules[]={"IsaacCSObservation","IsaacCSGridObservation","IsaacCSRoomObservation","IsaacCSLevelObservation","IsaacCSExplorationObservation","IsaacCSRoomSaveObservation","IsaacCSPlayerObservation","IsaacCSRunObservation","IsaacCSRandomObservation","IsaacCSPresentationObservation","IsaacCSModObservation"};
    if(domain>=11) { result.reason="unsupported_domain"; return result; }
    lua_getglobal(L,modules[domain]);
    if(!lua_istable(L,-1)) { lua_settop(L,top); result.reason="formal_observation_module_unavailable"; return result; }
    lua_getfield(L,-1,"Capture");
    lua_newtable(L);
    lua_pushinteger(L,objects); lua_setfield(L,-2,"objects");
    lua_pushinteger(L,nodes); lua_setfield(L,-2,"nodes");
    lua_pushinteger(L,depth); lua_setfield(L,-2,"depth");
    lua_pushinteger(L,limit); lua_setfield(L,-2,"bytes");
    lua_pushinteger(L,domain==5?262144:131072); lua_setfield(L,-2,"queries");
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
      // 同一原生复制窗口补齐18载体所需的具名Game／descriptor值，和正式Room消费端一起提交。
      if(domain==2&&result.complete&&!result.payload.empty()&&result.payload.back()=='}'){
        const auto context=",\"native_context\":"+CaptureNativeRoomContext();
        if(result.payload.size()+context.size()>(16u<<20)){result.complete=false;result.reason="native_context_byte_budget";}
        else result.payload.insert(result.payload.size()-1,context);
      }
    } else result.reason="lua_payload_budget";
  } catch(...) { result.complete=false; result.reason="lua_owned_value_budget"; }
  lua_settop(L,top); return result;
}
}

namespace cs::game {
void FinishLuaObservation(unsigned domain,std::uint64_t scope,bool committed) noexcept {
  auto *L=State(); if(!L||domain<4||domain>10) return;
  const auto top=lua_gettop(L); const char *names[]={"IsaacCSExplorationObservation","IsaacCSRoomSaveObservation","IsaacCSPlayerObservation","IsaacCSRunObservation","IsaacCSRandomObservation","IsaacCSPresentationObservation","IsaacCSModObservation"};
  lua_getglobal(L,names[domain-4]);
  if(lua_istable(L,-1)) { lua_getfield(L,-1,"Outcome"); if(lua_isfunction(L,-1)) {
    const auto s=std::to_string(scope); lua_pushlstring(L,s.data(),s.size()); lua_pushboolean(L,committed); lua_pcall(L,2,0,0);
  } }
  lua_settop(L,top);
}
}
