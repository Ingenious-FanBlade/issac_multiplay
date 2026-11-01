#include "HookSystem.h"
#include "hook_capacity.h"
#include "runtime.h"
#include <cstring>
namespace cs {
namespace {
// 唯一待注册清单，静态构造阶段不触碰 REPENTOGON。
// 容量与行数来自生成的 hook_capacity.h，生成期已校验行数不超限；
// 这里保留运行时检查，避免手工改动后静默丢弃。
Hook *pending[kHookPendingCapacity]{};
size_t used = 0;
// 容量不足时不静默跳过：保留前 8 条被拒记录，供 PrepareBindings 停用对应
// 能力组并写入 trace。计数另记，避免拒收超过保留容量时丢失总数。
constexpr size_t kRejectedKeep = 8;
Hook *rejected[kRejectedKeep]{};
size_t rejected_count = 0;
size_t rejected_total = 0;
} // namespace
void AddPending(Hook *h) noexcept {
  if (!h)
    return;
  if (used < kHookPendingCapacity) {
    pending[used++] = h;
    return;
  }
  ++rejected_total;
  if (rejected_count < kRejectedKeep)
    rejected[rejected_count++] = h;
}
Hook *const *Hooks(size_t &n) noexcept {
  n = used;
  return pending;
}
HookCapacity PendingHookCapacity() noexcept {
  return {kHookPendingCapacity, used, rejected_total};
}
Hook *const *RejectedHooks(size_t &count) noexcept {
  count = rejected_count;
  return rejected;
}
void RegisterHooks() {
  // tag 1.1.3 的公开注册入口是构造函数，元数据及对象常驻到进程结束。
  size_t installed = 0, deferred = 0;
  for (size_t i = 0; i < used; ++i) {
    auto &h = *pending[i];
    if (!Capabilities[size_t(h.group)])
      continue;
    new FunctionHook(h.name, *h.type, h.entry, h.super, h.priority);
    h.registered = true;
    ++installed;
    Trace("hook_registered", "%s group=%s priority=%d", h.id,
          GroupNames[size_t(h.group)], h.priority);
  }
  // 容量与安装结果必须显式可见：注册数不等于安装数，超限也不能静默。
  const auto capacity = PendingHookCapacity();
  Trace("hook_summary",
        "capacity=%u rows=%u used=%u rejected=%u installed=%u deferred=%u",
        unsigned(capacity.capacity), unsigned(kHookManifestRows),
        unsigned(capacity.used), unsigned(capacity.rejected),
        unsigned(installed), unsigned(deferred));
}
} // namespace cs
namespace cs::game {
bool ClientObjectPreStatsIsolationInstalled() noexcept {
  if(!Capabilities[size_t(Group::Client)])return false;
  size_t count=0;auto hooks=Hooks(count);
  for(size_t i=0;i<count;++i){const auto &h=*hooks[i];
    if(std::strcmp(h.id,"client_player_pre_stats")==0)return h.registered&&h.super&&*h.super;
  }
  return false;
}
}
