#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_entity_adapters.h"
#include "game/context.h"
#include "game/bridge.h"
#include "runtime.h"
#include <cstring>
#include <limits>
#include <type_traits>

namespace cs::game {
namespace {
thread_local bool initializing_class=false;
thread_local unsigned initializing_seed=0;
thread_local bool retiring_class=false;
// 在同一当前use中按类别安装；既不调用属性玩法setter，也不整体复制带owner的对象内存。
template<class T> bool Value(T &native,const ClientEntityField &f,bool check){
  if constexpr(std::is_same_v<T,BitSet128>){if(f.kind!=ClientEntityValueKind::Bits128)return false;
    BitSet128 bits(unsigned(f.bits),unsigned(f.bits>>32),unsigned(f.high_bits),unsigned(f.high_bits>>32));if(check)return std::memcmp(&native,&bits,sizeof(bits))==0;native=bits;
  }else if constexpr(std::is_same_v<T,Vector>){if(f.kind!=ClientEntityValueKind::Vector)return false;if(check)return native.x==f.vector.x&&native.y==f.vector.y;native=Vector(f.vector.x,f.vector.y);
  }else if constexpr(std::is_same_v<T,ColorMod>){if(f.kind!=ClientEntityValueKind::Color)return false;const auto &c=f.color;ColorMod color(c.tint[0],c.tint[1],c.tint[2],c.tint[3],c.offset[0],c.offset[1],c.offset[2],c.colorize[0],c.colorize[1],c.colorize[2],c.colorize[3]);if(check)return native==color;native=color;
  }else if constexpr(std::is_same_v<T,bool>){if(f.kind!=ClientEntityValueKind::Boolean)return false;if(check)return native==f.boolean;native=f.boolean;
  }else if constexpr(std::is_floating_point_v<T>){if(f.kind!=ClientEntityValueKind::Real)return false;if(check)return native==f.real;native=f.real;
  }else if constexpr(std::is_integral_v<T>){if(f.kind==ClientEntityValueKind::Bits){if(check)return native==T(f.bits);native=T(f.bits);}
    else {if(f.kind!=ClientEntityValueKind::Integer)return false;
      if constexpr(std::is_signed_v<T>){if(f.integer<(std::numeric_limits<T>::min)()||f.integer>(std::numeric_limits<T>::max)())return false;}
      else if(f.integer<0||std::uint64_t(f.integer)>(std::numeric_limits<T>::max)())return false;
      if(check)return native==T(f.integer);native=T(f.integer);}
  }else return false;return true;
}
bool Current(void *pointer,const ClientObjectDescription &d,const char *&reason){
  if(!pointer||!IsMain()||!ClientBusy()||!HasOwner(Owner::Manager)||!ContextHealthy()||!d.derived){reason="entity_class_context_or_payload";return false;}
  auto &e=*static_cast<Entity *>(pointer);if(e._type!=d.type||e._variant!=d.variant||e._subtype!=d.subtype){reason="entity_class_native_shape";return false;}
  if(d.category==1&&(*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(pointer)+0xc00)||*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(pointer)+0xf40))){reason="entity_npc_unexpected_private_owner";return false;}return true;
}
bool Prepare(void *,const ClientObjectDescription &d,const char *&reason) noexcept {
  if(!d.full_entity||!d.derived||d.category<1||d.category>4||!d.seed||!d.resource.exact_phase){reason="entity_class_target_contract";return false;}
  if((d.category==2&&d.type!=2)||(d.category==3&&d.type!=9)||(d.category==4&&d.type!=4)||
     (d.category==1&&(d.type<=9||d.type==964||d.type==1000||d.type==9001))){reason="entity_class_factory_category";return false;}
  // 这些专用draw仍依赖未解释的私有容器或Player复合owner；不能用通用NPC姿态冒充已支持。
  if(d.category==1&&(d.type==835||d.type==906||d.type==920||d.type==921||(d.type==951&&d.variant==3)||(d.type==965&&d.variant==10))){reason="entity_npc_private_draw_owner_pending_I25";return false;}
  const unsigned expected=d.category==1?2:d.category==2?3:d.category==4?5:0;
  if(d.derived->resources.size()!=expected){reason="entity_class_auxiliary_complete_set";return false;}
  if(!d.resource.champion_color_present||!d.resource.shaders_present){reason="entity_class_main_pose_incomplete";return false;}
  for(const auto &r:d.derived->resources)if(!r.is_null&&(!r.pose.champion_color_present||!r.pose.shaders_present)){reason="entity_class_auxiliary_pose_incomplete";return false;}
  reason="entity_class_prepared";return true;
}
bool BeforeInit(void *,void *pointer,const ClientObjectDescription &d,const ClientEntityOwners &,const ClientNativeRoots &,const char *&reason){
  if(!pointer||!d.derived||!IsMain()||!ClientBusy()||!ClientEntityInitIsolationInstalled()){reason="entity_class_init_context_or_random_isolation";return false;}
  auto &e=*static_cast<Entity *>(pointer);if(e._valid){reason="entity_class_live_pool_use";return false;}
  if(d.category==1&&(*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(pointer)+0xc00)||*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(pointer)+0xf40))){reason="entity_npc_previous_private_owner_unknown";return false;}
  // 配置加载仍由原生动态Init负责。新use不得携带上一use的公共关系进入派生初始化。
  e.SetParent(nullptr);e.SetChild(nullptr);e.SetTarget(nullptr);e.SetSpawnerEntity(nullptr);e._minecart.SetReference(nullptr);
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==d.category)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+offset)->SetReference(nullptr);
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
  e._pos=Vector(d.position.x,d.position.y);e._velocity=Vector(d.velocity.x,d.velocity.y);
  reason="entity_class_init_prepared";return true;
}
bool Initialize(void *,void *pointer,const ClientObjectDescription &d,const char *&reason){
  if(initializing_class||!pointer||!d.seed){reason="entity_class_init_nested_or_seed";return false;}
  struct Scope final {Scope(unsigned seed){initializing_class=true;initializing_seed=seed;}~Scope(){initializing_class=false;initializing_seed=0;}} scope(d.seed);
  static_cast<Entity *>(pointer)->Init(d.type,d.variant,d.subtype,d.seed);reason="entity_class_dynamic_init_completed";return true;
}
struct FlyingFrame final {Entity_NPC *npc;int expected;bool check,ok=false;};
int FlyingThunk(lua_State *L){auto &f=*static_cast<FlyingFrame *>(lua_touserdata(L,1));lua::luabridge::UserdataPtr::push(L,f.npc,lua::GetMetatableKey(lua::Metatables::ENTITY_NPC));const int object=lua_absindex(L,-1);
  if(!f.check){lua_getfield(L,object,f.expected<0?"ClearFlyingOverride":"SetFlyingOverride");lua_pushvalue(L,object);if(f.expected>=0)lua_pushboolean(L,f.expected);lua_call(L,f.expected<0?1:2,0);}
  lua_getfield(L,object,"GetFlyingOverride");lua_pushvalue(L,object);lua_call(L,1,1);f.ok=f.expected<0?lua_isnil(L,-1):lua_isboolean(L,-1)&&lua_toboolean(L,-1)==f.expected;return 0;}
