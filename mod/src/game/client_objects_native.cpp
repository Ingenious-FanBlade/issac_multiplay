#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_objects_native.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <new>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
constexpr unsigned kObjectBudget=512;
// 前缀表示已经进入的真实步骤，异常不能通过重试清除此责任。
enum Prefix : unsigned { Acquired=1,Roster=2,Init=4,Fields=8,Published=16,Resource=32,References=64,Removed=128,Detached=256,Checked=512 };
thread_local Entity_Player *initializing_player=nullptr;
thread_local bool pre_stats_skipped=false;
ClientNativeObjects *diagnostic_owner=nullptr;
bool Phase() noexcept {return IsMain()&&ClientBusy()&&HasOwner(Owner::Manager)&&ContextHealthy();}
struct Entry final {
  Entity *entity=nullptr;
  void *raw=nullptr;
  ClientObjectDescription description{};
  std::uint64_t instance=0;
  std::uintptr_t vptr=0;
  unsigned native_seed=0,native_index=0;
  unsigned prefix=0;
  std::uint64_t draw_diagnostic_revision=~std::uint64_t{0};
  unsigned draw_diagnostic_seen=0,draw_diagnostic_pending=0;
  bool ctor_entered=false,constructed=false,initialized=false,roster=false,retired=false,resource=false,checked=false;
};
bool CurrentUse(const Entry &e) noexcept {
  return e.entity&&!e.retired&&e.entity->_valid&&e.entity->_exists&&
      *reinterpret_cast<const std::uintptr_t *>(e.entity)==e.vptr&&
      e.entity->_initSeed==e.native_seed&&e.entity->_index==e.native_index&&
      e.entity->_type==e.description.type&&e.entity->_variant==e.description.variant&&e.entity->_subtype==e.description.subtype;
}
// 只有源码已解释的代表类别可以进入动态Init，未知类别在第一次写之前明确拒绝。
bool Supported(const ClientObjectDescription &d) noexcept {
  return d.seed&&((d.type==1&&d.variant==0&&d.subtype==0&&d.player_stats)||
      (d.type==1000&&d.variant==57&&d.subtype<=1));
}
bool ListValid(const EntityList_EL &list) noexcept {
  return !list._sublist&&list._size<=list._capacity&&list._capacity<=16384&&(!list._capacity||list._data);
}
unsigned Count(const EntityList_EL &list,const Entity *entity) noexcept {
  unsigned n=0;for(unsigned i=0;i<list._size;++i)n+=list._data[i]==entity;return n;
}
bool MemberSpace(EntityList &list,Entity &e) noexcept {
  auto &owner=e._type==1?list._persistentEL:list._mainEL;
  return ListValid(owner)&&ListValid(list._renderEL)&&ListValid(list._updateEL)&&
      owner._size<owner._capacity&&list._renderEL._size<list._renderEL._capacity&&
      ((e._flags&8)||list._updateEL._size<list._updateEL._capacity);
}
bool MemberMatches(EntityList &list,Entity &e) noexcept {
  if(!ListValid(list._mainEL)||!ListValid(list._persistentEL)||!ListValid(list._renderEL)||!ListValid(list._updateEL))return false;
  const auto main=Count(list._mainEL,&e),persistent=Count(list._persistentEL,&e);
  return main==(e._type==1?0u:1u)&&persistent==(e._type==1?1u:0u)&&Count(list._renderEL,&e)==1&&
      Count(list._updateEL,&e)==((e._flags&8)?0u:1u);
}
void Erase(EntityList_EL &list,Entity *entity) {
  for(unsigned i=list._size;i>0;--i)if(list._data[i-1]==entity)list.RemoveAt(i-1);
}
bool Graphics() noexcept {
  void *dc=nullptr,*context=nullptr;
  return ClientRendererHandles(dc,context)&&dc&&context&&wglGetCurrentDC()==dc&&
      wglGetCurrentContext()==context&&ClientNativeCpuQueuesEmpty();
}
bool Roots(const ClientNativeRoots &r) noexcept {
  auto *g=static_cast<Game *>(r.game);auto *room=static_cast<Room *>(r.room);
  return g&&g==g_Game&&room&&g->_room==room&&r.descriptor&&room->_descriptor==r.descriptor&&r.camera&&room->_Camera==r.camera;
}
bool ValuesMatch(const Entity &e,const ClientObjectDescription &d) noexcept {
  if(e._type!=d.type||e._variant!=d.variant||e._subtype!=d.subtype||!e._valid||!e._exists||e._dead||
     e._pos.x!=d.position.x||e._pos.y!=d.position.y||e._velocity.x!=d.velocity.x||e._velocity.y!=d.velocity.y||
     e._posOffset.x!=d.position_offset.x||e._posOffset.y!=d.position_offset.y||e._visible!=d.visible||
     e._depthOffset!=d.depth||e._size!=d.size||e._mass!=d.mass||e._collisionDamage!=d.collision_damage||
     e._entityCollisionClass!=d.entity_collision||e._entityGridCollisionClass!=d.grid_collision)return false;
  if(d.type==1){const auto &p=static_cast<const Entity_Player &>(e);
    return p._damage==d.damage&&p._movespeed==d.move_speed&&p._maxfiredelay==d.max_fire_delay&&p._shotspeed==d.shot_speed&&
        p._tearrange==d.tear_range&&p._luck==d.luck&&p._redHearts==d.hearts&&p._maxHearts==d.max_hearts&&
        p._soulHearts==d.soul_hearts&&p._blackHearts==d.black_hearts&&p._eternalHearts==d.eternal_hearts&&
        p._numCoins==d.coins&&p._numKeys==d.keys&&p._numBombs==d.bombs;
  }
  return true;
}
void WriteFields(Entity &e,const ClientObjectDescription &d) {
  e._pos=Vector(d.position.x,d.position.y);e._velocity=Vector(d.velocity.x,d.velocity.y);
  e._posOffset=Vector(d.position_offset.x,d.position_offset.y);e._visible=d.visible;
  e._depthOffset=d.depth;e._size=d.size;e._mass=d.mass;e._collisionDamage=d.collision_damage;
  e._entityCollisionClass=d.entity_collision;e._entityGridCollisionClass=d.grid_collision;
  if(d.type==1){auto &p=static_cast<Entity_Player &>(e);
    p._damage=d.damage;p._movespeed=d.move_speed;p._maxfiredelay=d.max_fire_delay;p._shotspeed=d.shot_speed;
    p._tearrange=d.tear_range;p._luck=d.luck;p._redHearts=d.hearts;p._maxHearts=d.max_hearts;
    p._soulHearts=d.soul_hearts;p._blackHearts=d.black_hearts;p._eternalHearts=d.eternal_hearts;
    p._numCoins=d.coins;p._numKeys=d.keys;p._numBombs=d.bombs;
  }
}
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
  lua_rawgetp(L,LUA_REGISTRYINDEX,&essm_delete_registry_key);
  if(lua_istable(L,-1)){
    // 同VM重复调用只核对；已有未知值或不同owner/闭包绝不通过覆盖恢复资格。
    const int snapshot=lua_absindex(L,-1);RawField(L,snapshot,"owner");
    const bool same_owner=lua_rawequal(L,owner,-1);lua_pop(L,1);
    RawField(L,snapshot,"callback");
    lua_pushboolean(L,same_owner&&lua_rawequal(L,callback,-1));return 1;
  }
  if(!lua_isnil(L,-1)){lua_pushboolean(L,0);return 1;}
  lua_pop(L,1);lua_createtable(L,0,2);const int snapshot=lua_absindex(L,-1);
  lua_pushliteral(L,"owner");lua_pushvalue(L,owner);lua_rawset(L,snapshot);
  lua_pushliteral(L,"callback");lua_pushvalue(L,callback);lua_rawset(L,snapshot);
  lua_pushvalue(L,snapshot);lua_rawsetp(L,LUA_REGISTRYINDEX,&essm_delete_registry_key);
  lua_pushboolean(L,1);return 1;
}
int UseEssmDeleteThunk(lua_State *L) {
  const bool execute=lua_toboolean(L,2)!=0;
  lua_rawgetp(L,LUA_REGISTRYINDEX,&essm_delete_registry_key);
  if(!lua_istable(L,-1)){lua_pushinteger(L,EssmBindingMissing);return 1;}
  const int snapshot=lua_absindex(L,-1);RawField(L,snapshot,"owner");
  if(!lua_istable(L,-1)){lua_pushinteger(L,EssmBindingChanged);return 1;}
  RawField(L,-1,"DeleteEntity");RawField(L,snapshot,"callback");
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
bool EssmDelete(Entity *entity,bool execute,const char *&reason) noexcept {
  if(!IsMain()||!g_LuaEngine||!g_LuaEngine->_state||ClientLuaStateIdentity()!=g_LuaEngine->_state){
    reason="objects_essm_delete_vm_unavailable";return false;
  }
  auto *L=g_LuaEngine->_state;const int top=lua_gettop(L);
  if(!lua_checkstack(L,12)){reason="objects_essm_delete_stack_budget";return false;}
  lua_pushcfunction(L,UseEssmDeleteThunk);
  lua_pushinteger(L,g_LuaEngine->GetMaskedPointer(reinterpret_cast<uintptr_t>(entity)));
  lua_pushboolean(L,execute);
  const bool called=lua_pcall(L,2,1,0)==LUA_OK;
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
// 所有分配/字段读取都在pcall内，正常/错误均恢复原栈；失败由调用者撤销Client准备资格。
bool ClientObjectSealEssmDeleteBinding(void *state) noexcept {
  if(!IsMain()||ClientBusy()||!state||!g_LuaEngine||state!=g_LuaEngine->_state||ClientLuaStateIdentity()!=state)return false;
  auto *L=static_cast<lua_State *>(state);const int top=lua_gettop(L);
  if(!lua_checkstack(L,12))return false;
  lua_pushcfunction(L,SealEssmDeleteThunk);
  const bool sealed=lua_pcall(L,0,1,0)==LUA_OK&&lua_toboolean(L,-1)!=0;
  lua_settop(L,top);return sealed;
}
// 映射拥有身份/阶段，不拥有Game原生内存；实际实体只由各自原生owner释放。
struct ClientNativeObjects final {
  std::map<ClientObjectKey,Entry> entries;
  Game *game=nullptr;Room *room=nullptr;
  std::uint64_t next_instance=0;
  std::uint64_t mutations=0;
  unsigned prefix=0;
  bool executing=false,uncertain=false,committed=false,abandoned=false,index_initialized=false;
};
namespace {
void Mark(ClientNativeObjects &o,unsigned prefix) noexcept {o.prefix|=prefix;++o.mutations;}
ClientObjectStep Result(ClientNativeObjects *o,bool done,bool failed,const char *reason,bool written=false) noexcept {
  unsigned pending=0;
  if(o)for(const auto &pair:o->entries)if(!pair.second.retired&&pair.second.entity)++pending;
  return {done,written,failed,o?o->prefix:0,pending,reason};
}
bool Current(ClientNativeObjects &o,const ClientNativeRoots &r) noexcept {
  if(!Roots(r))return false;
  if(!o.game){o.game=static_cast<Game *>(r.game);o.room=static_cast<Room *>(r.room);}
  return o.game==r.game&&o.room==r.room;
}
bool UniqueAddress(const ClientNativeObjects &o,const Entity *entity,const ClientObjectKey &key) noexcept {
  for(const auto &pair:o.entries)if(!(pair.first==key)&&pair.second.entity==entity&&!pair.second.retired)return false;
  return true;
}
// 413D00已核__thiscall(EntityList*,Entity*)，内部更新真实owner/render/update成员及类型查询缓存。
bool Publish(ClientNativeObjects &o,Entry &entry) {
  auto &e=*entry.entity;auto &list=o.room->_entityList;
  if(e._valid||!MemberSpace(list,e))return false;
  auto &index=*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(o.room)+0x1214);
  if(index==UINT32_MAX)return false;e._index=index++;
  entry.prefix|=Published;Mark(o,Published);
  reinterpret_cast<void (__thiscall *)(EntityList *,Entity *)>(Base+0x13d00)(&list,&e);
  if(!e._valid||!MemberMatches(list,e))return false;
  entry.native_index=e._index;return true;
}
bool Create(ClientNativeObjects &o,const ClientObjectDescription &d,const char *&reason) {
  reason="objects_create_unsupported";if(!Supported(d)||!Graphics()||o.entries.size()>=kObjectBudget||o.next_instance==UINT64_MAX)return false;
  auto found=o.entries.find(d.key);
  if(found!=o.entries.end()&&!found->second.retired){reason="objects_create_duplicate_or_partial";return false;}
  auto &entry=o.entries[d.key];entry={};entry.description=d;entry.instance=++o.next_instance;
  if(d.type==1){
    if(!ClientObjectPreStatsIsolationInstalled()){reason="objects_player_pre_isolation_uninstalled";return false;}
    auto &roster=o.game->_playerManager._playerList;
    if(!roster.empty()){reason="objects_main_player_roster_not_empty";return false;}
    if(!o.index_initialized){
      auto &list=o.room->_entityList;
      if(!ListValid(list._mainEL)||!ListValid(list._persistentEL)||!ListValid(list._renderEL)||
         list._mainEL._size||list._persistentEL._size||list._renderEL._size){reason="objects_initial_room_members_not_empty";return false;}
      // Room ctor未初始化+1214；普通Start前缀之外由本场景初始化本地Index分配器。
      // Index是原生成员地址空间，不能复制服务器Index，也不能直接使用wire key。
      Mark(o,Fields);*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(o.room)+0x1214)=0;
      o.index_initialized=true;
    }
    entry.prefix|=Acquired;Mark(o,Acquired);
    // 与98F63E相同的游戏分配器，避免跨CRT分配再交给native Free。
    entry.raw=reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(sizeof(Entity_Player));
    if(!entry.raw){reason="objects_player_allocation";return false;}
    entry.entity=static_cast<Entity_Player *>(entry.raw);entry.ctor_entered=true;
    static_cast<Entity_Player *>(entry.entity)->constructor();entry.constructed=true;
    auto *player=static_cast<Entity_Player *>(entry.entity);player->_playerIndex=0;player->_controllerIndex=-1;
    // 原生Pre会查询当前roster；先由PM持有，再派生Init，Room成员最后才发布。
    entry.prefix|=Roster;Mark(o,Roster);roster.push_back(player);entry.roster=true;
    entry.prefix|=Init;Mark(o,Init);initializing_player=player;pre_stats_skipped=false;
    try{player->Init(d.type,d.variant,d.subtype,d.seed);}catch(...){initializing_player=nullptr;throw;}
    initializing_player=nullptr;entry.initialized=true;
    if(!pre_stats_skipped){reason="objects_player_pre_isolation_not_observed";return false;}
  }else {
    if(!o.game->_entityFactory){reason="objects_factory_missing";return false;}
    entry.prefix|=Acquired;Mark(o,Acquired);
    // true对应J273池不足返回null，绝不覆盖仍valid的其它服务器key。
    entry.entity=o.game->_entityFactory->Create(d.type,true);
    if(!entry.entity){reason="objects_pool_exhausted";return false;}
    if(entry.entity->_valid||!UniqueAddress(o,entry.entity,d.key)){reason="objects_pool_use_conflict";return false;}
    entry.constructed=true;
    entry.prefix|=Init;Mark(o,Init);entry.entity->Init(d.type,d.variant,d.subtype,d.seed);entry.initialized=true;
  }
  auto &e=*entry.entity;
  if(!e._exists||e._dead||e._type!=d.type||e._variant!=d.variant||e._subtype!=d.subtype){reason="objects_init_postcondition";return false;}
  entry.vptr=*reinterpret_cast<const std::uintptr_t *>(&e);entry.native_seed=e._initSeed;
  entry.prefix|=Fields;Mark(o,Fields);WriteFields(e,d);
  entry.prefix|=Resource;Mark(o,Resource);
  const auto resource=ClientApplyResourcePose(&e._sprite,d.resource);
  if(!resource.complete){reason=resource.reason;return false;}entry.resource=true;
  if(!Publish(o,entry)){reason="objects_publish_postcondition";return false;}
  // 此处不调用PostLevelInitStats/EvaluateItems，也不从本地解锁/ItemPool补服务器目标。
  reason="objects_created";return true;
}
bool Remove(ClientNativeObjects &o,Entry &entry,const char *&reason) {
  if(entry.retired)return true;
  auto *e=entry.entity;
  if(!CurrentUse(entry)||e->_type!=1000||e->_variant!=57||!MemberMatches(o.room->_entityList,*e)){
    reason="objects_remove_instance_invalid";return false;
  }
  // 基ClearReferences会Remove +310附属对象；所选Effect实例必须确实无该未分类owner。
  if(*reinterpret_cast<Entity **>(reinterpret_cast<unsigned char *>(e)+0x310)){
    reason="objects_remove_auxiliary_owner_unknown";return false;
  }
  if(!EssmDelete(e,false,reason))return false;
  entry.checked=false;entry.prefix|=Removed;Mark(o,Removed);e->Remove();
  if(e->_exists){reason="objects_remove_not_logical";return false;}
  auto &list=o.room->_entityList;
  for(auto *el:{&list._mainEL,&list._persistentEL,&list._updateEL,&list._renderEL,&list._effectEL,&list._tempEL,&list._wispEL})
    if(!ListValid(*el)){reason="objects_remove_member_invalid";return false;}
  entry.prefix|=Detached;Mark(o,Detached);
  for(auto *el:{&list._updateEL,&list._renderEL,&list._effectEL,&list._tempEL,&list._wispEL,&list._mainEL,&list._persistentEL})Erase(*el,e);
  if(!EssmDelete(e,true,reason))return false;
  e->ClearReferences();e->_valid=false;
  // 类型查询缓存有自己的原生owner，不能只从renderEL删地址便宣称退休。
  reinterpret_cast<void (__thiscall *)(EntityList *,int,int,int)>(Base+0x16d10)(&list,int(e->_type),int(e->_variant),int(e->_subtype));
  entry.retired=true;entry.entity=nullptr;entry.raw=nullptr;entry.resource=false;
  reason="objects_removed_pool_slot_reusable";return true;
}
bool FixReferences(ClientNativeObjects &o,Entry &entry,const ClientObjectDescription &d,const char *&reason) {
  if(!CurrentUse(entry)){reason="objects_reference_source_invalid";return false;}
  Entity *values[4]{};
  for(unsigned n=0;n<4;++n)if(!d.references[n].is_null){
    const auto found=o.entries.find(d.references[n].target);
    if(found==o.entries.end()||!CurrentUse(found->second)){reason="objects_reference_target_missing";return false;}
    values[n]=found->second.entity;
  }
  auto &e=*entry.entity;
  const bool changed=e.GetParent()!=values[0]||e.GetChild()!=values[1]||e.GetTarget()!=values[2]||e.GetSpawnerEntity()!=values[3];
  if(changed){entry.prefix|=References;Mark(o,References);entry.checked=false;}
  // 用EntityPtr原生登记反向引用，不直接覆写四个指针。
  if(e.GetParent()!=values[0])e.SetParent(values[0]);if(e.GetChild()!=values[1])e.SetChild(values[1]);
  if(e.GetTarget()!=values[2])e.SetTarget(values[2]);if(e.GetSpawnerEntity()!=values[3])e.SetSpawnerEntity(values[3]);
  entry.description=d;reason="objects_references_bound";return true;
}
bool ReferencesMatch(ClientNativeObjects &o,Entry &entry,const ClientObjectDescription &d) {
  Entity *actual[]={entry.entity->GetParent(),entry.entity->GetChild(),entry.entity->GetTarget(),entry.entity->GetSpawnerEntity()};
  for(unsigned n=0;n<4;++n){Entity *expected=nullptr;
    if(!d.references[n].is_null){const auto found=o.entries.find(d.references[n].target);if(found==o.entries.end()||!CurrentUse(found->second))return false;expected=found->second.entity;}
    if(actual[n]!=expected)return false;
  }
  return true;
}
bool CheckAll(ClientNativeObjects &o,const char *&reason) {
  unsigned players=0;
  for(const auto &pair:o.entries)if(!pair.second.retired){
    const auto &e=pair.second;if(!CurrentUse(e)||!e.checked){reason="objects_commit_unchecked";return false;}players+=e.entity->_type==1;
  }
  if(players!=1||o.game->_playerManager._playerList.size()!=1){reason="objects_commit_roster";return false;}
  const auto *player=o.game->_playerManager._playerList[0];bool player_known=false;
  for(const auto &pair:o.entries)if(!pair.second.retired&&pair.second.entity==player)player_known=true;
  if(!player_known){reason="objects_commit_unmapped_player";return false;}
  auto &list=o.room->_entityList;
  for(auto *el:{&list._mainEL,&list._persistentEL,&list._updateEL,&list._renderEL,&list._effectEL,&list._tempEL,&list._wispEL}){
    if(!ListValid(*el)){reason="objects_commit_list_invalid";return false;}
    for(unsigned n=0;n<el->_size;++n){bool found=false;
      for(const auto &pair:o.entries)if(!pair.second.retired&&pair.second.entity==el->_data[n]){found=true;break;}
      if(!found){reason="objects_commit_unclassified_native_member";return false;}
    }
  }
  o.committed=true;diagnostic_owner=&o;reason="objects_committed";return true;
}
}
ClientNativeObjects *ClientNativeObjectsCreate() noexcept {return Phase()?new(std::nothrow) ClientNativeObjects:nullptr;}
void ClientNativeObjectsDestroy(ClientNativeObjects *o) noexcept {
  if(o&&IsMain()&&(o->entries.empty()||o->abandoned)){
    if(diagnostic_owner==o)diagnostic_owner=nullptr;delete o;
  }
}
ClientObjectStep ClientNativeObjectsExecute(ClientNativeObjects *o,const ClientNativeRoots &roots,const ClientObjectCommand &command) noexcept {
  if(!o||!Phase()||!Current(*o,roots))return Result(o,false,true,"objects_context_or_roots");
  if(o->uncertain||o->executing||o->abandoned)return Result(o,false,true,"objects_owner_uncertain_or_reentrant");
  if(!Graphics())return Result(o,false,false,"objects_graphics_or_cpu_inflight");
  // 具名原生应用根只覆盖本次已解释的primitive链；任何嵌套Gameplay Update仍由I3拒绝。
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)
    return Result(o,false,true,"objects_dispatch_admission");
  const bool read_only=command.operation==ClientObjectOperation::Check;
  const auto mutations=o->mutations;
  o->executing=true;const char *reason="objects_operation_failed";bool ok=false;
  if(!read_only)o->committed=false;
  try {
    const auto &d=command.value;auto found=o->entries.find(d.key);
    switch(command.operation){
      case ClientObjectOperation::Create:ok=Create(*o,d,reason);break;
      case ClientObjectOperation::Remove:
        if(found==o->entries.end()){reason="objects_remove_missing_key";break;}ok=Remove(*o,found->second,reason);break;
      case ClientObjectOperation::Replace:
        if(found==o->entries.end()){reason="objects_replace_missing_key";break;}
        ok=Remove(*o,found->second,reason)&&Create(*o,d,reason);break;
      case ClientObjectOperation::Change:
        if(found==o->entries.end()||!CurrentUse(found->second)){reason="objects_change_missing_key";break;}
        if(!Supported(d)||!MemberMatches(o->room->_entityList,*found->second.entity)){reason="objects_change_instance_invalid";break;}
        found->second.checked=false;ok=true;
        if(command.fields){found->second.prefix|=Fields;Mark(*o,Fields);WriteFields(*found->second.entity,d);}
        if(command.resource){
          found->second.prefix|=Resource;Mark(*o,Resource);
          const auto resource=ClientApplyResourcePose(&found->second.entity->_sprite,d.resource);ok=resource.complete;reason=resource.reason;
          if(ok)found->second.resource=true;
        }
        if(ok){found->second.description=d;reason="objects_changed";}break;
      case ClientObjectOperation::References:
        if(found==o->entries.end()){reason="objects_reference_missing_key";break;}ok=FixReferences(*o,found->second,d,reason);break;
      case ClientObjectOperation::Check:
        if(!d.key.id){ok=CheckAll(*o,reason);break;}
        if(found==o->entries.end()||!CurrentUse(found->second)){reason="objects_check_missing_key";break;}
        ok=ValuesMatch(*found->second.entity,d)&&MemberMatches(o->room->_entityList,*found->second.entity)&&
            ReferencesMatch(*o,found->second,d)&&ClientResourcePoseMatches(&found->second.entity->_sprite,d.resource);
        reason=ok?"objects_checked":"objects_target_postcondition";
        if(ok){found->second.checked=true;found->second.prefix|=Checked;o->prefix|=Checked;}break;
    }
  }catch(...){reason="objects_native_exception";initializing_player=nullptr;}
  ClientDispatchReturned(ClientEntry::NativeApply);
  if(!dispatch.Succeeded()){ok=false;reason="objects_dispatch_incomplete";}
  o->executing=false;if(!ok){o->uncertain=true;o->committed=false;}
  auto result=Result(o,ok,!ok,reason,o->mutations!=mutations);result.key=command.value.key;
  const auto actual=o->entries.find(command.value.key);
  if(actual!=o->entries.end()){result.instance=actual->second.instance;result.object_prefix=actual->second.prefix;}
  return result;
}
ClientObjectStep ClientNativeObjectsAbandon(ClientNativeObjects *o,const ClientNativeRoots &roots) noexcept {
  if(!o)return {true,false,false,0,0,"objects_no_owner"};
  if(!Phase()||o->executing)return Result(o,false,true,"objects_retire_context");
  if(o->abandoned)return Result(o,true,false,"objects_already_abandoned");
  if(!Current(*o,roots))return Result(o,false,true,"objects_retire_roots_changed");
  bool partial_pool=false;
  for(const auto &pair:o->entries){const auto &e=pair.second;
    // 首Player尚未交PM所有时，Room重建不能处理该半构造owner，不可丢记录。
    if(e.description.type==1&&e.entity&&!e.roster)return Result(o,false,true,"objects_retire_unpublished_player_owner");
    if(e.entity&&!e.retired&&!e.initialized)return Result(o,false,true,"objects_retire_incomplete_derived_init");
    if(e.entity&&!e.retired&&(e.prefix&Published)&&!CurrentUse(e))return Result(o,false,true,"objects_retire_instance_changed");
    if(e.entity&&!e.retired&&e.description.type!=1&&!(e.prefix&Published))partial_pool=true;
  }
  // 已取得并完成Init、但资源/Publish前失败的池use没有Room owner；必须在撤map前单独完成生命周期。
  if(partial_pool){
    if(!Graphics())return Result(o,false,false,"objects_retire_partial_pool_context");
    ClientDispatchScope dispatch(ClientOperation::Retire,ClientEntry::NativeTeardown);
    if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeTeardown)!=ClientEntryDecision::CallOriginal)
      return Result(o,false,true,"objects_retire_partial_pool_dispatch");
    bool ok=true;const char *reason="objects_retire_partial_pool_failed";
    try {
      for(auto &pair:o->entries){auto &entry=pair.second;auto *e=entry.entity;
        if(!e||entry.retired||entry.description.type==1||(entry.prefix&Published))continue;
        if((entry.prefix&Removed)||e->_valid||e->_type!=1000||e->_variant!=57){
          ok=false;reason="objects_retire_partial_pool_unknown";break;
        }
        if(*reinterpret_cast<Entity **>(reinterpret_cast<unsigned char *>(e)+0x310)){
          ok=false;reason="objects_retire_partial_pool_auxiliary_owner_unknown";break;
        }
        if(!EssmDelete(e,false,reason)){ok=false;break;}
        entry.prefix|=Removed;Mark(*o,Removed);if(e->_exists)e->Remove();
        if(e->_exists){ok=false;reason="objects_retire_partial_pool_remove_not_logical";break;}
        if(!EssmDelete(e,true,reason)){ok=false;break;}
        e->ClearReferences();e->_valid=false;entry.retired=true;entry.entity=nullptr;entry.raw=nullptr;
      }
    }catch(...){ok=false;reason="objects_retire_partial_pool_exception";}
    ClientDispatchReturned(ClientEntry::NativeTeardown);
    if(!dispatch.Succeeded()){ok=false;reason="objects_retire_partial_pool_dispatch_incomplete";}
    if(!ok)return Result(o,false,true,reason,true);
  }
  o->committed=false;o->abandoned=true;if(diagnostic_owner==o)diagnostic_owner=nullptr;
  const auto prefix=o->prefix;o->entries.clear();return {true,false,false,prefix,0,"objects_mappings_retired_native_world_owns_cleanup"};
}
bool ClientNativeObjectsVisit(const ClientNativeObjects *o,void *context,ClientObjectVisitor visitor) noexcept {
  if(!o||!visitor||!IsMain()||!o->committed||o->uncertain||o->executing||o->abandoned||g_Game!=o->game||g_Game->_room!=o->room)return false;
  for(const auto &pair:o->entries){const auto &e=pair.second;
    if(e.retired)continue;
    if(!e.checked||!CurrentUse(e)||!visitor(context,e.entity,pair.first,e.instance,e.entity->_type))return false;
  }
  return true;
}
// SDK的RenderBody/Head只读旁观：先以本方稳定map验证当前use，再读取正在调用的参数。
// 每个实际目标的Body/Head各一组前后记录；同值重复提交不产生新的原生写代次或逐帧日志。
void ClientObjectPlayerDrawDiagnostic(void *pointer,const void *position,bool body,bool after) noexcept {
  if(!Settings.diagnostics||!IsMain()||!ClientBusy()||!HasOwner(Owner::Render)||!ContextHealthy()||
     !pointer||!position||!__ptr_g_KAGE_Graphics_ImageManager||!__ptr_g_KAGE_Graphics_Manager)return;
  auto *o=diagnostic_owner;
  if(!o||!o->committed||o->executing||o->uncertain||o->abandoned||o->game!=g_Game||!g_Game||g_Game->_room!=o->room)return;
  Entry *entry=nullptr;
  for(auto &pair:o->entries)if(pair.second.entity==pointer){entry=&pair.second;break;}
  if(!entry||!entry->checked||entry->description.type!=1||!CurrentUse(*entry))return;
  const auto &player=*static_cast<const Entity_Player *>(pointer);const auto &sprite=player._sprite;
  if(!sprite._layerState||sprite._layerCount>64)return;
  if(entry->draw_diagnostic_revision!=o->mutations){
    entry->draw_diagnostic_revision=o->mutations;entry->draw_diagnostic_seen=entry->draw_diagnostic_pending=0;
  }
  const unsigned bit=body?1u:2u;
  if(after){if(!(entry->draw_diagnostic_pending&bit))return;entry->draw_diagnostic_pending&=~bit;}
  else {if(entry->draw_diagnostic_seen&bit)return;entry->draw_diagnostic_seen|=bit;entry->draw_diagnostic_pending|=bit;}
  const auto &point=*static_cast<const Vector *>(position);
  const auto &images=*__ptr_g_KAGE_Graphics_ImageManager;const auto &graphics=*__ptr_g_KAGE_Graphics_Manager;
  unsigned char hidden[2]{};
  std::memcpy(&hidden[0],reinterpret_cast<const unsigned char *>(pointer)+0x1398,1);
  std::memcpy(&hidden[1],reinterpret_cast<const unsigned char *>(pointer)+0x139a,1);
  int skin[2]{};std::memcpy(skin,reinterpret_cast<const unsigned char *>(pointer)+0x1880,sizeof(skin));
  const float override_alpha=g_ColorOverrideStack.empty()?1.0f:g_ColorOverrideStack.back()._tint[3];
  Trace("client_objects_player_draw","part=%s after=%u instance=%llu revision=%llu pos=%.4f,%.4f visible=%u hidden1398=%u hidden139a=%u skin1880=%d skin1884=%d main=%s overlay=%s alpha=%.4f flags=%u scale=%.4f,%.4f offset=%.4f,%.4f override_count=%u override_alpha=%.4f light=%u shader=%p target=%p ordinary=%u transparent=%u",
      body?"body":"head",after,static_cast<unsigned long long>(entry->instance),static_cast<unsigned long long>(o->mutations),
      point.x,point.y,player._visible,hidden[0],hidden[1],skin[0],skin[1],
      sprite._animState._animData?sprite._animState._animData->_name.c_str():"",
      sprite._overlayAnimState._animData?sprite._overlayAnimState._animData->_name.c_str():"",
      sprite._color._tint[3],sprite._bitflags,sprite._scale.x,sprite._scale.y,sprite._offset.x,sprite._offset.y,
      unsigned(g_ColorOverrideStack.size()),override_alpha,g_ANM2_LightRendering,graphics._shader,
      graphics.currentRenderTarget,unsigned(images._frameImages.size()),unsigned(images._transparentBatches.size()));
  const unsigned first=body?1u:4u,last=body?3u:10u;
  for(unsigned id=first;id<=last&&id<sprite._layerCount;++id){
    const auto &state=sprite._layerState[id];const auto *image=state._spriteSheet.image;
    const auto batch_live=[&](const KAGE_Graphics_RenderBatch *batch,bool transparent) noexcept {
      if(!image||!batch)return false;
      if(transparent){
        if(images._transparentBatches.size()>4096)return false;
        for(const auto &queued:images._transparentBatches)if(queued.first==batch&&queued.second==image)return true;
        return false;
      }
      if(images._frameImages.size()>4096||image->_renderBatches.size()>4096)return false;
      bool queued=false;for(const auto *current:images._frameImages)if(current==image){queued=true;break;}
      if(queued)for(const auto *current:image->_renderBatches)if(current==batch)return true;
      return false;
    };
    const auto batch_size=[&](const KAGE_Graphics_RenderBatch *batch,bool transparent,bool index) noexcept {
      // last指针本身可能是前帧缓存；必须先在本帧仍在途队列及所属image容器找到，才能解引用。
      if(!batch_live(batch,transparent))return 0u;const auto &buffer=index?batch->_indexBuffer:batch->_vertexBuffer;
      // 官方布局只解释了单buffer；未知active索引只输出0，不为诊断越界遍历。
      return buffer.container&&buffer.activeBufferIdx==0?buffer.container[0].size:0u;
    };
    const auto *opaque=image?image->_lastRenderBatch:nullptr,*transparent=image?image->_lastTransparentRenderBatch:nullptr;
    Trace("client_objects_player_layer","part=%s after=%u id=%u visible=%u flags=%u alpha=%.4f size=%.4f,%.4f pos=%.4f,%.4f image=%p opaque=%p opaque_live=%u vertices=%u indices=%u transparent=%p transparent_live=%u transparent_vertices=%u transparent_indices=%u",
        body?"body":"head",after,id,state._visible,state._bitflags,state._color._tint[3],state._size.x,state._size.y,state._pos.x,state._pos.y,image,
        opaque,batch_live(opaque,false),batch_size(opaque,false,false),batch_size(opaque,false,true),transparent,batch_live(transparent,true),batch_size(transparent,true,false),batch_size(transparent,true,true));
  }
}
bool ClientObjectSuppressPlayerPreStats(void *player) noexcept {
  if(!Phase()||!player||player!=initializing_player)return false;pre_stats_skipped=true;return true;
}
bool ClientObjectSkipPlayerPreStats() noexcept {return ClientObjectSuppressPlayerPreStats(initializing_player);}
} // namespace cs::game
