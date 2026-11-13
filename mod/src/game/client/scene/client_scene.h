#pragma once
#include "replica/model.h"
#include "replica/subject_key.h"
#include "replica/player_limits.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/player/client_player_diagnostics.h"
#include "game/client/display/client_display_native.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace cs::game {
// Client 场景维护原生发布、资源代次与线程借用。
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
  bool RetirementFault() const noexcept {return retirement_failed_;}
  std::vector<ClientOwnerFact> RetainedFacts() const;
  std::uint64_t LastStreamCommit() const noexcept {return stream_last_commit_;}
  void Pump() noexcept;
  bool DisplayReady() const noexcept;
  ClientNativeRoots NativeRoots() const noexcept;
  bool NativeDisplay() const noexcept;
  // 正式二进制批次借用已有 Game／Room；完整提交后才开放显示。
  void BeginBinary() noexcept {binary_scene_=true;}
  bool BinaryScene() const noexcept {return binary_scene_;}
  void PublishBinary(std::uint64_t revision) noexcept;
  void SuspendBinaryDisplay() noexcept {binary_published_=false;}
  void FinishBinary() noexcept;
  ClientDisplayStep PrepareBinaryDisplay(const ClientDisplayWorldRef &) noexcept;
  ClientDisplayStep InstallBinaryDisplay(const ClientDisplayWorldRef &) noexcept;
  ClientDisplayStep RestoreBinaryBoss(const ClientDisplayBoss &) noexcept;
  ClientDisplayStep InstallBinaryAudio(const ClientDisplayMusicRequest &,ClientDisplayArray<ClientDisplaySound>) noexcept;
  ClientDisplayStep ReplayBinaryEvent(const ClientDisplayEvent &) noexcept;
  bool BinaryDisplayComplete()const noexcept {return ClientDisplayCheckPublication(display_);}
  bool RetireBinaryDisplay(bool preserve_audio=false) noexcept;
  bool PreserveBinaryRoom() noexcept;
  bool DiscardBinaryHistory() noexcept;
  bool ReleaseBinaryDisplayRoots(bool keep_history=false) noexcept;
  void AbortBinaryDisplay() noexcept {ClientDisplayAbortPending(display_);}
  bool AdvanceDisplay(bool local_blocked) noexcept;
  bool BeginNativeRender() noexcept;
  void EndNativeRender() noexcept;
  bool RouteNativeRender(bool begin) noexcept;
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
  std::uint64_t AppliedRevision() const noexcept {return binary_published_?applied_revision_:0;}
  const ClientGenerations &Generations() const noexcept {return generations_;}
  bool HasNativePrefix() const noexcept {return native_dispatch_prefix_||result_.native_written||result_.pending_owners;}
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
  bool Enter(ClientPurpose purpose) noexcept;
  void Leave(ClientPurpose purpose) noexcept;
  void FailureResult(ClientResultKind kind,const char *phase,const char *reason) noexcept;
  std::uint64_t stream_last_commit_=0;
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
  replica::Scope waiting_scope_{};
  ClientDisplayNative *display_=nullptr;
  bool binary_scene_=false,binary_published_=false,display_history_=false;
  replica::Scope display_history_scope_{};
  bool retirement_complete_=false,retirement_failed_=false,recovery_forbidden_=false;
  std::uint64_t applied_revision_=0;
  ClientGenerations generations_{};
  ClientApplyResult result_{};
  unsigned active_native_=0, active_lua_=0, active_graphics_=0, active_observation_=0, active_dispatch_=0;
  void *current_dc_=nullptr, *current_context_=nullptr;
  void *observed_dc_=nullptr, *observed_context_=nullptr;
  void *lua_state_=nullptr;
  mutable std::atomic<bool> foreign_fault_{false};
};
} // namespace cs::game
