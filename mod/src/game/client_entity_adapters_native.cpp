#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_entity_adapters.h"
#include "game/client_entity_draw_native.h"
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
  if(d.category==1&&*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(pointer)+0xf40)){reason="entity_npc_unexpected_owned_gameplay_actor";return false;}return true;
}
// 公开引用仍用同一反向登记协议；Familiar.Player是裸借用，绝不套用EntityPtr布局。
unsigned ClassReferenceOffset(unsigned category,const std::string &id){
  if(category==5&&id=="EntityLaser.BounceLaser")return 0x4e8;
  if(category==7){if(id=="EntityPickup.GetPickupGhost")return 0x590;if(id=="EntityPickup.GetMegaChestLeftCollectible")return 0x594;if(id=="EntityPickup.GetMegaChestRightCollectible")return 0x598;}
  return 0;
}
bool Prepare(void *,const ClientObjectDescription &d,const char *&reason) noexcept {
  if(!d.full_entity||!d.derived||d.category<1||d.category>12||!d.seed||!d.resource.exact_phase||!d.derived->draw.present){reason="entity_class_target_contract";return false;}
  if((d.category==2&&d.type!=2)||(d.category==3&&d.type!=9)||(d.category==4&&d.type!=4)||
     (d.category==1&&(d.type<=9||d.type==1000||d.type==9001))){reason="entity_class_factory_category";return false;}
  const unsigned types[]={0,0,2,9,4,7,8,5,3,1000,6,9001};
  if(d.category>4&&d.category<12&&d.type!=types[d.category]){reason="entity_class_factory_category";return false;}
  if(d.category==10&&(d.variant<1||d.variant>18)){reason="entity_slot_variant_not_declared";return false;}
  if(d.category==12&&(d.type<=9||d.type==1000||d.type==9001)){reason="entity_dummy_native_type_conflict";return false;}
  // 复合NPC由完整deque/Key/曲线提供输入；Player复合分支使用同一具名Player owner。
  const auto &owned=d.derived->owned;using P=ClientEntityClassState::OwnedPresentation;
  const unsigned owned_sprites=owned.kind==P::MrMe?1:owned.kind==P::PickupGhost?4:owned.kind==P::IsaacsBody?5:owned.kind==P::AbyssLocust?owned.sprites_a+owned.sprites_b:0;
  const unsigned expected=(d.category==1?2:d.category==2?3:d.category==4?5:d.category==6?1:d.category==7?2:d.category==10?1:0)+owned_sprites;
  if(d.derived->resources.size()!=expected){reason="entity_class_auxiliary_complete_set";return false;}
  if(!d.resource.champion_color_present||!d.resource.shaders_present){reason="entity_class_main_pose_incomplete";return false;}
  for(const auto &r:d.derived->resources)if(!r.is_null&&(!r.pose.champion_color_present||!r.pose.shaders_present)){reason="entity_class_auxiliary_pose_incomplete";return false;}
  reason="entity_class_prepared";return true;
}
bool BeforeInit(void *,void *pointer,const ClientObjectDescription &d,const ClientEntityOwners &owners,const ClientNativeRoots &roots,const char *&reason){
  if(!pointer||!d.derived||!IsMain()||!ClientBusy()||!ClientEntityInitIsolationInstalled()){reason="entity_class_init_context_or_random_isolation";return false;}
  auto &e=*static_cast<Entity *>(pointer);if(e._valid){reason="entity_class_live_pool_use";return false;}
  if(d.category==1&&(*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(pointer)+0xc00)||*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(pointer)+0xf40))){reason="entity_npc_previous_private_owner_unknown";return false;}
  // 配置加载仍由原生动态Init负责。新use不得携带上一use的公共关系进入派生初始化。
  e.SetParent(nullptr);e.SetChild(nullptr);e.SetTarget(nullptr);e.SetSpawnerEntity(nullptr);e._minecart.SetReference(nullptr);
  for(const auto &r:d.relations)if(const auto offset=ClassReferenceOffset(d.category,r.member))reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+offset)->SetReference(nullptr);
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==d.category)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+offset)->SetReference(nullptr);
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
  e._pos=Vector(d.position.x,d.position.y);e._velocity=Vector(d.velocity.x,d.velocity.y);
  // 构造前置从同Key解析器取得真实当前实例；后置环仍等待全体发布后安装。
  for(const auto &r:d.relations)if(r.before_init&&!r.value_only){Entity *target=nullptr;
    if(!r.is_null){ClientEntityBorrow b;if(!owners.resolve||!owners.resolve(owners.context,roots,r.target,r.expected,b,reason))return false;target=static_cast<Entity *>(b.object);}
    if(r.member=="Entity.Parent")e.SetParent(target);else if(r.member=="Entity.SpawnerEntity")e.SetSpawnerEntity(target);
  }
  reason="entity_class_init_prepared";return true;
}
bool Initialize(void *,void *pointer,const ClientObjectDescription &d,const char *&reason){
  if(initializing_class||!pointer||!d.seed){reason="entity_class_init_nested_or_seed";return false;}
  struct Scope final {Scope(unsigned seed){initializing_class=true;initializing_seed=seed;}~Scope(){initializing_class=false;initializing_seed=0;}} scope(d.seed);
  auto &e=*static_cast<Entity *>(pointer);
  // Pickup/Familiar/Effect的派生Init包含生成/抽池/Player状态裁决；真实Factory/ctor已经建立成员owner。
  // 仅进入官方base Init及Plus链，随后由本方完整最终量/资源/关系安装；不将跳过计作原派生调用完成。
  if(d.category==7||d.category==8||d.category==9||d.category==10){
    e.Entity::Init(d.type,d.variant,d.subtype,d.seed);
    if(d.category==8){auto &f=static_cast<Entity_Familiar &>(e);
      // 派生ctor构造的容器/Pathfinder/ExtraData holder保持原owner。游戏Weapon只由被跳过的玩法Init创建，
      // 本用途不创建它，也不让未初始化的DB0/410标量变成可消费指针。
      f._weapon=nullptr;f._player=nullptr;f._item=nullptr;
    }
  }
  else e.Init(d.type,d.variant,d.subtype,d.seed);
  reason="entity_class_admitted_init_completed";return true;
}
struct FlyingFrame final {Entity_NPC *npc;int expected;bool check,ok=false;};
int FlyingThunk(lua_State *L){auto &f=*static_cast<FlyingFrame *>(lua_touserdata(L,1));lua::luabridge::UserdataPtr::push(L,f.npc,lua::GetMetatableKey(lua::Metatables::ENTITY_NPC));const int object=lua_absindex(L,-1);
  if(!f.check){lua_getfield(L,object,f.expected<0?"ClearFlyingOverride":"SetFlyingOverride");lua_pushvalue(L,object);if(f.expected>=0)lua_pushboolean(L,f.expected);lua_call(L,f.expected<0?1:2,0);}
  lua_getfield(L,object,"GetFlyingOverride");lua_pushvalue(L,object);lua_call(L,1,1);f.ok=f.expected<0?lua_isnil(L,-1):lua_isboolean(L,-1)&&lua_toboolean(L,-1)==f.expected;return 0;}
