#pragma once
#include <cstdint>
namespace cs::replica {
// 参与者、关系节点和全局备份槽分别计数；四组八活动身体及其非活动身体均在容量内。
inline constexpr unsigned kPlayerParticipants=4,kPlayerGroupMembers=64,kPlayerBodies=256,kPlayerBackupSlots=4;
inline constexpr unsigned kPlayerPlanSteps=65536,kPlayerReferences=262144,kPlayerResources=4096,kPlayerLayers=65536;
inline constexpr unsigned kPlayerContainerEntries=16384,kPlayerCostumesPerBody=1024,kPlayerAggregateEntries=1048576;
inline constexpr std::uint64_t kPlayerTargetBytes=32ull<<20,kPlayerTargetNodes=64ull<<20,kPlayerOwnedBytes=256ull<<20;
} // namespace cs::replica
