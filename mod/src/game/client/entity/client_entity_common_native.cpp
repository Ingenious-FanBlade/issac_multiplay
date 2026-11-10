#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/entity/client_objects_native.h"
#include "game/common/context.h"
#include "game/common/bridge.h"
#include <cstring>
#include <type_traits>

namespace cs::game {
static_assert(sizeof(ColorMod)==0x2c&&sizeof(Vector)==8,"J273公共值宽度必须与已核绑定一致");
namespace {
// 只装已解释的最终值；check分支真实读取native owner，绝不比较两份DTO。
template<class T> bool Value(T &native,const ClientEntityField &f,bool check) noexcept {
  if constexpr(std::is_same_v<T,bool>){if(f.kind!=ClientEntityValueKind::Boolean)return false;if(check)return native==f.boolean;native=f.boolean;}
  else if constexpr(std::is_floating_point_v<T>){if(f.kind!=ClientEntityValueKind::Real)return false;if(check)return native==f.real;native=f.real;}
  else if constexpr(std::is_integral_v<T>){
    if(f.kind==ClientEntityValueKind::Bits){if(check)return native==T(f.bits);native=T(f.bits);}
    else {if(f.kind!=ClientEntityValueKind::Integer)return false;if(check)return native==T(f.integer);native=T(f.integer);}
  }else if constexpr(std::is_same_v<T,Vector>){if(f.kind!=ClientEntityValueKind::Vector)return false;if(check)return native.x==f.vector.x&&native.y==f.vector.y;native=Vector(f.vector.x,f.vector.y);}
  else if constexpr(std::is_same_v<T,ColorMod>){if(f.kind!=ClientEntityValueKind::Color)return false;
    const auto &c=f.color;const ColorMod value(c.tint[0],c.tint[1],c.tint[2],c.tint[3],c.offset[0],c.offset[1],c.offset[2],c.colorize[0],c.colorize[1],c.colorize[2],c.colorize[3]);
    if(check)return native==value;native=value;}
  else return false;return true;
}
struct WaterFrame final {Entity *entity;unsigned flags;bool check,ok=false;};
int WaterThunk(lua_State *L){auto &v=*static_cast<WaterFrame *>(lua_touserdata(L,1));
  lua::luabridge::UserdataPtr::push(L,v.entity,lua::GetMetatableKey(lua::Metatables::ENTITY));const int entity=lua_absindex(L,-1);
  // 只调用已读官方leaf setter/getter，不提供任意方法名或远程RPC。
  if(!v.check){lua_getfield(L,entity,"SetWaterClipFlags");lua_pushvalue(L,entity);lua_pushinteger(L,v.flags);lua_call(L,2,0);}
  lua_getfield(L,entity,"GetWaterClipFlags");lua_pushvalue(L,entity);lua_call(L,1,1);
  v.ok=lua_isinteger(L,-1)&&lua_tointeger(L,-1)==v.flags;return 0;
}
bool Water(Entity &entity,const ClientEntityField &f,bool check) noexcept {
  if(f.integer<0||f.integer>UINT32_MAX||!g_LuaEngine||!g_LuaEngine->_state||ClientLuaStateIdentity()!=g_LuaEngine->_state)return false;
  auto *L=g_LuaEngine->_state;const int top=lua_gettop(L);if(!lua_checkstack(L,12))return false;
  WaterFrame frame{&entity,unsigned(f.integer),check};lua_pushcfunction(L,WaterThunk);lua_pushlightuserdata(L,&frame);
  const bool called=lua_pcall(L,1,0,0)==LUA_OK;lua_settop(L,top);return called&&frame.ok;
}
}
bool ClientEntityCommonFields(void *pointer,const std::vector<ClientEntityField> &fields,bool check,const char *&reason) noexcept {
  if(!pointer||!IsMain()||!ClientBusy()||!HasOwner(Owner::Manager)||!ContextHealthy()||!g_Game){reason="entity_common_context";return false;}
  auto &e=*static_cast<Entity *>(pointer);
  for(const auto &f:fields){bool found=false,ok=false;
#define CS_ENTITY_FIELD(id,field,kind) if(f.member==id){found=true;ok=Value(e.field,f,check);}
#define CS_ENTITY_RAW_FIELD(id,offset,type,kind) if(f.member==id){found=true;ok=Value(*reinterpret_cast<type *>(static_cast<unsigned char *>(pointer)+offset),f,check);}
#define CS_ENTITY_SPECIAL_FIELD(id,kind)
#include "game/client/entity/client_entity_fields.inc"
#undef CS_ENTITY_FIELD
#undef CS_ENTITY_RAW_FIELD
#undef CS_ENTITY_SPECIAL_FIELD
    if(f.member=="Entity.FrameCount"){
      found=true;std::uint32_t game_frame=0,base=0;std::memcpy(&game_frame,reinterpret_cast<unsigned char *>(g_Game)+0x25a70,4);
      std::memcpy(&base,static_cast<unsigned char *>(pointer)+0x328,4);
      if(check)ok=std::int32_t(game_frame-base)==f.integer;
      else {base=game_frame-std::uint32_t(f.integer);std::memcpy(static_cast<unsigned char *>(pointer)+0x328,&base,4);ok=true;}
    }
    if(f.member=="Entity.GetWaterClipFlags"){found=true;ok=Water(e,f,check);}
    if(!found||!ok){reason=check?"entity_common_native_postcondition":"entity_common_field_or_type";return false;}
  }reason=check?"entity_common_native_checked":"entity_common_installed";return true;
}
} // namespace cs::game
