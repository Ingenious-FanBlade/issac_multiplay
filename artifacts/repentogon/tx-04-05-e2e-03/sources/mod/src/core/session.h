#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace cs::core {

// 状态与原因的数值沿用原型；Negotiating 和控制器/恢复原因仅供历史对照。
enum class SessionState : std::uint32_t {
  Dormant,
  Negotiating,
  Warmup,
  Active,
  Retiring,
  Retired,
  Failed
};
enum class SessionCause : std::uint32_t {
  None,
  Configuration,
  NegotiationStarted,
  Prepared,
  BindingsReady,
  Stop,
  RunTeardown,
  RuntimeFailure,
  ControllerLost,
  RestoreFailure,
  PublicationFailure,
  HookFailure,
  CleanupComplete
};
enum class SessionMode : std::uint32_t { None, Solo };

// 身份由主线程适配层在启动前生成并冻结；Solo 的 ordinal 1 必须不存在。
// generation 是会话代次，独立于 Participant 的重绑代次，不进入输入 codec。
struct SessionIdentity {
  std::array<std::uint8_t, 16> session{};
  std::array<std::uint64_t, 2> clients{};
  std::uint64_t generation = 0;
};
struct SessionStatus {
  SessionState state;
  SessionCause cause;
};
const char *SessionStateName(SessionState state) noexcept;
const char *SessionCauseName(SessionCause cause) noexcept;
const char *SessionModeName(SessionMode mode) noexcept;

// 单次会话对象，不复位旧身份。每次启动重新构造 Session、时钟和参与者，
// 并提供新的 session id/generation；适配层把 Dormant/Retired 映射为界面的
// Idle。 所有变更由主线程执行，只有 Status
// 可跨线程读取；身份与模式不作跨线程发布。
class Session final {
public:
  bool Start(const SessionIdentity &identity,
             SessionMode mode = SessionMode::Solo) noexcept;
  bool Activate(bool host_bound, bool remote_bound = false) noexcept;
  bool BeginRetirement(SessionCause cause) noexcept;
  bool CompleteRetirement(bool callbacks_drained) noexcept;
  bool Fail(SessionCause cause) noexcept;
  SessionStatus Status() const noexcept;
  const SessionIdentity &Identity() const noexcept { return identity_; }
  SessionMode Mode() const noexcept { return mode_; }
  std::size_t ParticipantCount() const noexcept {
    return mode_ == SessionMode::Solo ? 1 : 0;
  }
  bool HasParticipant(std::uint32_t ordinal) const noexcept {
    return ordinal < ParticipantCount();
  }
  bool AcceptsInput() const noexcept {
    return Status().state == SessionState::Active;
  }
  bool AllowsBinding() const noexcept;
  bool AllowsCompletion() const noexcept;
  bool MatchesIdentity(const std::array<std::uint8_t, 16> &session,
                       std::uint64_t client,
                       std::uint32_t ordinal) const noexcept;

private:
  static constexpr std::uint32_t Pack(SessionState state,
                                      SessionCause cause) noexcept {
    return static_cast<std::uint32_t>(state) |
           (static_cast<std::uint32_t>(cause) << 8);
  }
  bool Move(SessionState from, SessionState to, SessionCause cause) noexcept;
  std::atomic<std::uint32_t> status_{
      Pack(SessionState::Dormant, SessionCause::None)};
  SessionIdentity identity_{};
  SessionMode mode_ = SessionMode::None;
};

} // namespace cs::core
