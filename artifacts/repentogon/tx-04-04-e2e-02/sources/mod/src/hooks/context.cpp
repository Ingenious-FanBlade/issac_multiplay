#include "runtime.h"
#include <exception>
namespace cs {
namespace {
// 每个线程保存自己的 owner 栈；非主线程仅计数，不参与游戏上下文。
thread_local std::array<Owner, 32> stack;
thread_local unsigned depth = 0;
std::atomic<uint64_t> faults{};
} // namespace
Owner CurrentOwner() noexcept {
  return depth ? stack[depth - 1] : Owner::None;
}
uint64_t ContextFaults() noexcept {
  return faults.load();
}
// 只使用固定容量存储，保证观测代码不会把异常带入原生调用链。
CallScope::CallScope(Hook &h) noexcept
    : hook(h), main(IsMain()), exceptions(std::uncaught_exceptions()) {
  if (h.calls.fetch_add(1) == 0)
    Trace("first_call", "%s owner=%u", h.id, unsigned(CurrentOwner()));
  if (!main) {
    h.foreign.fetch_add(1);
    return;
  }
  if (h.owner != Owner::None) {
    if (depth < stack.size()) {
      stack[depth++] = h.owner;
      pushed = true;
    } else {
      ++faults;
      Trace("context_error", "overflow id=%s", h.id);
    }
  }
}
// 异常和正常返回都弹出本层；只有正常返回才增加 completed。
CallScope::~CallScope() noexcept {
  if (exceptions == std::uncaught_exceptions())
    ++hook.completed;
  else {
    ++faults;
    Trace("context_error", "exception id=%s", hook.id);
  }
  if (pushed)
    --depth;
}
FrameScope::FrameScope(bool enabled) noexcept
    : main(enabled), exceptions(std::uncaught_exceptions()) {
  if (main)
    FrameBegin();
}
FrameScope::~FrameScope() noexcept {
  if (main)
    FrameEnd(exceptions == std::uncaught_exceptions());
}
} // namespace cs