bool Flying(void *pointer,const ClientObjectDescription &d,bool check){auto *L=static_cast<lua_State *>(ClientLuaStateIdentity());if(!L)return false;const int top=lua_gettop(L);FlyingFrame frame{static_cast<Entity_NPC *>(pointer),d.derived->flying_override,check};
  lua_pushcfunction(L,FlyingThunk);lua_pushlightuserdata(L,&frame);const bool called=lua_pcall(L,1,0,0)==LUA_OK;lua_settop(L,top);return called&&frame.ok;}
bool Fields(void *,void *pointer,const ClientObjectDescription &d,bool check,const char *&reason){
  if(!Current(pointer,d,reason))return false;
  for(const auto &f:d.derived->fields){bool found=false,ok=false;
#define CS_CLASS_FIELD(cat,id,offset,type,kind) if(d.category==cat&&f.member==id){found=true;ok=Value(*reinterpret_cast<type *>(static_cast<unsigned char *>(pointer)+offset),f,check);}
#include "game/client_entity_class_fields.inc"
#undef CS_CLASS_FIELD
    if(!found||!ok){reason=check?"entity_class_native_postcondition":"entity_class_field_or_width";return false;}
  }if(d.category==1&&!Flying(pointer,d,check)){reason="entity_npc_flying_override";return false;}
  reason=check?"entity_class_native_checked":"entity_class_final_values";return true;
}
ANM2 *Auxiliary(Entity &e,const ClientEntityClassState::Resource &r){
  if(r.member=="npc_overlay")return reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&e)+0xcd8);
  if(r.member=="npc_effect")return reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&e)+0xdec);
  if(r.member=="EntityTear.GetDeadEyeSprite")return &static_cast<Entity_Tear &>(e)._deadEyeANM2;
  if(r.member=="EntityTear.GetTearEffectSprite")return &static_cast<Entity_Tear &>(e)._tearEffectANM2;
  if(r.member=="EntityTear.GetTearHaloSprite")return &static_cast<Entity_Tear &>(e)._tearHaloANM2;
  if(r.member=="EntityBomb.GetCostumeLayerSprite"&&r.index<5)return &static_cast<Entity_Bomb &>(e)._bombCostumesSprites[r.index];return nullptr;
}
bool Resources(void *,void *pointer,const ClientObjectDescription &d,bool check,const char *&reason){
  if(!Current(pointer,d,reason))return false;auto &e=*static_cast<Entity *>(pointer);
  const auto apply=[&](ANM2 *sprite,const ClientResourcePose &pose){if(!sprite)return false;if(check)return ClientResourcePoseMatches(sprite,pose);const auto result=ClientApplyResourcePose(sprite,pose);if(!result.complete)reason=result.reason;return result.complete;};
  if(!apply(&e._sprite,d.resource))return false;for(const auto &r:d.derived->resources)if(!apply(Auxiliary(e,r),r.pose))return false;
  if(d.category==4){auto &bomb=static_cast<Entity_Bomb &>(e);
    // Source已给出主Sprite与五层最终结果。禁止Bomb Render重新随机重建costume；不推进计时/爆炸。
    if(check){if(bomb._loadCostumes){reason="entity_bomb_lazy_rebuild_enabled";return false;}}else bomb._loadCostumes=false;
  }reason=check?"entity_class_resources_checked":"entity_class_resources_installed";return true;
}
bool Relations(void *,void *pointer,const ClientObjectDescription &d,const ClientEntityOwners &owners,const ClientNativeRoots &roots,bool check,bool clear,const char *&reason){
  if(!Current(pointer,d,reason))return false;
  auto &e=*static_cast<Entity *>(pointer);
  for(const auto &r:d.relations){if(r.value_only||r.member.rfind("Entity.",0)==0)continue;Entity *target=nullptr;
    if(!clear&&!r.is_null){ClientEntityBorrow b;if(!owners.resolve||!owners.resolve(owners.context,roots,r.target,r.expected,b,reason))return false;target=static_cast<Entity *>(b.object);}
    unsigned native_offset=0;
#define CS_CLASS_LINK(cat,id,offset) if((cat==0||cat==d.category)&&r.member==id)native_offset=offset;
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
    if(native_offset){auto &ref=*reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+native_offset);
      if(check){if(ref.GetReference()!=target)return false;}else ref.SetReference(target);
    }else if(r.member=="EntityNPC.ParentNPC"||r.member=="EntityNPC.ChildNPC"){
      // 公开NPC父子只是公共EntityPtr的派生别名；不再写一份关系覆盖共同字段。
      if((r.member=="EntityNPC.ParentNPC"?e.GetParent():e.GetChild())!=target){reason="entity_npc_parent_child_alias_conflict";return false;}
    }else if(r.member=="EntityNPC.EntityRef"){
      auto &ref=*reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+0xbec);if(check){if(ref.GetReference()!=target)return false;}else ref.SetReference(target);
    }else if(r.member=="EntityNPC.GetSirenPlayerEntity"){
      // F40由原生清理作为拥有的actor处理，不能把借来的roster身体塞进去或清空未知owner。
      if(target||*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(pointer)+0xf40)){reason="entity_npc_owned_siren_actor_pending_Player20";return false;}
    }else if(r.member=="EntityTear.StickTarget"){
      // 公开Lua setter裸写，但原生6B17C0/663570使用同一种反向登记；采用官方EntityPtr helper维护该槽。
      auto &ref=*reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+0x478);if(check){if(ref.GetReference()!=target)return false;}else ref.SetReference(target);
    }else {reason="entity_class_relation_contract_missing";return false;}
  }
  reason="entity_class_relations_checked";return true;
}
bool Retire(void *,void *pointer,const ClientObjectDescription &d,const char *&reason){
  if(!Current(pointer,d,reason))return false;auto &e=*static_cast<Entity *>(pointer);
  if(retiring_class){reason="entity_class_nested_retirement";return false;}
  struct Scope final {Scope(){retiring_class=true;}~Scope(){retiring_class=false;}} scope;
  if(d.category==2)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+0x478)->SetReference(nullptr);
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==d.category)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+offset)->SetReference(nullptr);
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
  if(d.category==1)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+0xbec)->SetReference(nullptr);
  e.SetParent(nullptr);e.SetChild(nullptr);e.SetTarget(nullptr);e.SetSpawnerEntity(nullptr);e._minecart.SetReference(nullptr);
  // 共同执行器先撤全图引用，ESSM通知与列表摘除由唯一公共owner执行；本处不Kill/爆炸/掉落。
  e.Remove();e.ClearReferences();if(e._exists){reason="entity_class_retire_exists";return false;}reason="entity_class_retired";return true;
}
}
std::vector<ClientEntityClassAdapter> ClientEntityBuiltinAdapters(){std::vector<ClientEntityClassAdapter> out;
  for(unsigned category=1;category<=4;++category){ClientEntityClassAdapter a;a.category=category;a.prepare=Prepare;a.before_init=BeforeInit;a.initialize=Initialize;a.fields=Fields;a.resources=Resources;a.relations=Relations;a.retire=Retire;out.push_back(std::move(a));}return out;
}
bool ClientEntityInitializationRandom(unsigned &value) noexcept {if(!initializing_class||!IsMain()||!ClientBusy())return false;value=initializing_seed;return true;}
bool ClientEntitySuppressRetireSound() noexcept {return retiring_class&&IsMain()&&ClientBusy();}
} // namespace cs::game
