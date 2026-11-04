#pragma once
#include "game/client_players_native.h"
#include "replica/player_plan.h"

namespace cs::game {
// 与ClientObjects共享T和唯一Scene提交；installed仅是候选，不能作为后续差分A。
class ClientPlayers final {
public:
  ClientPlayers() noexcept=default;
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
  bool Commit(std::uint64_t attempt) noexcept;
  bool BeginAttempt(std::uint64_t attempt) noexcept;
  bool Configure(ClientPlayerAdapter) noexcept;
  ClientEntityOwners Owner() noexcept;
  bool Visit(void *,ClientObjectVisitor) const noexcept;
private:
  std::unique_ptr<replica::PlayerApplyPlan> plan_;
  std::shared_ptr<const replica::PlayerTarget> applied_;
  std::vector<ClientPlayerDescription> values_;
  std::array<ClientObjectReference,4> backups_{};
  ClientPlayerAdapter adapter_{};
  ClientNativePlayers *native_=nullptr;
  std::uint64_t attempt_=0;
  bool begun_=false,written_=false,ready_=false;
  std::string reason_;
};
} // namespace cs::game
