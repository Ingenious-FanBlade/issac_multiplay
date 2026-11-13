#include "game/client/player/client_player_equipment_native.h"
#include "game/tests/client_entity_e2e.h"
#include "LuaCore.h"
#include "game/observation/sources/lua_entity.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
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
void Integer(lua_State *L,const char *name,std::int64_t x){lua_pushinteger(L,x);lua_setfield(L,-2,name);}
void Boolean(lua_State *L,const char *name,bool x){lua_pushboolean(L,x);lua_setfield(L,-2,name);}
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
// 快速窗口只复制已正常更新身体的头动画字节，不遍历武器/库存，不推进原生动画。
int ClientPlayerRenderCaptureLua(lua_State *L){auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");const auto subject=ObservationSubject(p,false);
  if(!IsMain()||!p||subject.status!=1||subject.readiness<2||p->_headAnim.size()>2048)return luaL_error(L,"player_render_source_unavailable");
  std::string bytes;const char *digits="0123456789abcdef";bytes.reserve(p->_headAnim.size()*2);for(unsigned char c:p->_headAnim){bytes.push_back(digits[c>>4]);bytes.push_back(digits[c&15]);}
  lua_newtable(L);Integer(L,"version",1);lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");lua_pushlstring(L,bytes.data(),bytes.size());lua_setfield(L,-2,"head_animation_hex");return 1;
}
}
