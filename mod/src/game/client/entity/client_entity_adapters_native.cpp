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
bool InitializeEntity(void *pointer,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed,const char *&reason){
  if(initializing_class||!pointer||!seed){reason="entity_class_init_nested_or_seed";return false;}
  struct Scope final {Scope(unsigned seed){initializing_class=true;initializing_seed=seed;}~Scope(){initializing_class=false;initializing_seed=0;}} scope(seed);
  auto &e=*static_cast<Entity *>(pointer);
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
bool ClientBinaryEntityCleanup(void *object,unsigned category) noexcept {
  if(!object||!ClientBinaryInstallationScopeActive(true)||retiring_class)return false;
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
