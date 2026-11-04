#pragma once
#include "replica/entity_target.h"
#include <memory>

namespace cs::replica {
struct PlayerTarget;
// 计划只拥有不可变纯值；A必须是最后原生提交，不能传入网络Delta或未完成候选。
enum class EntityAction : unsigned { Unlink, Remove, Acquire, Initialize, Fields, Resources, Publish, Link, Check, CheckAll };
enum class EntityDependencyKind : unsigned { Entity, Player, Grid, Room, Descriptor, Weapon, Historical, AuthorityValue };
enum class EntityDependencyPhase : unsigned { BeforeInit, AfterInstances, ValueOnly };
struct EntityDependency final {
  SubjectKey source{},target{};
  std::string path,member,status,contract;
  EntityDependencyKind kind=EntityDependencyKind::Entity;
  EntityDependencyPhase phase=EntityDependencyPhase::AfterInstances;
  EntityCategory expected=EntityCategory::Unknown;
  bool has_target=false,is_null=false;
};
struct EntityPlanStep final {
  EntityAction action=EntityAction::Check;
  SubjectKey key{};
  // Remove/Unlink可以读取旧A；其余只读T。下标只在各自不可变目标内有效。
  unsigned object=0;
  bool from_applied=false;
};
struct EntityApplyPlan final {
  std::shared_ptr<const EntityTarget> applied,target;
  // Scene与对象计划共享同一已关联Player目标，后续I06/I07接原生owner时不重解析另一个T。
  std::shared_ptr<const PlayerTarget> players;
  std::vector<EntityPlanStep> steps;
  std::vector<EntityDependency> dependencies;
  std::vector<SubjectKey> external_players;
  // 存在但不可见与缺席删除分开；相同版本相同值没有任何原生操作。
  bool duplicate=false;
};
struct EntityPlanLimits final {unsigned objects=16384,steps=131073,dependencies=262144;};
EntityApplyPlan PlanEntityTarget(std::shared_ptr<const EntityTarget> target,
    std::shared_ptr<const EntityTarget> applied={},EntityPlanLimits={});
// 所有引用保留源字段身份；只有具名关系写入，历史/查询事实不伪装成EntityPtr。
EntityDependency DescribeEntityDependency(const EntityObjectTarget &,const EntityReferenceTarget &);
} // namespace cs::replica
