#pragma once
#include "replica/model.h"
#include "replica/room_grid_plan.h"
#include "replica/player_target.h"
#include "replica/player_plan.h"
#include "replica/player_limits.h"
#include "game/client/player/client_player_control.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/scene/client_native_world.h"
#include "game/client/grid/client_grid_events.h"
#include "game/client/entity/client_entity_events.h"
#include "game/client/entity/client_entity_budget.h"
#include "game/client/player/client_player_budget.h"
#include "game/client/player/client_player_diagnostics.h"
#include "game/client/session/client_stream.h"
#include "replica/stream_config.h"
#include "replica/stream_metrics.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace cs::game {
class ClientObjects;
class ClientPresentation;
class ClientGrids;
// Client 原生场景与连接分开；这里只保存纯值目标和提交，不跨线程拥有原生对象。
enum class ClientSceneState : unsigned { Waiting, Preparing, Applying, Displaying, Retiring, Failed };
enum class ClientPurpose : unsigned { NativeApply, NativeRetire, Lua, Graphics, RenderObservation, Dispatch, LocalSettings };
enum class ClientResultKind : unsigned {
  WaitingDependency, DuplicateTarget, UnsupportedRequirement, ContextOrThreadFault,
  BudgetExhausted, PartialNativeFailure, Retiring, NativeCommitted
};
// 每种借用各有代次；稳定 key 或相同地址不能延长旧实例、Lua 和 context 的寿命。
struct ClientGenerations final {
  std::uint64_t scene=0, instance=0, resource=0, lua=0, context=0;
  bool operator==(const ClientGenerations &) const = default;
};
// 借用票据仅含本地代次和 owner token；没有可跨线程解引用的原生地址。
struct ClientBorrowToken final {
  std::uint64_t id=0;
  ClientPurpose purpose=ClientPurpose::NativeApply;
  ClientGenerations generations{};
  std::uint64_t owner_token=0;
  bool active=false;
};
// 失败时可复制的资源owner事实；不包含原生地址，也不授予跨线程借用资格。
struct ClientOwnerFact final {
  std::uint64_t ticket=0,resource=0,lua=0,context=0;
  unsigned kind=0,phase=0,cpu=0,gpu=0;
  bool failed=false;
};
// 结果是事件时点的纯值快照，不随场景后续变化自动刷新；native_committed 仅由完整原生提交设置。
struct ClientApplyResult final {
  replica::Scope scope{};
  std::uint64_t revision=0, attempt=0;
  ClientGenerations generations{};
  ClientResultKind kind=ClientResultKind::WaitingDependency;
  std::array<char,32> phase{};
  std::array<char,96> reason{};
  unsigned executed_prefix=0, pending_owners=0;
  unsigned native_owners=0;
  bool native_written=false, native_committed=false, old_display_valid=false;
  // 仅确认18的 Room/Grid 范围，不代表完整11域或网络 Applied ACK。
  unsigned committed_domains=0,grid_prefix=0,grid_owners=0;
  // Entity包含完整Player域；19仅提交非Player合同和具名Player支撑，不设置完整Entity位。
  bool non_player_entity_applied=false,player_support_applied=false,full_entity_applied=false;
  bool player_bodies_applied=false,player_relations_applied=false,player_resources_applied=false,player_domain_applied=false;
  unsigned entity_members=0,entity_non_players=0,entity_players=0;
  unsigned failure_executed_prefix=0;
  replica::SubjectKey failed_key{};
  std::array<char,24> failed_domain{};
  std::array<char,48> recovery{};
  std::array<char,32> failure_phase{};
  std::array<char,96> failure_reason{};
  // 等待原因仅为纯值诊断，不冒充失败；连接恢复时随真实Result保存首次现场。
  std::array<char,128> stream_wait_reason{};
  std::uint64_t stream_wait_revision=0;
  unsigned stream_wait_kind=0;
  // 当前Player步骤与首次Player失败分开；自动恢复、后续退休和新phase不覆盖首因。
  ClientPlayerDiagnostics player_step{},player_failure{};
  std::array<char,192> retirement_cause{},retirement_failure{};
};
// 有界的主线程场景 owner；DTO接纳、原生应用、发布和实际退休分别推进。
class ClientScene final {
public:
  ClientScene() noexcept;
  ~ClientScene();
  void Begin(const replica::Scope &waiting_scope) noexcept;
  void Retire(const char *reason) noexcept;
  void Fail(const char *reason) noexcept;
  bool CanReturnToMenu() const noexcept;
  bool AdmitTarget(const replica::Scope &, std::uint64_t revision,
                   const replica::Value &) noexcept;
  bool BeginStream(const replica::Scope &,std::uint64_t route,std::uint64_t manifest,std::uint64_t generation,replica::stream::RuntimeConfig config={},std::uint64_t prepare_received_ms=0) noexcept;
  bool AdmitStream(const replica::stream::Message &,const char *&reason) noexcept;
  void PumpStream() noexcept;
  void ObserveStreamQueueAge(std::uint64_t us) noexcept {stream_metrics_.duration[unsigned(replica::stream::CpuStage::Queue)].Observe(us);}
  bool NetworkStream() const noexcept {return network_stream_started_;}
  bool StreamFailureWritten() const noexcept {return stream_write_failure_||(stream_&&(!stream_->baseline_committed||stream_->structure_rebuild)&&result_.native_written); }
  bool StreamRecoverable() const noexcept {return !retirement_failed_&&!recovery_forbidden_&&result_.kind!=ClientResultKind::ContextOrThreadFault;}
  bool RetirementFault() const noexcept {return retirement_failed_;}
  std::vector<ClientOwnerFact> RetainedFacts() const;
  const replica::stream::Metrics &StreamMetrics() const noexcept {return stream_metrics_;}
  std::uint64_t LastStreamCommit() const noexcept {return stream_last_commit_;}
  // 陈旧提示仍按最后提交；真实结构事务由同一2秒无进度／120秒绝对期限收口。
  bool StreamTransactionActive(std::uint64_t now) const noexcept;

