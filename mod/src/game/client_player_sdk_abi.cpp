#include "EntityPlus.h"
#include "game/client_player_spoof_layout.h"
// 官方头带有与本DLL无关的配置全局初始化，只在独立编译对象中核对ABI，不链接进产品。
using namespace cs::game::player_sdk;
static_assert(sizeof(PlusHeader)==sizeof(EntityPlus));
static_assert(sizeof(Innate)==sizeof(ItemSpoofSystem::InnateItems));
static_assert(sizeof(Spoof)==sizeof(ItemSpoofSystem::CollectibleSpoof));
static_assert(sizeof(Spoof)==sizeof(ItemSpoofSystem::TrinketSpoof));
static_assert(sizeof(PlusItems)==sizeof(ItemSpoofSystem::PlayerItemSpoofs));
