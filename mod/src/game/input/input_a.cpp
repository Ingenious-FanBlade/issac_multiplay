#include "game/input/input_a.h"
#include "game/client/session/client.h"
#include "game/common/context.h"
#include "game/authority/lifecycle.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "replica/stream_e2e.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>

namespace cs::game {
namespace {
// 物理轮次可多于Authority tick；只保存尚未入统一队列的边沿，不保存旧世界状态。
// 每个新鲜Authority-pre最多提交一个上升沿样本，重复同键不会被位或合并丢失。
struct PendingEdge final { uint32_t mask = 0; uint64_t sample = 0; };
// 只保存当前已知物理源；设备消失/身份不明后首次重见只建立 held 基线。
struct Source {
  core::Binding pair{};
  core::ShootingDirections shooting;
  core::InputStateDraftV2 sample{}, logged{};
  uint32_t held = 0;
  uint64_t frame = 0, serial = 0;
  std::array<PendingEdge, 16> edges{};
  unsigned edge_head = 0, edge_count = 0;
  uint64_t edge_session = 0, edge_generation = 0;
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
  if (ClientDeviceReturned(device, success, polls, poll_ok))
    return;
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
  DiscardInputEdges();
  source.known = false;
  source.fresh = false;
  source.held = 0;
  source.shooting = {};
  source.pair = {};
  source.sample = {};
}
void DiscardInputEdges() noexcept {
  source.sample.edge_flags = 0;
  source.edge_head = source.edge_count = 0;
  source.edge_session = source.edge_generation = 0;
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
    DiscardInputEdges();
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
  // 只在当前唯一绑定的有效会话内暂存；暂停/加载/换代/设备变化立即清除。
  // held历史仍随物理源更新，恢复时不会为一直按住的键重新制造上升沿。
  auto *r = Runtime();
  const bool eligible = r && Armed() && !r->blocks && !r->console_blocked &&
      !r->retiring && !r->failure && !InBlockedContext() &&
      r->players[0].BoundValid() && source.pair == r->players[0].Bound();
  if (!eligible) {
    DiscardInputEdges();
  } else {
    const auto session = SessionGeneration(*r), generation = r->players[0].Generation();
    if (source.edge_session != session || source.edge_generation != generation) {
      // 清旧代次积压，但本次有效新鲜采样的真实边沿仍可入队。
      DiscardInputEdges();
      source.edge_session = session; source.edge_generation = generation;
    }
    if (state.edge_flags) {
      if (source.edge_count == source.edges.size()) {
        FailRuntime("physical_edge_queue_full"); return;
      }
      source.edges[(source.edge_head + source.edge_count) % source.edges.size()] =
          {state.edge_flags, source.serial};
      ++source.edge_count;
    }
    source.sample.edge_flags = state.edge_flags;
  }
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
  if (ClientBusy()) { ClientInputFinished(); return; }
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
  if (source.edge_session != SessionGeneration(r) || source.edge_generation != generation)
    DiscardInputEdges();
  const auto edge = source.edge_count ? source.edges[source.edge_head] : PendingEdge{};
  state.edge_flags = edge.mask;
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
#ifdef ISAACCS_E2E
    replica::stream::E2EInput("physical-input",{0,0,0,cmd.state.edge_flags,generation,cmd.sequence,cmd.target_tick,completed,{cmd.state.move_x,cmd.state.move_y,cmd.state.aim_x,cmd.state.aim_y}});
    if(result==core::Decision::Expired)replica::stream::E2EInput("input-expired",{0,0,0,0,generation,cmd.sequence,cmd.target_tick,completed,{}});
#endif
    Trace("submit",
          "ordinal=0 generation=%llu sample=%llu sequence=%llu target=%llu "
          "loop=%llu session_generation=%llu "
          "decision=%u edge=%u move=%d,%d aim=%d,%d held=%u edge_sample=%llu",
          generation, source.serial, cmd.sequence, cmd.target_tick, r.loop,
          r.session.Identity().generation,
          unsigned(result), cmd.state.edge_flags, cmd.state.move_x, cmd.state.move_y,
          cmd.state.aim_x, cmd.state.aim_y, cmd.state.held_flags, value.edge_flags ? edge.sample : 0);
    return result == core::Decision::Accepted ||
           result == core::Decision::Duplicate;
  };
  if (!submit(state, target))
    return false;
  // admission成功才消费一次；之后的新鲜连续样本和启动补帧不重放该边沿。
  if (source.edge_count) {
    source.edge_head = static_cast<unsigned>((source.edge_head + 1) % source.edges.size());
    --source.edge_count;
  }
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
