#include "IsaacRepentance.h"
#include "game/client_inspect_value.h"
#include "binding/world_layout_constants.h"
#include "game/client_players_native.h"
#include "game/client_dto_memory.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_player_roles.h"
#include "game/client_entity_draw_native.h"
#include "game/client_player_isolation.h"
#include "game/client_player_e2e.h"
#ifdef ISAACCS_E2E
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#endif
#include "replica/player_limits.h"
#include "game/context.h"
#include "game/bridge.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <set>
#include <cstdio>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
// 进入前登记前缀；任何异常均不重入构造/Init/Free，也不把半写入候选升级为A。
enum Prefix : unsigned {
  Raw=unsigned(ClientPlayerPrefix::Raw),CtorEntered=unsigned(ClientPlayerPrefix::CtorEntered),Constructed=unsigned(ClientPlayerPrefix::Constructed),
  InitEntered=unsigned(ClientPlayerPrefix::InitEntered),Initialized=unsigned(ClientPlayerPrefix::Initialized),State=unsigned(ClientPlayerPrefix::State),
  Resources=unsigned(ClientPlayerPrefix::Resources),Roster=unsigned(ClientPlayerPrefix::Roster),Published=unsigned(ClientPlayerPrefix::Published),
  Linked=unsigned(ClientPlayerPrefix::Linked),Checked=unsigned(ClientPlayerPrefix::Checked),FreeEntered=unsigned(ClientPlayerPrefix::FreeEntered)
};
struct Entry final {
  ClientPlayerDescription description;
  Entity_Player *player=nullptr;
  ClientPlayerEquipmentOwner *equipment=nullptr;
  ClientPlayerHudOwner *hud=nullptr;
  std::uint64_t instance=0;
  std::uintptr_t vptr=0;
  unsigned seed=0,index=0,prefix=0,ready=0;
  bool retired=false;
};
struct Token final {ClientNativePlayers *owner=nullptr;};
std::uint64_t next_player_instance=0,next_player_epoch=0;
bool Phase() noexcept {return IsMain()&&ClientBusy()&&HasOwner(Owner::Manager)&&ContextHealthy();}
bool Graphics() noexcept {void *dc=nullptr,*rc=nullptr;return ClientRendererHandles(dc,rc)&&dc&&rc&&wglGetCurrentDC()==dc&&wglGetCurrentContext()==rc&&ClientNativeCpuQueuesEmpty();}
bool ListValid(const EntityList_EL &l){return !l._sublist&&l._size<=l._capacity&&l._capacity<=16384&&(!l._capacity||l._data);}
unsigned Count(const EntityList_EL &l,const Entity *e){unsigned n=0;for(unsigned i=0;i<l._size;++i)n+=l._data[i]==e;return n;}
void Erase(EntityList_EL &l,Entity *e){for(unsigned i=l._size;i>0;--i)if(l._data[i-1]==e)l.RemoveAt(i-1);}
bool SameRoots(const ClientNativeRoots &r){return g_Game&&r.game==g_Game&&r.room==g_Game->_room&&r.room&&r.descriptor==g_Game->_room->_descriptor&&r.camera==g_Game->_room->_Camera;}
}
// PlayerManager roster只借用本方对象；备份槽仅在本owner纯值表里关联，不写原生全局槽。
struct ClientNativePlayers final {
  std::map<ClientObjectKey,Entry> entries;
  std::vector<ClientPlayerDescription> desired;
  std::array<ClientObjectReference,4> backups{};
  // 自有镜像槽只借本owner实例；不覆盖Authority PlayerManager的四个持久原生槽。
  std::array<Entity_Player *,4> backup_players{};
  ClientNativeRoots roots{};
  ClientPlayerAdapter adapter;
  std::shared_ptr<Token> token;
  std::uint64_t epoch=0,scope=0,revision=0,committed_revision=0;
  unsigned prefix=0;
  std::size_t cursor=0,preparation_cursor=0,finish_cursor=0,unlink_cursor=0;
  bool busy=false,fault=false,structure_ready=false,sets_ready=false,links_ready=false,ready=false,retiring=false,unlinked=false,roster_removed=false;
  bool retirement_fault=false;
  std::array<char,128> retirement_reason{};
  ClientObjectKey last_key{};
  ClientObjectKey dependency{};
  std::array<char,256> field{};
  const char *phase="begin";
};
namespace {
ClientObjectStep Result(ClientNativePlayers *o,bool done,bool failed,const char *why,bool written=false){
  unsigned pending=0;if(o)for(const auto &pair:o->entries)pending+=pair.second.player&&!pair.second.retired;
  ClientObjectStep result{done,written,failed,o?o->prefix:0,pending,why};
  result.domain="player";
  if(o)result.phase=o->phase;
  if(o)result.object_prefix=o->prefix;
  if(o){result.key=o->last_key;if(failed){result.dependency=o->dependency;result.field=o->field;}const auto it=o->entries.find(o->last_key);if(it!=o->entries.end()){result.instance=it->second.instance;result.shape_revision=it->second.description.entity.shape_revision;result.object_prefix=it->second.prefix;result.body_role=it->second.description.body_role;result.player_type=it->second.description.player_type;}}
  return result;
}
bool Current(const ClientNativePlayers &o,const Entry &e){
  const auto *p=e.player;return p&&!e.retired&&(e.prefix&Initialized)&&*reinterpret_cast<const std::uintptr_t *>(p)==e.vptr&&
      p->_initSeed==e.seed&&p->_type==1&&p->_variant==e.description.entity.variant&&p->_subtype==e.description.entity.subtype&&p->_playerType==e.description.player_type&&
      (!(e.prefix&Published)||(p->_valid&&p->_index==e.index))&&SameRoots(o.roots);
}
bool ValidateSets(ClientNativePlayers &o,const char *&reason){
  if(!SameRoots(o.roots)){reason="players_roots_changed";return false;}
  auto &roster=static_cast<Game *>(o.roots.game)->_playerManager._playerList;
  std::vector<Entity_Player *> expected;
  for(const auto &d:o.desired)if(d.roster_index>=0){if(unsigned(d.roster_index)>=o.desired.size()){reason="players_roster_range";return false;}}
  for(unsigned i=0;i<o.desired.size();++i)for(const auto &d:o.desired)if(d.roster_index==int(i)){
    auto found=o.entries.find(d.entity.key);if(found==o.entries.end()||!Current(o,found->second)){reason="players_roster_use";return false;}expected.push_back(found->second.player);}
  if(expected.size()!=roster.size()||!std::equal(expected.begin(),expected.end(),roster.begin())){reason="players_roster_order";return false;}
  auto &list=static_cast<Room *>(o.roots.room)->_entityList;
  for(unsigned i=0;i<4;++i){const auto &r=o.backups[i];Entity_Player *expected=nullptr;
    if(!r.present){reason="players_four_backup_slots_required";return false;}
    if(!r.is_null){const auto entry=o.entries.find(r.target);if(entry==o.entries.end()||!Current(o,entry->second)){reason="players_backup_instance_not_current";return false;}expected=entry->second.player;}
    if(o.backup_players[i]!=expected){reason="players_backup_slot_instance_postcondition";return false;}
  }
  for(auto *l:{&list._mainEL,&list._persistentEL,&list._renderEL,&list._updateEL})if(!ListValid(*l)){reason="players_list_storage";return false;}
  for(const auto &pair:o.entries){const auto &e=pair.second;if(!Current(o,e)){reason="players_instance_or_shape_changed";return false;}
    const auto &d=e.description;
    if(!d.role||e.player->_playerIndex!=d.role->player_index||(d.roster_index>=0&&e.player->_playerIndex!=d.roster_index)){reason="players_local_roster_index_mismatch";return false;}
    if(e.player->_valid!=d.native_valid||Count(list._mainEL,e.player)||Count(list._persistentEL,e.player)!=unsigned(d.room_member)||Count(list._renderEL,e.player)!=unsigned(d.render_member)||Count(list._updateEL,e.player)!=unsigned(d.update_member)){
      reason="players_native_membership_mismatch";return false;}
  }return true;
}
bool Borrow(void *context,const ClientNativeRoots &roots,const ClientObjectKey &key,unsigned expected,ClientEntityBorrow &out,const char *&reason) noexcept {
  auto *token=static_cast<Token *>(context);auto *o=token?token->owner:nullptr;
  if(!o||!IsMain()||!ClientPlayerBorrowAllowed()||o->fault||o->retiring||o->busy||!SameRoots(roots)||roots.room!=o->roots.room||(expected!=0&&expected!=13)){reason="players_borrow_context_or_retired";return false;}
  const auto hit=o->entries.find(key);if(hit==o->entries.end()||!(hit->second.ready&unsigned(ClientPlayerUse::Structure))||!Current(*o,hit->second)){
    reason="players_key_or_current_use_not_ready";return false;}
  const auto &e=hit->second;out={e.player,e.instance,o->epoch,0};reason="players_borrowed_current_structure";return true;
}
bool Witness(void *context,const ClientNativeRoots &roots,const ClientObjectKey &key,const ClientEntityBorrow &b,const char *&reason) noexcept {
  ClientEntityBorrow current;if(!Borrow(context,roots,key,0,current,reason))return false;
  if(current.object!=b.object||current.instance!=b.instance||current.owner_epoch!=b.owner_epoch){reason="players_borrow_generation_changed";return false;}return true;
}
bool Requested(void *context,const ClientNativeRoots &roots,const ClientObjectDescription &d,const ClientEntityBorrow &b,const char *&reason) noexcept {
  if(!Witness(context,roots,d.key,b,reason))return false;
  auto &o=*static_cast<Token *>(context)->owner;const auto &entry=o.entries.at(d.key);
  if(entry.description.entity.shape_revision!=d.shape_revision||entry.description.entity.class_values!=d.class_values||entry.description.entity.common_values!=d.common_values||!(entry.ready&unsigned(ClientPlayerUse::State))){reason="players_requested_target_state_not_ready";return false;}return true;
}
bool Sets(void *context,const ClientNativeRoots &roots,const char *&reason) noexcept {
  auto *token=static_cast<Token *>(context);auto *o=token?token->owner:nullptr;
  if(!o||!IsMain()||o->fault||o->retiring||!SameRoots(roots)||roots.room!=o->roots.room||!o->sets_ready){reason="players_sets_not_ready";return false;}
  try{return ValidateSets(*o,reason);}catch(...){reason="players_sets_allocation";return false;}
}
bool DrawMember(void *context,const ClientNativeRoots &roots,const ClientObjectKey &key,const ClientEntityBorrow &b,bool &include,const char *&reason) noexcept {
  if(!Witness(context,roots,key,b,reason))return false;auto &o=*static_cast<Token *>(context)->owner;const auto &e=o.entries.at(key);
  if(!o.ready||!(e.ready&unsigned(ClientPlayerUse::Draw))){reason="players_draw_not_ready";return false;}include=e.description.render_member;return true;
}
struct RelationContext final {ClientNativePlayers *owner;const ClientEntityOwners *entities;bool inspect=false;};
// 仅本次关系安装栈可借正在执行的自身owner；公开借用仍拒绝重入，不扩大到其他用途。
bool ResolveRelation(void *context,const ClientNativeRoots &roots,const ClientObjectKey &key,unsigned expected,ClientEntityBorrow &out,const char *&reason) noexcept {
  auto &c=*static_cast<RelationContext *>(context);auto &o=*c.owner;
  if(!Phase()||(!o.busy&&!c.inspect)||(c.inspect&&(o.busy||o.fault||o.retiring||!o.ready||!o.committed_revision))||!SameRoots(roots)||roots.room!=o.roots.room){reason="players_relation_resolver_context";return false;}
  if(!c.inspect)o.dependency=key;
  const auto i=o.entries.find(key);if(i!=o.entries.end()){
    if(!Current(o,i->second)||(expected!=0&&expected!=13)){reason="players_relation_body_use_or_type";return false;}
    out={i->second.player,i->second.instance,o.epoch,0};return true;
  }
  if(!c.entities->resolve){reason="players_cross_domain_owner_missing";return false;}
  return c.entities->resolve(c.entities->context,roots,key,expected,out,reason);
}
// 通用层只处理有准确字段合同的边；每个裸借用在换实例/退休前统一撤掉。
bool Relations(ClientNativePlayers &o,Entry &entry,const ClientEntityOwners &entities,bool check,bool clear,const char *&reason,bool inspect=false){
  auto &p=*entry.player;
  // 同步执行记录当前待解析边；inspect不改owner摘要，也不因检查失败使同步失效。
  const auto note=[&](const char *field,ClientObjectKey key){if(!inspect){o.dependency=key;std::snprintf(o.field.data(),o.field.size(),"%s",field);}};
  const auto resolve=[&](const ClientObjectKey &key,unsigned expected,Entity *&entity){
    const auto body=o.entries.find(key);if(body!=o.entries.end()){if(!Current(o,body->second))return false;entity=body->second.player;return expected==0||expected==13;}
    ClientEntityBorrow b;if(!entities.resolve||!entities.resolve(entities.context,o.roots,key,expected,b,reason))return false;entity=static_cast<Entity *>(b.object);return entity!=nullptr;
  };
  for(const auto &r:entry.description.entity.relations){if(r.value_only||r.member.rfind("Entity.",0)!=0)continue;Entity *target=nullptr;
    note(r.member.c_str(),r.target);
    if(!clear&&!r.is_null&&!resolve(r.target,r.expected,target))return false;
    EntityPtr *slot=r.member=="Entity.Parent"?&p._parent:r.member=="Entity.Child"?&p._child:r.member=="Entity.Target"?&p._target:r.member=="Entity.SpawnerEntity"?&p._spawnerEntity:r.member=="Entity.GetMinecart"?&p._minecart:nullptr;
    if(!slot)continue;if(check){if(slot->GetReference()!=target){reason="players_entity_relation_postcondition";return false;}}else slot->SetReference(target);
  }
  // 公开main/other/sub是带角色条件的getter；写真实storage，不将GetMainTwin(self)误写成自环。
  for(auto name:{"twin_storage","sub_storage","backup_storage","replaced_storage"}){
    const auto r=entry.description.body_relations.find(name);Entity *target=nullptr;
    if(r==entry.description.body_relations.end()){reason="players_native_relation_source_missing";return false;}
    note(name,r->second.target);
    if(!clear&&!r->second.is_null&&!resolve(r->second.target,0,target))return false;
    if(std::strcmp(name,"twin_storage")==0){if(check){if(p._twinPlayer.GetReference()!=target){reason="players_twin_storage_mismatch";return false;}}else p._twinPlayer.SetReference(target);}
    else {
      Entity_Player **slot=std::strcmp(name,"backup_storage")==0?&p._backupPlayer:std::strcmp(name,"replaced_storage")==0?&p._replacedPlayer:nullptr;
      if(slot){if(check){if(*slot!=target){reason="players_raw_body_link_mismatch";return false;}}else *slot=static_cast<Entity_Player *>(target);}
      else {const auto offset=world::layout::LayoutConstants().player_sub_storage_offset;Entity *actual=nullptr;std::memcpy(&actual,reinterpret_cast<unsigned char *>(&p)+offset,4);if(check){if(actual!=target){reason="players_sub_storage_mismatch";return false;}}else std::memcpy(reinterpret_cast<unsigned char *>(&p)+offset,&target,4);}
    }
  }
  // held是裸借用；它不拥有独立Key，不调用TryHold/Throw/Suplex重放Authority动作。
  for(const auto &r:entry.description.entity.relations)if(r.member=="EntityPlayer.GetHeldEntity"){
    note(r.member.c_str(),r.target);
    Entity *target=nullptr;if(!clear&&!r.is_null&&!resolve(r.target,r.expected,target))return false;
    if(check){if(p._heldEntity!=target){reason="players_held_relation_mismatch";return false;}}else p._heldEntity=target;
  }
  // Weapon动态mainEntity/角色私有拥有关系由对应owner安装，绝不把所有派生型写成base+8。
  if(!o.adapter.relations){reason="players_role_relations_adapter_pending";return false;}
  RelationContext context{&o,&entities,inspect};ClientEntityOwners resolver{&context,ResolveRelation};
  note("equipment.weapons",{});
  if(entry.equipment&&!ClientPlayerEquipmentRelations(entry.equipment,&p,entry.description,resolver,o.roots,check,clear,reason))return false;
  return o.adapter.relations(o.adapter.context,&p,entry.description,resolver,o.roots,check,clear,reason);
}
}
ClientNativePlayers *ClientNativePlayersCreate() noexcept {
  if(!Phase()||next_player_epoch==UINT64_MAX)return nullptr;
  try{auto o=std::make_unique<ClientNativePlayers>();o->token=std::make_shared<Token>();o->token->owner=o.get();o->epoch=++next_player_epoch;return o.release();}catch(...){return nullptr;}
}
std::uint64_t ClientNativePlayersValueBytes(const ClientNativePlayers *o){if(!o)return 0;std::uint64_t n=sizeof(*o)+dto_memory::Extra(o->desired);for(const auto &p:o->entries)n+=sizeof(p)+64+dto_memory::Extra(p.second.description);return n;}
void ClientNativePlayersDestroy(ClientNativePlayers *o) noexcept {
  if(!o||!IsMain()||o->busy)return;for(const auto &p:o->entries)if(p.second.player)return;
  o->token->owner=nullptr;delete o;
}
unsigned ClientNativePlayersOwnerCount(const ClientNativePlayers *o) noexcept {
  if(!IsMain()||!o)return 0;unsigned n=0;for(const auto &e:o->entries)n+=e.second.player!=nullptr;return n;
}
// 完整小步骤内先核全部实例，再安装/回读；失败前缀只交Scene退休，不重演玩法。
ClientObjectStep ClientNativePlayersFast(ClientNativePlayers *o,const ClientPlayerFastValues &v) noexcept {
  if(!o||!Phase()||o->busy||o->fault||!o->ready||!o->committed_revision||!SameRoots(o->roots))return Result(o,false,true,"players_fast_owner");
  const auto it=o->entries.find(v.key);if(it==o->entries.end()||!Current(*o,it->second)||it->second.description.entity.shape_revision!=v.shape||it->second.description.binding_revision!=v.binding)return Result(o,false,true,"players_fast_instance_binding");
  if(!Graphics())return Result(o,false,false,"players_fast_inflight");
  if(it->second.description.player_type==29||it->second.description.player_type==38)return Result(o,false,false,"players_fast_cross_body_requires_ordinary");
  if(!v.render||!ClientPlayerRenderCompatible(it->second.equipment,it->second.player,*v.render))return Result(o,false,false,"players_fast_resource_requires_ordinary");
  for(const auto &f:v.common)if(f.member=="Entity.GetEntityFlags"&&((it->second.player->_flags^f.bits)&8))return Result(o,false,false,"players_fast_membership_requires_ordinary");
  ClientPlayerDescription expected;try{expected=ClientPlayerFastDescription(it->second.description,v);}catch(...){return Result(o,false,true,"players_fast_expected_allocation");}
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)return Result(o,false,true,"players_fast_dispatch");
  auto &e=it->second;const char *reason="players_fast_complete";bool ok=false;o->busy=true;o->last_key=v.key;o->phase="fast";e.prefix|=State|Resources;o->prefix|=State|Resources;
  try{ClientPlayerIsolationScope isolation(ClientPlayerOperation::Install);ok=isolation.Ready()&&
      ClientEntityCommonFields(e.player,v.common,false,reason)&&ClientPlayerScalarFields(e.player,v.player,false,reason);
    if(ok)ok=ClientPlayerRenderInstall(e.equipment,e.player,*v.render,false,reason);
    if(ok)ok=Current(*o,e)&&ClientEntityCommonFields(e.player,v.common,true,reason)&&ClientPlayerScalarFields(e.player,v.player,true,reason)&&ClientPlayerRenderInstall(e.equipment,e.player,*v.render,true,reason);
    if(ok)e.description=std::move(expected);
  }catch(...){reason="players_fast_native_exception";}
  o->busy=false;ClientDispatchReturned(ClientEntry::NativeApply);ok=ok&&dispatch.Succeeded();if(!ok){o->fault=true;o->ready=false;}
  return Result(o,ok,!ok,reason,true);
}
ClientObjectStep ClientNativePlayersStream(ClientNativePlayers *o,const std::vector<ClientPlayerDescription> &values,const ClientEntityOwners &entities) noexcept {
  if(!o||!Phase()||o->busy||o->fault||!o->ready||!SameRoots(o->roots)||values.size()!=o->desired.size())return Result(o,false,true,"players_stream_owner");
  const char *reason="players_stream_complete";
  try{for(const auto &d:values){auto it=o->entries.find(d.entity.key);if(it==o->entries.end()||!Current(*o,it->second)||d.entity.shape_revision!=it->second.description.entity.shape_revision||d.player_type!=it->second.description.player_type||!o->adapter.prepare(o->adapter.context,d,reason)||!ClientPlayerStatePreflight(d,reason)||!ClientPlayerEquipmentPreflight(d,reason))return Result(o,false,true,reason);}}
  catch(...){return Result(o,false,true,"players_stream_preflight");}
  if(!Graphics())return Result(o,false,false,"players_stream_inflight");
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)return Result(o,false,true,"players_stream_dispatch");
  bool ok=true;o->busy=true;o->ready=false;o->phase="stream";
  try{ClientPlayerIsolationScope isolation(ClientPlayerOperation::Install);ok=isolation.Ready();
    for(const auto &d:values){if(!ok)break;auto &e=o->entries.at(d.entity.key);o->last_key=d.entity.key;e.prefix|=State|Resources;o->prefix|=State|Resources;e.description=d;
      ok=ClientPlayerStateFields(e.player,d,false,reason)&&ClientPlayerEquipmentFields(e.player,d,false,reason)&&ClientPlayerRoleFields(e.player,d,false,reason)&&o->adapter.install(o->adapter.context,e.player,d,false,false,reason)&&ClientPlayerEquipmentResources(e.equipment,e.player,d,false,reason)&&o->adapter.install(o->adapter.context,e.player,d,true,false,reason)&&ClientPlayerHudInstall(e.hud,e.player,d,false,reason);}
    if(ok)for(auto &pair:o->entries)if(!Relations(*o,pair.second,entities,false,false,reason)){ok=false;break;}
    if(ok)for(auto &pair:o->entries){auto &e=pair.second;const auto &d=e.description;RelationContext context{o,&entities};ClientEntityOwners resolver{&context,ResolveRelation};
      if(!Current(*o,e)||!Relations(*o,e,entities,true,false,reason)||!ClientPlayerRoleRelations(e.player,d,resolver,o->roots,reason)||!ClientPlayerStateFields(e.player,d,true,reason)||!ClientPlayerEquipmentFields(e.player,d,true,reason)||!ClientPlayerRoleFields(e.player,d,true,reason)||!ClientPlayerEquipmentResources(e.equipment,e.player,d,true,reason)||!ClientPlayerHudInstall(e.hud,e.player,d,true,reason)||!o->adapter.install(o->adapter.context,e.player,d,false,true,reason)||!o->adapter.install(o->adapter.context,e.player,d,true,true,reason)){ok=false;break;}}
    if(ok)ok=ValidateSets(*o,reason);
    if(ok)o->desired=values;
  }catch(...){ok=false;reason="players_stream_native_exception";}
  o->busy=false;ClientDispatchReturned(ClientEntry::NativeApply);ok=ok&&dispatch.Succeeded();o->ready=ok;if(!ok)o->fault=true;return Result(o,ok,!ok,reason,true);
}
bool ClientNativePlayersBegin(ClientNativePlayers *o,const ClientNativeRoots &roots,std::uint64_t scope,std::uint64_t revision,const std::vector<ClientPlayerDescription> &values,const std::array<ClientObjectReference,4> &backups,const ClientPlayerAdapter &adapter,const char *&reason) noexcept {
  if(!o||!Phase()||o->fault||o->busy||o->retiring||!SameRoots(roots)||!scope||!revision||values.size()>replica::kPlayerBodies){reason="players_begin_context_or_budget";return false;}
  // 后续字段/资源/角色adapter未闭合时在第一次原生写前拒绝，不能制造半初始化可见身体。
  if(!adapter.prepare||!adapter.install||!adapter.retire||!adapter.relations){reason="players_role_draw_adapter_incomplete";return false;}
  if(!ClientCallbackGuardReady()||!ClientLuaGuardReady()||!ClientPlayerIsolationInstalled()){reason="players_official_callbacks_or_persistence_guard_unavailable";return false;}
  try{
    if(o->revision&&revision<=o->revision){reason="players_native_revision_not_new";return false;}
    if(o->roots.game&&(o->roots.game!=roots.game||o->roots.room!=roots.room)){reason="players_scope_requires_retirement";return false;}
    std::set<ClientObjectKey> keys;std::set<int> roster_indices;
    for(const auto &d:values){
      if(!d.entity.key.run||d.entity.key.space!=1||!d.entity.key.id||!keys.insert(d.entity.key).second||!ClientPlayerRole(d.player_type)||!d.role||d.entity.type!=1||d.entity.variant||!d.entity.seed||
          (d.roster_index>=0&&!roster_indices.insert(d.roster_index).second)||(d.can_consume&&(!d.active||!d.membership_known))||((d.render_member||d.update_member)&&!d.room_member)){
        reason="players_identity_role_or_membership_conflict";return false;}
      for(auto name:{"sub_storage","twin_storage","backup_storage","replaced_storage"})if(!d.body_relations.count(name)){reason="players_storage_source_unavailable";return false;}
      if(!adapter.prepare(adapter.context,d,reason))return false;
      if(!ClientPlayerStatePreflight(d,reason)||!ClientPlayerEquipmentPreflight(d,reason))return false;
      auto hit=o->entries.find(d.entity.key);if(hit!=o->entries.end()){
        const auto &before=hit->second.description;
        if(before.entity.shape_revision!=d.entity.shape_revision||before.player_type!=d.player_type||before.entity.variant!=d.entity.variant||before.entity.subtype!=d.entity.subtype||before.entity.seed!=d.entity.seed){reason="players_shape_requires_scene_rebuild";return false;}}}
    for(unsigned i=0;i<roster_indices.size();++i)if(!roster_indices.count(int(i))){reason="players_roster_not_contiguous";return false;}
    for(const auto &r:backups)if(!r.present||(!r.is_null&&!keys.count(r.target))){reason="players_backup_target_unavailable";return false;}
    for(const auto &p:o->entries)if(std::none_of(values.begin(),values.end(),[&](const auto &v){return v.entity.key==p.first;})){reason="players_remove_requires_scene_rebuild";return false;}
    auto &roster=static_cast<Game *>(roots.game)->_playerManager._playerList;
    for(auto *p:roster)if(std::none_of(o->entries.begin(),o->entries.end(),[&](const auto &v){return v.second.player==p;})){reason="players_foreign_roster_owner";return false;}
    o->desired=values;o->backups=backups;o->adapter=adapter;o->roots=roots;o->scope=scope;o->revision=revision;o->cursor=o->preparation_cursor=o->finish_cursor=0;
    o->ready=o->structure_ready=o->sets_ready=o->links_ready=false;o->committed_revision=0;
    for(auto &p:o->entries)p.second.ready=unsigned(ClientPlayerUse::Structure);
    reason="players_native_plan_ready";return true;
  }catch(...){reason="players_begin_allocation";return false;}
}
ClientObjectStep ClientNativePlayersStructure(ClientNativePlayers *o) noexcept {
  if(!o||!Phase()||o->busy||o->fault||!SameRoots(o->roots))return Result(o,false,true,"players_structure_context");
  if(o->structure_ready)return Result(o,true,false,"players_structure_ready");
  if(!Graphics())return Result(o,false,false,"players_graphics_or_cpu_inflight");
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)return Result(o,false,true,"players_structure_dispatch");
  const char *reason="players_structure_progress";bool changed=false,ok=true;o->busy=true;
  o->dependency={};o->field={};
  try{
    if(o->cursor<o->desired.size()){
      const auto &d=o->desired[o->cursor];o->last_key=d.entity.key;auto &e=o->entries[d.entity.key];
      if(!e.player){if(next_player_instance==UINT64_MAX)throw std::bad_alloc();e.description=d;e.instance=++next_player_instance;
        o->phase="allocate";ClientPlayerTestBoundary("player-allocate-before",d.entity.key,e.instance,e.prefix);
        e.prefix|=Raw;o->prefix|=Raw;changed=true;
        e.player=static_cast<Entity_Player *>(reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(sizeof(Entity_Player)));
        if(!e.player)throw std::bad_alloc();
        ClientPlayerTestBoundary("player-allocate-after",d.entity.key,e.instance,e.prefix);
        // 仅初始化尚未构造的自有raw存储；不是把零伪装成Source值，也不覆盖任何存活容器。
        std::memset(e.player,0,sizeof(Entity_Player));
        o->phase="construct";ClientPlayerTestBoundary("player-ctor-before",d.entity.key,e.instance,e.prefix);
        {ClientPlayerIsolationScope isolation(ClientPlayerOperation::Construct);
          if(!isolation.Ready())throw std::bad_alloc();e.prefix|=CtorEntered;o->prefix|=CtorEntered;e.player->constructor();}e.prefix|=Constructed;o->prefix|=Constructed;
        ClientPlayerTestBoundary("player-ctor-after",d.entity.key,e.instance,e.prefix);
        e.player->_playerIndex=-1;e.player->_controllerIndex=-1;e.player->_playerType=d.player_type;
        // 基Init仍经过唯一常驻Hook及官方Plus链；不用Player派生Init/Pre/Join生成额外身体或装备。
        o->phase="initialize";ClientPlayerTestBoundary("player-init-before",d.entity.key,e.instance,e.prefix);
        {ClientPlayerIsolationScope isolation(ClientPlayerOperation::BaseInit);
          if(!isolation.Ready())throw std::bad_alloc();e.prefix|=InitEntered;o->prefix|=InitEntered;reinterpret_cast<void (__thiscall *)(Entity *,unsigned,unsigned,unsigned,unsigned)>(Base+0x2a4a20)(e.player,d.entity.type,d.entity.variant,d.entity.subtype,d.entity.seed);}
        e.prefix|=Initialized;o->prefix|=Initialized;e.vptr=*reinterpret_cast<std::uintptr_t *>(e.player);e.seed=e.player->_initSeed;
        ClientPlayerTestBoundary("player-init-after",d.entity.key,e.instance,e.prefix);
      }else {if(!Current(*o,e)){ok=false;reason="players_existing_instance_changed";}e.description=d;}
      if(ok){e.ready=unsigned(ClientPlayerUse::Structure);++o->cursor;}
    }else if(o->preparation_cursor<o->desired.size()){
      // 先让全部Key拥有base结构，再安装身体子owner；装备关系仍等Entity/Grid全体就绪。
      const auto &d=o->desired[o->preparation_cursor];o->last_key=d.entity.key;auto &e=o->entries.at(d.entity.key);
      ClientPlayerIsolationScope isolation(ClientPlayerOperation::Install);if(!isolation.Ready())throw std::bad_alloc();
      if(!Current(*o,e)){ok=false;reason="players_state_use_changed";}
      if(ok&&!e.equipment){e.equipment=ClientPlayerEquipmentCreate();if(!e.equipment){ok=false;reason="players_equipment_owner_allocation";}}
      if(ok&&!e.hud){e.hud=ClientPlayerHudCreate();if(!e.hud){ok=false;reason="players_hud_owner_allocation";}}
      if(ok&&d.state_changed){o->phase="fields";ClientPlayerTestBoundary("player-fields-before",d.entity.key,e.instance,e.prefix);changed=true;e.prefix|=State;o->prefix|=State;ok=ClientPlayerStateFields(e.player,d,false,reason)&&ClientPlayerEquipmentFields(e.player,d,false,reason)&&ClientPlayerRoleFields(e.player,d,false,reason)&&o->adapter.install(o->adapter.context,e.player,d,false,false,reason);if(ok)ClientPlayerTestBoundary("player-fields-after",d.entity.key,e.instance,e.prefix);}
      if(ok&&d.resources_changed){o->phase="resources";ClientPlayerTestBoundary("player-resources-before",d.entity.key,e.instance,e.prefix);changed=true;e.prefix|=Resources;o->prefix|=Resources;ok=ClientPlayerEquipmentResources(e.equipment,e.player,d,false,reason)&&o->adapter.install(o->adapter.context,e.player,d,true,false,reason);if(ok)ClientPlayerTestBoundary("player-resources-after",d.entity.key,e.instance,e.prefix);}
      if(ok){o->phase="hud";ClientPlayerTestBoundary("player-hud-before",d.entity.key,e.instance,e.prefix);e.prefix|=unsigned(ClientPlayerPrefix::HudEntered);o->prefix|=unsigned(ClientPlayerPrefix::HudEntered);changed=true;ok=ClientPlayerHudInstall(e.hud,e.player,d,false,reason);if(ok)ClientPlayerTestBoundary("player-hud-after",d.entity.key,e.instance,e.prefix);}
      if(ok){e.ready|=unsigned(ClientPlayerUse::State);++o->preparation_cursor;}
    }else{
      auto &roster=static_cast<Game *>(o->roots.game)->_playerManager._playerList;std::vector<Entity_Player *> ordered;
      for(unsigned i=0;i<o->desired.size();++i)for(const auto &d:o->desired)if(d.roster_index==int(i))ordered.push_back(o->entries.at(d.entity.key).player);
      // 使用真实vector操作，不复制容器头；进入前登记，分配失败也保留全部对象owner。
      o->phase="roster";o->last_key={};ClientPlayerTestBoundary("player-roster-before",{},0,o->prefix);
      changed=true;o->prefix|=Roster;roster.assign(ordered.begin(),ordered.end());
      ClientPlayerTestBoundary("player-roster-after",{},0,o->prefix);
      for(unsigned i=0;i<4;++i)o->backup_players[i]=o->backups[i].is_null?nullptr:o->entries.at(o->backups[i].target).player;
      auto &list=static_cast<Room *>(o->roots.room)->_entityList;
      auto &index=*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(o->roots.room)+0x1214);
      if(!(o->prefix&Published)&&o->entries.size()==o->desired.size()&&ListValid(list._mainEL)&&ListValid(list._persistentEL)&&!list._mainEL._size&&!list._persistentEL._size)index=0;
      for(auto &pair:o->entries){auto &e=pair.second;const auto &d=e.description;e.prefix|=Roster;o->last_key=pair.first;
        // 非roster身体也保留正式Source的索引；IsHologram读取双方索引，不能无条件写-1。
        e.player->_playerIndex=d.role->player_index;
        if(d.room_member&&!(e.prefix&Published)){
          o->phase="publish";
          ClientPlayerTestBoundary("player-publish-before",d.entity.key,e.instance,e.prefix);
          for(auto *l:{&list._persistentEL,&list._renderEL,&list._updateEL})if(!ListValid(*l)||l->_size>=l->_capacity)throw std::bad_alloc();
          if(index==UINT32_MAX)throw std::bad_alloc();e.player->_index=index++;e.index=e.player->_index;e.prefix|=Published;o->prefix|=Published;
          reinterpret_cast<void (__thiscall *)(EntityList *,Entity *)>(Base+0x13d00)(&list,e.player);
          ClientPlayerTestBoundary("player-publish-after",d.entity.key,e.instance,e.prefix);
        }else if(!d.room_member&&(e.prefix&Published)){ok=false;reason="players_room_membership_change_requires_scene_rebuild";break;}
        if(!d.room_member)e.player->_valid=d.native_valid;
        // exists/dead/ghost不决定EL资格；同实例的复活/可见变化按正式最终成员集合恢复。
        for(const auto &member:{std::pair<EntityList_EL *,bool>{&list._renderEL,d.render_member},{&list._updateEL,d.update_member}}){
          auto &l=*member.first;if(!member.second)Erase(l,e.player);
          else if(!Count(l,e.player)){if(!ListValid(l)||l._size>=l._capacity)throw std::bad_alloc();l._data[l._size++]=e.player;}
        }
      }
      if(ok){o->sets_ready=ValidateSets(*o,reason);ok=o->sets_ready;o->structure_ready=ok;}
    }
  }
