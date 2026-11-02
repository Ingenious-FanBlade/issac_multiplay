#pragma once
#include "replica/model.h"

namespace cs::replica {
// I26：限制完整候选的准备成本；原生448格、8门别名与资源总量分别计算。
// 一个Scene至多持有A和一个T；不截断必需成员，超限在任何原生写入前拒绝。
struct RoomGridBudgetUsage final {
  unsigned nodes=0, resources=0, layers=0, aliases=0, fragments=0, unadopted_grid_indices=0;
  std::uint64_t bytes=0;
};
inline constexpr unsigned kRoomGridResources=4096,kRoomGridLayers=65536,
  kRoomGridFragments=32768,kRoomGridAliases=456;
RoomGridBudgetUsage MeasureRoomGridBudget(const Value &);
} // namespace cs::replica
