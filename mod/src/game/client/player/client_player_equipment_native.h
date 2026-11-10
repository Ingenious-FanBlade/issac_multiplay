#pragma once
#include "IsaacRepentance.h"
#include "game/client/player/client_player_equipment.h"

namespace cs::game {
// 仅SDK主线程实现可见；这些地址不进入纯值描述、网络、后台队列或HUD视图。
struct ClientPlayerEquipmentOwner final {
  struct WeaponSlot final {
    Weapon *pointer=nullptr;
    unsigned type=0;
    std::uint64_t generation=0;
    bool create_entered=false,destroy_entered=false,uncertain=false;
  };
  struct Costumes final {
    CostumeSpriteDesc *first=nullptr,*last=nullptr,*capacity=nullptr;
    void *allocation=nullptr;
    unsigned constructed=0,released=0;
    bool constructor_entered=false,destructor_entered=false,uncertain=false;
  };
  Entity_Player *player=nullptr;
  std::array<WeaponSlot,5> weapons{};
  Costumes candidate{},displaced{};
  CostumeSpriteDesc *installed=nullptr;
  unsigned installed_count=0;
  std::uint64_t next_generation=0;
  std::uint64_t costume_generation=0;
  bool fault=false,retired=false;
};
bool ClientPlayerWeaponsPreflight(const ClientPlayerDescription &,const char *&);
bool ClientPlayerWeaponsInstall(ClientPlayerEquipmentOwner &,Entity_Player &,const ClientPlayerDescription &,bool,const char *&);
bool ClientPlayerWeaponsRelations(ClientPlayerEquipmentOwner &,Entity_Player &,const ClientPlayerDescription &,const ClientEntityOwners &,const ClientNativeRoots &,bool,bool,const char *&);
bool ClientPlayerWeaponsRetire(ClientPlayerEquipmentOwner &,Entity_Player &,const char *&);
// 在Player当前合法来源窗口调用，返回纯值和仅在本次Lua调用内复制的ANM/Entity借用。
void ClientPlayerWeaponsCaptureLua(void *lua,Entity_Player &);
} // namespace cs::game