#ifdef ISAACCS_E2E
  catch(const ClientPlayerInjected &fault){ok=false;reason=fault.point;}
#endif
  catch(...){ok=false;reason="players_structure_exception_prefix_retained";}
  o->busy=false;ClientDispatchReturned(ClientEntry::NativeApply);if(!dispatch.Succeeded()){ok=false;reason="players_structure_dispatch_incomplete";}
  if(!ok)o->fault=true;return Result(o,ok&&o->structure_ready,!ok,reason,changed);
}
ClientObjectStep ClientNativePlayersFinish(ClientNativePlayers *o,const ClientEntityOwners &entities,bool finish) noexcept {
  if(!o||!Phase()||!o->structure_ready||o->busy||o->fault)return Result(o,false,true,"players_finish_context");
  if(!Graphics())return Result(o,false,false,"players_finish_graphics_or_cpu_inflight");
  if(!finish)return Result(o,false,false,"players_relations_waiting_cross_domain_instances");
  if(o->ready)return Result(o,true,false,"players_checked_pending_scene_commit");
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)return Result(o,false,true,"players_link_dispatch");
  o->busy=true;bool ok=true;const char *reason="players_relations_progress";
  o->dependency={};o->field={};
  ClientPlayerIsolationScope isolation(ClientPlayerOperation::Relations);
  if(!isolation.Ready()){o->busy=false;return Result(o,false,true,"players_relations_isolation");}
  try{
    // twin/sub/backup/Weapon可形成环：全部身体先安装边，再逐体读取条件getter。
    // 不能在另一身体尚未链接时误判失败，也不能让部分组提前取得draw资格。
    if(!o->links_ready&&o->finish_cursor<o->desired.size()){
      o->phase="relations";
      o->last_key=o->desired[o->finish_cursor].entity.key;auto &e=o->entries.at(o->last_key);ClientPlayerTestBoundary("player-relations-before",o->last_key,e.instance,e.prefix);e.prefix|=Linked;o->prefix|=Linked;
      if(!Current(*o,e)){ok=false;reason="players_check_use_changed";}
      else ok=Relations(*o,e,entities,false,false,reason);
      if(ok){ClientPlayerTestBoundary("player-relations-after",o->last_key,e.instance,e.prefix);++o->finish_cursor;}
    }else if(!o->links_ready){o->links_ready=true;o->finish_cursor=0;}
    else if(o->finish_cursor<o->desired.size()){
      o->phase="check";
      o->last_key=o->desired[o->finish_cursor].entity.key;auto &e=o->entries.at(o->last_key);
      ClientPlayerTestBoundary("player-check-before",o->last_key,e.instance,e.prefix);
      if(!Current(*o,e)){ok=false;reason="players_check_use_changed";}
      else {RelationContext context{o,&entities};ClientEntityOwners resolver{&context,ResolveRelation};
        ok=Relations(*o,e,entities,true,false,reason)&&ClientPlayerRoleRelations(e.player,e.description,resolver,o->roots,reason)&&ClientPlayerStateFields(e.player,e.description,true,reason)&&ClientPlayerEquipmentFields(e.player,e.description,true,reason)&&ClientPlayerRoleFields(e.player,e.description,true,reason)&&ClientPlayerEquipmentResources(e.equipment,e.player,e.description,true,reason)&&ClientPlayerHudInstall(e.hud,e.player,e.description,true,reason)&&o->adapter.install(o->adapter.context,e.player,e.description,false,true,reason)&&o->adapter.install(o->adapter.context,e.player,e.description,true,true,reason);}
      if(ok){e.prefix|=Checked;e.ready|=unsigned(ClientPlayerUse::Relations)|unsigned(ClientPlayerUse::Draw);ClientPlayerTestBoundary("player-check-after",o->last_key,e.instance,e.prefix);++o->finish_cursor;}
    }else{o->ready=ValidateSets(*o,reason);ok=o->ready;}
  }