bool Flying(void *pointer,const ClientObjectDescription &d,bool check){auto *L=static_cast<lua_State *>(ClientLuaStateIdentity());if(!L)return false;const int top=lua_gettop(L);FlyingFrame frame{static_cast<Entity_NPC *>(pointer),d.derived->flying_override,check};
  lua_pushcfunction(L,FlyingThunk);lua_pushlightuserdata(L,&frame);const bool called=lua_pcall(L,1,0,0)==LUA_OK;lua_settop(L,top);return called&&frame.ok;}
// 官方KnifePlus hitboxSource只有受控leaf接口；不扫描或伪造官方私有holder。
struct KnifeLinkFrame final {Entity_Knife *knife;Entity *target;bool check,ok=false;};
int KnifeLinkThunk(lua_State *L){auto &f=*static_cast<KnifeLinkFrame *>(lua_touserdata(L,1));lua::luabridge::UserdataPtr::push(L,f.knife,lua::GetMetatableKey(lua::Metatables::ENTITY_KNIFE));const int object=lua_absindex(L,-1);
  if(!f.check){lua_getfield(L,object,"SetHitboxParentKnife");lua_pushvalue(L,object);if(f.target)lua::luabridge::UserdataPtr::push(L,f.target,lua::GetMetatableKey(lua::Metatables::ENTITY_KNIFE));else lua_pushnil(L);lua_call(L,2,0);}
  lua_getfield(L,object,"GetHitboxParentKnife");lua_pushvalue(L,object);lua_call(L,1,1);
  f.ok=f.target?lua::GetLuabridgeUserdata<Entity_Knife *>(L,-1,lua::Metatables::ENTITY_KNIFE,"EntityKnife")==f.target:lua_isnil(L,-1);return 0;}
bool KnifeLink(void *pointer,Entity *target,bool check){auto *L=static_cast<lua_State *>(ClientLuaStateIdentity());if(!L)return false;const int top=lua_gettop(L);KnifeLinkFrame frame{static_cast<Entity_Knife *>(pointer),target,check};
  lua_pushcfunction(L,KnifeLinkThunk);lua_pushlightuserdata(L,&frame);const bool ok=lua_pcall(L,1,0,0)==LUA_OK;lua_settop(L,top);return ok&&frame.ok;}
