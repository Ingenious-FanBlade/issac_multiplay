#include "game/client/player/client_player_equipment_native.h"
#include "game/tests/client_player_e2e.h"
#include "LuaCore.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <cmath>
#include <set>
#include <stdexcept>

namespace cs::game {
namespace {
using State=ClientPlayerEquipment::Weapon;
// VA表来自J273动态vtable/17槽、工厂及析构复读；表内数值全是产品常量，不接受协议提供偏移。
struct Contract final {unsigned vtable=0,main=0;std::vector<unsigned> refs;};
const Contract &Of(unsigned type){static const std::array<Contract,16> table={{
  {},{0x731ea4,0,{8}},{0x731db4,0x48,{8,0x48}},{0x731aac,0x44,{8,0x44}},
  {0x731d64,0x44,{8,0x44,0x48,0x4c,0x50}},{0x731a64,0x44,{8,0x44}},{0x731e5c,0x44,{8,0x44}},
  {0x731dfc,0,{8}},{0x731a1c,0x44,{8,0x44}},{0x731cd4,0,{8}},
  {0x731d1c,0x44,{8,0x44,0x48,0x4c,0x50,0x54,0x58}},{0x731d1c,0x44,{8,0x44,0x48,0x4c,0x50,0x54,0x58}},
  {0x731b6c,0x4c,{8,0x4c,0x50}},{0x731eec,0x44,{8,0x44}},{0x731c1c,0,{8}},{0x731bc4,0x44,{8,0x44}}
}};if(type<1||type>15)throw std::runtime_error("weapon_type_range");return table[type];}
// slot7/8只读真实chargebar结果；已复读9C5E80/9C4E60，只消费公开base最终量。
// 其他未被Client draw/HUD消费的Fire/Update缓存保留Authority-only，绝不逐字节复制。
float ChargeBar(Weapon &w,unsigned slot){auto *vt=*reinterpret_cast<std::uintptr_t **>(&w);return reinterpret_cast<float (__thiscall *)(Weapon *)>(vt[7+slot])(&w);}
EntityPtr &Link(Weapon &w,unsigned offset){return *reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(&w)+offset);}
std::string LinkName(unsigned offset){return "entity_"+std::to_string(offset);}
bool Type(Weapon *p,unsigned type){return p&&p->_weaponType==int(type)&&*reinterpret_cast<std::uintptr_t *>(p)==Base+Of(type).vtable;}
bool Same(const ClientObjectReference &a,const ClientObjectReference &b){return a.present==b.present&&a.is_null==b.is_null&&(a.is_null||a.target==b.target);}
bool Public(Weapon &w,const State &s,bool check,const char *&reason){if(check){if(w._fireDelay!=s.delay||w._maxFireDelay!=s.max_delay||w._charge!=s.charge||w._direction.x!=s.direction.x||w._direction.y!=s.direction.y||w._modifiers!=s.modifiers||w._numFired!=s.num_fired||w.GetMaxCharge()!=s.max_charge||w.IsAxisAligned()!=s.axis_aligned||ChargeBar(w,0)!=s.charge_bar[0]||ChargeBar(w,1)!=s.charge_bar[1]){reason="weapon_public_native_postcondition";return false;}}
  else {w._fireDelay=s.delay;w._maxFireDelay=s.max_delay;w._charge=s.charge;w._direction={s.direction.x,s.direction.y};w._modifiers=s.modifiers;w._numFired=s.num_fired;}return true;}
// ClearReferences会Remove攻击Entity，Urn还会停音；此处先用真实EntityPtr注销全部边，再调用
// 已核定的动态Free。base析构的Player/Familiar写入由owner=null避免，Entity19继续唯一拥有实体。
bool Release(ClientPlayerEquipmentOwner &owner,Entity_Player &p,unsigned slot,const char *&reason){auto &entry=owner.weapons[slot];if(entry.uncertain||entry.create_entered||entry.destroy_entered){reason="weapon_uncertain_lifetime_prefix_retained";return false;}
  if(p._weapon[slot]!=entry.pointer){reason="weapon_foreign_player_slot_owner";return false;}if(!entry.pointer)return true;
  if(!Type(entry.pointer,entry.type)){reason="weapon_dynamic_type_changed_retained";return false;}
  for(unsigned offset:Of(entry.type).refs)Link(*entry.pointer,offset).SetReference(nullptr);entry.pointer->_owner=nullptr;
  entry.destroy_entered=true;try{auto free=*reinterpret_cast<std::uintptr_t **>(entry.pointer);reinterpret_cast<void (__thiscall *)(void *,unsigned)>(free[0])(entry.pointer,1);}
  catch(...){entry.uncertain=true;owner.fault=true;reason="weapon_Free_exception_retained";return false;}
  p._weapon[slot]=nullptr;entry.pointer=nullptr;entry.type=0;entry.destroy_entered=false;return true;}
}
void ClientPlayerWeaponsCaptureLua(void *state,Entity_Player &p){auto *L=static_cast<lua_State *>(state);lua_newtable(L);std::set<Weapon *> unique;
  for(unsigned slot=0;slot<5;++slot){auto *w=p._weapon[slot];lua_newtable(L);lua_pushboolean(L,w!=nullptr);lua_setfield(L,-2,"present");
    if(w){if(w->_weaponType<1||w->_weaponType>15||!Type(w,unsigned(w->_weaponType))||!unique.insert(w).second)throw std::runtime_error("weapon_source_type_or_slot_alias");const auto &contract=Of(unsigned(w->_weaponType));
      lua_pushinteger(L,w->_weaponType);lua_setfield(L,-2,"type");lua_pushliteral(L,"public-base-only;gameplay-caches-authority-only");lua_setfield(L,-2,"scalar_policy");
      lua_newtable(L);for(unsigned i=0;i<2;++i){const auto amount=ChargeBar(*w,i);if(!std::isfinite(amount))throw std::runtime_error("weapon_charge_bar_nonfinite");lua_pushnumber(L,amount);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"charge_bar");
      lua_newtable(L);for(unsigned offset:contract.refs){const auto name=LinkName(offset);lua::luabridge::UserdataPtr::push(L,Link(*w,offset).GetReference(),lua::GetMetatableKey(lua::Metatables::ENTITY));lua_setfield(L,-2,name.c_str());}lua_setfield(L,-2,"links");
      lua_newtable(L);unsigned i=0;for(unsigned offset:contract.refs){lua_pushstring(L,LinkName(offset).c_str());lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"link_names");
      // 这些具体动态类没有内嵌ANM；item animation通过+8的真实EntityPtr交给Entity19。
      lua_newtable(L);lua_setfield(L,-2,"sprites");}
    lua_rawseti(L,-2,slot+1);}lua_setfield(L,-2,"weapons");}
