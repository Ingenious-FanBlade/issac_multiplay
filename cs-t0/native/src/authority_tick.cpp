#include "isaac_cs_t0/authority_tick.hpp"
#include "isaac_cs_t0/input_core.hpp"
#include <limits>

namespace isaac_cs_t0::coop_active {
const char* TickDecisionName(TickDecision decision) noexcept
{
    switch(decision) {
    case TickDecision::Idle:return "Idle";
    case TickDecision::Started:return "Started";
    case TickDecision::Committed:return "Committed";
    case TickDecision::Inactive:return "Inactive";
    case TickDecision::Blocked:return "Blocked";
    case TickDecision::NoScope:return "NoScope";
    case TickDecision::Closed:return "Closed";
    case TickDecision::WrongThread:return "WrongThread";
    case TickDecision::InvalidSource:return "InvalidSource";
    case TickDecision::InvalidLoop:return "InvalidLoop";
    case TickDecision::Reentry:return "Reentry";
    case TickDecision::DuplicateLoop:return "DuplicateLoop";
    case TickDecision::WrongReturn:return "WrongReturn";
    case TickDecision::AbnormalReturn:return "AbnormalReturn";
    case TickDecision::MissingReturn:return "MissingReturn";
    case TickDecision::Exhausted:return "Exhausted";
    case TickDecision::Cancelled:return "Cancelled";
    case TickDecision::DrainBudgetExceeded:return "DrainBudgetExceeded";
    }
    return "UNKNOWN";
}
bool AuthorityTickClock::ConfigureFutureWindow(std::uint64_t future_window) noexcept
{
    if(window_configured_ || !future_window ||
       future_window>input::kRuntimeFutureWindowMaximum) return false;
    if(pending_valid_ || Current()!=0 || last_started_loop_!=0) return false;
    future_window_=future_window;window_configured_=true;return true;
}
bool AuthorityTickClock::ValidSource(const TickBoundary& boundary) noexcept
{
    if(!boundary.owner) return false;
    if(boundary.kind==TickKind::StandardRoom)
        return boundary.return_rva==0x2FBA52u && !boundary.player_return_rva;
    return boundary.kind==TickKind::OddPlayer && boundary.return_rva==0x555246u &&
        boundary.player_return_rva==0x2FD60Cu;
}
TickDecision AuthorityTickClock::CheckStart(const TickBoundary& boundary,bool active,bool blocked) const noexcept
{
    if(closed_) return TickDecision::Closed;
    if(pending_valid_) return TickDecision::Reentry;
    if(!active) return TickDecision::Inactive;
    if(blocked) return TickDecision::Blocked;
    if(!boundary.thread || boundary.thread!=boundary.main_thread ||
       (owner_thread_ && owner_thread_!=boundary.thread)) return TickDecision::WrongThread;
    if(!ValidSource(boundary)) return TickDecision::InvalidSource;
    if(!boundary.loop) return TickDecision::InvalidLoop;
    if(boundary.loop<=last_started_loop_) return TickDecision::DuplicateLoop;
    // All configured target and edge-target calculations stay representable
    // before any producer consumes a script or allocates a sequence.
    if(future_window_<input::kRuntimeFutureWindowMinimum ||
       future_window_>input::kRuntimeFutureWindowMaximum ||
       Completed()>(std::numeric_limits<std::uint64_t>::max)()-future_window_-1)
        return TickDecision::Exhausted;
    return TickDecision::Started;
}
TickDecision AuthorityTickClock::Begin(const TickBoundary& boundary,bool active,bool blocked) noexcept
{
    const auto decision=CheckStart(boundary,active,blocked);
    if(decision==TickDecision::Inactive || decision==TickDecision::Blocked || decision==TickDecision::Closed)
        return decision;
    if(decision!=TickDecision::Started) return Cancel(decision);
    pending_=boundary;pending_valid_=true;owner_thread_=boundary.thread;last_started_loop_=boundary.loop;
    current_.store(Completed()+1,std::memory_order_release);
    return TickDecision::Started; // Reserving a candidate never advances completed.
}
TickDecision AuthorityTickClock::Cancel(TickDecision reason) noexcept
{
    pending_valid_=false;closed_=true;current_.store(0,std::memory_order_release);
    return reason;
}
TickDecision AuthorityTickClock::CheckComplete(const TickBoundary& boundary,bool normal_return,bool active) const noexcept
{
    if(closed_) return TickDecision::Closed;
    if(!pending_valid_) return TickDecision::NoScope;
    if(!active) return TickDecision::Cancelled;
    if(!normal_return) return TickDecision::AbnormalReturn;
    if(!boundary.thread || boundary.thread!=boundary.main_thread || boundary.thread!=owner_thread_)
        return TickDecision::WrongThread;
    if(!(boundary==pending_)) return TickDecision::WrongReturn;
    return TickDecision::Committed;
}
TickDecision AuthorityTickClock::Complete(const TickBoundary& boundary,bool normal_return,bool active) noexcept
{
    const auto decision=CheckComplete(boundary,normal_return,active);
    if(decision==TickDecision::Closed || decision==TickDecision::NoScope)return decision;
    if(decision!=TickDecision::Committed)return Cancel(decision);
    // Room/death generations and collector completeness are intentionally not
    // clock conditions: the confirmed native body has actually returned normally.
    completed_.store(Current(),std::memory_order_release);
    current_.store(0,std::memory_order_release);pending_valid_=false;
    return TickDecision::Committed;
}
TickDecision AuthorityTickClock::EndOuter(std::uint64_t loop,bool active) noexcept
{
    if(!pending_valid_) return TickDecision::Idle;
    // A pending scope at any outer return is missing its paired native post,
    // including when a changed loop token reveals an escaped/nested scope.
    (void)loop;
    return Cancel(active?TickDecision::MissingReturn:TickDecision::Cancelled);
}
} // namespace isaac_cs_t0::coop_active