bool Fields(void *,void *pointer,const ClientObjectDescription &d,bool check,const char *&reason){
  if(!Current(pointer,d,reason))return false;
  if(d.category==5&&static_cast<Entity_Laser *>(pointer)->_homingLaser._entity!=pointer){reason="entity_laser_inline_owner";return false;}
  for(const auto &f:d.derived->fields){bool found=false,ok=false;
#define CS_CLASS_FIELD(cat,id,offset,type,kind) if(d.category==cat&&f.member==id){found=true;ok=Value(*reinterpret_cast<type *>(static_cast<unsigned char *>(pointer)+offset),f,check);}
#include "game/client_entity_class_fields.inc"
#undef CS_CLASS_FIELD
    if(!found||!ok){reason=check?"entity_class_native_postcondition":"entity_class_field_or_width";return false;}
  }if(d.category==1&&!Flying(pointer,d,check)){reason="entity_npc_flying_override";return false;}
  if(d.category==11){const auto &s=*d.derived;auto *bytes=static_cast<unsigned char *>(pointer);
    std::array<unsigned char,256> text{};std::memcpy(text.data(),s.text.data(),s.text.size());
    if(check){if(std::memcmp(bytes+0x418,text.data(),text.size())||std::memcmp(bytes+0x518,s.text_color.data(),16)||std::memcmp(bytes+0x528,&s.text_color_extra,4)){reason="entity_text_native_postcondition";return false;}}
    else {std::memcpy(bytes+0x418,text.data(),text.size());std::memcpy(bytes+0x518,s.text_color.data(),16);std::memcpy(bytes+0x528,&s.text_color_extra,4);}
  }
  if(d.derived->has_lifted){const auto &s=d.derived->lifted;auto &g=*reinterpret_cast<GridEntityDesc *>(static_cast<unsigned char *>(pointer)+0x420);
    if(check){if(g._type!=s.type||g._variant!=s.variant||g._state!=s.state||g._spawnCount!=s.spawn_count||g._spawnSeed!=s.spawn_seed||g._varData!=s.var_data||g._initialized!=s.initialized||g._variableSeed!=s.variable_seed){reason="entity_lifted_descriptor_native_postcondition";return false;}}
    else {g._type=s.type;g._variant=s.variant;g._state=s.state;g._spawnCount=s.spawn_count;g._spawnSeed=s.spawn_seed;g._varData=s.var_data;g._initialized=s.initialized;g._variableSeed=s.variable_seed;}
  }
  if(!ClientEntityCollections(pointer,d,check,reason))return false;
  if(!ClientEntityNpcCollections(pointer,d,check,reason))return false;
  if(!ClientEntityOwnedPresentation(pointer,d,check,reason))return false;
  reason=check?"entity_class_native_checked":"entity_class_final_values";return true;
}
ANM2 *Auxiliary(Entity &e,const ClientEntityClassState::Resource &r){
  if(r.member=="npc_overlay")return reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&e)+0xcd8);
  if(r.member=="npc_effect")return reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&e)+0xdec);
  if(r.member=="EntityTear.GetDeadEyeSprite")return &static_cast<Entity_Tear &>(e)._deadEyeANM2;
  if(r.member=="EntityTear.GetTearEffectSprite")return &static_cast<Entity_Tear &>(e)._tearEffectANM2;
  if(r.member=="EntityTear.GetTearHaloSprite")return &static_cast<Entity_Tear &>(e)._tearHaloANM2;
  if(r.member=="EntityBomb.GetCostumeLayerSprite"&&r.index<5)return &static_cast<Entity_Bomb &>(e)._bombCostumesSprites[r.index];
  if(r.member=="EntityPickup.GetPriceSprite")return &static_cast<Entity_Pickup &>(e)._priceANM2;
  if(r.member=="EntitySlot.GetPrizeSprite")return &static_cast<Entity_Slot &>(e)._prizeAnm2;
  if(r.member=="pickup_alternate")return &static_cast<Entity_Pickup &>(e)._altPedestalANM2;
  if(r.member=="knife_dead_eye")return *reinterpret_cast<ANM2 **>(Base+0x7e5594);
  if(r.member=="owned_sprite")return static_cast<ANM2 *>(ClientEntityOwnedSprite(&e,e._type==3?8:9,r.index));
  return nullptr;
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
    unsigned native_offset=ClassReferenceOffset(d.category,r.member);
