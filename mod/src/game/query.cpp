#include "game/query.h"
#include "binding/offsets.h"
#include "game/consumers.h"
#include "game/context.h"
#include "game/state.h"
#include "game/tick.h"

namespace cs::game {
// 返回 true 表示本次属于受控 A gameplay 查询，即使资格不足也必须输出中性。
bool OverrideQuery(void *device, int action, QueryKind kind,
                   float &result) noexcept {
  if (!IsMain() || !Armed() || action < 0 || action >= 12)
    return false;
  auto *context = CurrentConsumer();
  if (!context || !ControlledPlayer(context->player))
    return false;
  auto &r = *Runtime();
  RuntimeHealthy();
  ++r.overrides;
  result = 0;
  const auto tick = r.pipeline.SimulationTick();
  auto &p = r.players[0];
  int device_id = -1, controller = -1;
  if (tick && !r.blocks && !r.failure && ContextHealthy() &&
      !InBlockedContext() && p.BoundValid() &&
      p.Bound().player == context->player && p.Bound().device == device &&
      ReadAt(device, offset::DeviceId, device_id) &&
      ReadAt(context->player, offset::ControllerIndex, controller) &&
      device_id == p.Bound().key && controller == p.Bound().key) {
    result =
        p.Read(unsigned(action), static_cast<core::Query>(kind),
               static_cast<core::Consumer>(context->kind), p.Bound(), tick);
  }
  RecordQuery(context->kind, kind, action, result);
  return true;
}
} // namespace cs::game
