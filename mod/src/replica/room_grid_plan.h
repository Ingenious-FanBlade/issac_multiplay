#pragma once
#include "replica/room_grid_target.h"

namespace cs::replica {
enum class RoomPlanKind : unsigned { Keep, Change, Rebuild, Duplicate };
enum class RoomGridOperation : unsigned {
  RoomGeometry, GridRemove, GridRetireInstance, GridCreate, GridChange, GridReplace,
  References, Neighbors, Resources, PlayerMembership, Environment, Commit
};
// after是本计划中已完成步骤的序号；external是其它域的真实主体依赖，不能用占位对象满足。
struct RoomGridPlanStep final {
  RoomGridOperation operation=RoomGridOperation::Commit;
  SubjectKey key{};
  std::vector<unsigned> after;
  std::vector<SubjectKey> external;
};
struct RoomGridPlan final {
  RoomPlanKind room=RoomPlanKind::Rebuild;
  std::shared_ptr<const RoomGridTarget> applied,target;
  std::vector<RoomGridPlanStep> steps;
};
enum class PlanCancellation : unsigned { PreserveApplied, RetireNative };
// 该判定使用本次执行前缀而非旧A是否曾写过原生；写后绝不宣称回滚。
PlanCancellation CancelRoomGridPlan(bool attempt_written,unsigned executed_prefix) noexcept;
RoomGridPlan PlanRoomGridTarget(std::shared_ptr<const RoomGridTarget> target,
                               std::shared_ptr<const RoomGridTarget> applied={},RoomGridLimits={});
} // namespace cs::replica
