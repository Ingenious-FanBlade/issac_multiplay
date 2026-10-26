#include "core/tick_clock.h"
#include "core/input_core.h"
#include <limits>

namespace cs::core {
const char *TickDecisionName(TickDecision decision) noexcept {
  switch (decision) {
  case TickDecision::Idle:
    return "Idle";
  case TickDecision::Started:
    return "Started";
  case TickDecision::Committed:
    return "Committed";
  case TickDecision::Inactive:
    return "Inactive";
  case TickDecision::Blocked:
    return "Blocked";
  case TickDecision::NoScope:
    return "NoScope";
  case TickDecision::Closed:
    return "Closed";
  case TickDecision::WrongThread:
    return "WrongThread";
  case TickDecision::InvalidSource:
    return "InvalidSource";
  case TickDecision::InvalidLoop:
    return "InvalidLoop";
  case TickDecision::Reentry:
    return "Reentry";
  case TickDecision::DuplicateLoop:
    return "DuplicateLoop";
  case TickDecision::WrongReturn:
    return "WrongReturn";
  case TickDecision::AbnormalReturn:
    return "AbnormalReturn";
  case TickDecision::MissingReturn:
    return "MissingReturn";
  case TickDecision::Exhausted:
    return "Exhausted";
  case TickDecision::Cancelled:
    return "Cancelled";
  case TickDecision::DrainBudgetExceeded:
    return "DrainBudgetExceeded";
  }
  return "UNKNOWN";
}
bool TickClock::ConfigureFutureWindow(std::uint64_t future_window) noexcept {
  if (window_configured_ || !future_window ||
      future_window > input::kRuntimeFutureWindowMaximum)
    return false;
  if (pending_valid_ || Current() != 0 || last_started_loop_ != 0)
    return false;
  future_window_ = future_window;
  window_configured_ = true;
  return true;
}
bool TickClock::ValidSource(const TickBoundary &boundary) noexcept {
  if (!boundary.scope_token)
    return false;
  if (boundary.kind == TickKind::StandardRoom)
    return boundary.owner_kind == TickOwnerKind::Game;
  return boundary.kind == TickKind::OddPlayer &&
         boundary.owner_kind == TickOwnerKind::OddNormal;
}
// 只接纳主线程中已分类且本轮唯一的普通模拟入口。
TickDecision TickClock::CheckStart(const TickBoundary &boundary, bool active,
                                   bool blocked) const noexcept {
  if (closed_)
    return TickDecision::Closed;
  if (pending_valid_)
    return TickDecision::Reentry;
  if (!active)
    return TickDecision::Inactive;
  if (blocked)
    return TickDecision::Blocked;
  if (!boundary.thread || boundary.thread != boundary.main_thread ||
      (owner_thread_ && owner_thread_ != boundary.thread))
    return TickDecision::WrongThread;
  if (!ValidSource(boundary))
    return TickDecision::InvalidSource;
  if (!boundary.loop)
    return TickDecision::InvalidLoop;
  if (boundary.loop <= last_started_loop_)
    return TickDecision::DuplicateLoop;
  // 在源分配 sequence 之前，保证 candidate、未来目标及边沿目标均可表示。
  if (future_window_ < input::kRuntimeFutureWindowMinimum ||
      future_window_ > input::kRuntimeFutureWindowMaximum ||
      Completed() >
          (std::numeric_limits<std::uint64_t>::max)() - future_window_ - 1)
    return TickDecision::Exhausted;
  return TickDecision::Started;
}
TickDecision TickClock::Begin(const TickBoundary &boundary, bool active,
                              bool blocked) noexcept {
  const auto decision = CheckStart(boundary, active, blocked);
  if (decision == TickDecision::Inactive || decision == TickDecision::Blocked ||
      decision == TickDecision::Closed)
    return decision;
  if (decision != TickDecision::Started)
    return Cancel(decision);
  pending_ = boundary;
  pending_valid_ = true;
  owner_thread_ = boundary.thread;
  last_started_loop_ = boundary.loop;
  current_.store(Completed() + 1, std::memory_order_release);
  return TickDecision::Started; // 预留 candidate 不推进 completed。
}
TickDecision TickClock::Cancel(TickDecision reason) noexcept {
  pending_valid_ = false;
  closed_ = true;
  current_.store(0, std::memory_order_release);
  return reason;
}
// 返回事件必须带回同一个 owner/token/loop/thread，不接受替代返回。
TickDecision TickClock::CheckComplete(const TickBoundary &boundary,
                                      bool normal_return,
                                      bool active) const noexcept {
  if (closed_)
    return TickDecision::Closed;
  if (!pending_valid_)
    return TickDecision::NoScope;
  if (!active)
    return TickDecision::Cancelled;
  if (!normal_return)
    return TickDecision::AbnormalReturn;
  if (!boundary.thread || boundary.thread != boundary.main_thread ||
      boundary.thread != owner_thread_)
    return TickDecision::WrongThread;
  if (!(boundary == pending_))
    return TickDecision::WrongReturn;
  return TickDecision::Committed;
}
TickDecision TickClock::Complete(const TickBoundary &boundary,
                                 bool normal_return, bool active) noexcept {
  const auto decision = CheckComplete(boundary, normal_return, active);
  if (decision == TickDecision::Closed || decision == TickDecision::NoScope)
    return decision;
  if (decision != TickDecision::Committed)
    return Cancel(decision);
  // 原生窗口已正常返回；房间或绑定后续失效不抹掉这个已完成的事实。
  completed_.store(Current(), std::memory_order_release);
  current_.store(0, std::memory_order_release);
  pending_valid_ = false;
  return TickDecision::Committed;
}
TickDecision TickClock::EndOuter(std::uint64_t loop, bool active) noexcept {
  if (!pending_valid_)
    return TickDecision::Idle;
  // 外层结束仍有 pending，说明配对返回缺失；不同 loop 也不能替它收尾。
  (void)loop;
  return Cancel(active ? TickDecision::MissingReturn : TickDecision::Cancelled);
}
} // namespace cs::core
