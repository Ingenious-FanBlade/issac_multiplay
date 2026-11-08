#include "EntityPlus.h"
#include "game/client_player_spoof_layout.h"
#include <cstddef>
#include <cstdint>
// 官方头带有与本DLL无关的配置全局初始化，只在独立编译对象中核对ABI，不链接进产品。
using namespace cs::game::player_sdk;
static_assert(sizeof(PlusHeader)==sizeof(EntityPlus));
// 第3步只读来源按当前Win32/v143官方ABI解释optional；不能将未设置payload复制出去。
static_assert(sizeof(std::optional<bool>)==2 && sizeof(std::optional<std::uint32_t>)==8);
static_assert(offsetof(EntityPlus,isFlyingOverride)==4);
static_assert(offsetof(EntityPlus,waterClipInfoFlagsOverride)==8);
static_assert(offsetof(std::_Optional_destruct_base<bool>,_Has_value)==1);
static_assert(offsetof(std::_Optional_destruct_base<std::uint32_t>,_Has_value)==4);
static_assert(sizeof(std::unique_ptr<EntityPlus>)==4);
static_assert(sizeof(Innate)==sizeof(ItemSpoofSystem::InnateItems));
static_assert(sizeof(Spoof)==sizeof(ItemSpoofSystem::CollectibleSpoof));
static_assert(sizeof(Spoof)==sizeof(ItemSpoofSystem::TrinketSpoof));
static_assert(sizeof(PlusItems)==sizeof(ItemSpoofSystem::PlayerItemSpoofs));
