#pragma once
#include "replica/player_target.h"
#include "replica/entity_plan.h"
#include <map>
#include <set>

namespace cs::replica {
// 计划只比较最后提交A和完整T；结构就绪与最终状态/资源/绘制资格分别推进。
enum class PlayerAction : unsigned { Unlink, Remove, Acquire, Initialize, Membership, State, Resources, Link, Check };
enum class PlayerChildKind : unsigned { Inventory, Active, Pocket, Effects, Costume, Weapon, History };
struct PlayerChildChange final {
  SubjectKey body{};
  PlayerChildKind kind=PlayerChildKind::Inventory;
  bool create=false,remove=false,changed=false;
};
struct PlayerPlanStep final {
  PlayerAction action=PlayerAction::Check;
  SubjectKey key{};
  unsigned body=0;
  bool from_applied=false;
};
struct PlayerBodyChange final {
  SubjectKey key{};
  bool create=false,replace=false,membership=false,state=false,resources=false,relations=false;
};
struct PlayerApplyPlan final {
  std::shared_ptr<const PlayerTarget> applied,target;
  std::vector<PlayerPlanStep> steps;
  std::vector<PlayerChildChange> children;
  std::vector<PlayerBodyChange> bodies;
  std::vector<EntityDependency> dependencies;
  bool duplicate=false,rebuild_scene=false;
  // 写前可取消并保留A；任一原生前缀开始后仅允许整场景恢复，不采用半应用值。
  unsigned cost=0;
};
PlayerApplyPlan PlanPlayerTarget(std::shared_ptr<const PlayerTarget>,std::shared_ptr<const PlayerTarget> applied={});
// 生命周期属于Scene连接，独立于为换形而销毁的native owner；只在完整T提交时提升。
struct PlayerLifecycle final {
  std::uint64_t run=0;
  std::map<SubjectKey,std::uint64_t> current;
  std::set<SubjectKey> retired;
};
std::shared_ptr<const PlayerLifecycle> PlanPlayerLifecycle(const PlayerTarget &,const PlayerLifecycle *);
} // namespace cs::replica
