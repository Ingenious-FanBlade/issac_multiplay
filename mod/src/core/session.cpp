#include "core/session.h"
#include <algorithm>

namespace cs::core {

const char *SessionStateName(SessionState state) noexcept {
  switch (state) {
  case SessionState::Dormant:
    return "Dormant";
  case SessionState::Negotiating:
    return "Negotiating";
  case SessionState::Warmup:
    return "Warmup";
  case SessionState::Active:
    return "Active";
  case SessionState::Retiring:
    return "Retiring";
  case SessionState::Retired:
    return "Retired";
  case SessionState::Failed:
    return "Failed";
  }
  return "UNKNOWN";
}
const char *SessionCauseName(SessionCause cause) noexcept {
  switch (cause) {
  case SessionCause::None:
    return "None";
  case SessionCause::Configuration:
    return "Configuration";
  case SessionCause::NegotiationStarted:
    return "NegotiationStarted";
  case SessionCause::Prepared:
    return "Prepared";
  case SessionCause::BindingsReady:
    return "BindingsReady";
  case SessionCause::Stop:
    return "Stop";
  case SessionCause::RunTeardown:
    return "RunTeardown";
  case SessionCause::RuntimeFailure:
    return "RuntimeFailure";
  case SessionCause::ControllerLost:
    return "ControllerLost";
  case SessionCause::RestoreFailure:
    return "RestoreFailure";
  case SessionCause::PublicationFailure:
    return "PublicationFailure";
  case SessionCause::HookFailure:
    return "HookFailure";
  case SessionCause::CleanupComplete:
    return "CleanupComplete";
  case SessionCause::PeerBye:
    return "PeerBye";
  case SessionCause::PeerTimeout:
    return "PeerTimeout";
  case SessionCause::PeerProtocol:
    return "PeerProtocol";
  case SessionCause::PeerLost:
    return "PeerLost";
  }
  return "UNKNOWN";
}
const char *SessionModeName(SessionMode mode) noexcept {
  switch (mode) {
  case SessionMode::None:
    return "None";
  case SessionMode::Solo:
    return "Solo";
  case SessionMode::Host:
    return "Host";
  }
  return "UNKNOWN";
}
SessionStatus Session::Status() const noexcept {
  const auto status = status_.load(std::memory_order_acquire);
  return {static_cast<SessionState>(status & 0xFFu),
          static_cast<SessionCause>(status >> 8)};
}
bool Session::Move(SessionState from, SessionState to,
                   SessionCause cause) noexcept {
  auto status = status_.load(std::memory_order_acquire);
  for (;;) {
    const auto state = static_cast<SessionState>(status & 0xFFu);
    if (state == to)
      return true;
    if (state != from)
      return false;
    if (status_.compare_exchange_weak(status, Pack(to, cause),
                                      std::memory_order_acq_rel))
      return true;
  }
}
// 网络监听与握手不放入 core；Host 在外层确认握手后与 Solo 共用 Warmup。
// 启动前校验完整身份，失败不发布半份身份，也不允许重试一次性 Session。
bool Session::Start(const SessionIdentity &identity,
                    SessionMode mode) noexcept {
  if (Status().state != SessionState::Dormant)
    return false;
  bool valid_clients = mode == SessionMode::Solo || mode == SessionMode::Host;
  for (std::size_t i = 1; i < kMaximumParticipants; ++i) {
    if (mode == SessionMode::Solo)
      valid_clients &= identity.clients[i] == 0;
    else {
      valid_clients &= identity.clients[i] != 0;
      for (std::size_t j = 0; j < i; ++j)
        valid_clients &= identity.clients[i] != identity.clients[j];
    }
  }
  if (!valid_clients || !identity.generation || !identity.clients[0] ||
      std::all_of(identity.session.begin(), identity.session.end(),
                  [](auto byte) { return byte == 0; })) {
    Fail(SessionCause::Configuration);
    return false;
  }
  identity_ = identity;
  mode_ = mode;
  present_ = 1;
  return Move(SessionState::Dormant, SessionState::Warmup,
              SessionCause::Prepared);
}
bool Session::Activate(bool host_bound, bool remote_bound) noexcept {
  (void)remote_bound;
  if (AcceptsInput())
    return true;
  const bool bindings_ready = host_bound && HasParticipant(0);
  return bindings_ready && Move(SessionState::Warmup, SessionState::Active,
                                SessionCause::BindingsReady);
}
std::size_t Session::ParticipantCount() const noexcept {
  std::size_t count = 0;
  for (std::size_t i = 0; i < ParticipantLimit(); ++i)
    count += HasParticipant(static_cast<std::uint32_t>(i));
  return count;
}
bool Session::AddParticipant(std::uint32_t ordinal) noexcept {
  if (!ordinal || ordinal >= ParticipantLimit() || !AllowsBinding() ||
      (retired_ & (1u << ordinal)))
    return false;
  present_ |= 1u << ordinal;
  return true;
}
bool Session::RemoveParticipant(std::uint32_t ordinal) noexcept {
  if (!ordinal || ordinal >= ParticipantLimit())
    return false;
  present_ &= ~(1u << ordinal);
  retired_ |= 1u << ordinal;
  return true;
}
// 退休立即关闭新的 admission；已经开始的模拟仍可正常返回并完成发布。
bool Session::BeginRetirement(SessionCause cause) noexcept {
  // PeerLost 与其它 Peer 原因一样只属于 Host 的有序退休；Solo 不接受。
  const bool peer_cause =
      cause == SessionCause::PeerBye || cause == SessionCause::PeerTimeout ||
      cause == SessionCause::PeerProtocol || cause == SessionCause::PeerLost;
  if (cause != SessionCause::Stop && cause != SessionCause::RunTeardown &&
      !(mode_ == SessionMode::Host && peer_cause))
    return false;
  auto status = status_.load(std::memory_order_acquire);
  for (;;) {
    const auto state = static_cast<SessionState>(status & 0xFFu);
    if (state == SessionState::Failed || state == SessionState::Dormant)
      return false;
    if (state == SessionState::Retiring || state == SessionState::Retired)
      return true;
    if (status_.compare_exchange_weak(status,
                                      Pack(SessionState::Retiring, cause),
                                      std::memory_order_acq_rel))
      return true;
  }
}
bool Session::CompleteRetirement(bool callbacks_drained) noexcept {
  if (!callbacks_drained)
    return false;
  if (Status().state == SessionState::Failed)
    return true; // 清理不能抹掉原始失败。
  return Move(SessionState::Retiring, SessionState::Retired,
              SessionCause::CleanupComplete);
}
bool Session::Fail(SessionCause cause) noexcept {
  // ControllerLost/RestoreFailure 仅保留枚举；常驻 Hook 路线没有这些生效路径。
  // Peer 原因走 BeginRetirement，不把在途 tick 直接变成失败窗口。
  if (cause != SessionCause::Configuration &&
      cause != SessionCause::RuntimeFailure &&
      cause != SessionCause::PublicationFailure &&
      cause != SessionCause::HookFailure)
    return false;
  auto status = status_.load(std::memory_order_acquire);
  for (;;) {
    const auto state = static_cast<SessionState>(status & 0xFFu);
    if (state == SessionState::Failed || state == SessionState::Retired)
      return false;
    if (status_.compare_exchange_weak(status, Pack(SessionState::Failed, cause),
                                      std::memory_order_acq_rel))
      return true;
  }
}
bool Session::AllowsBinding() const noexcept {
  const auto state = Status().state;
  return state == SessionState::Warmup || state == SessionState::Active;
}
bool Session::AllowsCompletion() const noexcept {
  const auto state = Status().state;
  return state == SessionState::Active || state == SessionState::Retiring;
}
bool Session::MatchesIdentity(const std::array<std::uint8_t, 16> &session,
                              std::uint64_t client,
                              std::uint32_t ordinal) const noexcept {
  return HasParticipant(ordinal) && session == identity_.session &&
         client == identity_.clients[ordinal];
}

} // namespace cs::core
