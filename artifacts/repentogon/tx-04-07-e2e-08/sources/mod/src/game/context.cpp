#include "game/context.h"
#include "game/tick.h"
#include "game/state.h"
#include <limits>

namespace cs::game {
namespace {
struct OwnerFrame {
  Owner owner;
  uint64_t token;
};
thread_local std::array<OwnerFrame, 64> stack{};
thread_local unsigned depth = 0;
thread_local uint64_t next_token = 0;
thread_local bool healthy = true;
std::atomic<uint64_t> errors{};
struct OddMarker {
  void *player = nullptr;
  uint64_t owner = 0, token = 0;
  bool consumed = false;
  unsigned player_entries = 0;
};
thread_local OddMarker odd;
} // namespace
Owner TopOwner() noexcept {
  return depth ? stack[depth - 1].owner : Owner::None;
}
uint64_t OwnerToken(Owner owner) noexcept {
  for (unsigned i = depth; i; --i)
    if (stack[i - 1].owner == owner)
      return stack[i - 1].token;
  return 0;
}
bool HasOwner(Owner owner) noexcept { return OwnerToken(owner) != 0; }
bool ContextHealthy() noexcept { return healthy; }
bool InBlockedContext() noexcept {
  return HasOwner(Owner::Start) || HasOwner(Owner::Restore) ||
         HasOwner(Owner::Lifecycle) || HasOwner(Owner::Door) ||
         HasOwner(Owner::NestedDoor) || HasOwner(Owner::Join);
}
uint64_t ContextErrors() noexcept { return errors.load(); }
void ContextFault(const char *reason) noexcept {
  ++errors;
  healthy = false;
  Trace("context_error", "reason=%s depth=%u", reason, depth);
  FailRuntime(reason);
}
uint64_t PushOwner(Owner owner) noexcept {
  if (!IsMain() || owner == Owner::None)
    return 0;
  if (depth == stack.size() ||
      next_token == (std::numeric_limits<uint64_t>::max)()) {
    ContextFault("owner_capacity");
    return 0;
  }
  const auto token = ++next_token;
  stack[depth++] = {owner, token};
  return token;
}
void PopOwner(uint64_t token) noexcept {
  if (!token)
    return;
  if (!depth || stack[depth - 1].token != token) {
    ContextFault("owner_token_mismatch");
    return;
  }
  --depth;
}
void BeginContextFrame() noexcept {
  if (depth != 1 || TopOwner() != Owner::Manager || odd.token) {
    ContextFault("manager_context_not_drained");
    return;
  }
  // 只有前一轮已完整弹栈才允许下一次会话重新建立正常上下文。
  healthy = true;
}
void __stdcall EnterOddNormal(void *player) noexcept {
  if (!IsMain())
    return;
  if (odd.token || TopOwner() != Owner::Odd) {
    ContextFault("odd_marker_reentry");
    return;
  }
  odd = {player, OwnerToken(Owner::Odd), PushOwner(Owner::OddNormal), false};
  if (Armed())
    Trace("odd_call", "phase=enter token=%llu owner_token=%llu loop=%llu", odd.token, odd.owner, ManagerLoop());
}
void __stdcall LeaveOddNormal() noexcept {
  if (!IsMain())
    return;
  if (odd.token)
  {
    if (Armed())
      Trace("odd_call", "phase=leave token=%llu owner_token=%llu loop=%llu player_entries=%u", odd.token, odd.owner, ManagerLoop(), odd.player_entries);
    PopOwner(odd.token);
  }
  odd = {};
}
bool TakeOddNormal(void *player) noexcept {
  if (!ContextHealthy() || TopOwner() != Owner::OddNormal ||
      odd.player != player || odd.owner != OwnerToken(Owner::Odd) ||
      odd.consumed)
    return false;
  odd.consumed = true;
  return true;
}
void RecordOddPlayerEntry() noexcept {
  if (TopOwner() == Owner::OddNormal && odd.token)
    ++odd.player_entries;
  else if (HasOwner(Owner::Odd))
    Trace("odd_special_player", "loop=%llu normal_marker=0 tick=%llu", ManagerLoop(),
          Runtime() ? Runtime()->pipeline.SimulationTick() : 0);
}
void ClearOddMarkers(uint64_t odd_token) noexcept {
  // 原生异常可能越过裸 shim，Odd RAII 在弹出 owner 前收走尚未离开的标记。
  if (odd.owner == odd_token)
    LeaveOddNormal();
}
} // namespace cs::game
