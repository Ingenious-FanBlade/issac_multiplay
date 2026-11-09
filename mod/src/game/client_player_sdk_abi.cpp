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
// 第4步派生Plus只收窄既存伴随来源；公共前缀仍与第3步相同。
static_assert(sizeof(EntityFamiliarPlus)==0x1c && offsetof(EntityFamiliarPlus,cachedMultiplier)==0x10);
static_assert(offsetof(EntityFamiliarPlus,cachedMultiplierPlayer)==0x18);
static_assert(sizeof(EntityKnifePlus)==0x14 && offsetof(EntityKnifePlus,hitboxSource)==0x10);
// 三类Plus只同步公共前缀；派生字段由Host使用，未知对齐字节不作填充推断。
static_assert(sizeof(EntityTearPlus)==0x18 && offsetof(EntityTearPlus,initSound)==0x10);
static_assert(sizeof(EntityPickupPlus)==0x14 && offsetof(EntityPickupPlus,canRerollOverride)==0x10);
static_assert(sizeof(EntityLaserPlus)==0x1c && offsetof(EntityLaserPlus,recalculateSamplesNextUpdate)==0x10);
static_assert(offsetof(EntityLaserPlus,initSound)==0x14);
// 第4步Slot字段严格保持官方声明宽度；SDK大小与J273池步长分开记录。
static_assert(sizeof(Entity_Slot)==0x548 && offsetof(Entity_Slot,_state)==0x410);
static_assert(offsetof(Entity_Slot,_timeout)==0x41c && sizeof(Entity_Slot::_timeout)==2);
static_assert(offsetof(Entity_Slot,_prizeAnm2)==0x42c && offsetof(Entity_Slot,_prizeCollectible)==0x540);
static_assert(sizeof(Entity_Projectile)==0x470 && sizeof(Entity_Effect)==0x490);
static_assert(offsetof(Entity_Effect,_varData)==0x420);
// 第4步NPC复用SDK前缀；复合deque和辅助Sprite的偏移另以J273静态指令核定。
static_assert(sizeof(Entity_NPC)==0xf60 && sizeof(NPCAI_Pathfinder)==0x750);
static_assert(offsetof(Entity_NPC,_pathfinder)==0x414 && offsetof(Entity_NPC,_state)==0xb64);
static_assert(offsetof(Entity_NPC,_deliriumBossType)==0xbc8 && offsetof(Entity_NPC,_deliriumCycleID)==0xbe0);
static_assert(offsetof(Entity_NPC,_dirtColor)==0xcac && offsetof(Entity_NPC,_hitList)==0xf00);
static_assert(offsetof(Entity_NPC,_controllerID)==0xf1c && offsetof(Entity_NPC,_sirenPlayerEntity)==0xf40);
static_assert(sizeof(Innate)==sizeof(ItemSpoofSystem::InnateItems));
static_assert(sizeof(Spoof)==sizeof(ItemSpoofSystem::CollectibleSpoof));
static_assert(sizeof(Spoof)==sizeof(ItemSpoofSystem::TrinketSpoof));
static_assert(sizeof(PlusItems)==sizeof(ItemSpoofSystem::PlayerItemSpoofs));
