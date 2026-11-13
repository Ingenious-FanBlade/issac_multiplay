#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/player/client_player_roles.h"
#include "game/client/isolation/client_player_isolation.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
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
void Integer(lua_State *L,const char *name,int v){lua_pushinteger(L,v);lua_setfield(L,-2,name);}
void Boolean(lua_State *L,const char *name,bool v){lua_pushboolean(L,v);lua_setfield(L,-2,name);}
void Text(lua_State *L,const char *name,const char *s){if(!s){lua_pushliteral(L,"");lua_setfield(L,-2,name);return;}
  unsigned count=0;while(count<1024&&s[count])++count;if(count==1024)throw std::runtime_error("player_hud_cache_string_budget");
  lua_pushlstring(L,s,count);lua_setfield(L,-2,name);}
}
void ClientPlayerHudCapture(void *lua,void *pointer){auto *L=static_cast<lua_State *>(lua);auto &p=*static_cast<Entity_Player *>(pointer);const auto &pair=Slot(p);
  lua_newtable(L);Integer(L,"version",1);Boolean(L,"present",pair.hud!=nullptr);
  if(pair.hud){if(!IsMain()||!g_Game||!PairValid(pair)||pair.hud->_player!=&p||pair.hud->_HUD!=&g_Game->_hud)throw std::runtime_error("player_hud_current_native_owner");
    auto &h=*pair.hud;Integer(L,"index",h._playerHudIndex);Integer(L,"color",Raw<std::int16_t>(h,0xa));Boolean(L,"mantle",Raw<bool>(h,0x6a0));
    Integer(L,"red_flash",h._redHeartFlashCountdown);Integer(L,"soul_flash",h._soulHeartFlashCountdown);Integer(L,"gold_flash",h._goldHeartFlashCountdown);
    lua_newtable(L);for(unsigned i=0;i<24;++i){const auto &v=h._heart[i];lua_newtable(L);Boolean(L,"visible",v._visibility);Boolean(L,"eternal",v._eternalHeartVisibility);Boolean(L,"golden",v._goldenHeartVisibility);Boolean(L,"fading",v._fadeHeart);Integer(L,"flash",v._flashType);Text(L,"animation",v._spriteAnim);Text(L,"overlay",v._spriteOverlayAnim);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"hearts");
  }lua_setfield(L,-2,"hud");
}
}
