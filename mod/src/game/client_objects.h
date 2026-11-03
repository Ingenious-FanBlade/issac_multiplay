#pragma once
#include "game/client_objects_native.h"
#include "replica/model.h"
#include "replica/entity_plan.h"
#include <memory>
#include <string>
#include <vector>

namespace cs::game {
// 纯值A→T计划与原生身份表分别拥有寿命；执行失败后不再按历史A继续差分。
class ClientObjects final {
public:
  ClientObjects() noexcept;
  ~ClientObjects();
  bool PrepareTarget(const replica::Value &target,const replica::Value *applied,const char *&reason) noexcept;
  // 产品类别模块和Player20在候选进入前配置；不允许在在途执行中替换owner合同。
  bool ConfigureEntityOwners(ClientEntityOwners,std::vector<ClientEntityClassAdapter>) noexcept;
  ClientEntityOwners PlayerOwner() noexcept;
  bool CancelPlanned() noexcept;
  ClientObjectStep Step(const ClientNativeRoots &,unsigned budget=16) noexcept;
  ClientObjectStep Retire(const ClientNativeRoots &) noexcept;
  bool Visit(void *context,ClientObjectVisitor visitor) const noexcept;
private:
  ClientNativeObjects *native_=nullptr;
  std::vector<ClientObjectCommand> commands_;
  std::unique_ptr<replica::Json> candidate_,committed_;
  std::unique_ptr<replica::EntityApplyPlan> entity_plan_;
  std::shared_ptr<const replica::EntityTarget> entity_applied_;
  std::vector<ClientObjectDescription> entity_target_values_,entity_old_values_,entity_applied_values_;
  ClientEntityOwners entity_owners_{};
  std::vector<ClientEntityClassAdapter> entity_adapters_;
  bool entity_preflight_=false;
  std::string reason_;
  std::size_t cursor_=0;
  unsigned prefix_=0;
  bool failed_=false,written_=false,retired_=false;
};
} // namespace cs::game
