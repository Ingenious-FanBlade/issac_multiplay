#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace cs::core {

// 既有状态与原因的数值沿用原型；新原因只追加，Ack 继续使用既有状态值。
// Negotiating 和控制器/恢复原因仅供历史对照，网络协商由外层管理。
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
  CleanupComplete,
  PeerBye,
  PeerTimeout,
  PeerProtocol
};
enum class SessionMode : std::uint32_t { None, Solo, Host };

// 身份由主线程适配层在启动前生成并冻结；ordinal 0=A、1=B。
// Solo 的 ordinal 1 必须不存在；Host 的两个 client id 必须非零且互异。
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
  // Solo 直接准备；Host 由适配层确认握手完成后调用，两种模式均进入 Warmup。
  bool Start(const SessionIdentity &identity,
             SessionMode mode = SessionMode::Solo) noexcept;
  // 只检查首次激活的绑定条件，Active 后不把 B 的就绪变成逐 tick 等待条件。
  bool Activate(bool host_bound, bool remote_bound = false) noexcept;
  // Peer 原因仅用于 Host 的有序退休；允许已经开始的窗口正常提交与发布。
  bool BeginRetirement(SessionCause cause) noexcept;
  bool CompleteRetirement(bool callbacks_drained) noexcept;
  bool Fail(SessionCause cause) noexcept;
  SessionStatus Status() const noexcept;
  const SessionIdentity &Identity() const noexcept { return identity_; }
  SessionMode Mode() const noexcept { return mode_; }
  std::size_t ParticipantCount() const noexcept {
    switch (mode_) {
    case SessionMode::Solo:
      return 1;
    case SessionMode::Host:
      return 2;
    default:
      return 0;
    }
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
