#include "game/client_player_equipment_native.h"
#include "game/client_entity_e2e.h"
#include "LuaCore.h"
#include "game/world_sources/lua_entity.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <stdexcept>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
constexpr unsigned Limit=16384;
static_assert(_ITERATOR_DEBUG_LEVEL==0&&sizeof(TemporaryEffect)==0x10&&sizeof(CostumeSpriteDesc)==0x130&&sizeof(PlayerCostumeMap)==0x10,"J273 equipment ABI");
bool Window(){void *dc=nullptr,*rc=nullptr;return IsMain()&&ClientBusy()&&ContextHealthy()&&!InBlockedContext()&&HasOwner(Owner::Manager)&&ClientLuaGuardReady()&&ClientCallbackGuardReady()&&ClientRendererHandles(dc,rc)&&dc&&rc&&wglGetCurrentDC()==dc&&wglGetCurrentContext()==rc&&ClientNativeCpuQueuesEmpty();}
void Integer(lua_State *L,const char *name,std::int64_t x){lua_pushinteger(L,x);lua_setfield(L,-2,name);}
void Boolean(lua_State *L,const char *name,bool x){lua_pushboolean(L,x);lua_setfield(L,-2,name);}
// 只解析当前机器的配置；Null配置和NullItem(类型0)是不同状态，不能按id==0合并。
ItemConfig_Item *Config(bool present,int type,int id){if(!present)return nullptr;if(!g_Manager||!g_Manager->GetItemConfig()||id<0)return nullptr;auto &c=*g_Manager->GetItemConfig();
  auto *item=type==0?c.GetNullItem(unsigned(id)):type==2?c.GetTrinket(unsigned(id)):c.GetCollectible(id);return item&&item->id==id&&item->type==type?item:nullptr;}
bool Configs(const ClientPlayerEquipment &s,const char *&reason){for(const auto &e:s.effects)if(e.item_present&&!Config(true,e.type,e.id)){reason="player_effect_local_config";return false;}
  for(const auto &c:s.costumes)if(c.item_present&&!Config(true,c.item_type,c.item)){reason="player_costume_local_config";return false;}return true;}
bool Pose(ANM2 &sprite,const ClientResourcePose &p,bool check,const char *&reason,bool *uncertain=nullptr){if(check){if(!ClientResourcePoseMatches(&sprite,p,&reason))return false;return true;}
  const auto result=ClientApplyResourcePose(&sprite,p);reason=result.reason;
  // 资源服务会捕获原生异常；实际成员已进入写入时，不能继续析构不确定的内嵌ANM。
  if(uncertain&&result.written&&!result.complete&&!std::strcmp(result.reason,"resource_native_exception"))*uncertain=true;
  return result.complete;}
ClientPlayerEquipmentOwner::Costumes View(Entity_Player &p){ClientPlayerEquipmentOwner::Costumes out;
  struct Header {CostumeSpriteDesc *first,*last,*capacity;};static_assert(sizeof(p._costumeSpriteDescs)==sizeof(Header),"Release costume vector geometry");
  const auto &v=*reinterpret_cast<const Header *>(&p._costumeSpriteDescs);out.first=v.first;out.last=v.last;out.capacity=v.capacity;return out;}
void Publish(Entity_Player &p,const ClientPlayerEquipmentOwner::Costumes &v){struct Header {CostumeSpriteDesc *first,*last,*capacity;};auto &h=*reinterpret_cast<Header *>(&p._costumeSpriteDescs);h.first=v.first;h.last=v.last;h.capacity=v.capacity;}
void *Allocate(std::size_t bytes){return reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(bytes);}
void Free(void *p){reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(p);}
// 与游戏Release STL相同的大数组对齐。只分配本机新存储，不移植Source vector头或ANM字节。
void AllocateCostumes(ClientPlayerEquipmentOwner::Costumes &v,unsigned count){if(!count)return;const auto bytes=std::size_t(count)*sizeof(CostumeSpriteDesc);v.allocation=Allocate(bytes+(bytes>=4096?39:0));if(!v.allocation)throw std::bad_alloc();
  auto address=reinterpret_cast<std::uintptr_t>(v.allocation);if(bytes>=4096){address=(address+35)&~std::uintptr_t(31);reinterpret_cast<void **>(address)[-1]=v.allocation;}
  v.first=v.last=reinterpret_cast<CostumeSpriteDesc *>(address);v.capacity=v.first+count;}