bool ClientPlayerWeaponsPreflight(const ClientPlayerDescription &d,const char *&reason){if(!d.equipment){reason="weapon_target_missing";return false;}
  for(const auto &s:d.equipment->weapons){if(!s.present)continue;const auto &c=Of(unsigned(s.type));if(!s.owner.present||!s.main.present||s.links.size()!=c.refs.size()||!s.sprites.empty()){reason="weapon_incomplete_contract";return false;}
    if(!c.main&&!s.main.is_null){reason="weapon_type_has_no_main_entity";return false;}for(unsigned offset:c.refs){auto i=s.links.find(LinkName(offset));if(i==s.links.end()||!i->second.present||(offset==c.main&&!Same(i->second,s.main))){reason="weapon_typed_main_or_link_missing";return false;}}
    // table是当前游戏动态注册表，不从静态BSS的零推断不可创建，也不自行注册工厂。
    if(!*reinterpret_cast<void **>(Base+0x7e65a8+4*unsigned(s.type))){reason="weapon_native_factory_unavailable";return false;}}
  return true;}
bool ClientPlayerWeaponsInstall(ClientPlayerEquipmentOwner &owner,Entity_Player &p,const ClientPlayerDescription &d,bool check,const char *&reason){if(!d.equipment||owner.fault||owner.retired){reason="weapon_owner_not_current";return false;}
  for(unsigned slot=0;slot<5;++slot){const auto &s=d.equipment->weapons[slot];auto &e=owner.weapons[slot];if(e.uncertain||e.create_entered||e.destroy_entered||p._weapon[slot]!=e.pointer){reason="weapon_slot_or_lifetime_conflict";return false;}
    if(check){if(bool(e.pointer)!=s.present||(s.present&&(!Type(e.pointer,unsigned(s.type))||!Public(*e.pointer,s,true,reason))))return false;continue;}
    if(ClientPlayerTestFault("player-weapon-before",d.entity.key,0,unsigned(ClientPlayerPrefix::Resources),slot,e.generation)){reason="player-weapon-before";return false;}
    if(e.pointer&&(!s.present||e.type!=unsigned(s.type)))if(!Release(owner,p,slot,reason))return false;
    if(!s.present)continue;if(!e.pointer){if(owner.next_generation==UINT64_MAX){reason="weapon_generation_exhausted";return false;}e.create_entered=true;
      // fresh factory对象的Reset只处理已构造的空引用；不调用Fire/Update，不发布额外Entity。
      try{e.pointer=Isaac::CreateWeapon(WeaponType(s.type),&p);}catch(...){e.uncertain=true;owner.fault=true;reason="weapon_factory_Init_exception_retained";return false;}
      if(!e.pointer){e.create_entered=false;reason="weapon_factory_returned_null";return false;}e.type=unsigned(s.type);e.generation=++owner.next_generation;e.create_entered=false;p._weapon[slot]=e.pointer;
      if(!Type(e.pointer,e.type)){e.uncertain=true;owner.fault=true;reason="weapon_factory_dynamic_type_mismatch_retained";return false;}}
    if(!Public(*e.pointer,s,false,reason))return false;
    if(ClientPlayerTestFault("player-weapon-after",d.entity.key,0,unsigned(ClientPlayerPrefix::Resources),slot,e.generation)){reason="player-weapon-after";return false;}}
  return true;}
