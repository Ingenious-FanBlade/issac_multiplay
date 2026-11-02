#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_grid_native.h"
#include "game/client_lifetime.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
constexpr const char *cast_methods[]={"ToDecoration","ToDoor","ToFire","ToGravity","ToLock","ToPit","ToPoop","ToPressurePlate","ToRock","ToSpikes","ToStairs","ToStatue","ToTeleporter","ToTNT","ToTrapDoor","ToWall","ToWeb"};
template<class T> T Read(const void *p,unsigned offset){T v{};std::memcpy(&v,static_cast<const unsigned char *>(p)+offset,sizeof(T));return v;}
template<class T> void Write(void *p,unsigned offset,const T &v){std::memcpy(static_cast<unsigned char *>(p)+offset,&v,sizeof(T));}
template<class T> struct NativeArray final {T *begin=nullptr,*end=nullptr,*capacity=nullptr;};
static_assert(sizeof(ClientGridEmber)==24&&sizeof(ClientPoseVector)==8&&sizeof(NativeArray<int>)==12);
bool Rock(int type){return (type>=2&&type<=6)||type==22||type==24||type==25||type==26||type==27;}
// SDK 少声明一格；所有实际成员读写统一使用已核 +24 的 448 指针区，不越过门表。
GridEntity *Grid(Room &room,unsigned index) noexcept {GridEntity *p=nullptr;if(index<448)std::memcpy(&p,reinterpret_cast<unsigned char *>(&room)+0x24+index*4,4);return p;}
void Grid(Room &room,unsigned index,GridEntity *p) noexcept {std::memcpy(reinterpret_cast<unsigned char *>(&room)+0x24+index*4,&p,4);}
bool Window(const ClientNativeRoots &roots) noexcept {
  void *dc=nullptr,*context=nullptr;
  return IsMain()&&TopOwner()==Owner::Manager&&ClientBusy()&&ClientPersistenceBlocked()&&ContextHealthy()&&!InBlockedContext()&&
      g_Game&&g_Game->_room&&roots.room&&roots.descriptor&&roots.game==g_Game&&roots.room==g_Game->_room&&roots.descriptor==g_Game->_room->_descriptor&&
      ClientRendererHandles(dc,context)&&dc&&context&&dc==wglGetCurrentDC()&&context==wglGetCurrentContext()&&ClientNativeCpuQueuesEmpty();
}
// 工厂尚未发布时由本表拥有；发布后成员责任仍由同一表追踪，Room 析构前须完成撤销。
struct Entry final {
  ClientGridDescription description;
  GridEntity *grid=nullptr;
  std::uint64_t instance=0;
  unsigned prefix=0;
  bool factory_entered=false,constructed=false,published=false,ready=false,release_entered=false,uncertain=false;
  lua_State *vm=nullptr;
  int pinned=LUA_NOREF;
};
struct LuaFrame final {Entry *entry=nullptr;bool apply=false;bool ok=false;};
void Push(lua_State *L,GridEntity *grid){lua::luabridge::UserdataPtr::push(L,grid,lua::GetMetatableKey(lua::Metatables::GRID_ENTITY));}
// 只使用正式公开转换得到目标 userdata；验证所有 17 个 cast，而非用 type 猜类布局。
bool Cast(lua_State *L,unsigned index){
  lua_getfield(L,-1,cast_methods[index]);lua_pushvalue(L,-2);lua_call(L,1,1);return !lua_isnil(L,-1);
}
void Value(lua_State *L,const ClientGridField &field){
  if(field.kind==ClientGridValueKind::Integer)lua_pushinteger(L,field.integer);
  else if(field.kind==ClientGridValueKind::Boolean)lua_pushboolean(L,field.boolean);
  else if(field.null_text)lua_pushnil(L);else lua_pushlstring(L,field.text.data(),field.text.size());
}
bool Same(lua_State *L,const ClientGridField &field){
  if(field.kind==ClientGridValueKind::Integer)return lua_isinteger(L,-1)&&lua_tointeger(L,-1)==field.integer;
  if(field.kind==ClientGridValueKind::Boolean)return lua_isboolean(L,-1)&&bool(lua_toboolean(L,-1))==field.boolean;
  if(field.null_text)return lua_isnil(L,-1);std::size_t size=0;const char *s=lua_tolstring(L,-1,&size);return s&&size==field.text.size()&&!std::memcmp(s,field.text.data(),size);
}
// 所有元方法、Lua 分配和 longjmp 在 pcall 内；字符串根保留到实例退休，防止原绑定保存借用。
int FieldsThunk(lua_State *L){
  auto &frame=*static_cast<LuaFrame *>(lua_touserdata(L,1));auto &entry=*frame.entry;const auto &d=entry.description;
  lua_createtable(L,int(d.fields.size()+1),0);const int pins=lua_absindex(L,-1);Push(L,entry.grid);const int base=lua_absindex(L,-1);
  if(frame.apply){
    // 在第一个 raw const-char* setter 之前先拥有根；失败也保留新旧字符串到安全退休。
    if(entry.pinned>=0){lua_rawgeti(L,LUA_REGISTRYINDEX,entry.pinned);lua_rawseti(L,pins,0);}
    lua_pushvalue(L,pins);const int next=luaL_ref(L,LUA_REGISTRYINDEX);
    if(entry.pinned>=0)luaL_unref(L,LUA_REGISTRYINDEX,entry.pinned);entry.pinned=next;entry.vm=L;
  }
  for(unsigned i=0;i<17;++i){const bool actual=Cast(L,i);if(actual!=bool(d.cast_mask&(1u<<i)))return 0;lua_pop(L,1);}
  unsigned pin=0;
  for(const auto &field:d.fields){
    lua_settop(L,base);if(!Cast(L,field.cast))return 0;const int object=lua_absindex(L,-1);
    if(frame.apply){
      // Door.Slot 只有公开 getter；J273 注册点确认它读取 +158，具名安装后仍用 getter 核对。
      if(field.cast==1&&field.name=="Slot")Write(entry.grid,0x158,field.integer);
      else{Value(L,field);lua_pushvalue(L,-1);lua_rawseti(L,pins,++pin);lua_setfield(L,object,field.name.c_str());}}
    lua_getfield(L,object,field.name.c_str());if(!Same(L,field))return 0;
  }
  if(d.cast_mask&(1u<<8)){lua_settop(L,base);if(!Cast(L,8))return 0;
    // J273 SetBigRockFrame(-1) 会随机重选普通岩石；负 sentinel 由来源 Variant/Anim 原样承载。
    if(frame.apply&&d.big_rock_frame>=0){lua_getfield(L,-1,"SetBigRockFrame");lua_pushvalue(L,-2);lua_pushinteger(L,d.big_rock_frame);lua_call(L,2,0);}
    lua_getfield(L,-1,"GetBigRockFrame");lua_pushvalue(L,-2);lua_call(L,1,1);if(lua_tointeger(L,-1)!=d.big_rock_frame)return 0;}
  lua_settop(L,base);
  // 官方副表按 index 而非主体拥有；同格 Door-only/装饰允许不同默认裁切。
  // 常驻 wrapper 在官方原链返回后安装当前实例的有效值，不创建相互污染的永久 override。
  lua_getfield(L,base,"GetWaterClipFlags");lua_pushvalue(L,base);lua_call(L,1,1);if(lua_tointeger(L,-1)!=d.water_clip)return 0;
  lua_settop(L,pins);
  if(frame.apply){lua_pushnil(L);lua_rawseti(L,pins,0);}
  frame.ok=true;return 0;
}
bool Fields(Entry &entry,bool apply) noexcept {
  if(!g_LuaEngine||!g_LuaEngine->_state||!ClientLuaGuardReady()||!ClientCallbackGuardReady())return false;
  auto *L=g_LuaEngine->_state;if(entry.vm&&entry.vm!=L)return false;const int top=lua_gettop(L);if(!lua_checkstack(L,24))return false;
  LuaFrame frame{&entry,apply,false};lua_pushcfunction(L,FieldsThunk);lua_pushlightuserdata(L,&frame);
  const bool result=lua_pcall(L,1,0,0)==LUA_OK&&frame.ok;lua_settop(L,top);return result;
}
ANM2 *Sprite(Entry &entry,const ClientGridSprite &sprite) noexcept {
  if(sprite.role==0)return &entry.grid->_sprite;
  // 官方 GetExtraSprite.cpp 明确返回这个内嵌成员；不依赖旧 ExtraSprite 属性的副本语义。
  if(sprite.role==1&&entry.description.type==16)return &static_cast<GridEntity_Door *>(entry.grid)->_extraSprite;
  // TimerPlate 公开 getter 返回 ANM2 的值副本；不能弹栈后把副本指针当原生成员。
  // 官方 setter/getter 与工厂都确认本体 +154 是已构造的内嵌 ANM2。
  if(sprite.role==2)return reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(entry.grid)+0x154);
  if((sprite.role==3||sprite.role==4)&&entry.description.type==13)
    return reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(entry.grid)+(sprite.role==3?0x154:0x27c));
  return nullptr;
}
bool Resources(Entry &entry,bool apply) noexcept {
  for(const auto &source:entry.description.sprites){auto *sprite=Sprite(entry,source);
    if(!source.present){if(sprite&&sprite->_loaded){if(!apply)return false;sprite->destructor();sprite->constructor();}continue;}if(!sprite)return false;
    if(apply){const auto result=ClientApplyResourcePose(sprite,source.pose);if(!result.complete){if(std::strcmp(result.reason,"resource_native_exception")==0)entry.uncertain=true;return false;}}
    else if(!ClientResourcePoseMatches(sprite,source.pose))return false;}
  return true;
}
bool CommonMatches(const Entry &entry) noexcept {
  const auto &d=entry.description;const auto *g=entry.grid;if(!g)return false;const auto &s=g->_desc;
  return s._type==d.type&&s._variant==d.variant&&s._state==d.desc_state&&s._varData==d.desc_var_data&&s._spawnCount==d.spawn_count&&
    s._spawnSeed==d.spawn_seed&&s._variableSeed==d.variable_seed&&s._initialized==d.source_initialized&&g->_gridIndex==unsigned(d.index)&&
    g->_collisionClass==d.collision&&g->_spawnFrame==d.spawn_frame;
}
void Common(Entry &entry) noexcept {
  auto &g=*entry.grid;const auto &d=entry.description;auto &s=g._desc;
  s._type=d.type;s._variant=d.variant;s._state=d.desc_state;s._varData=d.desc_var_data;s._spawnCount=d.spawn_count;
  s._spawnSeed=d.spawn_seed;s._variableSeed=d.variable_seed;s._initialized=d.source_initialized;
  g._gridIndex=unsigned(d.index);g._spawnFrame=d.spawn_frame;g._collisionClass=d.collision;
}
// Fire 的 Ember 和 Rock 的碎屑坐标是 Render 直接读取的值容器；本 owner 借出自有缓冲。
// 游戏的 Free 之前必须归还空 header，不能由游戏 std::vector 析构释放本方 std::vector 内存。
void Runtime(Entry &entry) noexcept {
  auto *g=entry.grid;auto &d=entry.description;
  if(d.type==13){Write(g,0x268,d.fire_timer);Write(g,0x26c,d.fire_style);auto *p=d.embers.empty()?nullptr:d.embers.data();
    Write(g,0x270,NativeArray<ClientGridEmber>{p,p?p+d.embers.size():nullptr,p?p+d.embers.size():nullptr});}
  if(Rock(d.type)){auto *p=d.rubble.empty()?nullptr:d.rubble.data();Write(g,0x184,NativeArray<ClientPoseVector>{p,p?p+d.rubble.size():nullptr,p?p+d.rubble.size():nullptr});}
  if(d.type==16)Write(g,0x154,d.door_animation_mode);
}
void DetachRuntime(Entry &entry) noexcept {
  if(entry.description.type==13)Write(entry.grid,0x270,NativeArray<ClientGridEmber>{});
  if(Rock(entry.description.type))Write(entry.grid,0x184,NativeArray<ClientPoseVector>{});
}
bool PositionMatches(Room &room,const Entry &entry) noexcept {
  const auto &d=entry.description;const auto index=unsigned(d.index);const float x=float(index%room._gridWidth)*40+40,y=float(index/room._gridWidth)*40+120;
  Vector render;entry.grid->GetRenderPosition(&render);
  // GetRenderPosition 是依赖本地视口的纯投影；来源结果已在写前按来源输入核对。
  // 不要求不同分辨率的 Client 屏幕坐标等于 Authority 屏幕坐标。
  return d.position.x==x&&d.position.y==y&&std::isfinite(render.x)&&std::isfinite(render.y);
}
// 先撤全部实际别名，不调用 Room 删除中的路径/装饰裁决，不运行 Destroy/掉落/钥匙路径。
void Unpublish(Room &room,GridEntity *grid) noexcept {
  for(unsigned i=0;i<448;++i)if(Grid(room,i)==grid)Grid(room,i,nullptr);
  for(auto &door:room._doors)if(door==grid)door=nullptr;
}
}
struct ClientNativeGrids final {
  std::map<ClientObjectKey,std::unique_ptr<Entry>> entries;
  Room *room=nullptr;
  ClientObjectKey room_key{};
  std::uint64_t next_instance=0;
  unsigned prefix=0;
  bool written=false,uncertain=false;
};
namespace {
ClientNativeGrids *water_owner=nullptr;
ClientGridStep Result(ClientNativeGrids *owner,bool done,bool failed,const char *reason){return {done,owner&&owner->written,failed,owner?owner->prefix:0,owner?unsigned(owner->entries.size()):0,reason};}
bool Owned(const ClientNativeGrids &owner,const Room &room,const Entry &entry){
  if(owner.room!=&room||!entry.grid||!entry.constructed||entry.uncertain||entry.release_entered)return false;
  if(entry.description.grid_member&&Grid(*owner.room,unsigned(entry.description.index))!=entry.grid)return false;
  return true;
}
bool Remove(ClientNativeGrids &owner,Room &room,Entry &entry){
  if(!entry.grid)return !entry.factory_entered||entry.constructed;
  if(entry.uncertain||entry.release_entered||!entry.constructed)return false;
  // 清别名后不再允许 Render/借用，唯一 vtable Free 调用负责真实分配大小及派生 ANM 析构。
  if(owner.prefix==UINT32_MAX)return false;
  ++owner.prefix;owner.written=true;entry.ready=false;Unpublish(room,entry.grid);entry.published=false;
  DetachRuntime(entry);
  entry.release_entered=true;auto **table=*reinterpret_cast<void ***>(entry.grid);
  reinterpret_cast<void (__thiscall *)(GridEntity *,unsigned)>(table[0])(entry.grid,1);
  entry.grid=nullptr;
  if(entry.pinned>=0){if(!g_LuaEngine||g_LuaEngine->_state!=entry.vm)return false;luaL_unref(entry.vm,LUA_REGISTRYINDEX,entry.pinned);entry.pinned=LUA_NOREF;}
  return true;
}
ClientGridStep Execute(ClientNativeGrids *owner,const ClientNativeRoots &roots,const ClientGridCommand &command){
  if(!owner||!Window(roots)||owner->uncertain)return Result(owner,false,true,"grid_native_context");
  auto &room=*static_cast<Room *>(roots.room);const auto &d=command.value;
  if(owner->prefix>UINT32_MAX-2)return Result(owner,false,true,"grid_prefix_exhausted");
  if(owner->room&&owner->room!=&room)return Result(owner,false,true,"grid_room_generation_changed");
  owner->room=&room;auto found=owner->entries.find(d.key);
  if(command.operation==ClientGridOperation::Remove){if(found==owner->entries.end())return Result(owner,false,true,"grid_remove_owner_missing");
    if(!Remove(*owner,room,*found->second))return Result(owner,false,true,"grid_remove_prefix_uncertain");owner->entries.erase(found);return Result(owner,true,false,"grid_removed");}
  if(d.key.space!=2||!d.key.run||!d.key.id||d.room.space!=7||d.index<0||d.index>=448||d.type<1||d.type>27||!d.revision)
    return Result(owner,false,true,"grid_description_invalid");
  if(owner->room_key.id&&!(owner->room_key==d.room))return Result(owner,false,true,"grid_room_key_changed");owner->room_key=d.room;
  if(command.operation==ClientGridOperation::Create){
    if(found!=owner->entries.end()||(d.grid_member&&Grid(room,unsigned(d.index))))return Result(owner,false,true,"grid_create_slot_or_key_occupied");
    if(water_owner&&water_owner!=owner)return Result(owner,false,true,"grid_multiple_world_owners");water_owner=owner;
    if(owner->entries.size()>=456||owner->next_instance==UINT64_MAX)return Result(owner,false,true,"grid_owner_budget");
    auto entry=std::make_unique<Entry>();entry->description=d;entry->instance=++owner->next_instance;
    found=owner->entries.emplace(d.key,std::move(entry)).first;auto &e=*found->second;
    ++owner->prefix;owner->written=true;e.factory_entered=true;e.prefix|=1;
    // 官方工厂 wrapper 保留 REPENTOGON Hook 链/Plus 重置；不调用含派生玩法的 Init/PostInit。
    e.grid=Isaac::CreateGridEntity(d.type,unsigned(d.index));e.constructed=true;if(!e.grid)return Result(owner,false,true,"grid_factory_null");e.prefix|=2;
  }else if(found==owner->entries.end())return Result(owner,false,true,"grid_instance_missing");
  auto &e=*found->second;
  if(command.operation==ClientGridOperation::Check){
    if(!Owned(*owner,room,e)||!e.ready||!CommonMatches(e)||!Fields(e,false)||!Resources(e,false)||!PositionMatches(room,e))return Result(owner,false,true,"grid_final_state_or_membership_changed");
    return Result(owner,true,false,"grid_instance_checked");}
  if(command.operation==ClientGridOperation::Change){
    if(!Owned(*owner,room,e)||e.description.index!=d.index||e.description.type!=d.type||e.description.variant!=d.variant||e.description.revision!=d.revision)
      return Result(owner,false,true,"grid_change_requires_replace");e.description=d;}
  ++owner->prefix;owner->written=true;e.ready=false;e.prefix|=4;Common(e);Runtime(e);
  if(!Fields(e,true))return Result(owner,false,true,"grid_public_fields_or_cast");e.prefix|=8;
  if(!Resources(e,true))return Result(owner,false,true,"grid_resource_install");e.prefix|=16;
  if(d.grid_member){if(Grid(room,unsigned(d.index))&&Grid(room,unsigned(d.index))!=e.grid)return Result(owner,false,true,"grid_publish_slot_conflict");Grid(room,unsigned(d.index),e.grid);}
  e.published=true;e.prefix|=32;e.ready=true;return Result(owner,true,false,"grid_instance_installed");
}
}
ClientNativeGrids *ClientNativeGridsCreate() noexcept {return IsMain()&&ClientBusy()?new(std::nothrow) ClientNativeGrids:nullptr;}
bool ClientNativeGridsBeginAttempt(ClientNativeGrids *owner) noexcept {
  if(!owner||!IsMain()||TopOwner()!=Owner::Manager||!ClientBusy()||owner->uncertain)return false;
  // 只清本次执行元数据；已提交实例、构造前缀与本地单调代次保留。
  owner->prefix=0;owner->written=false;return true;
}
void ClientNativeGridsDestroy(ClientNativeGrids *owner) noexcept {
  // 场景若在不确定原生前缀下被强制销毁，保持隔离；不在 C++ 析构中盲目补跑游戏析构。
  if(owner&&owner->entries.empty()){if(water_owner==owner)water_owner=nullptr;delete owner;}
}
ClientGridStep ClientNativeGridsExecute(ClientNativeGrids *owner,const ClientNativeRoots &roots,const ClientGridCommand &command) noexcept {
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)return Result(owner,false,true,"grid_apply_dispatch");
  ClientGridStep result;
  try{result=Execute(owner,roots,command);}catch(...){if(owner)owner->uncertain=true;result=Result(owner,false,true,"grid_native_exception");}
  ClientDispatchReturned(ClientEntry::NativeApply);if(!dispatch.Succeeded())return Result(owner,false,true,"grid_apply_dispatch_return");return result;
}
ClientGridStep ClientNativeGridsBindDoors(ClientNativeGrids *owner,const ClientNativeRoots &roots,const std::array<ClientGridDoorBinding,8> &bindings) noexcept {
  if(!owner||!Window(roots)||owner->uncertain)return Result(owner,false,true,"grid_door_context");
  auto &room=*static_cast<Room *>(roots.room);std::array<GridEntity *,8> doors{};
  // 完整解析全部槽后一次发布别名；路由只存纯值，不调用过门 FSM 或 GetRoomByIdx。
  for(unsigned slot=0;slot<8;++slot){const auto &b=bindings[slot];
    if(b.slot!=int(slot)||b.target==-19||b.candidate==-19||b.enabled!=bool(room._descriptor->AllowedDoors&(1u<<slot))||b.candidate!=room._descriptor->Doors[slot])return Result(owner,false,true,"grid_door_descriptor_conflict");
    if(b.is_null)continue;auto found=owner->entries.find(b.actual);
    if(!b.shape_allowed||found==owner->entries.end()||!found->second->ready||found->second->description.type!=16||!(found->second->description.door_aliases&(1u<<slot)))return Result(owner,false,true,"grid_door_subject_conflict");
    doors[slot]=found->second->grid;
  }
  if(std::equal(doors.begin(),doors.end(),room._doors))return Result(owner,true,false,"grid_door_aliases_unchanged");
  if(owner->prefix==UINT32_MAX)return Result(owner,false,true,"grid_prefix_exhausted");
  ++owner->prefix;owner->written=true;std::copy(doors.begin(),doors.end(),room._doors);return Result(owner,true,false,"grid_door_aliases_installed");
}
ClientGridStep ClientNativeGridsRetire(ClientNativeGrids *owner,const ClientNativeRoots &roots) noexcept {
  if(!owner)return {true,false,false,0,0,"grid_empty"};if(owner->entries.empty()){if(water_owner==owner)water_owner=nullptr;owner->room=nullptr;owner->room_key={};owner->written=false;return Result(owner,true,false,"grid_retired");}
  if(!Window(roots)||owner->uncertain||owner->room!=roots.room)return Result(owner,false,true,"grid_retire_context_or_prefix");
  try{auto i=owner->entries.begin();if(!Remove(*owner,*owner->room,*i->second))return Result(owner,false,true,"grid_retire_uncertain");owner->entries.erase(i);
    auto result=Result(owner,owner->entries.empty(),false,"grid_retire_progress");
    if(result.done){if(water_owner==owner)water_owner=nullptr;owner->room=nullptr;owner->room_key={};owner->written=false;owner->prefix=0;}return result;}
  catch(...){owner->uncertain=true;return Result(owner,false,true,"grid_retire_exception");}
}
bool ClientNativeGridsVisit(const ClientNativeGrids *owner,void *context,ClientGridVisitor visitor) noexcept {
  if(!owner||!visitor||!IsMain()||owner->uncertain||!g_Game||owner->room!=g_Game->_room)return false;
  for(const auto &pair:owner->entries){const auto &e=*pair.second;if(!e.ready||!Owned(*owner,*owner->room,e)||!visitor(context,e.grid,pair.first,e.instance,e.description.type))return false;}return true;
}
void ClientGridWaterClip(void *pointer,void *out) noexcept {
  if(!IsMain()||!ClientBusy()||!pointer||!out||!water_owner||water_owner->uncertain||!g_Game||water_owner->room!=g_Game->_room)return;
  for(const auto &pair:water_owner->entries){const auto &e=*pair.second;
    if(e.grid==pointer&&e.constructed&&!e.release_entered){static_cast<WaterClipInfo *>(out)->bitFlags=e.description.water_clip;return;}}
}
int ClientGridCaptureLua(void *state){
  auto *L=static_cast<lua_State *>(state);if(!IsMain()){lua_pushnil(L);return 1;}
  auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");if(!grid){lua_pushnil(L);return 1;}
  lua_createtable(L,0,6);lua_pushinteger(L,1);lua_setfield(L,-2,"version");lua_pushinteger(L,grid->_spawnFrame);lua_setfield(L,-2,"spawn_frame");
  const auto number=[L](const char *name,float value){if(!std::isfinite(value))luaL_error(L,"grid_runtime_nonfinite");lua_pushnumber(L,value);lua_setfield(L,-2,name);};
  lua_createtable(L,0,3);number("width",g_WIDTH);number("height",g_HEIGHT);
  number("pixel_scale",*reinterpret_cast<const float *>(Base+0x788730)* *reinterpret_cast<const float *>(Base+0x7883e8));lua_setfield(L,-2,"render_inputs");
  if(grid->GetType()==16){lua_pushboolean(L,Read<bool>(grid,0x154));lua_setfield(L,-2,"door_animation_mode");}
  const auto valid=[](const auto &v,unsigned stride){const auto b=reinterpret_cast<std::uintptr_t>(v.begin),e=reinterpret_cast<std::uintptr_t>(v.end),c=reinterpret_cast<std::uintptr_t>(v.capacity);
    return e>=b&&c>=e&&e-b<=4096*stride&&c-b<=65536*stride&&(e-b)%stride==0&&(c-b)%stride==0&&(b||(!e&&!c));};
  if(grid->GetType()==13){lua_pushinteger(L,Read<int>(grid,0x268));lua_setfield(L,-2,"fire_timer");lua_pushinteger(L,Read<int>(grid,0x26c));lua_setfield(L,-2,"fire_style");
    const auto v=Read<NativeArray<ClientGridEmber>>(grid,0x270);if(!valid(v,24))return luaL_error(L,"grid_ember_vector");lua_newtable(L);unsigned i=0;
    for(auto *p=v.begin;p!=v.end;++p){lua_createtable(L,0,6);number("x",p->x);number("y",p->y);number("vx",p->vx);number("vy",p->vy);
      lua_pushinteger(L,p->age);lua_setfield(L,-2,"age");lua_pushinteger(L,p->max_life);lua_setfield(L,-2,"max_life");lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"embers");}
  if(Rock(grid->GetType())){const auto v=Read<NativeArray<ClientPoseVector>>(grid,0x184);if(!valid(v,8))return luaL_error(L,"grid_rubble_vector");lua_newtable(L);unsigned i=0;
    for(auto *p=v.begin;p!=v.end;++p){lua_createtable(L,0,2);number("x",p->x);number("y",p->y);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"rubble");}return 1;
}
int ClientGridSpritesLua(void *state){
  auto *L=static_cast<lua_State *>(state);auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");lua_newtable(L);
  if(IsMain()&&grid&&grid->GetType()==13)for(unsigned i=0;i<2;++i){auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(grid)+(i?0x27c:0x154));
    lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_rawseti(L,-2,i+1);}return 1;
}
} // namespace cs::game