bool ReleaseCostumes(ClientPlayerEquipmentOwner::Costumes &v,const char *&reason){if(v.uncertain||v.destructor_entered||v.constructor_entered){reason="player_costume_uncertain_prefix_retained";return false;}
  while(v.released<v.constructed){v.destructor_entered=true;try{v.first[v.released]._sprite.destructor();}catch(...){v.uncertain=true;reason="player_costume_destructor_exception_retained";return false;}v.destructor_entered=false;++v.released;}
  if(v.first){void *base=v.allocation;if(!base){const auto bytes=std::size_t(v.capacity-v.first)*sizeof(CostumeSpriteDesc);base=bytes>=4096?reinterpret_cast<void **>(v.first)[-1]:v.first;}v.destructor_entered=true;Free(base);}
  v={};return true;}
bool Meta(CostumeSpriteDesc &n,const ClientPlayerEquipment::Costume &c,bool check){auto *config=Config(c.item_present,c.item_type,c.item);
  if(check)return n._item==config&&n._priority==c.priority&&n._itemAnimPlay==c.item_animation&&n._isFlying==c.flying&&n._hasOverlay==c.overlay&&n._hasSkinAlt==c.skin_alt&&n._defaultSkinColor==c.default_skin&&n._skinColor==c.skin&&n._overwriteColor==c.overwrite&&n._itemStateOnly==c.state_only&&n._playerType==c.player_type;
  n._item=config;n._priority=c.priority;n._itemAnimPlay=c.item_animation;n._isFlying=c.flying;n._hasOverlay=c.overlay;n._hasSkinAlt=c.skin_alt;n._defaultSkinColor=c.default_skin;n._skinColor=c.skin;n._overwriteColor=c.overwrite;n._itemStateOnly=c.state_only;n._playerType=c.player_type;return true;}
