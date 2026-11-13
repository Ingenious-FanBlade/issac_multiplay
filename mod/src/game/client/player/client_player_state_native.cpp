#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/player/client_player_spoof_layout.h"
#include "game/client/player/client_player_state.h"
#include "game/tests/client_entity_e2e.h"
#include "game/observation/sources/lua_entity.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
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
// E2E 对照只读取适用于当前角色的原生字段。
bool Applicable(std::string_view id,int type) noexcept {
  if(id=="EntityPlayer.GetPoopMana")return type==25;
  if(id=="NativePlayer.MaggyDrainCooldown"||id=="NativePlayer.MaggySwingCooldown")return type==22;
  if(id=="NativePlayer.EveSumptoriumCharge")return type==26;
  if(id=="EntityPlayer.GetSoulCharge")return type==18;
  if(id=="EntityPlayer.GetBloodCharge")return type==36;
  return true;
}

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
#define CS_PLAYER_FIELD(id,field,type,kind) if(Applicable(id,p._playerType))Scalar(L,id,field);else {lua_newtable(L);lua_pushliteral(L,"NotApplicable");lua_setfield(L,-2,"status");lua_setfield(L,-2,id);}
#include "game/client/player/client_player_fields.inc"
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
}
int ClientPlayerStateCaptureLua(lua_State *L){const auto started=ClientEntityTestClock();auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");const auto subject=ObservationSubject(p,false);const int top=lua_gettop(L);bool failed=false;
  lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  if(!IsMain()||!p||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  try{Capture(L,*p);}catch(...){failed=true;}ClientEntityTestTime("player_source_final_state",ClientEntityTestClock()-started);if(failed){lua_settop(L,top);return luaL_error(L,"player_state_capture_failed");}return 1;
}
} // namespace cs::game