  std::shared_ptr<const replica::stream::Message> StreamControl() const noexcept;
  void StreamControlSent() noexcept;
  bool AdmitGridIndependent(const replica::Message &,const char *&reason) noexcept;
  bool VisitGridIndependent(void *,ClientGridEvents::Visitor) noexcept;
  bool CompleteGridIndependent(std::uint64_t,std::uint64_t,const char *) noexcept;
  bool AdmitEntityIndependent(const replica::Message &,const char *&reason) noexcept;
  bool VisitEntityIndependent(void *,ClientEntityEvents::Visitor) noexcept;
  bool CompleteEntityIndependent(replica::Kind,std::uint64_t,std::uint64_t,const char *) noexcept;
  // Manager原链短路后每轮至多执行一个完整子步骤，不推进Authority tick。
  void Pump() noexcept;
  bool DisplayReady() const noexcept;
  ClientNativeRoots NativeRoots() const noexcept;
  const replica::Value *Published() const noexcept;
#ifdef ISAACCS_E2E
  std::string TestInspect() const noexcept;
  std::string TestStreamReject(unsigned tx,const std::string &name) noexcept;
#endif
  void RenderPublished() noexcept;
  bool Borrow(ClientPurpose purpose,ClientBorrowToken &out) noexcept;
  bool Valid(const ClientBorrowToken &borrow) const noexcept;
  void Release(ClientBorrowToken &borrow) noexcept;
  bool Retain(ClientRetainedKind kind,ClientReleasePhase phase,void *owned,ClientRetainedRelease release,
              ClientRetainedTicket &ticket) noexcept;
  bool QueueCpuUse(const ClientRetainedTicket &ticket) noexcept;
  bool QueueGpuUse(const ClientRetainedTicket &ticket) noexcept;
  void CpuUseComplete(const ClientRetainedTicket &ticket) noexcept;
  void GpuUseComplete(const ClientRetainedTicket &ticket) noexcept;
  void RetireRetained(const ClientRetainedTicket &ticket) noexcept;
  bool UseLuaRegistry(const ClientRetainedTicket &ticket,void *&owned) noexcept;
  void DrainRetained() noexcept;
  // 破坏实例/内嵌 ANM 前在取得本次借用之前调用；只检查退尽，不阻塞当前栈。
  bool DestructiveReady() const noexcept;
  bool ConsumeForeignFault() noexcept;
  // Manager 资源窗口和 Render 窗口共用真实 renderer/OS-current 检查。
  void ContextWindow() noexcept;
  void RenderWindow() noexcept;
  void ObserveLuaState(void *state) noexcept;
  void LuaClosing() noexcept;
  void ContextClosing() noexcept;
  void InvalidateInstance() noexcept;
  void InvalidateResource() noexcept;
  // 以下只读视图仅供主线程使用；跨线程只能用上层已复制的状态发布。
  ClientSceneState State() const noexcept { return state_; }
  ClientApplyResult Result() const noexcept { return result_; }
  replica::RoomGridBudgetUsage Budget() const noexcept { return room_grid_target_?room_grid_target_->budget:replica::RoomGridBudgetUsage{}; }
  ClientEntityBudget EntityBudget() const noexcept;
  ClientPlayerBudget PlayerBudget() const noexcept;
  std::uint64_t TargetRevision() const noexcept {return target_revision_;}
  // 目标角色／用途位集仅来自已接纳T；不重新读取原生身体或查询游戏状态。
  std::uint64_t PlayerRoleTypes() const noexcept;
  unsigned PlayerBodyRoles() const noexcept;
  unsigned EntityIndependentPending() const noexcept {return entity_events_.Pending();}
  ClientEntityEvents::Statistics EntityIndependentStatus() const noexcept {return entity_events_.Status();}
  // A仅在完整原生提交后有效，部分写入或退休不继续暴露旧提交版本。
  std::uint64_t AppliedRevision() const noexcept { return applied_valid_?applied_revision_:0; }
  // 等待场景仍沿16发送意图；正式Player目标仅从唯一Scene提交A公开控制关系。
  bool HasPlayerControl() const noexcept {return bool(player_target_)||bool(player_applied_);}
  ClientPlayerControlView PlayerControl(unsigned ordinal) const noexcept;
  // 仅发布同一A；旧shared_ptr可作历史快照，继续展示必须通过PlayerDisplayCurrent重新核对。
  std::shared_ptr<const ClientPlayerDisplayView> PlayerDisplay() const noexcept;
  bool PlayerDisplayCurrent(const std::shared_ptr<const ClientPlayerDisplayView> &) const noexcept;
  bool RecoveryPending() const noexcept;
  // 摘要不推进恢复；只有Pump在实际安全退休后改变生命周期。
  const char *RecoveryState() const noexcept;
  void CancelRecovery() noexcept {stop_requested_=true;}
  const ClientGenerations &Generations() const noexcept { return generations_; }
  bool HasNativePrefix() const noexcept { return native_dispatch_prefix_ || ClientNativeWorldHasPrefix(native_world_) || result_.native_written || result_.pending_owners; }
  // 分派原调用前记录“可能已执行”的不可逆前缀，直到 I8 实际退休后才可清除。
  bool NoteNativeDispatchEntry() noexcept;
private:
  struct RetainedEntry final {
    ClientRetainedTicket ticket{};
    ClientRetainedKind kind=ClientRetainedKind::Native;
    ClientReleasePhase phase=ClientReleasePhase::Manager;
    void *owned=nullptr;
    ClientRetainedRelease release=nullptr;
    ClientGenerations generations{};
    unsigned cpu_queued=0,gpu_queued=0;
    bool retiring=false,releasing=false,failed=false;
  };
  RetainedEntry *FindRetained(const ClientRetainedTicket &ticket) noexcept;
  bool ResourceUseValid(const RetainedEntry &entry) const noexcept;
  bool GraphicsWindowValid() const noexcept;
  void FinishRetirementIfDrained() noexcept;
  void NoteNativeStep(const ClientNativeStep &,const char *phase) noexcept;
  bool Enter(ClientPurpose purpose) noexcept;
  void Leave(ClientPurpose purpose) noexcept;
  void FailureResult(ClientResultKind kind,const char *phase,const char *reason) noexcept;
  void NotePlayerFailure(const ClientPlayerDiagnostics &) noexcept;
  void NoteObjectStep(const ClientObjectStep &,const char *phase) noexcept;
  void SetCommittedRange() noexcept;
  void CommitStream(const replica::stream::Range &,bool baseline);
  std::unique_ptr<ClientStreamState> stream_;
  replica::stream::Metrics stream_metrics_;
  std::uint64_t stream_last_commit_=0;
  bool network_stream_started_=false,stream_write_failure_=false;
  std::string stream_reason_;
  struct BorrowEntry final {
    std::uint64_t id=0,scene=0,owner_token=0;
    ClientPurpose purpose=ClientPurpose::NativeApply;
  };
  std::array<BorrowEntry,16> borrows_{};
  // 一个Player资源子步骤内暂留main/13 inline/costume候选，释放须等用途借用退出。
  // 覆盖已接纳的单身体1024 costume上限，并保留原有16 owner及其余内嵌资源余量。
  std::array<RetainedEntry,replica::kPlayerCostumesPerBody+32> retained_{};
  std::uint64_t next_retained_=0;
  bool draining_=false;
  bool native_dispatch_prefix_=false;
  bool lua_closing_=false,context_closing_=false;
  std::uint64_t next_borrow_=0;
  ClientSceneState state_=ClientSceneState::Waiting;
  replica::Scope waiting_scope_{}, target_scope_{}, applied_scope_{};
  std::array<replica::Scope,128> retired_scopes_{};
  unsigned retired_scope_count_=0;
  // 冷应用的全部DOM/派生计划共用一次保守额度；A和T各持有独立租约，退休后才释放。
  memory::Lease target_memory_,applied_memory_,planning_memory_;
  std::unique_ptr<replica::Json> target_, applied_;
  std::shared_ptr<const replica::RoomGridTarget> room_grid_target_,room_grid_applied_;
  std::shared_ptr<const replica::PlayerTarget> player_target_,player_applied_;
  std::shared_ptr<const replica::PlayerLifecycle> player_lifecycle_,player_next_lifecycle_;
  replica::RoomGridPlan room_grid_plan_{};
  std::unique_ptr<ClientObjects> objects_;
  std::unique_ptr<ClientPresentation> presentation_;
  std::unique_ptr<ClientGrids> grids_;
  // 重建的纯值计划在退休前一次解析并保留；未知T不能先撤A才被拒绝。
  std::unique_ptr<ClientObjects> rebuild_objects_;
  std::unique_ptr<ClientPresentation> rebuild_presentation_;
  std::unique_ptr<ClientGrids> rebuild_grids_;
  ClientGridEvents grid_events_;
  ClientEntityEvents entity_events_;
  ClientNativeWorldOwner *native_world_=nullptr;
  LegacyClientNativeSceneDescription native_description_{};
  bool applied_valid_=false,rebuild_pending_=false,objects_planned_=false,
       objects_retired_=false,pumping_=false,attempt_invalidated_=false,
       objects_complete_=false,presentation_retired_=false,retirement_complete_=false;
  bool grids_planned_=false,grids_complete_=false,grids_retired_=false,references_complete_=false;
  bool stop_requested_=false,recovery_attempted_=false,retirement_failed_=false,recovery_forbidden_=false;
  std::uint64_t target_revision_=0, applied_revision_=0, next_attempt_=0;
  ClientGenerations generations_{};
  ClientApplyResult result_{};
  unsigned active_native_=0, active_lua_=0, active_graphics_=0, active_observation_=0, active_dispatch_=0;
  void *current_dc_=nullptr, *current_context_=nullptr;
  void *observed_dc_=nullptr, *observed_context_=nullptr;
  void *lua_state_=nullptr;
  mutable std::atomic<bool> foreign_fault_{false};
};
} // namespace cs::game
