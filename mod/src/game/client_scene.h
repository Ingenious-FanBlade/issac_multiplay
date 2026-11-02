#pragma once
#include "replica/model.h"
#include "replica/room_grid_plan.h"
#include "game/client_lifetime.h"
#include "game/client_native_world.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace cs::game {
class ClientObjects;
class ClientPresentation;
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
  // Manager原链短路后每轮至多执行一个完整子步骤，不推进Authority tick。
  void Pump() noexcept;
  bool DisplayReady() const noexcept;
  ClientNativeRoots NativeRoots() const noexcept;
  const replica::Value *Published() const noexcept;
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
  // A仅在完整原生提交后有效，部分写入或退休不继续暴露旧提交版本。
  std::uint64_t AppliedRevision() const noexcept { return applied_valid_?applied_revision_:0; }
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
  struct BorrowEntry final {
    std::uint64_t id=0,scene=0,owner_token=0;
    ClientPurpose purpose=ClientPurpose::NativeApply;
  };
  std::array<BorrowEntry,16> borrows_{};
  std::array<RetainedEntry,16> retained_{};
  std::uint64_t next_retained_=0;
  bool draining_=false;
  bool native_dispatch_prefix_=false;
  bool lua_closing_=false,context_closing_=false;
  std::uint64_t next_borrow_=0;
  ClientSceneState state_=ClientSceneState::Waiting;
  replica::Scope waiting_scope_{}, target_scope_{}, applied_scope_{};
  std::array<replica::Scope,16> retired_scopes_{};
  unsigned retired_scope_count_=0;
  std::unique_ptr<replica::Json> target_, applied_;
  std::shared_ptr<const replica::RoomGridTarget> room_grid_target_,room_grid_applied_;
  replica::RoomGridPlan room_grid_plan_{};
  std::unique_ptr<ClientObjects> objects_;
  std::unique_ptr<ClientPresentation> presentation_;
  ClientNativeWorldOwner *native_world_=nullptr;
  ClientNativeSceneDescription native_description_{};
  bool applied_valid_=false,rebuild_pending_=false,objects_planned_=false,
       objects_retired_=false,pumping_=false,attempt_invalidated_=false,
       objects_complete_=false,presentation_retired_=false,retirement_complete_=false;
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
