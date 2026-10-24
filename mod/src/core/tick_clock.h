#pragma once

#include "core/input_core.h"
#include <atomic>
#include <cstdint>

namespace cs::core {

enum class TickKind : std::uint32_t { StandardRoom = 1, OddPlayer = 2 };
enum class TickDecision : std::uint32_t {
  Idle,
  Started,
  Committed,
  Inactive,
  Blocked,
  NoScope,
  Closed,
  WrongThread,
  InvalidSource,
  InvalidLoop,
  Reentry,
  DuplicateLoop,
  WrongReturn,
  AbnormalReturn,
  MissingReturn,
  Exhausted,
  Cancelled,
  DrainBudgetExceeded
};
// owner_kind 是适配层已分类的调用来源；OddNormal 只能由普通调用点 shim 建立。
enum class TickOwnerKind : std::uint32_t { None, Game, OddNormal };

// 完全不含游戏地址或 RVA。scope_token 由主线程上下文栈分配，成对返回必须一致；
// loop 是最外层 Manager 观测编号，只用来防止同一轮重复开始，不直接产生 tick。
struct TickBoundary {
  TickKind kind = TickKind::StandardRoom;
  std::uint64_t loop = 0;
  TickOwnerKind owner_kind = TickOwnerKind::None;
  std::uint64_t scope_token = 0;
  std::uint32_t thread = 0, main_thread = 0;
  bool operator==(const TickBoundary &) const = default;
};
const char *TickDecisionName(TickDecision decision) noexcept;

// 单一主线程修改时钟；current 保留待提交 candidate，completed 到 TopExit
// 才推进。 查询侧通过 TickPipeline::SimulationTick 限定实际可读取 current
// 的模拟窗口。 拒绝异常来源后关闭本会话时钟，不能靠下一轮继续使用未收束的状态。
class TickClock final {
public:
  explicit TickClock(std::uint64_t initial_completed = 0) noexcept
      : completed_(initial_completed) {}
  bool ConfigureFutureWindow(std::uint64_t future_window) noexcept;
  std::uint64_t FutureWindow() const noexcept { return future_window_; }
  TickDecision CheckStart(const TickBoundary &boundary, bool active,
                          bool blocked) const noexcept;
  TickDecision Begin(const TickBoundary &boundary, bool active,
                     bool blocked) noexcept;
  TickDecision Complete(const TickBoundary &boundary, bool normal_return,
                        bool completion_allowed) noexcept;
  TickDecision CheckComplete(const TickBoundary &boundary, bool normal_return,
                             bool completion_allowed) const noexcept;
  TickDecision EndOuter(std::uint64_t loop, bool active) noexcept;
  TickDecision Cancel(TickDecision reason) noexcept;
  std::uint64_t Current() const noexcept {
    return current_.load(std::memory_order_acquire);
  }
  std::uint64_t Completed() const noexcept {
    return completed_.load(std::memory_order_acquire);
  }
  bool Closed() const noexcept { return closed_; }

private:
  static bool ValidSource(const TickBoundary &boundary) noexcept;
  std::atomic<std::uint64_t> current_{0}, completed_{0};
  TickBoundary pending_{};
  std::uint64_t last_started_loop_ = 0;
  std::uint32_t owner_thread_ = 0;
  std::uint64_t future_window_ = input::kMaximumFutureTicks;
  bool window_configured_ = false;
  bool pending_valid_ = false, closed_ = false;
};

} // namespace cs::core
