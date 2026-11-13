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
} // namespace cs::game