#ifdef ISAACCS_E2E
  catch(const ClientPlayerInjected &fault){ok=false;reason=fault.point;}
#endif
  catch(...){ok=false;reason="players_relations_exception_retained";}
  o->busy=false;ClientDispatchReturned(ClientEntry::NativeApply);if(!dispatch.Succeeded()){ok=false;reason="players_link_dispatch_incomplete";}
  if(!ok)o->fault=true;return Result(o,ok&&o->ready,!ok,reason,true);
}
ClientEntityOwners ClientNativePlayersOwner(ClientNativePlayers *o) noexcept {
  if(!IsMain()||!o)return {};ClientEntityOwners out{o->token.get(),Borrow,nullptr,Requested,o->token};out.player_current=Witness;out.player_sets=Sets;out.player_draw=DrawMember;return out;
}
bool ClientNativePlayersBorrow(ClientNativePlayers *o,const ClientNativeRoots &roots,const ClientObjectKey &key,std::uint64_t scope,std::uint64_t shape,ClientPlayerUse use,ClientEntityBorrow &out,const char *&reason) noexcept {
  if(!o||!IsMain()||scope!=o->scope){reason="players_borrow_scope_or_thread";return false;}
  if(!Borrow(o->token.get(),roots,key,0,out,reason))return false;
  const auto &e=o->entries.at(key);const auto mask=unsigned(use);
  if(e.description.entity.shape_revision!=shape||(e.ready&mask)!=mask||(use==ClientPlayerUse::Draw&&(!o->ready||!e.description.render_member))){out={};reason="players_borrow_shape_role_or_purpose_not_ready";return false;}return true;
}
bool ClientNativePlayersCommit(ClientNativePlayers *o,std::uint64_t revision) noexcept {
  if(!IsMain()||!o||o->fault||!o->ready||o->revision!=revision)return false;o->committed_revision=revision;return true;
}
bool ClientNativePlayersVisit(ClientNativePlayers *o,void *context,ClientObjectVisitor visitor) noexcept {
  if(!IsMain()||!ClientPlayerBorrowAllowed()||!o||o->fault||o->retiring||!o->ready||!visitor)return false;
  for(const auto &p:o->entries)if(!Current(*o,p.second)||!visitor(context,p.second.player,p.first,p.second.instance,1))return false;return true;
}
#ifdef ISAACCS_E2E
// 回读当前原生容器及公开语义的已审阅leaf；任何不一致只使本次inspect失败。
// 不进入用途分派、不安装姿态、不设置fault/ready，也不把目标DTO原样打印作后态。
std::string ClientNativePlayersInspect(ClientNativePlayers *o,const ClientEntityOwners &entities) noexcept {
  if(!Phase()||!ClientPlayerBorrowAllowed()||!o||o->busy||o->fault||o->retiring||!o->ready||
      !o->committed_revision||o->committed_revision!=o->revision||!SameRoots(o->roots)||!Graphics())
    return "ok=0\nerror=player_inspect_context_or_uncommitted\n";
  const char *reason="player_inspect_postcondition";
  try{
    if(!ValidateSets(*o,reason))return "ok=0\nerror="+std::string(reason)+"\n";
    rapidjson::Document rows(rapidjson::kArrayType);auto &a=rows.GetAllocator();
    const auto decimal=[&](std::uint64_t n){const auto s=std::to_string(n);return rapidjson::Value(s.c_str(),rapidjson::SizeType(s.size()),a);};
    for(auto &pair:o->entries){auto &e=pair.second;const auto &d=e.description;
      RelationContext context{o,&entities,true};ClientEntityOwners resolver{&context,ResolveRelation};
      if(!Current(*o,e)||!e.equipment||!e.hud||!Relations(*o,e,entities,true,false,reason,true)||
          !ClientPlayerRoleRelations(e.player,d,resolver,o->roots,reason)||!ClientPlayerStateFields(e.player,d,true,reason)||
          !ClientPlayerEquipmentFields(e.player,d,true,reason)||!ClientPlayerRoleFields(e.player,d,true,reason)||
          !ClientPlayerEquipmentResources(e.equipment,e.player,d,true,reason)||!ClientPlayerHudInstall(e.hud,e.player,d,true,reason)||
          !o->adapter.install(o->adapter.context,e.player,d,false,true,reason)||!o->adapter.install(o->adapter.context,e.player,d,true,true,reason))
        return "ok=0\nerror="+std::string(reason)+"\nplayer_key_run="+std::to_string(pair.first.run)+"\nplayer_key_id="+std::to_string(pair.first.id)+"\n";
      auto &p=*e.player;rapidjson::Value row(rapidjson::kObjectType);
      row.AddMember("run",decimal(pair.first.run),a);row.AddMember("space",pair.first.space,a);row.AddMember("id",decimal(pair.first.id),a);
      row.AddMember("instance",decimal(e.instance),a);row.AddMember("shape",decimal(d.entity.shape_revision),a);row.AddMember("prefix",e.prefix,a);
      row.AddMember("player_type",p._playerType,a);row.AddMember("player_index",p._playerIndex,a);row.AddMember("controller_index",p._controllerIndex,a);
      row.AddMember("exists",p._exists,a);row.AddMember("valid",p._valid,a);row.AddMember("red_hearts",p._redHearts,a);
      rapidjson::Value fields(rapidjson::kObjectType);
#define Raw ClientInspectRaw
#define CS_PLAYER_FIELD(id,field,type,kind) if(ClientPlayerStateApplicable(id,p._playerType))ClientInspectField(fields,id,field,a);
#include "game/client_player_fields.inc"
#undef CS_PLAYER_FIELD
#undef Raw
      row.AddMember("fields",std::move(fields),a);
      rapidjson::Value position(rapidjson::kArrayType);position.PushBack(p._pos.x,a);position.PushBack(p._pos.y,a);row.AddMember("position",std::move(position),a);
      auto &list=static_cast<Room *>(o->roots.room)->_entityList;
      row.AddMember("room_members",Count(list._persistentEL,&p),a);row.AddMember("render_members",Count(list._renderEL,&p),a);row.AddMember("update_members",Count(list._updateEL,&p),a);
      row.AddMember("history_items",unsigned(p._history._historyItems.size()),a);row.AddMember("effects",unsigned(p._temporaryeffects._effects.size()),a);
      row.AddMember("costumes",unsigned(p._costumeSpriteDescs.size()),a);row.AddMember("hud_present",p._playerHUD!=nullptr,a);
      rapidjson::Value weapons(rapidjson::kArrayType);for(unsigned slot=0;slot<5;++slot){rapidjson::Value w(rapidjson::kObjectType);w.AddMember("slot",slot,a);w.AddMember("present",p._weapon[slot]!=nullptr,a);weapons.PushBack(std::move(w),a);}row.AddMember("weapons",std::move(weapons),a);
      rows.PushBack(std::move(row),a);
    }
    rapidjson::StringBuffer text;rapidjson::Writer<rapidjson::StringBuffer> writer(text);rows.Accept(writer);
    return "ok=1\nplayer_native_checked=1\nplayer_revision="+std::to_string(o->committed_revision)+"\nplayers="+std::string(text.GetString(),text.GetSize())+"\n";
  }catch(...){return "ok=0\nerror=player_inspect_allocation_or_read\n";}
}
#endif
// 第一阶段只撤销资格和出边。所有身体完成后，非Player owner才能开始释放自己的对象。
// 每次至多处理一个身体，任何不确定前缀锁定首因，不在下一帧重复进入同一清理调用。
ClientObjectStep ClientNativePlayersUnlink(ClientNativePlayers *o,const ClientNativeRoots &roots) noexcept {
  if(!o)return {true,false,false,0,0,"players_no_links"};
  if(!IsMain())return Result(o,false,true,"players_unlink_thread");
  o->phase="unlink";
  o->retiring=true;o->ready=false;o->committed_revision=0;
  for(auto &p:o->entries)p.second.ready=0;
  if(o->retirement_fault)return Result(o,false,true,o->retirement_reason.data());
  if(o->unlinked||o->entries.empty()){o->unlinked=true;return Result(o,true,false,"players_all_links_revoked");}
  if(!Phase()||o->busy||!SameRoots(roots)||roots.room!=o->roots.room)return Result(o,false,true,"players_unlink_roots_or_context");
  if(!Graphics())return Result(o,false,false,"players_unlink_waiting_inflight");
  if(!ClientCallbackGuardReady()||!ClientLuaGuardReady())return Result(o,false,true,"players_unlink_guards");
  ClientDispatchScope dispatch(ClientOperation::Retire,ClientEntry::NativeTeardown);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeTeardown)!=ClientEntryDecision::CallOriginal)return Result(o,false,true,"players_unlink_dispatch");
  bool ok=true;const char *reason="players_unlink_progress";o->busy=true;
  ClientPlayerIsolationScope isolation(ClientPlayerOperation::Retire);
  if(!isolation.Ready()){o->busy=false;return Result(o,false,true,"players_unlink_isolation");}
  try{ClientEntityOwners none{};o->backup_players={};Entry *next=nullptr;
    for(auto &pair:o->entries)if(pair.second.player&&!(pair.second.prefix&unsigned(ClientPlayerPrefix::Unlinked))){next=&pair.second;break;}
    if(next){auto &e=*next;o->last_key=e.description.entity.key;
      if((e.prefix&CtorEntered)&&!(e.prefix&Constructed)){ok=false;reason="players_partial_ctor_retained";}
      else if((e.prefix&InitEntered)&&!(e.prefix&Initialized)){ok=false;reason="players_partial_init_retained";}
      else if((e.prefix&Initialized)&&!Relations(*o,e,none,false,true,reason))ok=false;
      if(ok&&(e.prefix&Constructed)&&!ClientPlayerHudRetire(e.hud,e.player,reason))ok=false;
      if(ok&&e.hud){e.prefix|=unsigned(ClientPlayerPrefix::HudRetired);o->prefix|=unsigned(ClientPlayerPrefix::HudRetired);}
      if(ok){e.prefix|=unsigned(ClientPlayerPrefix::Unlinked);o->prefix|=unsigned(ClientPlayerPrefix::Unlinked);++o->unlink_cursor;}
    }else o->unlinked=true;
  }catch(...){ok=false;reason="players_unlink_exception_prefix_retained";}
  o->busy=false;ClientDispatchReturned(ClientEntry::NativeTeardown);
  if(!dispatch.Succeeded()){ok=false;reason="players_unlink_dispatch_incomplete";}
  if(!ok){o->fault=o->retirement_fault=true;std::snprintf(o->retirement_reason.data(),o->retirement_reason.size(),"%s",reason);reason=o->retirement_reason.data();}
  return Result(o,ok&&o->unlinked,!ok,reason,true);
}
ClientObjectStep ClientNativePlayersRetire(ClientNativePlayers *o,const ClientNativeRoots &roots) noexcept {
  if(!o)return {true,false,false,0,0,"players_no_owner"};
  if(!IsMain())return {false,false,true,0,0,"players_retire_thread"};
  o->phase="retire";
  if(IsMain()&&o->entries.empty())return Result(o,true,false,"players_unwritten_owner_retired");
  if(o->retirement_fault)return Result(o,false,true,o->retirement_reason.data());
  if(!o->unlinked)return ClientNativePlayersUnlink(o,roots);
  if(!Phase()||o->busy||!SameRoots(roots)||roots.room!=o->roots.room)return Result(o,false,true,"players_retire_roots_or_thread");
  if(!Graphics())return Result(o,false,false,"players_retire_inflight");
  if(!ClientCallbackGuardReady()||!ClientLuaGuardReady())return Result(o,false,true,"players_retire_callback_guard_unavailable");
  // 撤销所有借用优先；故障owner和已进入Free前缀不自动重试，留在当前Scene保护窗口。
  o->retiring=true;o->ready=false;o->committed_revision=0;for(auto &p:o->entries)p.second.ready=0;
  ClientDispatchScope dispatch(ClientOperation::Retire,ClientEntry::NativeTeardown);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeTeardown)!=ClientEntryDecision::CallOriginal)return Result(o,false,true,"players_retire_dispatch");
  bool ok=true,done=false;const char *reason="players_retire_progress";o->busy=true;
  ClientPlayerIsolationScope isolation(ClientPlayerOperation::Retire);
  if(!isolation.Ready()){o->busy=false;return Result(o,false,true,"players_retire_isolation");}
  try{
    if(!o->roster_removed){auto &roster=static_cast<Game *>(roots.game)->_playerManager._playerList;
      for(auto *p:roster)if(std::none_of(o->entries.begin(),o->entries.end(),[&](const auto &e){return e.second.player==p;})){ok=false;reason="players_retire_foreign_roster";break;}
      if(ok){roster.clear();o->roster_removed=true;}
    }else{
      Entry *next=nullptr;for(auto &pair:o->entries)if(pair.second.player&&(!next||pair.second.instance>next->instance))next=&pair.second;
      if(!next)done=true;else {auto &e=*next;o->last_key=e.description.entity.key;
        if(e.prefix&FreeEntered){ok=false;reason="players_free_prefix_uncertain_retained";}
        else if(!(e.prefix&CtorEntered)){e.prefix|=FreeEntered;o->prefix|=FreeEntered;
          reinterpret_cast<void (__cdecl *)(void *)>(Base+0x686bc9)(e.player);e.player=nullptr;e.retired=true;e.prefix|=unsigned(ClientPlayerPrefix::Freed);}
        else {const auto mark=[&](ClientPlayerPrefix prefix){e.prefix|=unsigned(prefix);o->prefix|=unsigned(prefix);};
          ok=o->adapter.retire(o->adapter.context,e.player,e.description,e.prefix,reason);if(ok)mark(ClientPlayerPrefix::AdapterRetired);
          if(ok){ok=ClientPlayerEquipmentRetire(e.equipment,e.player,reason);if(ok)mark(ClientPlayerPrefix::EquipmentRetired);}
          if(ok){ok=ClientPlayerStateRetire(e.player,reason);if(ok)mark(ClientPlayerPrefix::StateRetired);}
          if(ok){auto &p=*e.player;const auto &layout=world::layout::LayoutConstants();Entity *sub=nullptr;std::memcpy(&sub,reinterpret_cast<unsigned char *>(&p)+layout.player_sub_storage_offset,4);
          if(sub||p._twinPlayer.GetReference()||p._backupPlayer||p._replacedPlayer||p._heldEntity||p._markedTarget||p._megaBlastLaser||p._playerHUD){ok=false;reason="players_retire_remaining_owned_or_borrowed_relationship";}
          for(auto *weapon:p._weapon)if(weapon){ok=false;reason="players_retire_weapon_owner_pending";}
          for(unsigned i=0;i<layout.game_player_borrow_count;++i){Entity_Player *global=nullptr;std::memcpy(&global,reinterpret_cast<unsigned char *>(roots.game)+layout.game_player_borrow_first_offset+i*4,4);if(global==e.player){ok=false;reason="players_retire_global_player_owner_still_borrowing";}}
          auto &list=static_cast<Room *>(roots.room)->_entityList;
          for(auto *l:{&list._mainEL,&list._persistentEL,&list._renderEL,&list._updateEL,&list._effectEL,&list._tempEL,&list._wispEL})if(!ListValid(*l)){ok=false;reason="players_retire_list_invalid";break;}
          if(ok){for(auto *l:{&list._mainEL,&list._persistentEL,&list._renderEL,&list._updateEL,&list._effectEL,&list._tempEL,&list._wispEL})Erase(*l,e.player);mark(ClientPlayerPrefix::Detached);}
          if(ok){if(e.prefix&Published){reinterpret_cast<void (__thiscall *)(EntityList *,int,int,int)>(Base+0x16d10)(&list,1,int(e.player->_variant),int(e.player->_subtype));mark(ClientPlayerPrefix::CountRetired);}
            e.player->_valid=false;e.prefix|=FreeEntered;o->prefix|=FreeEntered;e.player->Free(1);e.player=nullptr;e.retired=true;mark(ClientPlayerPrefix::Freed);ClientPlayerEquipmentDestroy(e.equipment);e.equipment=nullptr;ClientPlayerHudDestroy(e.hud);e.hud=nullptr;}
          }
        }
      }
    }
  }catch(...){ok=false;reason="players_retire_exception_prefix_retained";}
  o->busy=false;ClientDispatchReturned(ClientEntry::NativeTeardown);if(!dispatch.Succeeded()){ok=false;reason="players_retire_dispatch_incomplete";}
  if(!ok){o->fault=o->retirement_fault=true;std::snprintf(o->retirement_reason.data(),o->retirement_reason.size(),"%s",reason);reason=o->retirement_reason.data();}return Result(o,done&&ok,!ok,reason,true);
}
} // namespace cs::game
