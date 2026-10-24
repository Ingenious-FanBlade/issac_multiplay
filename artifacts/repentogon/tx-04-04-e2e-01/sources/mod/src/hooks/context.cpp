#include "runtime.h"
#include <exception>
namespace cs {
namespace {
thread_local std::array<Owner,32> stack;
thread_local unsigned depth=0;
std::atomic<uint64_t> faults{};
}
Owner CurrentOwner() noexcept { return depth ? stack[depth-1] : Owner::None; }
uint64_t ContextFaults() noexcept { return faults.load(); }
CallScope::CallScope(Hook& h) noexcept : hook(h),main(IsMain()),exceptions(std::uncaught_exceptions()) {
    if (h.calls.fetch_add(1)==0) Trace("first_call","%s owner=%u",h.id,unsigned(CurrentOwner()));
    if (!main) { h.foreign.fetch_add(1); return; }
    if (h.owner!=Owner::None) {
        if (depth<stack.size()) { stack[depth++]=h.owner; pushed=true; }
        else { ++faults; Trace("context_error","overflow id=%s",h.id); }
    }
}
CallScope::~CallScope() noexcept {
    if (exceptions==std::uncaught_exceptions()) ++hook.completed;
    else { ++faults; Trace("context_error","exception id=%s",hook.id); }
    if (pushed) --depth;
}
}
