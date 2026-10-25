#include "game/input_a.h"
#include "game/context.h"
#include "game/lifecycle.h"
#include "game/state.h"
#include "game/tick.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>

namespace cs::game {
namespace {
// 只保存当前已知物理源；设备消失/身份不明后首次重见只建立 held 基线。
struct Source {
  core::Binding pair{};
  core::ShootingDirections shooting;
  core::InputStateDraftV2 sample{}, logged{};
  uint32_t held = 0;
  uint64_t frame = 0, serial = 0, last_edge_serial = 0;
  uint64_t published_serial = 0, published_completed = 0,
           published_generation = 0;
  bool known = false, fresh = false, logged_valid = false;
} source;
thread_local DeviceScope *device_scope = nullptr;
} // namespace
DeviceScope::DeviceScope(void *value) noexcept
    : device(value), main(IsMain()), exceptions(std::uncaught_exceptions()) {
  if (!main)
    return;
  previous = device_scope;
  device_scope = this;
  if (TopOwner() == Owner::Device && HasOwner(Owner::Input) &&
      HasOwner(Owner::Manager)) {
    token = OwnerToken(Owner::Device);
    // 采样的新鲜度统一使用 Manager 轮次；scope token 只作配对，二者不可互换。
    loop = ManagerLoop();
  }
}
DeviceScope::~DeviceScope() noexcept {
  if (!main)
    return;
  if (exceptions != std::uncaught_exceptions())
    FailRuntime("device_update_exception");
  if (device_scope != this)
    ContextFault("device_scope_mismatch");
  device_scope = previous;
}
void PollReturned(void *device, bool success) noexcept {
  if (!IsMain() || !device_scope || !device_scope->token ||
      device_scope->device != device)
    return;
  ++device_scope->polls;
  device_scope->poll_ok = success;
}
void DeviceScope::Returned(bool success) {
  if (!main || !token || !ContextHealthy())
    return;
  if (TopOwner() != Owner::Device || OwnerToken(Owner::Device) != token ||
      !HasOwner(Owner::Input) || ManagerLoop() != loop) {
    ForgetInputDevice();
    FailRuntime("device_sample_scope_mismatch",
                core::TickDecision::WrongReturn);
    return;
  }
  // 原生将在失败返回后移除/销毁设备；先退休源历史，防止同轮地址复用冒充旧设备。
  if (!success) {
    auto *r = Runtime();
    const bool tracked = source.known && source.pair.device == device;
    const bool bound = r && !r->finished &&
                       (r->players[0].Bound().device == device ||
                        r->topology[0].pair.device == device);
    if (tracked || bound) {
      ForgetInputDevice();
      if (r && !r->finished)
        InvalidateBinding(*r, "device_update_failed", true);
    }
    return;
  }
  const auto topology = ReadTopology();
  if (!topology.unique || topology.pair.device != device)
    return;
  if (polls != 1 || !poll_ok) {
    if (Armed())
      FailRuntime("device_poll_witness_missing");
    return;
  }
  PhysicalActions actions;
  if (!ReadPhysicalActions(device, actions)) {
    ForgetInputDevice();
    if (Armed())
      FailRuntime("physical_query_unavailable",
                  core::TickDecision::InvalidSource);
    return;
  }
  CaptureInput(device, loop, actions);
}
void ForgetInputDevice() noexcept {
  source.known = false;
  source.fresh = false;
  source.held = 0;
  source.shooting = {};
  source.pair = {};
  source.sample = {};
}
void DiscardInputEdges() noexcept {
  source.sample.edge_flags = 0;
  source.last_edge_serial = source.serial;
}
// Poll 只记录结果；此处运行在公共更新成功返回后，读取最终 action 状态。
void CaptureInput(void *device, uint64_t loop, const PhysicalActions &actions) {
  const auto topology = ReadTopology();
  if (!topology.unique || topology.pair.device != device) {
    ForgetInputDevice();
    return;
  }
  const bool same = source.known && source.pair.device == device &&
                    source.pair.key == topology.pair.key;
  if (!same) {
    source.shooting = {};
    source.held = 0;
  }
  core::input::RawInputSample raw;
  uint32_t held = 0;
  for (unsigned i = 0; i < 4; ++i)
    if (actions.pressed[8 + i])
      held |= 1u << i;
  raw.move_x = actions.values[1] - actions.values[0];
  raw.move_y = actions.values[3] - actions.values[2];
  raw.held_flags = ((held >> 1) & 7u) << 1;
  raw.edge_flags = same ? held & ~source.held : 0;
  core::InputStateDraftV2 state;
  const std::array<float, 4> aim{actions.values[4], actions.values[5],
                                 actions.values[6], actions.values[7]};
  if (!core::CanonicalizeShooting(raw, aim, source.shooting, state)) {
    ForgetInputDevice();
    FailRuntime("noncanonical_physical_sample");
    return;
  }
  if (source.serial == (std::numeric_limits<uint64_t>::max)()) {
    FailRuntime("sample_sequence_exhausted");
    return;
  }
  source.known = true;
  source.fresh = true;
  source.pair = topology.pair;
  source.held = held;
  source.frame = loop;
  source.sample = state;
  ++source.serial;
  // 屏蔽时仅推进物理历史，不把该区间产生的边沿留给恢复后的 Authority。
  if (auto *r = Runtime();
      r && (r->blocks || r->console_blocked || r->retiring))
    DiscardInputEdges();
  if (Armed() && (!source.logged_valid ||
                  std::memcmp(&source.logged, &source.sample, sizeof(state)))) {
    Trace(
        "sample",
        "ordinal=0 sample=%llu loop=%llu move=%d,%d aim=%d,%d held=%u edge=%u",
        source.serial, loop, state.move_x, state.move_y, state.aim_x,
        state.aim_y, source.sample.held_flags, source.sample.edge_flags);
    source.logged = source.sample;
    source.logged_valid = true;
  }
}
void InputFrameFinished() noexcept {
  if (!IsMain())
    return;
  if (!ReadTopology().unique)
    ForgetInputDevice();
}
// 所有本地样本先 MakeCommand，再走流水线统一字节 admission，绝不直接写快照。
bool SubmitInput(SessionRuntime &r) noexcept {
  auto &player = r.players[0];
  if (!source.fresh || !source.known || source.frame != r.loop ||
      !(source.pair == player.Bound()) || !player.BoundValid())
    return true;
  const auto completed = r.clock.Completed(), generation = player.Generation();
  if (source.published_serial == source.serial &&
      source.published_completed == completed &&
      source.published_generation == generation)
    return true;
  auto state = source.sample;
  if (source.last_edge_serial == source.serial)
    state.edge_flags = 0;
  const bool startup = player.StartupPending();
  uint64_t target = 0;
  if (!(startup
            ? core::NextStartupTarget(completed, target)
            : core::NextInputTarget(completed, player.RuntimeConfig(), target)))
    return false;
  const auto submit = [&](const core::InputStateDraftV2 &value,
                          uint64_t tick) noexcept {
    const auto cmd = player.MakeCommand(value, tick);
    const auto result = r.pipeline.AcceptInput(0, cmd, generation);
    Trace("submit",
          "ordinal=0 generation=%llu sample=%llu sequence=%llu target=%llu "
          "loop=%llu session_generation=%llu "
          "decision=%u edge=%u move=%d,%d aim=%d,%d held=%u",
          generation, source.serial, cmd.sequence, cmd.target_tick, r.loop,
          r.session.Identity().generation,
          unsigned(result), cmd.state.edge_flags, cmd.state.move_x, cmd.state.move_y,
          cmd.state.aim_x, cmd.state.aim_y, cmd.state.held_flags);
    return result == core::Decision::Accepted ||
           result == core::Decision::Duplicate;
  };
  if (!submit(state, target))
    return false;
  source.last_edge_serial = source.serial;
  if (startup) {
    state.edge_flags = 0;
    if (!core::NextInputTarget(completed, player.RuntimeConfig(), target) ||
        !submit(state, target))
      return false;
  }
  source.published_serial = source.serial;
  source.published_completed = completed;
  source.published_generation = generation;
  return true;
}
} // namespace cs::game
