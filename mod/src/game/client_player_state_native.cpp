#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_player_spoof_layout.h"
#include "game/client_player_state.h"
#include "game/client_entity_e2e.h"
#include "game/world_sources/lua_entity.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <set>
#include <stdexcept>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace cs::game {
namespace {
constexpr unsigned Limit=16384;
// 官方1.1.3 EntityPlus/ItemSpoofSystem源码布局；只改真实容器内容，保留原虚表及holder。
// 禁用调试迭代器，使用与官方相同的v143 /MD；不向官方容器移交本地自定义allocator。
static_assert(_ITERATOR_DEBUG_LEVEL==0,"Player最终容器要求官方Release STL ABI");
using player_sdk::Timed;using player_sdk::Innate;using player_sdk::Spoof;using player_sdk::PlusItems;
// holder由官方Entity ctor拥有，Plus由官方base Init拥有；读取不调用会分配的新建fallback。
PlusItems *Items(Entity_Player &p){const auto address=(std::uintptr_t(p._entityExtraDataPtrUpperBytes)<<16)|p._entityExtraDataPtrLowerBytes;
  if(!address)return nullptr;auto *plus=*reinterpret_cast<player_sdk::PlusHeader **>(address);
  if(!plus||std::strcmp(typeid(*plus).name(),"class EntityPlayerPlus"))return nullptr;
  return reinterpret_cast<PlusItems *>(reinterpret_cast<unsigned char *>(plus)+sizeof(player_sdk::PlusHeader));
}
void Number(lua_State *L,const char *key,float n){if(!std::isfinite(n))throw std::runtime_error("player_state_nonfinite");lua_pushnumber(L,n);lua_setfield(L,-2,key);}
void Integer(lua_State *L,const char *key,std::int64_t n){lua_pushinteger(L,n);lua_setfield(L,-2,key);}
void Boolean(lua_State *L,const char *key,bool b){lua_pushboolean(L,b);lua_setfield(L,-2,key);}
template<class T> void Scalar(lua_State *L,const char *key,T n){if constexpr(std::is_same_v<T,bool>)Boolean(L,key,n);else if constexpr(std::is_floating_point_v<T>)Number(L,key,n);else Integer(L,key,n);}
template<class T> bool Bounded(const T &v){return v.size()<=Limit;}
template<class T> T &Raw(Entity_Player &p,unsigned offset){return *reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(&p)+offset);}
void Scalar(lua_State *L,const char *key,const BitSet128 &v){lua_newtable(L);const unsigned words[]={v.ll,v.lh,v.hl,v.hh};for(unsigned i=0;i<4;++i){lua_pushinteger(L,words[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,key);}
void Scalar(lua_State *L,const char *key,const ColorMod &v){lua_newtable(L);unsigned n=0;for(float x:v._tint){if(!std::isfinite(x))throw std::runtime_error("player_color_nonfinite");lua_pushnumber(L,x);lua_rawseti(L,-2,++n);}for(float x:v._colorize){if(!std::isfinite(x))throw std::runtime_error("player_color_nonfinite");lua_pushnumber(L,x);lua_rawseti(L,-2,++n);}for(float x:v._offset){if(!std::isfinite(x))throw std::runtime_error("player_color_nonfinite");lua_pushnumber(L,x);lua_rawseti(L,-2,++n);}lua_setfield(L,-2,key);}
template<class T> void Integers(lua_State *L,const char *key,const T &v){if(!Bounded(v))throw std::runtime_error("player_inventory_budget");lua_newtable(L);unsigned i=0;for(auto n:v){lua_pushinteger(L,n);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,key);}
void CaptureSpoof(lua_State *L,const char *key,const Spoof &s){
  if(!Bounded(s.blocked)||!Bounded(s.groups))throw std::runtime_error("player_spoof_budget");lua_newtable(L);
  const std::set<int> blocked(s.blocked.begin(),s.blocked.end());Integers(L,"blocked",blocked);lua_newtable(L);unsigned index=0,total=0;
  for(const auto &pair:s.groups){const auto &g=pair.second;if(g.persistent.size()>Limit||g.temporary.size()>Limit)throw std::runtime_error("player_innate_budget");total+=unsigned(g.persistent.size()+g.temporary.size());if(total>Limit||pair.first.size()>4096)throw std::runtime_error("player_innate_budget");
    lua_newtable(L);lua_pushlstring(L,pair.first.data(),pair.first.size());lua_setfield(L,-2,"key");lua_newtable(L);unsigned i=0;
    const std::map<int,int> sorted(g.persistent.begin(),g.persistent.end());
    for(const auto &p:sorted){lua_newtable(L);Integer(L,"id",p.first);Integer(L,"count",p.second);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"persistent");
    lua_newtable(L);i=0;std::unordered_map<int,int> totals;
    for(const auto &t:g.temporary){if(t.count<=0||std::int64_t(totals[t.id])+t.count>INT32_MAX)throw std::runtime_error("player_innate_count");totals[t.id]+=t.count;
      lua_newtable(L);Integer(L,"id",t.id);Integer(L,"count",t.count);Integer(L,"timer",t.timer);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"temporary");
    if(totals!=g.totals)throw std::runtime_error("player_innate_cache_mismatch");lua_rawseti(L,-2,++index);}
  lua_setfield(L,-2,"groups");lua_setfield(L,-2,key);
}
// QueueItemData位置由J273 getter补证；所有ItemConfig指针都只在本机解析。
QueueItemData &Queue(Entity_Player &p){return *reinterpret_cast<QueueItemData *>(reinterpret_cast<unsigned char *>(&p)+0x17b4);}
void Capture(lua_State *L,Entity_Player &p){
  lua_pushinteger(L,1);lua_setfield(L,-2,"version");lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  auto *items=Items(p);if(!items){lua_pushliteral(L,"Unavailable");lua_setfield(L,-2,"status");return;}
  Integer(L,"player_type",p._playerType);
  lua_newtable(L);
#define CS_PLAYER_FIELD(id,field,type,kind) if(ClientPlayerStateApplicable(id,p._playerType))Scalar(L,id,field);else {lua_newtable(L);lua_pushliteral(L,"NotApplicable");lua_setfield(L,-2,"status");lua_setfield(L,-2,id);}
#include "game/client_player_fields.inc"
#undef CS_PLAYER_FIELD
  lua_setfield(L,-2,"values");Integers(L,"collectibles",p._collectibleCounts);Integers(L,"voided",p._voidedCollectibles);
  if(!Bounded(p._itemWispsList))throw std::runtime_error("player_wisp_budget");lua_newtable(L);unsigned wisp_index=0;for(const auto &w:p._itemWispsList){lua_newtable(L);Integer(L,"id",w.first);Integer(L,"count",w.second);lua_rawseti(L,-2,++wisp_index);}lua_setfield(L,-2,"wisps");
  if(!Bounded(p._smeltedTrinkets))throw std::runtime_error("player_smelted_budget");lua_newtable(L);unsigned i=0;
  for(const auto &s:p._smeltedTrinkets){lua_newtable(L);Integer(L,"normal",s._trinketNum);Integer(L,"golden",s._goldenTrinketNum);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,"smelted");
  lua_newtable(L);for(i=0;i<2;++i){lua_pushinteger(L,p._trinketsID[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"trinkets");
  lua_newtable(L);for(i=0;i<4;++i){const auto &a=p._activeDesc[i];lua_newtable(L);Integer(L,"item",a._item);Integer(L,"item_pool",a._unk);Integer(L,"charge",a._charge);Integer(L,"battery",a._batteryCharge);Integer(L,"sub",a._subCharge);Integer(L,"cooldown",a._timeRechargeCooldown);
    float partial=0;std::memcpy(&partial,&a._partialCharge,4);Number(L,"partial",partial);Integer(L,"var_data",a._varData);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"active");
  lua_newtable(L);for(i=0;i<4;++i){auto *pocket=p.GetPocketItem(i);if(!pocket)throw std::runtime_error("player_pocket_storage");lua_newtable(L);Integer(L,"id",pocket->_id);Integer(L,"type",pocket->_type);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"pocket");
  const auto &q=Queue(p);lua_newtable(L);Integer(L,"item",q._item?q._item->id:0);Integer(L,"type",q._item?q._item->type:0);Integer(L,"charge",q._charge);Integer(L,"flags",q._flags);Integer(L,"var_data",q._vardata);lua_setfield(L,-2,"queue");
  CaptureSpoof(L,"collectible_spoof",items->collectible);CaptureSpoof(L,"trinket_spoof",items->trinket);
  lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
}
template<class T> bool Store(T &native,const ClientEntityField &f,bool check){
  if constexpr(std::is_same_v<T,bool>){if(f.kind!=ClientEntityValueKind::Boolean)return false;if(check)return native==f.boolean;native=f.boolean;}
  else if constexpr(std::is_floating_point_v<T>){if(f.kind!=ClientEntityValueKind::Real)return false;if(check)return native==f.real;native=f.real;}
  else if constexpr(std::is_same_v<T,BitSet128>){const BitSet128 value(unsigned(f.bits),unsigned(f.bits>>32),unsigned(f.high_bits),unsigned(f.high_bits>>32));if(check)return std::memcmp(&native,&value,16)==0;native=value;}
  else if constexpr(std::is_same_v<T,ColorMod>){const auto &c=f.color;const ColorMod value(c.tint[0],c.tint[1],c.tint[2],c.tint[3],c.offset[0],c.offset[1],c.offset[2],c.colorize[0],c.colorize[1],c.colorize[2],c.colorize[3]);if(check)return native==value;native=value;}
  else {if(f.kind!=ClientEntityValueKind::Integer)return false;if(check)return native==T(f.integer);native=T(f.integer);}return true;
}
template<class T> bool Clear(T &native,bool check){const T value{};if(check)return std::memcmp(&native,&value,sizeof(T))==0;native=value;return true;}
ItemConfig_Item *Config(int id,bool trinket=false){if(!id||(trinket&&(unsigned(id)&~0xffffu)))return nullptr;return g_Manager&&g_Manager->GetItemConfig()?(trinket?g_Manager->GetItemConfig()->GetTrinket(id&0x7fff):g_Manager->GetItemConfig()->GetCollectible(id)):nullptr;}
bool ValidateConfig(const ClientPlayerState &s,const char *&reason){
  if(!g_Manager||!g_Manager->GetItemConfig()){reason="player_item_config_unavailable";return false;}
  for(unsigned i=0;i<s.collectibles.size();++i)if(s.collectibles[i]<0||(s.collectibles[i]&&!Config(i))){reason="player_collectible_config_or_count";return false;}
  for(auto id:s.voided)if(!Config(id)){reason="player_voided_config";return false;}
  for(const auto &w:s.wisps)if(!Config(w.first)){reason="player_wisp_config";return false;}
  for(auto id:s.trinkets)if(id&&!Config(int(id),true)){reason="player_trinket_config";return false;}
  for(unsigned i=0;i<s.smelted.size();++i)if(s.smelted[i][0]<0||s.smelted[i][1]<0||((s.smelted[i][0]||s.smelted[i][1])&&!Config(i,true))){reason="player_smelted_config_or_count";return false;}
  for(const auto &a:s.active)if(a.item&&!Config(a.item)){reason="player_active_config";return false;}
  for(const auto &p:s.pocket)if(p.id){
    if(p.type==1&&!g_Manager->GetItemConfig()->GetCard(p.id)){reason="player_card_config";return false;}
    if(p.type==0&&((p.id&~0x80f)!=0||(p.id&0x7ff)<1||(p.id&0x7ff)>14)){reason="player_pill_color";return false;}
    if(p.type==2&&!s.active[unsigned(p.id-1)].item){reason="player_pocket_active_unresolved";return false;}}
  const auto &q=s.queue;if((q.item&&(!Config(q.item,q.type==2)||Config(q.item,q.type==2)->type!=q.type))||(!q.item&&q.type)){reason="player_queue_config";return false;}
  for(unsigned i=0;i<2;++i){for(auto id:s.spoofs[i].blocked)if(!Config(id,i==1)){reason="player_blocked_config";return false;}
    for(const auto &g:s.spoofs[i].groups){for(const auto &p:g.second.persistent)if(!Config(p.first,i==1)){reason="player_innate_config";return false;}
      for(const auto &t:g.second.temporary)if(!Config(t.id,i==1)){reason="player_innate_temporary_config";return false;}}}
  return true;
}
// 各候选容器先分配完再swap；保留官方虚表，空目标也会清除旧group/blocked/计数缓存。
Spoof Candidate(const ClientPlayerState::Spoof &source){Spoof c{};c.blocked.insert(source.blocked.begin(),source.blocked.end());for(const auto &pair:source.groups){auto &g=c.groups[pair.first];g.persistent.insert(pair.second.persistent.begin(),pair.second.persistent.end());for(const auto &t:pair.second.temporary){g.temporary.push_back({t.id,t.count,t.timer});g.totals[t.id]+=t.count;}}return c;}
bool SameSpoof(const Spoof &actual,const Spoof &wanted){if(actual.blocked!=wanted.blocked||actual.groups.size()!=wanted.groups.size())return false;
  for(const auto &p:wanted.groups){auto hit=actual.groups.find(p.first);if(hit==actual.groups.end())return false;const auto &a=hit->second,&b=p.second;
    if(a.persistent!=b.persistent||a.totals!=b.totals||a.temporary.size()!=b.temporary.size())return false;
    for(unsigned i=0;i<a.temporary.size();++i)if(a.temporary[i].id!=b.temporary[i].id||a.temporary[i].count!=b.temporary[i].count||a.temporary[i].timer!=b.temporary[i].timer)return false;}return true;}
}
int ClientPlayerStateCaptureLua(lua_State *L){const auto started=ClientEntityTestClock();auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");const auto subject=ObservationSubject(p,false);const int top=lua_gettop(L);bool failed=false;
  lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  if(!IsMain()||!p||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  try{Capture(L,*p);}catch(...){failed=true;}ClientEntityTestTime("player_source_final_state",ClientEntityTestClock()-started);if(failed){lua_settop(L,top);return luaL_error(L,"player_state_capture_failed");}return 1;
}
bool ClientPlayerStatePreflight(const ClientPlayerDescription &d,const char *&reason){if(!IsMain()||!ClientBusy()||!d.state||d.player_type!=d.state->player_type){reason="player_final_state_missing_type_conflict_or_wrong_thread";return false;}
  for(unsigned i=0;i<4;++i)if(!d.active_backups[i].present){reason="player_esau_backup_relation_missing";return false;}return ValidateConfig(*d.state,reason);}
bool ClientPlayerStateFields(void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){
  if(!pointer||!d.state||!IsMain()||!ClientBusy()||!HasOwner(Owner::Manager)||!ContextHealthy()||!ClientLuaGuardReady()||!ClientCallbackGuardReady()){reason="player_final_state_context";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);if(p._type!=1||p._playerType!=d.player_type||!ValidateConfig(*d.state,reason))return false;auto *items=Items(p);if(!items){reason="player_official_plus_owner_missing";return false;}
  const auto &s=*d.state;
  // 不适用成员仍保持NotApplicable语义；本地专用存储清零仅为退休旧角色缓存，不伪造Source读值。
  for(const auto &id:s.not_applicable){bool found=false,ok=true;
#define CS_PLAYER_FIELD(name,field,type,kind) if(id==name){found=true;ok=Clear(field,check);}
#include "game/client_player_fields.inc"
#undef CS_PLAYER_FIELD
    if(!found||!ok){reason="player_not_applicable_stale_storage";return false;}}
  for(const auto &f:s.fields){bool found=false,ok=false;
#define CS_PLAYER_FIELD(id,field,type,kind) if(f.member==id){found=true;ok=Store(field,f,check);}
#include "game/client_player_fields.inc"
#undef CS_PLAYER_FIELD
    if(!found||!ok){reason=f.member.c_str();return false;}}
  decltype(p._collectibleCounts) counts(s.collectibles.begin(),s.collectibles.end());decltype(p._voidedCollectibles) voided(s.voided.begin(),s.voided.end());
  decltype(p._smeltedTrinkets) smelted;for(const auto &t:s.smelted){SmeltedTrinketDesc row{};row._trinketNum=t[0];row._goldenTrinketNum=t[1];smelted.push_back(row);}
  decltype(p._itemWispsList) wisps(s.wisps.begin(),s.wisps.end());
  auto collectible=Candidate(s.spoofs[0]),trinket=Candidate(s.spoofs[1]);
  if(check){if(p._collectibleCounts!=counts||p._voidedCollectibles!=voided||p._itemWispsList!=wisps||p._smeltedTrinkets.size()!=smelted.size()||items->last_type!=d.player_type||!SameSpoof(items->collectible,collectible)||!SameSpoof(items->trinket,trinket)){reason="player_inventory_native_postcondition";return false;}
    for(unsigned i=0;i<smelted.size();++i)if(p._smeltedTrinkets[i]._trinketNum!=smelted[i]._trinketNum||p._smeltedTrinkets[i]._goldenTrinketNum!=smelted[i]._goldenTrinketNum){reason="player_smelted_native_postcondition";return false;}}
  else {p._collectibleCounts.swap(counts);p._voidedCollectibles.swap(voided);p._itemWispsList.swap(wisps);p._smeltedTrinkets.swap(smelted);items->collectible.blocked.swap(collectible.blocked);items->collectible.groups.swap(collectible.groups);items->trinket.blocked.swap(trinket.blocked);items->trinket.groups.swap(trinket.groups);items->last_type=d.player_type;}
  for(unsigned i=0;i<2;++i){if(check){if(p._trinketsID[i]!=s.trinkets[i]){reason="player_trinket_slot_postcondition";return false;}}else p._trinketsID[i]=s.trinkets[i];}
  for(unsigned i=0;i<4;++i){const auto &a=s.active[i];auto &n=p._activeDesc[i];float partial=0;std::memcpy(&partial,&n._partialCharge,4);
    if(check){if(n._item!=a.item||n._unk!=a.item_pool||n._charge!=a.charge||n._batteryCharge!=a.battery||n._subCharge!=a.sub||n._timeRechargeCooldown!=a.cooldown||partial!=a.partial||n._varData!=a.var_data){reason="player_active_slot_postcondition";return false;}}
    else {n._item=a.item;n._unk=a.item_pool;n._charge=a.charge;n._batteryCharge=a.battery;n._subCharge=a.sub;n._timeRechargeCooldown=a.cooldown;std::memcpy(&n._partialCharge,&a.partial,4);n._varData=a.var_data;}
    auto *pocket=p.GetPocketItem(i);if(!pocket){reason="player_pocket_owner";return false;}
    if(check){if(pocket->_id!=s.pocket[i].id||pocket->_type!=s.pocket[i].type){reason="player_pocket_slot_postcondition";return false;}}else {pocket->_id=s.pocket[i].id;pocket->_type=s.pocket[i].type;}}
  const auto &q=s.queue;auto &n=Queue(p);auto *config=Config(q.item,q.type==2);
  if(check){if(n._item!=config||n._charge!=q.charge||n._flags!=q.flags||n._vardata!=q.var_data){reason="player_queue_native_postcondition";return false;}}
  else {n._item=config;n._charge=q.charge;n._flags=q.flags;n._vardata=q.var_data;}
  // WaterClip公开getter的原链会查询HasCollectible；base Init不分配Player库存，
  // 因此先安装真实库存与Plus容器，再调用公共字段的setter/getter，避免读取空库存。
  if(!ClientEntityCommonFields(pointer,d.entity.common_fields,check,reason))return false;
  reason=check?"player_final_values_checked":"player_final_values_installed";return true;
}
bool ClientPlayerScalarFields(void *pointer,const std::vector<ClientEntityField> &fields,bool check,const char *&reason) noexcept {
  if(!pointer||!IsMain()||!ClientBusy()||!HasOwner(Owner::Manager)||!ContextHealthy()){reason="player_fast_scalar_context";return false;}auto &p=*static_cast<Entity_Player *>(pointer);
  for(const auto &f:fields){bool found=false,ok=false;
#define CS_PLAYER_FIELD(id,field,type,kind) if(f.member==id){found=true;ok=Store(field,f,check);}
#include "game/client_player_fields.inc"
#undef CS_PLAYER_FIELD
    if(!found||!ok){reason="player_fast_scalar_postcondition";return false;}}
  reason="player_fast_scalars_complete";return true;
}
bool ClientPlayerStateRetire(void *pointer,const char *&reason){
  if(!pointer||!IsMain()||!ClientBusy()){reason="player_state_retire_context";return false;}
  // 库存及Plus容器由原生Player/官方Plus析构，slice不重复释放；只撤本机配置借用。
  Queue(*static_cast<Entity_Player *>(pointer))._item=nullptr;reason="player_state_native_owners_retained_until_Free";return true;
}
} // namespace cs::game
