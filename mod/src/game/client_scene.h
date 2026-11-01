#pragma once
#include "replica/model.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace cs::game {
// Client 原生场景与连接分开；这里只保存纯值目标和提交，不跨线程拥有原生对象。
enum class ClientSceneState : unsigned { Waiting, Preparing, Applying, Displaying, Retiring, Failed };
enum class ClientPurpose : unsigned { NativeApply, NativeRetire, Lua, Graphics, RenderObservation, LocalSettings };
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
  bool native_written=false, native_committed=false, old_display_valid=false;
};
// 有界的主线程场景 owner；目前没有世界网络载荷和可调用原生 adapter，接纳只会明确拒绝。
class ClientScene final {
public:
  void Begin(const replica::Scope &waiting_scope) noexcept;
  void Retire(const char *reason) noexcept;
  void Fail(const char *reason) noexcept;
  bool CanReturnToMenu() const noexcept;
  bool AdmitTarget(const replica::Scope &, std::uint64_t revision,
                   const replica::Value &) noexcept;
  bool Borrow(ClientPurpose purpose,ClientBorrowToken &out) noexcept;
  bool Valid(const ClientBorrowToken &borrow) const noexcept;
  void Release(ClientBorrowToken &borrow) noexcept;
  bool ConsumeForeignFault() noexcept;
  void RenderWindow() noexcept;
  void ObserveLuaState(void *state) noexcept;
  void LuaClosing() noexcept;
  void ContextClosing() noexcept;
  void InvalidateInstance() noexcept;
  void InvalidateResource() noexcept;
  // 以下只读视图仅供主线程使用；跨线程只能用上层已复制的状态发布。
  ClientSceneState State() const noexcept { return state_; }
  ClientApplyResult Result() const noexcept { return result_; }
  const ClientGenerations &Generations() const noexcept { return generations_; }
  bool HasNativePrefix() const noexcept { return result_.native_written || result_.pending_owners; }
private:
  bool Enter(ClientPurpose purpose) noexcept;
  void Leave(ClientPurpose purpose) noexcept;
  void FailureResult(ClientResultKind kind,const char *phase,const char *reason) noexcept;
  struct BorrowEntry final {std::uint64_t id=0;ClientPurpose purpose=ClientPurpose::NativeApply;};
  std::array<BorrowEntry,16> borrows_{};
  std::uint64_t next_borrow_=0;
  ClientSceneState state_=ClientSceneState::Waiting;
  replica::Scope waiting_scope_{}, target_scope_{}, applied_scope_{};
  std::unique_ptr<replica::Json> target_, applied_;
  std::uint64_t target_revision_=0, applied_revision_=0, next_attempt_=0;
  ClientGenerations generations_{};
  ClientApplyResult result_{};
  unsigned active_native_=0, active_lua_=0, active_graphics_=0, active_observation_=0;
  void *current_dc_=nullptr, *current_context_=nullptr;
  void *observed_dc_=nullptr, *observed_context_=nullptr;
  void *lua_state_=nullptr;
  std::atomic<bool> foreign_fault_{false};
};
} // namespace cs::game
