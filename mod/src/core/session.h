#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace cs::core {

// 参与者容量与 Player 身体数量无关；一个控制组可以拥有多个原生身体。
inline constexpr std::size_t kMaximumParticipants = 4;

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
  PeerProtocol,
  // tx-04-07：远端 Player 被原生移除或归属矛盾（U10）。追加在末尾，保留既有数值；
  // 只在 Host 上作为有序退休原因，不作为网络线程的探测结果。
  PeerLost
};
enum class SessionMode : std::uint32_t { None, Solo, Host };

// 身份在启动前生成并冻结；0为Host，1..3为预留远端身份，实际名单由presence表达。
// Solo只有0；Host预留ID非零互异，离开不移动其他人的ordinal。
// generation 是会话代次，独立于 Participant 的重绑代次，不进入输入 codec。
struct SessionIdentity {
  std::array<std::uint8_t, 16> session{};
  std::array<std::uint64_t, kMaximumParticipants> clients{};
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
  // Solo与Host均直接准备Warmup；稳定Host可先单人运行，Peer随后独立握手加入。
  bool Start(const SessionIdentity &identity,
             SessionMode mode = SessionMode::Solo) noexcept;
  // 首次激活只要求Host绑定；任何日常tick均不等待全员。
  bool Activate(bool host_bound, bool remote_bound = false) noexcept;
  // Peer 原因仅用于 Host 的有序退休；允许已经开始的窗口正常提交与发布。
  bool BeginRetirement(SessionCause cause) noexcept;
  bool CompleteRetirement(bool callbacks_drained) noexcept;
  bool Fail(SessionCause cause) noexcept;
  SessionStatus Status() const noexcept;
  const SessionIdentity &Identity() const noexcept { return identity_; }
  SessionMode Mode() const noexcept { return mode_; }
  std::size_t ParticipantCount() const noexcept;
  // ordinal 在会话中冻结；离开留下空位，不压缩其他人的序号，也不复用退休身份。
  std::size_t ParticipantLimit() const noexcept {
    return mode_ == SessionMode::Host ? kMaximumParticipants
           : mode_ == SessionMode::Solo ? 1 : 0;
  }
  bool AddParticipant(std::uint32_t ordinal) noexcept;
  bool RemoveParticipant(std::uint32_t ordinal) noexcept;
  bool HasParticipant(std::uint32_t ordinal) const noexcept {
    return ordinal < ParticipantLimit() && (present_ & (1u << ordinal));
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
  std::uint32_t present_ = 0, retired_ = 0;
};

} // namespace cs::core
