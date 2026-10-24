#pragma once
#include "isaac_cs_t0/input_core.hpp"
#include <atomic>
#include <cstdint>

namespace isaac_cs_t0::coop_active {
enum class TickKind : std::uint32_t { StandardRoom=1,OddPlayer=2 };
enum class TickDecision : std::uint32_t {
    Idle,Started,Committed,Inactive,Blocked,NoScope,Closed,WrongThread,InvalidSource,
    InvalidLoop,Reentry,DuplicateLoop,WrongReturn,AbnormalReturn,MissingReturn,Exhausted,Cancelled,DrainBudgetExceeded
};
struct TickBoundary {
    TickKind kind=TickKind::StandardRoom;
    std::uint64_t loop=0;
    std::uintptr_t owner=0; // Local call identity only, never a network identity.
    std::uint32_t thread=0,main_thread=0,return_rva=0,player_return_rva=0;
    bool operator==(const TickBoundary&) const=default;
};
const char* TickDecisionName(TickDecision decision) noexcept;

// A single main-thread owner mutates this clock. Counter reads can also occur in
// initializer/drained restoration diagnostics, so only those values are atomic.
// The explicit initial value permits an offline boundary scenario; Native uses 0.
class AuthorityTickClock final {
public:
    explicit AuthorityTickClock(std::uint64_t initial_completed=0) noexcept:completed_(initial_completed) {}
    // One-shot before the first scope. Offline scenarios may use it after
    // constructing a non-default initial completed tick.
    bool ConfigureFutureWindow(std::uint64_t future_window) noexcept;
    std::uint64_t FutureWindow() const noexcept {return future_window_;}
    TickDecision CheckStart(const TickBoundary& boundary,bool active,bool blocked) const noexcept;
    TickDecision Begin(const TickBoundary& boundary,bool active,bool blocked) noexcept;
    TickDecision Complete(const TickBoundary& boundary,bool normal_return,bool active) noexcept;
    TickDecision CheckComplete(const TickBoundary& boundary,bool normal_return,bool active) const noexcept;
    TickDecision EndOuter(std::uint64_t loop,bool active) noexcept;
    TickDecision Cancel(TickDecision reason) noexcept;
    std::uint64_t Current() const noexcept {return current_.load(std::memory_order_acquire);}
    std::uint64_t Completed() const noexcept {return completed_.load(std::memory_order_acquire);}
    bool Closed() const noexcept {return closed_;} // Main-thread only.
private:
    static bool ValidSource(const TickBoundary& boundary) noexcept;
    std::atomic<std::uint64_t> current_{0},completed_{0};
    TickBoundary pending_{};
    std::uint64_t last_started_loop_=0;
    std::uint32_t owner_thread_=0;
    std::uint64_t future_window_=input::kMaximumFutureTicks;
    bool window_configured_=false;
    bool pending_valid_=false,closed_=false;
};
} // namespace isaac_cs_t0::coop_active
