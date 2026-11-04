#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_player_roles.h"
#include "game/client_player_isolation.h"
#include "game/client_entity_draw_native.h"
#include "game/bridge.h"
#include "runtime.h"
#include <cstring>
#include <new>
#include <stdexcept>
namespace cs::game {
namespace {
// SDK只命名第一个word；实际J273对象拥有HUD smart pair，禁止把Game内嵌槽当其拥有物。
struct Pair final {PlayerHUD *hud=nullptr;void *control=nullptr;};
Pair &Slot(Entity_Player &p){return *reinterpret_cast<Pair *>(reinterpret_cast<unsigned char *>(&p)+0x20a8);}
template<class T> T &Raw(PlayerHUD &h,unsigned offset){return *reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(&h)+offset);}
bool PairValid(const Pair &p){return p.hud&&p.control&&*reinterpret_cast<std::uintptr_t *>(p.control)==Base+0x71badc;}
bool Window(){return IsMain()&&ClientBusy()&&HasOwner(Owner::Manager)&&ContextHealthy()&&ClientLuaGuardReady()&&ClientCallbackGuardReady()&&ClientEntityDrawDrained()&&ClientNativeCpuQueuesEmpty();}
void Integer(lua_State *L,const char *name,int v){lua_pushinteger(L,v);lua_setfield(L,-2,name);}
void Boolean(lua_State *L,const char *name,bool v){lua_pushboolean(L,v);lua_setfield(L,-2,name);}
void Text(lua_State *L,const char *name,const char *s){if(!s){lua_pushliteral(L,"");lua_setfield(L,-2,name);return;}
  unsigned count=0;while(count<1024&&s[count])++count;if(count==1024)throw std::runtime_error("player_hud_cache_string_budget");
  lua_pushlstring(L,s,count);lua_setfield(L,-2,name);}
bool Cache(PlayerHUD &h,const ClientPlayerSpecialState::Hud &s,bool check,const char *&reason){
  if(check){if(h._playerHudIndex!=s.index||Raw<std::int16_t>(h,0xa)!=s.color||h._redHeartFlashCountdown!=s.red_flash||h._soulHeartFlashCountdown!=s.soul_flash||h._goldHeartFlashCountdown!=s.gold_flash||Raw<bool>(h,0x6a0)!=s.mantle){reason="player_hud_header_postcondition";return false;}}
  else {h._playerHudIndex=std::int16_t(s.index);Raw<std::int16_t>(h,0xa)=std::int16_t(s.color);h._redHeartFlashCountdown=std::int8_t(s.red_flash);h._soulHeartFlashCountdown=std::int8_t(s.soul_flash);h._goldHeartFlashCountdown=std::int8_t(s.gold_flash);Raw<bool>(h,0x6a0)=s.mantle;}
  for(unsigned i=0;i<24;++i){auto &n=h._heart[i];const auto &v=s.hearts[i];
    if(check){if(n._visibility!=v.visible||n._eternalHeartVisibility!=v.eternal||n._goldenHeartVisibility!=v.golden||n._fadeHeart!=v.fading||n._flashType!=v.flash||!n._spriteAnim||!n._spriteOverlayAnim||v.animation!=n._spriteAnim||v.overlay!=n._spriteOverlayAnim){reason="player_hud_heart_postcondition";return false;}}
    else {n._visibility=v.visible;n._eternalHeartVisibility=v.eternal;n._goldenHeartVisibility=v.golden;n._fadeHeart=v.fading;n._flashType=std::int8_t(v.flash);n._spriteAnim=v.animation.c_str();n._spriteOverlayAnim=v.overlay.c_str();}
  }return true;
}
}
// 只负责本地factory前缀，实际HUD及control仍是Player唯一拥有的真实原生对象。
struct ClientPlayerHudOwner final {Entity_Player *player=nullptr;Pair owned{};bool factory_entered=false,release_entered=false,uncertain=false;};
ClientPlayerHudOwner *ClientPlayerHudCreate() noexcept {return IsMain()?new(std::nothrow) ClientPlayerHudOwner:nullptr;}
void ClientPlayerHudDestroy(ClientPlayerHudOwner *o) noexcept {if(!o||!IsMain()||o->owned.hud||o->owned.control||o->factory_entered||o->release_entered||o->uncertain)return;delete o;}
// 在已核定同use来源窗口内复制实际动态HUD；不使用会fallback的Lua Getter伪造动态拥有关系。
void ClientPlayerHudCapture(void *lua,void *pointer){auto *L=static_cast<lua_State *>(lua);auto &p=*static_cast<Entity_Player *>(pointer);const auto &pair=Slot(p);
  lua_newtable(L);Integer(L,"version",1);Boolean(L,"present",pair.hud!=nullptr);
  if(pair.hud){if(!IsMain()||!g_Game||!PairValid(pair)||pair.hud->_player!=&p||pair.hud->_HUD!=&g_Game->_hud)throw std::runtime_error("player_hud_current_native_owner");
    auto &h=*pair.hud;Integer(L,"index",h._playerHudIndex);Integer(L,"color",Raw<std::int16_t>(h,0xa));Boolean(L,"mantle",Raw<bool>(h,0x6a0));
    Integer(L,"red_flash",h._redHeartFlashCountdown);Integer(L,"soul_flash",h._soulHeartFlashCountdown);Integer(L,"gold_flash",h._goldHeartFlashCountdown);
    lua_newtable(L);for(unsigned i=0;i<24;++i){const auto &v=h._heart[i];lua_newtable(L);Boolean(L,"visible",v._visibility);Boolean(L,"eternal",v._eternalHeartVisibility);Boolean(L,"golden",v._goldenHeartVisibility);Boolean(L,"fading",v._fadeHeart);Integer(L,"flash",v._flashType);Text(L,"animation",v._spriteAnim);Text(L,"overlay",v._spriteOverlayAnim);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"hearts");
  }lua_setfield(L,-2,"hud");
}
bool ClientPlayerHudRetire(ClientPlayerHudOwner *o,void *pointer,const char *&reason){
  if(!o)return true;if(!Window()||!pointer||o->uncertain||o->factory_entered||o->release_entered){reason="player_hud_retire_prefix_retained";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);auto &pair=Slot(p);
  if((o->player&&o->player!=&p)||pair.hud!=o->owned.hud||pair.control!=o->owned.control){reason="player_hud_foreign_pair_retained";return false;}
  if(pair.control){if(!PairValid(pair)){reason="player_hud_release_contract_missing";return false;}
    o->release_entered=true;try{reinterpret_cast<void (__thiscall *)(Pair *)>(Base+0x3d2b20)(&pair);}catch(...){o->uncertain=true;reason="player_hud_release_exception_retained";return false;}o->release_entered=false;
  }
  pair={};o->owned={};reason="player_hud_native_pair_retired";return true;
}
bool ClientPlayerHudInstall(ClientPlayerHudOwner *o,void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){
  if(!Window()||!o||!pointer||!d.special||o->uncertain||o->factory_entered||o->release_entered){reason="player_hud_install_context";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);auto &pair=Slot(p);const auto &s=d.special->hud;
  if((o->player&&o->player!=&p)||pair.hud!=o->owned.hud||pair.control!=o->owned.control){reason="player_hud_pair_owner_changed";return false;}if(!check)o->player=&p;
  if(!s.present){if(check){if(pair.hud||pair.control){reason="player_hud_null_postcondition";return false;}return true;}return ClientPlayerHudRetire(o,&p,reason);}
  if(!pair.hud){if(check){reason="player_hud_missing_native_dependency";return false;}
    // VA980020: ECX=HUD，栈(output pair*, Player*)，ret8。直接交给空Player槽，避免临时smart别名。
    // 只构造HUD和真实ReferenceCount，8326C0清其自有缓存；不调用Player/HUD Update。
    o->factory_entered=true;try{reinterpret_cast<Pair *(__thiscall *)(HUD *,Pair *,Entity_Player *)>(Base+0x580020)(&g_Game->_hud,&pair,&p);}
    catch(...){o->uncertain=true;reason="player_hud_factory_exception_prefix_retained";return false;}
    o->factory_entered=false;o->owned=pair;
  }
  if(!PairValid(pair)||pair.hud->_player!=&p||pair.hud->_HUD!=&g_Game->_hud){if(!check)o->uncertain=true;reason="player_hud_factory_owner_postcondition";return false;}
  return Cache(*pair.hud,s,check,reason);
}
} // namespace cs::game
