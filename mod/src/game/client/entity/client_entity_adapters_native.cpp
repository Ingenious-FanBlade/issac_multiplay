#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/entity/client_entity_adapters.h"
#include "game/common/context.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <cstring>
#include <limits>
#include <type_traits>

namespace cs::game {
namespace {
thread_local bool initializing_class=false;
thread_local unsigned initializing_seed=0;
thread_local bool retiring_class=false;
// 只在当前 Manager 内的原生 Morph 中启用，Host、原版与其他实体仍走常驻 Hook 的原链。
struct MorphScope {void *object=nullptr;unsigned category=0;bool pickup_init=false;};
thread_local MorphScope morph;
// SDK的Entity方法包装器内部仍按vtable分派，C++基类限定也不能选择base原生实现。
// 直达唯一Hook清单world_entity_init/remove/clear_references的已登记入口；保留官方与自有常驻链。
void BaseEntityInit(Entity &entity,unsigned type,unsigned variant,unsigned subtype,unsigned seed){
  reinterpret_cast<void (__thiscall *)(Entity *,unsigned,unsigned,unsigned,unsigned)>(Base+0x2a4a20)(
      &entity,type,variant,subtype,seed);
}
// 本方Familiar没有Player编队/Weapon玩法owner；先撤全部关系和typed附属owner后只清公共use。
void BaseEntityRetire(Entity &entity){
  reinterpret_cast<void (__thiscall *)(Entity *)>(Base+0x2a8000)(&entity);
  reinterpret_cast<void (__thiscall *)(Entity *)>(Base+0x2a8060)(&entity);
}
void InitializeEntityClass(Entity &e,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed){
  // Pickup/Familiar/Effect的派生Init包含生成/抽池/Player状态裁决；真实Factory/ctor已经建立成员owner。
  // 仅进入官方base Init及Plus链，随后由本方完整最终量/资源/关系安装；不将跳过计作原派生调用完成。
  if(category==7||category==8||category==9||category==10){
    BaseEntityInit(e,type,variant,subtype,seed);
    if(category==8){auto &f=static_cast<Entity_Familiar &>(e);
      // 派生ctor构造的容器/Pathfinder/ExtraData holder保持原owner。游戏Weapon只由被跳过的玩法Init创建，
      // 本用途不创建它，也不让未初始化的DB0/410标量变成可消费指针。
      f._weapon=nullptr;f._player=nullptr;f._item=nullptr;
    }
  }
  else e.Init(type,variant,subtype,seed);
}
bool TryInitializeEntityClass(Entity &e,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed){
  __try {InitializeEntityClass(e,category,type,variant,subtype,seed);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool InitializeEntity(void *pointer,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed,const char *&reason){
  if(initializing_class||!pointer||!seed){reason="entity_class_init_nested_or_seed";return false;}
  struct Scope final {Scope(unsigned seed){initializing_class=true;initializing_seed=seed;}~Scope(){initializing_class=false;initializing_seed=0;}} scope(seed);
  if(!TryInitializeEntityClass(*static_cast<Entity *>(pointer),category,type,variant,subtype,seed)){
    reason="entity_class_init_seh";return false;
  }
  reason="entity_class_admitted_init_completed";return true;
}
// 官方KnifePlus hitboxSource只有受控leaf接口；不扫描或伪造官方私有holder。
struct KnifeLinkFrame final {Entity_Knife *knife;Entity *target;bool check,ok=false;};
int KnifeLinkThunk(lua_State *L){auto &f=*static_cast<KnifeLinkFrame *>(lua_touserdata(L,1));lua::luabridge::UserdataPtr::push(L,f.knife,lua::GetMetatableKey(lua::Metatables::ENTITY_KNIFE));const int object=lua_absindex(L,-1);
  if(!f.check){lua_getfield(L,object,"SetHitboxParentKnife");lua_pushvalue(L,object);if(f.target)lua::luabridge::UserdataPtr::push(L,f.target,lua::GetMetatableKey(lua::Metatables::ENTITY_KNIFE));else lua_pushnil(L);lua_call(L,2,0);}
  lua_getfield(L,object,"GetHitboxParentKnife");lua_pushvalue(L,object);lua_call(L,1,1);
  f.ok=f.target?lua::GetLuabridgeUserdata<Entity_Knife *>(L,-1,lua::Metatables::ENTITY_KNIFE,"EntityKnife")==f.target:lua_isnil(L,-1);return 0;}
bool KnifeLink(void *pointer,Entity *target,bool check){auto *L=static_cast<lua_State *>(ClientLuaStateIdentity());if(!L)return false;const int top=lua_gettop(L);KnifeLinkFrame frame{static_cast<Entity_Knife *>(pointer),target,check};
  lua_pushcfunction(L,KnifeLinkThunk);lua_pushlightuserdata(L,&frame);const bool ok=lua_pcall(L,1,0,0)==LUA_OK;lua_settop(L,top);return ok&&frame.ok;}
}
bool ClientEntityNpcRetire(void *,const char *&);
bool ClientEntityInitializationRandom(unsigned &value) noexcept {if(!initializing_class||!IsMain()||!ClientBusy())return false;value=initializing_seed;return true;}
// 二进制承载复用相同的类别 Init 和局部随机隔离。调用方负责预检、关系和前缀登记。
bool ClientBinaryEntityInitialize(void *object,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed,const char *&reason) {
  return ClientBinaryInstallationScopeActive(false)&&InitializeEntity(object,category,type,variant,subtype,seed,reason);
}
bool ClientMorphScopedEntity(const void *object,unsigned category) noexcept {
  return object&&morph.object==object&&morph.category==category&&ClientBusy()&&ClientBinaryInstallationScopeActive(false);
}
bool ClientMorphPickupModifiers(const void *return_address) noexcept {
  return ClientMorphScopedEntity(morph.object,7)&&
    reinterpret_cast<std::uintptr_t>(return_address)==Base+0x2dffd3;
}
bool ClientMorphPickupInit(void *object,unsigned type,int variant,int subtype,unsigned) {
  if(!ClientMorphScopedEntity(object,7))return false;
  if(!morph.pickup_init){
    morph.pickup_init=true;
    // Host 的正常 Morph 可以换 seed。复用池槽时直接用目标的 InitSeed，避免在本地再抽一次。
    BaseEntityInit(*static_cast<Entity *>(object),type,unsigned(variant),unsigned(subtype),initializing_seed);
  }
  return true;
}
namespace {
void MorphPickup(Entity_Pickup &pickup,unsigned type,unsigned variant,unsigned subtype) {
  auto &ignore_modifiers=*reinterpret_cast<unsigned *>(Base+0x7e49f0);
  const auto previous=ignore_modifiers;
  __try {pickup.Morph(int(type),int(variant),int(subtype),false,true,true);}
  __finally {ignore_modifiers=previous;}
}
bool MorphEntityCall(Entity &e,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed,
    int champion) noexcept {
  __try {
    if(!e._exists||!e._valid)return false;
    const auto *vptr=*reinterpret_cast<void **>(&e);
    if(reinterpret_cast<std::uintptr_t>(vptr)!=Base+(category==7?0x718e8cu:0x7187d0u))return false;
    const auto index=e._index,collision=e._collisionIndex;
    const auto position=e._pos,velocity=e._velocity;
    if(category==7){
      auto &pickup=static_cast<Entity_Pickup &>(e);
      // 只调用非商店普通底座和宝箱。分叉底座／展示实体继续走 Remove／Init。
      const auto ordinary=[](unsigned v){return (v>=50&&v<=58)||v==60||v==100;};
      if(type!=5||e._type!=5||!ordinary(e._variant)||!ordinary(variant)||
          pickup._price||pickup._pickupGhost||pickup._megaChestCollectible[0]||pickup._megaChestCollectible[1])return false;
      MorphPickup(pickup,type,variant,subtype);
      if(!morph.pickup_init)return false;
    }else{
      // 6B7730 会消费共享的单次 NPC 初始化位，客机不能消费正在等待的原生操作。
      if(type<10||type>=1000||e._type<10||e._type>=1000||e._initSeed!=seed||
          *reinterpret_cast<const unsigned char *>(Base+0x7e49ec))return false;
      static_cast<Entity_NPC &>(e).Morph(int(type),int(variant),int(subtype),champion);
    }
    if(*reinterpret_cast<void **>(&e)!=vptr||e._index!=index||!e._valid||!e._exists||
        e._type!=type||e._variant!=variant||e._subtype!=subtype||e._initSeed!=seed)return false;
    e._collisionIndex=collision;e._pos=position;e._velocity=velocity;
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
bool ClientBinaryEntityMorph(void *object,unsigned category,unsigned type,unsigned variant,unsigned subtype,
    unsigned seed,int champion) noexcept {
  if(!object||morph.object||initializing_class||(category!=1&&category!=7)||!seed||
      !ClientBinaryInstallationScopeActive(false)||!ClientMorphIsolationInstalled())return false;
  // 复用隔离 Init 的 RNG 入口；并在 SEH 返回后归还作用域，避免状态泄漏到原版播放。
  struct Scope {
    Scope(void *object,unsigned category,unsigned seed){
      morph={object,category,false};initializing_class=true;initializing_seed=seed;
    }
    ~Scope(){morph={};initializing_class=false;initializing_seed=0;}
  } scope(object,category,seed);
  return MorphEntityCall(*static_cast<Entity *>(object),category,type,variant,subtype,seed,champion);
}
bool ClientBinaryEntityCleanup(void *object,unsigned category,bool init_auxiliary) noexcept {
  // 附属物只能由同一 Init 窗口的池位差集进入此入口。
  if(!object||!ClientBinaryInstallationScopeActive(!init_auxiliary)||retiring_class)return false;
  struct Scope {Scope(){retiring_class=true;}~Scope(){retiring_class=false;}} scope;
  try {
    auto &e=*static_cast<Entity *>(object);
    // 类别特有引用在派生清理前拆除，避免移除目标图里的另一个实体。
    e.SetParent(nullptr);e.SetChild(nullptr);e.SetTarget(nullptr);e.SetSpawnerEntity(nullptr);e._minecart.SetReference(nullptr);
    for(const auto offset:{0x478u,0x4e8u,0x590u,0x594u,0x598u,0xbecu}){
      if((category==2&&offset==0x478)||(category==5&&offset==0x4e8)||
         (category==7&&offset>=0x590&&offset<=0x598)||(category==1&&offset==0xbec))
        reinterpret_cast<EntityPtr *>(static_cast<unsigned char *>(object)+offset)->SetReference(nullptr);
    }
    if(category==1){const char *why=nullptr;if(!ClientEntityNpcRetire(object,why))return false;}
    if(category==6&&!KnifeLink(object,nullptr,false))return false;
    if(category==8){
      auto &f=static_cast<Entity_Familiar &>(e);
      if(f._weapon||*reinterpret_cast<void **>(static_cast<unsigned char *>(object)+0xecc))return false;
      f._player=nullptr;BaseEntityRetire(e);
    }else {e.Remove();e.ClearReferences();}
    return !e._exists;
  }catch(...){return false;}
}
bool ClientEntitySuppressRetireSound() noexcept {return retiring_class&&IsMain()&&ClientBusy();}
} // namespace cs::game
