#pragma once
#include "game/client_players_native.h"
#include "replica/player_plan.h"
#include "game/client_player_budget.h"
#include "game/client_player_diagnostics.h"
#include "replica/stream_materialize.h"

namespace cs::game {
struct ClientPlayerHistoryView;
// 与ClientObjects共享T和唯一Scene提交；installed仅是候选，不能作为后续差分A。
class ClientPlayers final {
public:
  ClientPlayers() noexcept;
  ~ClientPlayers();
  bool Prepare(std::shared_ptr<const replica::PlayerTarget>,const char *&) noexcept;
  bool Cancel() noexcept;
  bool RequiresRebuild() const noexcept {return plan_&&plan_->rebuild_scene;}
  bool Active() const noexcept {return bool(plan_)||bool(applied_);}
  unsigned Cost() const noexcept {return plan_?plan_->cost:0;}
  unsigned OwnerCount() const noexcept {return ClientNativePlayersOwnerCount(native_);}
  ClientObjectStep Structure(const ClientNativeRoots &) noexcept;
  ClientObjectStep Finish(const ClientEntityOwners &,bool finish) noexcept;
  ClientObjectStep Retire(const ClientNativeRoots &) noexcept;
  ClientObjectStep Unlink(const ClientNativeRoots &) noexcept;
  bool Commit(std::uint64_t attempt) noexcept;
  bool BeginAttempt(std::uint64_t attempt) noexcept;
  bool Configure(ClientPlayerAdapter) noexcept;
  ClientEntityOwners Owner() noexcept;
  bool Visit(void *,ClientObjectVisitor) const noexcept;
  // 主线程只读取得最后Scene提交的身体历史；候选、取消或退休不污染HUD视图。
  std::shared_ptr<const ClientPlayerHistoryView> History(const ClientObjectKey &) const noexcept;
  std::shared_ptr<const ClientPlayerDisplayView> Display() const noexcept;
  bool Committed(const replica::Scope &,std::uint64_t revision) const noexcept;
  // 稳定结构下的完整普通值与独立身体子帧分别编译；编译阶段不借用原生对象。
  std::vector<ClientPlayerDescription> CompileStream(const replica::stream::Store &,const replica::stream::RangeData &,
      const std::map<ClientObjectKey,replica::Json> &,const std::map<replica::SubjectKey,replica::stream::Range> &);
  ClientPlayerFastValues CompileFast(const replica::stream::Store &,const replica::stream::RangeData &) const;
  ClientObjectStep InstallStream(const std::vector<ClientPlayerDescription> &,const ClientEntityOwners &) noexcept;
  ClientObjectStep InstallFast(const ClientPlayerFastValues &) noexcept;
  const ClientObjectDescription *StreamDescription(const ClientObjectKey &key) const noexcept {for(const auto &d:values_)if(d.entity.key==key)return &d.entity;return nullptr;}
  ClientPlayerBudget Budget() const noexcept {return budget_;}
  std::uint64_t StreamValueBytes() const;
  ClientPlayerDiagnostics Diagnostics() const noexcept {return diagnostic_;}
#ifdef ISAACCS_E2E
  std::string TestInspect(const ClientEntityOwners &) const noexcept;
#endif
  ClientObjectReference Backup(unsigned slot) const noexcept;
private:
  std::unique_ptr<replica::PlayerApplyPlan> plan_;
  std::shared_ptr<const replica::PlayerTarget> applied_;
  std::vector<ClientPlayerDescription> values_;
  std::map<ClientObjectKey,std::shared_ptr<const ClientPlayerHistoryView>> histories_,candidate_histories_;
  std::array<ClientObjectReference,4> backups_{};
  std::shared_ptr<const ClientPlayerDisplayView> display_,candidate_display_;
  ClientPlayerBudget budget_{},applied_budget_{};
  ClientPlayerAdapter adapter_{};
  ClientNativePlayers *native_=nullptr;
  std::uint64_t attempt_=0;
  bool begun_=false,written_=false,ready_=false;
  std::string reason_;
  ClientPlayerDiagnostics diagnostic_{};
};
} // namespace cs::game