bool ClientPlayerWeaponsRelations(ClientPlayerEquipmentOwner &owner,Entity_Player &p,const ClientPlayerDescription &d,const ClientEntityOwners &entities,const ClientNativeRoots &roots,bool check,bool clear,const char *&reason){
  for(unsigned slot=0;slot<5;++slot){auto &e=owner.weapons[slot];if(e.uncertain||e.create_entered||e.destroy_entered||e.pointer!=p._weapon[slot]){reason="weapon_relation_lifetime_conflict";return false;}if(!e.pointer)continue;if(!Type(e.pointer,e.type)){reason="weapon_relation_dynamic_type";return false;}
    const auto &s=d.equipment->weapons[slot];const auto resolve=[&](const ClientObjectReference &ref,Entity *&target){target=nullptr;if(clear||ref.is_null)return true;ClientEntityBorrow b;if(!ref.present||!entities.resolve||!entities.resolve(entities.context,roots,ref.target,13,b,reason)||!b.object)return false;target=static_cast<Entity *>(b.object);return true;};
    Entity *target=nullptr;if(!clear&&(!s.present||!resolve(s.owner,target)))return false;if(check){if(e.pointer->_owner!=target){reason="weapon_owner_postcondition";return false;}}else e.pointer->_owner=target;
    for(unsigned offset:Of(e.type).refs){target=nullptr;if(!clear){auto found=s.links.find(LinkName(offset));if(found==s.links.end()||!resolve(found->second,target))return false;}
      if(check){if(Link(*e.pointer,offset).GetReference()!=target){reason="weapon_entityptr_postcondition";return false;}}else Link(*e.pointer,offset).SetReference(target);}
    if(check&&!clear){target=nullptr;if(!resolve(s.main,target)||e.pointer->GetMainEntity()!=target){reason="weapon_dynamic_main_entity_postcondition";return false;}}}
  return true;}
bool ClientPlayerWeaponsRetire(ClientPlayerEquipmentOwner &owner,Entity_Player &p,const char *&reason){for(unsigned slot=0;slot<5;++slot)if(!Release(owner,p,slot,reason))return false;reason="weapon_owned_slots_retired";return true;}
} // namespace cs::game
