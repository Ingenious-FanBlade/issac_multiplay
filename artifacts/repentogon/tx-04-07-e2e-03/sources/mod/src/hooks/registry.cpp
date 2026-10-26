#include "HookSystem.h"
#include "runtime.h"
namespace cs {
namespace {
// 唯一待注册清单，静态构造阶段不触碰 REPENTOGON。
Hook *pending[64]{};
size_t used = 0;
} // namespace
void AddPending(Hook *h) noexcept {
  if (used < 64)
    pending[used++] = h;
}
Hook *const *Hooks(size_t &n) noexcept {
  n = used;
  return pending;
}
void RegisterHooks() {
  // tag 1.1.3 的公开注册入口是构造函数，元数据及对象常驻到进程结束。
  for (size_t i = 0; i < used; ++i) {
    auto &h = *pending[i];
    if (!Capabilities[size_t(h.group)])
      continue;
    new FunctionHook(h.name, *h.type, h.entry, h.super, h.priority);
    h.registered = true;
    Trace("hook_registered", "%s group=%s priority=%d", h.id, GroupNames[size_t(h.group)],
          h.priority);
  }
}
} // namespace cs