bool Layers(Entity_Player &p,const ClientPlayerEquipment &s,bool check){for(unsigned i=0;i<15;++i){const auto &x=s.layers[i];auto &n=p._playerCostumeMap[i];if(check){if(n._index!=x.costume||n._layerID!=x.layer||n._priority!=x.priority||n._isBodyLayer!=x.body)return false;}else {n._index=x.costume;n._layerID=x.layer;n._priority=x.priority;n._isBodyLayer=x.body;}}return true;}
// 全量换装先构造全部候选资源，之后换入实际Player容器；旧ANM逐项析构，空目标也会清旧项。
bool Costumes(ClientPlayerEquipmentOwner &owner,Entity_Player &p,const ClientPlayerEquipment &s,bool check,const char *&reason){auto actual=View(p);const auto count=p._costumeSpriteDescs.size();
  if(count>1024||actual.first!=owner.installed||count!=owner.installed_count){reason="player_costume_foreign_or_changed_owner";return false;}
  if(check){if(count!=s.costumes.size()){reason="player_costume_count_postcondition";return false;}for(unsigned i=0;i<count;++i)if(!Meta(actual.first[i],s.costumes[i],true)||!Pose(actual.first[i]._sprite,s.costumes[i].sprite,true,reason))return false;return Layers(p,s,true);}
  if(owner.candidate.first||owner.displaced.first){reason="player_costume_unfinished_candidate_retained";return false;}
  bool same_structure=count==s.costumes.size();for(unsigned i=0;same_structure&&i<count;++i)same_structure=Meta(actual.first[i],s.costumes[i],true)&&actual.first[i]._sprite._filename==s.costumes[i].sprite.path;
  if(same_structure){for(unsigned i=0;i<count;++i)if(!Pose(actual.first[i]._sprite,s.costumes[i].sprite,false,reason,&owner.fault))return false;return Layers(p,s,false);}
  if(owner.costume_generation==UINT64_MAX){reason="player_costume_generation_exhausted";return false;}
  auto &next=owner.candidate;AllocateCostumes(next,unsigned(s.costumes.size()));
  for(unsigned i=0;i<s.costumes.size();++i){auto &n=next.first[i];next.constructor_entered=true;n._sprite.constructor();next.constructor_entered=false;++next.constructed;++next.last;Meta(n,s.costumes[i],false);
    // 已完成Ctor的候选只有未知原生写入才保留；写前预算/缺资源拒绝仍可唯一退休。
    if(!Pose(n._sprite,s.costumes[i].sprite,false,reason,&next.uncertain))return false;}
  owner.displaced=actual;owner.displaced.constructed=unsigned(count);Publish(p,next);owner.installed=next.first;owner.installed_count=unsigned(s.costumes.size());++owner.costume_generation;next={};
  if(!Layers(p,s,false))return false;return ReleaseCostumes(owner.displaced,reason);}
}
// 正式补充与公开只读Getter属于同一ordinary/use，未知来源不伪装空效果或空武器。
int ClientPlayerEquipmentCaptureLua(lua_State *L){const auto started=ClientEntityTestClock();const auto top=lua_gettop(L);auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");const auto subject=ObservationSubject(p,false);bool failed=false;
  lua_newtable(L);Integer(L,"version",1);lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  if(!IsMain()||!p||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  try {auto &effects=p->_temporaryeffects;if(effects._effects.size()>Limit)throw std::runtime_error("player_effect_budget");
    lua_newtable(L);std::uint32_t cache=0;std::memcpy(&cache,&effects,4);Integer(L,"cache_flags",cache);Boolean(L,"evaluate",effects._shouldEvaluateCache);Boolean(L,"disabled",effects._disabled);lua_newtable(L);unsigned i=0;
    for(const auto &e:effects._effects){lua_newtable(L);Boolean(L,"item_present",e._item!=nullptr);Integer(L,"type",e._item?e._item->type:0);Integer(L,"id",e._item?e._item->id:0);Integer(L,"count",e._count);Integer(L,"cooldown",e._cooldown);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"entries");lua_setfield(L,-2,"effects");
    lua_newtable(L);for(i=0;i<15;++i){lua_pushinteger(L,p->_playerForms[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"forms");
    std::uint8_t history_disabled=0;std::memcpy(&history_disabled,reinterpret_cast<unsigned char *>(&p->_history)+0xc,1);Boolean(L,"history_mutation_disabled",history_disabled!=0);
    if(p->_headAnim.size()>2048)throw std::runtime_error("player_head_animation_budget");std::string head_hex;head_hex.reserve(p->_headAnim.size()*2);const char *digits="0123456789abcdef";
    for(unsigned char c:p->_headAnim){head_hex.push_back(digits[c>>4]);head_hex.push_back(digits[c&15]);}lua_pushlstring(L,head_hex.data(),head_hex.size());lua_setfield(L,-2,"head_animation_hex");
    ClientPlayerWeaponsCaptureLua(L,*p);lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
  }catch(...){failed=true;}ClientEntityTestTime("player_source_equipment",ClientEntityTestClock()-started);if(failed){lua_settop(L,top);return luaL_error(L,"player_equipment_capture_failed");}return 1;
}
ClientPlayerEquipmentOwner *ClientPlayerEquipmentCreate() noexcept {return IsMain()?new(std::nothrow) ClientPlayerEquipmentOwner:nullptr;}
void ClientPlayerEquipmentDestroy(ClientPlayerEquipmentOwner *o) noexcept {if(!o||!IsMain()||o->fault||o->candidate.first||o->displaced.first)return;for(const auto &w:o->weapons)if(w.pointer||w.create_entered||w.uncertain)return;delete o;}
bool ClientPlayerEquipmentPreflight(const ClientPlayerDescription &d,const char *&reason){if(!IsMain()||!d.equipment||!d.equipment->history||!(d.equipment->history->body==d.entity.key)||d.equipment->history->shape!=d.entity.shape_revision){reason="player_equipment_source_or_history_identity";return false;}
  return Configs(*d.equipment,reason)&&ClientPlayerWeaponsPreflight(d,reason);}
bool ClientPlayerEquipmentFields(void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){if(!Window()||!pointer||!d.equipment||!d.equipment->history){reason="player_equipment_fields_context";return false;}auto &p=*static_cast<Entity_Player *>(pointer);const auto &s=*d.equipment;if(!Configs(s,reason))return false;
  decltype(p._temporaryeffects._effects) candidate;candidate.reserve(s.effects.size());for(const auto &e:s.effects){TemporaryEffect n{};n._item=Config(e.item_present,e.type,e.id);n._count=e.count;n._cooldown=e.cooldown;candidate.push_back(n);}
  auto &effects=p._temporaryeffects;
  std::uint32_t cache=0;std::memcpy(&cache,&effects,4);
  if(check){if(cache!=s.effects_cache||effects._effects.size()!=candidate.size()||effects._disabled!=s.effects_disabled||effects._shouldEvaluateCache!=s.effects_evaluate||effects._player!=&p){reason="player_effect_owner_or_flags_postcondition";return false;}
    for(unsigned i=0;i<candidate.size();++i)if(effects._effects[i]._item!=candidate[i]._item||effects._effects[i]._count!=candidate[i]._count||effects._effects[i]._cooldown!=candidate[i]._cooldown){reason="player_effect_ordered_values_postcondition";return false;}}
  else {effects._effects.swap(candidate);std::memcpy(&effects,&s.effects_cache,4);effects._disabled=s.effects_disabled;effects._shouldEvaluateCache=s.effects_evaluate;effects._player=&p;}
  for(unsigned i=0;i<15;++i){if(check){if(p._playerForms[i]!=s.forms[i]){reason="player_form_counter_postcondition";return false;}}else p._playerForms[i]=s.forms[i];}
  decltype(p._history._historyItems) history;history.reserve(s.history->items.size());
  for(const auto &x:s.history->items){History_HistoryItem n(TrinketType(0),0,0,0,0,0);n._time=x.time;n._isTrinket=x.trinket;n._itemID=x.id;n._levelStage=x.stage;n._stageType=x.stage_type;n._roomType=x.room;n._itemPoolType=x.pool;history.push_back(n);}
  if(check){if(p._history._historyItems.size()!=history.size()){reason="player_history_count_postcondition";return false;}for(unsigned i=0;i<history.size();++i){const auto &a=p._history._historyItems[i],&b=history[i];if(a._time!=b._time||a._isTrinket!=b._isTrinket||a._itemID!=b._itemID||a._levelStage!=b._levelStage||a._stageType!=b._stageType||a._roomType!=b._roomType||a._itemPoolType!=b._itemPoolType){reason="player_history_ordered_postcondition";return false;}}}
  else p._history._historyItems.swap(history);
  // Add/RemoveHistoryItem在+0xC读取禁用字节；不调用这些方法，不触碰未核定的+0x10。
  auto *disabled=reinterpret_cast<std::uint8_t *>(&p._history)+0xc;
  if(check){if((*disabled!=0)!=s.history_mutation_disabled){reason="player_history_mutation_disabled_postcondition";return false;}}else *disabled=s.history_mutation_disabled?1:0;
  reason=check?"player_effect_forms_history_checked":"player_effect_forms_history_installed";return true;
}
bool ClientPlayerEquipmentResources(ClientPlayerEquipmentOwner *owner,void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){if(!Window()||!owner||owner->fault||owner->retired||!pointer||!d.equipment){reason="player_equipment_resource_context";return false;}auto &p=*static_cast<Entity_Player *>(pointer);if(owner->player&&owner->player!=&p){reason="player_equipment_instance_changed";return false;}if(!check)owner->player=&p;
  try {const auto &s=*d.equipment;
    // ctor构造真实字符串；773990写HeadLeft/Up/Right/Down，783680读取并派生Overlay。
    // 标准字符串赋值维护SSO/heap，不复制Authority的24字节对象表示。
    if(check){if(p._headAnim!=s.head_animation){reason="player_head_animation_postcondition";return false;}}else p._headAnim=s.head_animation;
    if(!Pose(p._sprite,s.main,check,reason,&owner->fault))return false;for(unsigned i=0;i<13;++i)if(!Pose(*reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&p)+0x41c+i*0x114),s.inline_sprites[i],check,reason,&owner->fault))return false;
    if(!Costumes(*owner,p,s,check,reason)||!ClientPlayerWeaponsInstall(*owner,p,d,check,reason))return false;return true;
  }catch(...){if(!check)owner->fault=true;reason="player_equipment_resource_exception_prefix_retained";return false;}}
bool ClientPlayerEquipmentRelations(ClientPlayerEquipmentOwner *o,void *p,const ClientPlayerDescription &d,const ClientEntityOwners &entities,const ClientNativeRoots &roots,bool check,bool clear,const char *&reason){
  // 字段阶段失败时子owner可能尚未进入资源前缀；clear仍核对五槽为空，允许安全退休已构造Player。
  if(!Window()||!o||!p||(o->player&&o->player!=p)||(!o->player&&!clear)||!d.equipment){reason="player_equipment_relation_context";return false;}return ClientPlayerWeaponsRelations(*o,*static_cast<Entity_Player *>(p),d,entities,roots,check,clear,reason);}
bool ClientPlayerEquipmentRetire(ClientPlayerEquipmentOwner *o,void *pointer,const char *&reason){if(!o)return true;if(!Window()||!pointer||o->fault){reason="player_equipment_retire_context_or_uncertain";return false;}auto &p=*static_cast<Entity_Player *>(pointer);if(o->player&&o->player!=&p){reason="player_equipment_retire_instance";return false;}
  try {if(!ClientPlayerWeaponsRetire(*o,p,reason)||!ReleaseCostumes(o->candidate,reason)||!ReleaseCostumes(o->displaced,reason))return false;
    // 当前costume与内嵌ANM仍由Player析构，子owner不重复析构。先清层映射，撤销外部借用。
    for(auto &l:p._playerCostumeMap){l._index=-1;l._layerID=0;l._priority=0;l._isBodyLayer=false;}o->retired=true;return true;
  }catch(...){o->fault=true;reason="player_equipment_retire_exception_retained";return false;}}
} // namespace cs::game
