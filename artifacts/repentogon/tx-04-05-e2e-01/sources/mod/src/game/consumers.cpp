#include "game/consumers.h"
#include "binding/offsets.h"
#include "game/context.h"
#include "game/lifecycle.h"
#include "game/state.h"
#include "game/tick.h"
#include <cmath>
#include <exception>

namespace cs::game {
bool BeginOddPlayer(void *player) noexcept;
namespace {
thread_local ConsumerScope *consumer = nullptr;
thread_local PlayerScope *outer_player = nullptr;
} // namespace
ConsumerScope *CurrentConsumer() noexcept { return consumer; }
bool ControlledPlayer(void *player) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || !player)
    return false;
  if (player == r->controlled_player || player == r->topology.pair.player)
    return true;
  // 停止后的在途回调仍可能替换 A；只识别列表首项，不给新对象开启物理输入旁路。
  return ReadTopology().pair.player == player;
}
ConsumerScope::ConsumerScope(void *value, ConsumerKind type) noexcept
    : player(value), kind(type), main_(IsMain()),
      exceptions_(std::uncaught_exceptions()) {
  if (!main_)
    return;
  previous = consumer;
  consumer = this;
  if (!Armed())
    return;
  auto &r = *Runtime();
  RuntimeHealthy();
  RefreshLifecycle(r, false);
  if (!ControlledPlayer(player))
    return;
  const auto tick = r.pipeline.SimulationTick();
  if (tick && unsigned(kind) > 0 && unsigned(kind) <= 6)
    ++r.metrics.consumers[unsigned(kind) - 1];
  if (kind == ConsumerKind::Shooting) {
    call_original_ = tick && !r.blocks && r.players[0].BoundValid() &&
                     r.players[0].Bound().player == player &&
                     ContextHealthy() && !InBlockedContext() && !r.failure;
    if (!call_original_) {
      ++r.suppressed;
      if (tick)
        ++r.metrics.shooting_suppressed;
    }
    // 每次分派记录决定，harness 可核对无 tick 时没有原调用，不能用 wrapper 计数替代。
    Trace("shooting_dispatch",
          "ordinal=0 original=%u tick=%llu blocks=%u loop=%llu", call_original_, tick,
          r.blocks, r.loop);
    r.last_dispatch_suppressed = !call_original_;
  }
}
ConsumerScope::~ConsumerScope() noexcept {
  if (!main_)
    return;
  if (consumer != this)
    ContextFault("consumer_scope_mismatch");
  consumer = previous;
  if (exceptions_ != std::uncaught_exceptions())
    FailRuntime("consumer_exception");
}
void RecordQuery(ConsumerKind type, QueryKind query, int action, float result) noexcept {
  auto *r = Runtime();
  if (!r || !r->pipeline.SimulationTick() || unsigned(type) < 1 ||
      unsigned(type) > 6 || unsigned(query) < 1 || unsigned(query) > 3)
    return;
  ++r->metrics.reads[unsigned(type) - 1][unsigned(query) - 1];
  if (result != 0)
    ++r->metrics.nonzero[unsigned(type) - 1][unsigned(query) - 1];
  if (type == ConsumerKind::Bomb && query == QueryKind::Triggered && action == 8) {
    auto &m = r->metrics;
    if (!m.bomb_queries)
      m.first_bomb_query = result;
    else if (m.first_bomb_query != result)
      ++m.bomb_query_mismatches;
    ++m.bomb_queries;
  }
}
PlayerScope::PlayerScope(void *value) noexcept
    : player(value), main(IsMain()), exceptions(std::uncaught_exceptions()) {
  if (!main)
    return;
  previous = outer_player;
  outer_player = this;
  if (!Armed())
    return;
  RecordOddPlayerEntry();
  BeginOddPlayer(player);
  auto &r = *Runtime();
  controlled = ControlledPlayer(player);
  tick = r.pipeline.SimulationTick();
  generation = r.players[0].Generation();
  if (controlled && tick)
    ++r.metrics.player_entered;
}
void PlayerNativeReached(void *player) noexcept {
  if (!IsMain() || !outer_player || outer_player->player != player)
    return;
  if (outer_player->native_reached) {
    FailRuntime("player_native_reentry");
    return;
  }
  outer_player->native_reached = true;
  if (auto *r = Runtime(); r && outer_player->controlled &&
                           outer_player->tick &&
                           r->pipeline.SimulationTick() == outer_player->tick)
    ++r->metrics.native_reached;
}
PlayerScope::~PlayerScope() noexcept {
  if (!main)
    return;
  const bool normal = exceptions == std::uncaught_exceptions();
  if (outer_player != this)
    ContextFault("player_scope_mismatch");
  outer_player = previous;
  auto *r = Runtime();
  if (!r || r->finished)
    return;
  if (!normal) {
    FailRuntime("player_update_exception");
    return;
  }
  if (controlled && tick && r->pipeline.SimulationTick() == tick) {
    if (!native_reached)
      ++r->metrics.chain_skipped;
    else if (r->players[0].BoundValid() &&
             r->players[0].Generation() == generation &&
             r->players[0].Bound().player == player) {
      auto &m = r->metrics;
      m.player_fields_known =
          ReadAt(player, offset::MoveDirection, m.move_direction) &&
          ReadAt(player, offset::VelocityX, m.velocity_x) &&
          ReadAt(player, offset::VelocityY, m.velocity_y) &&
          ReadAt(player, offset::ShootX, m.shoot_x) &&
          ReadAt(player, offset::ShootY, m.shoot_y) &&
          ReadAt(player, offset::ShootFlag, m.shoot_flag) &&
          std::isfinite(m.velocity_x) && std::isfinite(m.velocity_y) &&
          std::isfinite(m.shoot_x) && std::isfinite(m.shoot_y);
    }
  }
  RefreshLifecycle(*r, false);
}
} // namespace cs::game
