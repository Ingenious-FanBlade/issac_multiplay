#include "game/common/context.h"
#include "game/authority/tick.h"
#include "runtime.h"
#include <exception>
namespace cs {
// owner 栈统一由 game/context 管理；非主线程仅计数和发布失败通知。
Owner CurrentOwner() noexcept { return game::TopOwner(); }
uint64_t ContextFaults() noexcept { return game::ContextErrors(); }
// 只使用固定容量存储，保证观测代码不会把异常带入原生调用链。
CallScope::CallScope(Hook &h) noexcept
    : hook(h), main(IsMain()), exceptions(std::uncaught_exceptions()) {
  if (h.calls.fetch_add(1) == 0)
    Trace("first_call", "%s owner=%u", h.id, unsigned(CurrentOwner()));
  if (!main) {
    h.foreign.fetch_add(1);
    game::ForeignCall();
    return;
  }
  if (h.owner != Owner::None) {
    token = game::PushOwner(h.owner);
    pushed = token != 0;
  }
}
// 异常和正常返回都弹出本层；只有正常返回才增加 completed。
CallScope::~CallScope() noexcept {
  if (exceptions == std::uncaught_exceptions()) {
    // 首次进入与首次正常返回分开记录，不能由 first_call 推断原函数已返回。
    if (hook.completed.fetch_add(1) == 0)
      Trace("first_return", "%s", hook.id);
  } else {
    if (main)
      game::ContextFault("native_exception");
    Trace("context_error", "exception id=%s", hook.id);
  }
  if (pushed)
    game::PopOwner(token);
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
