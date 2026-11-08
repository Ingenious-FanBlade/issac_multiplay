#pragma once
#include "game/client_objects_native.h"
#include "replica/model.h"
#include "replica/entity_plan.h"
#include "game/client_entity_budget.h"
#include "game/client_player_budget.h"
#include "game/client_player_diagnostics.h"
#include "game/client_players_native.h"
#include "replica/stream_materialize.h"
#include <memory>
#include <string>
#include <vector>

namespace cs::game {
class ClientPlayers;
struct ClientPlayerAdapter;
// 纯值A→T计划与原生身份表分别拥有寿命；执行失败后不再按历史A继续差分。
class ClientObjects final {
public:
  ClientObjects() noexcept;
  ~ClientObjects();
  bool PrepareTarget(const replica::Value &target,const replica::Value *applied,const char *&reason,const replica::RoomGridTarget *room_grid=nullptr,std::shared_ptr<const replica::PlayerTarget> players={}) noexcept;
  // Effect136内嵌描述与Grid18接口在同一不可变T内核对；安装完成后仍由Scene统一提交。
  bool CheckLifted(const replica::LiftedGridTarget &,const char *&reason) const noexcept;
  // 产品类别模块和Player20在候选进入前配置；不允许在在途执行中替换owner合同。
  bool ConfigureEntityOwners(ClientEntityOwners,std::vector<ClientEntityClassAdapter>) noexcept;
  ClientEntityOwners PlayerOwner() noexcept;
  bool PlayerRebuildRequired() const noexcept;
  bool ConfigurePlayerAdapter(ClientPlayerAdapter) noexcept;
  bool CancelPlanned() noexcept;
  ClientObjectStep Step(const ClientNativeRoots &,unsigned budget=16,bool finish=true) noexcept;
  bool InstancesReady() const noexcept { return instances_ready_; }
  // 只有唯一Scene可把已检查候选提升为A；内部枚举必须携带同一attempt。
  bool BeginAttempt(std::uint64_t) noexcept;
  bool Commit(std::uint64_t) noexcept;
  bool VisitPrepared(std::uint64_t,void *,ClientObjectVisitor) const noexcept;
  bool MatchIndependent(const ClientObjectKey &,std::uint64_t shape,std::uint64_t instance) const noexcept;
  ClientEntityBudget Budget() const noexcept {auto b=budget_;b.completed=unsigned(cursor_+player_cursor_);return b;}
  ClientPlayerBudget PlayerBudget() const noexcept;
  std::uint64_t StreamValueBytes() const;
  ClientPlayerDiagnostics PlayerDiagnostics() const noexcept;
  std::shared_ptr<const ClientPlayerDisplayView> PlayerDisplay() const noexcept;
  bool PlayersCommitted(const replica::Scope &,std::uint64_t revision) const noexcept;
  void CompileStream(const replica::stream::Store &,const replica::stream::RangeData &,
      const replica::RoomGridTarget &,const std::map<replica::SubjectKey,replica::stream::Range> &,std::vector<ClientObjectDescription> &,std::vector<ClientPlayerDescription> &) const;
  ClientPlayerFastValues CompileFast(const replica::stream::Store &,const replica::stream::RangeData &) const;
  ClientObjectStep InstallStream(const ClientNativeRoots &,const std::vector<ClientObjectDescription> &,const std::vector<ClientPlayerDescription> &) noexcept;
  ClientObjectStep InstallFast(const ClientPlayerFastValues &) noexcept;
  ClientObjectStep Retire(const ClientNativeRoots &) noexcept;
  bool Visit(void *context,ClientObjectVisitor visitor) const noexcept;
#ifdef ISAACCS_E2E
  std::string TestInspect() const noexcept;
#endif
private:
  unsigned OwnerCount() const noexcept;
  ClientNativeObjects *native_=nullptr;
  std::unique_ptr<ClientPlayers> players_;
  bool players_structured_=false,players_complete_=false;
  bool player_prepare_failed_=false;
  std::vector<ClientObjectCommand> commands_;
  std::unique_ptr<replica::Json> candidate_,committed_;
  std::unique_ptr<replica::Json> installed_;
  std::unique_ptr<replica::EntityApplyPlan> entity_plan_;
  std::shared_ptr<const replica::EntityTarget> entity_applied_;
  std::shared_ptr<const replica::EntityTarget> entity_installed_;
  std::vector<ClientObjectDescription> entity_installed_values_;
  std::vector<ClientObjectDescription> entity_target_values_,entity_old_values_,entity_applied_values_;
  ClientEntityOwners entity_owners_{};
  std::vector<ClientEntityClassAdapter> entity_adapters_;
  bool entity_preflight_=false,capacity_checked_=false;
  bool instances_ready_=false,ready_=false;
  std::uint64_t attempt_=0;
  ClientEntityBudget budget_{};
  std::vector<ClientObjectCommand> player_commands_;
  std::size_t player_cursor_=0;
  std::string reason_;
  std::size_t cursor_=0;
  unsigned prefix_=0;
  bool failed_=false,written_=false,retired_=false;
};
} // namespace cs::game