#define CS_CLASS_LINK(cat,id,offset) if((cat==0||cat==d.category)&&r.member==id)native_offset=offset;
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
    if(native_offset){auto &ref=*reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+native_offset);
      if(check){if(ref.GetReference()!=target)return false;}else ref.SetReference(target);
    }else if(d.category==1&&r.member.rfind("NativeNPC.Composite:",0)==0){
      if(!ClientEntityNpcRelation(pointer,r,target,check,clear,reason))return false;
    }else if(r.member=="EntityFamiliar.Player"){
      auto &p=static_cast<Entity_Familiar &>(e)._player;
      // 安装Authority已求出的轨道/跟随结果，不进会改Player编队和计数的Player setter。
      if(target&&!target->ToPlayer()){reason="entity_familiar_owner_not_player";return false;}if(check){if(p!=target)return false;}else p=static_cast<Entity_Player *>(target);
    }else if(r.member=="EntityKnife.GetHitboxParentKnife"){
      if(target&&target->_type!=8){reason="entity_knife_hitbox_source_type";return false;}if(!KnifeLink(pointer,target,check)){reason="entity_knife_plus_hitbox_reference";return false;}
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
  for(const auto &r:d.relations)if(const auto offset=ClassReferenceOffset(d.category,r.member))reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+offset)->SetReference(nullptr);
  if(d.category==6&&!KnifeLink(pointer,nullptr,false)){reason="entity_knife_plus_retirement";return false;}
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==d.category)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+offset)->SetReference(nullptr);
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
  if(d.category==1)reinterpret_cast<EntityPtr *>(reinterpret_cast<unsigned char *>(pointer)+0xbec)->SetReference(nullptr);
  if(d.category==1&&!ClientEntityNpcRetire(pointer,reason))return false;
  e.SetParent(nullptr);e.SetChild(nullptr);e.SetTarget(nullptr);e.SetSpawnerEntity(nullptr);e._minecart.SetReference(nullptr);
  // 共同执行器先撤全图引用，ESSM通知与列表摘除由唯一公共owner执行；本处不Kill/爆炸/掉落。
  if(d.category==8){
    // 本方仅安装跟随/轨道的最终几何，没有加入Player AI编队或创建Weapon。
    // 原派生Remove/Clear会减Player计数、改Player位置/装备及Grid，因此本用途只执行已采用的base清理。
    auto &f=static_cast<Entity_Familiar &>(e);
    if(!ClientEntityRetireOwnedPresentation(pointer,d.category,reason))return false;
    if(f._weapon||*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(pointer)+0xecc)){reason="entity_familiar_unexpected_owned_gameplay_state";return false;}
    e.Entity::Remove();e.Entity::ClearReferences();f._player=nullptr;
  }else {e.Remove();e.ClearReferences();}
  if(e._exists){reason="entity_class_retire_exists";return false;}reason="entity_class_retired";return true;
}
// Dummy没有通用Factory case；本方只借用真实已构造专用池空槽，不覆盖活槽或伪造vptr。
void *AcquireDummy(void *,const ClientObjectDescription &,const ClientNativeRoots &roots,const char *&reason){
  auto *game=static_cast<Game *>(roots.game);
  if(!IsMain()||!ClientBusy()||!game||game!=g_Game||!game->_entityFactory){reason="entity_dummy_factory_context";return nullptr;}
  auto *pool=reinterpret_cast<unsigned char *>(game->_entityFactory)+0x675650;
  for(unsigned i=0;i<128;++i){auto *e=reinterpret_cast<Entity *>(pool+i*0x410);
    if(*reinterpret_cast<std::uintptr_t *>(e)!=Base+0x7181b8){reason="entity_dummy_pool_dynamic_type";return nullptr;}
    if(!e->_valid&&!e->_exists){reason="entity_dummy_dedicated_pool_acquired";return e;}
  }reason="entity_dummy_dedicated_pool_exhausted";return nullptr;
}
}
std::vector<ClientEntityClassAdapter> ClientEntityBuiltinAdapters(){std::vector<ClientEntityClassAdapter> out;
  for(unsigned category=1;category<=12;++category){ClientEntityClassAdapter a;a.category=category;a.prepare=Prepare;a.before_init=BeforeInit;a.initialize=Initialize;a.fields=Fields;a.resources=Resources;a.relations=Relations;a.retire=Retire;if(category==12)a.acquire=AcquireDummy;out.push_back(std::move(a));}return out;
}
void *ClientEntityAuxiliarySprite(void *pointer,const ClientEntityClassState::Resource &r){return pointer?Auxiliary(*static_cast<Entity *>(pointer),r):nullptr;}
bool ClientEntityInitializationRandom(unsigned &value) noexcept {if(!initializing_class||!IsMain()||!ClientBusy())return false;value=initializing_seed;return true;}
bool ClientEntitySuppressRetireSound() noexcept {return retiring_class&&IsMain()&&ClientBusy();}
} // namespace cs::game
