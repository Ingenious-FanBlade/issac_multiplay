#include "game/consumers.h"
#include "binding/offsets.h"
#include "game/context.h"
#include "game/lifecycle.h"
#include "game/state.h"
#include "game/tick.h"
#include "game/world_bridge.h"
#include <cmath>
#include <exception>

namespace cs::game {
bool BeginOddPlayer(void *player) noexcept;
namespace {
thread_local ConsumerScope *consumer = nullptr;
thread_local PlayerScope *outer_player = nullptr;
} // namespace
ConsumerScope *CurrentConsumer() noexcept { return consumer; }
// 同一函数按冻结 ordinal 识别 A/B；旧关联只作中性拦截，不成为新的绑定依据。
int ControlledOrdinal(void *player) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || !player)
    return -1;
  const unsigned count = r->session.ParticipantLimit();
  int found = -1;
  for (unsigned i = 0; i < count; ++i) {
    // 历史集合只拦截中性查询，不赋予当前资格。
    if (player == r->controlled_player[i] ||
        r->topology[i].Contains(player, false)) {
      // RefreshLifecycle已撤销冲突组资格；保留一个拦截归属，避免退回物理输入。
      if (found >= 0 && found != int(i))
        return found;
      found = int(i);
    }
  }
  if (found >= 0)
    return found;
  for (unsigned i = 0; i < count; ++i) {
    if (!r->session.HasParticipant(i))
      continue;
    const auto observed =
        ReadTopology(i, i ? r->Remote(i).device.Object() : nullptr);
    if (observed.Contains(player, false))
      return int(i);
  }
  return -1;
}
bool ControlledPlayer(void *player) noexcept {
  return ControlledOrdinal(player) >= 0;
}
ConsumerScope::ConsumerScope(void *value, ConsumerKind type) noexcept
    : player(value), kind(type), lease_(HostBusy()), main_(IsMain()),
      exceptions_(std::uncaught_exceptions()) {
  if (!main_)
    return;
  previous = consumer;
  consumer = this;
  if (!Armed())
    return;
  // 后端锁先于原生 consumer 内部 InputManager 的 vector 锁取得。
  if (!lease_.Ready())
    InputLeaseUnavailable();
  auto &r = *Runtime();
  RuntimeHealthy();
  RefreshLifecycle(r, false, player);
  const auto ordinal = ControlledOrdinal(player);
  if (ordinal < 0)
    return;
  const auto tick = r.pipeline.SimulationTick();
  if (tick && outer_player && outer_player->player == player)
    outer_player->consumer_mask |= 1u << unsigned(kind);
  if (tick && unsigned(kind) > 0 && unsigned(kind) <= 6)
    ++r.metrics[ordinal].consumers[unsigned(kind) - 1];
  if (kind == ConsumerKind::Shooting) {
    call_original_ = tick && !r.blocks && r.players[ordinal].BoundValid() &&
                     r.topology[ordinal].Contains(player) && ContextHealthy() &&
                     !InBlockedContext() && !r.failure;
    // 新代次尚无可消费命令时，短暂暂停 Shooting，避免把握手空档变成武器松键。
    // Authority 不等待远端；首条命令（包括真实中性）或六 tick 上限均结束例外。
    const auto &recovery = r.shooting_recovery[ordinal];
    const bool startup_deferred =
        call_original_ &&
        recovery.generation == r.players[ordinal].Generation() &&
        !recovery.source_seen && !r.players[ordinal].CurrentSequence() &&
        tick >= recovery.first_tick &&
        tick - recovery.first_tick < kShootingRecoveryTicks;
    if (startup_deferred) {
      call_original_ = false;
      ++r.metrics[ordinal].shooting_startup_deferred;
    }
    if (!call_original_) {
      ++r.suppressed;
      if (tick)
        ++r.metrics[ordinal].shooting_suppressed;
    }
    // 每次分派记录决定，harness 可核对无 tick 时没有原调用，不能用 wrapper
    // 计数替代。session/participant generation、owner token 与 room/加载窗口
    // 分开记录，避免把内部 Player 更新当成 Authority tick。
    const auto owner = TopOwner();
    Trace(
        "shooting_dispatch",
        "ordinal=%u original=%u tick=%llu blocks=%u loop=%llu generation=%llu "
        "session_generation=%llu owner=%u owner_token=%llu room=%p index=%u "
        "loading_episode=%llu participant_generation=%llu startup_deferred=%u "
        "recovery_first_tick=%llu recovery_limit=%llu input_sequence=%llu "
        "recovery_source_seen=%u",
        unsigned(ordinal), call_original_, tick, r.blocks, r.loop,
        r.players[ordinal].Generation(), r.session.Identity().generation,
        unsigned(owner), OwnerToken(owner), r.topology[ordinal].room.room,
        r.topology[ordinal].room.index, r.loading.active ? r.loading.id : 0,
        r.players[ordinal].Generation(), startup_deferred, recovery.first_tick,
        kShootingRecoveryTicks, r.players[ordinal].CurrentSequence(),
        recovery.source_seen);
    r.last_dispatch_suppressed[ordinal] = !call_original_;
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
// 完整键比较：ordinal/tick/participant generation/资格段/consumer/类型/action
// 全部相同才允许把两次原生读取当作同一快照。无 tick 的查询只进加载窗口汇总。
void RecordQuery(unsigned ordinal, ConsumerKind type, QueryKind query,
                 int action, float result) noexcept {
  if (!Settings.diagnostics)
    return;
  auto *r = Runtime();
  if (!r || !r->session.HasParticipant(ordinal) || unsigned(type) < 1 ||
      unsigned(type) > 6 || unsigned(query) < 1 || unsigned(query) > 3 ||
      action < 0 || action >= int(kQueryActionCount))
    return;
  const auto tick = r->pipeline.SimulationTick();
  if (!tick) {
    // 加载窗口内没有 tick：只累计中性与上下文，不写 current/previous。
    RecordLoadingQuery(result != 0, true);
    return;
  }
  auto &m = r->metrics[ordinal];
  const unsigned consumer_index = unsigned(type) - 1;
  const unsigned query_index = unsigned(query) - 1;
  ++m.reads[consumer_index][query_index];
  if (result != 0)
    ++m.nonzero[consumer_index][query_index];
  // 原 bomb 字段保留为派生诊断；比较采用下面的完整键，避免跨失效段误报。
  const bool bomb = type == ConsumerKind::Bomb &&
                    query == QueryKind::Triggered && action == 8;
  if (bomb)
    ++m.bomb_queries;
  ++m.query_reads;
  m.query_read_mask |= 1u << action;
  ++r->query.reads;
  auto &coverage =
      r->query.coverage[ordinal][consumer_index][unsigned(action)][query_index];
  ++coverage.reads;
  // 非有限结果单列诊断，不作为首次有限样本，也不把 NaN 与 NaN 比较成重复值。
  if (!std::isfinite(result)) {
    ++coverage.nonfinite;
    ++r->query.nonfinite;
    return;
  }
  // 分桶只覆盖当前 tick：新 tick 开始时释放旧键，场景累计继续保留。
  if (r->query_bucket_tick != tick) {
    for (auto &bucket : r->query_buckets)
      bucket = {};
    r->query_bucket_tick = tick;
  }
  QueryKey key;
  key.tick = tick;
  key.generation = r->players[ordinal].Generation();
  key.ordinal = ordinal;
  key.consumer = unsigned(type);
  key.query = unsigned(query);
  key.action = unsigned(action);
  key.epoch = r->query_epoch;
  QueryRepeatBucket *bucket = nullptr;
  for (auto &candidate : r->query_buckets) {
    if (candidate.used && candidate.key == key) {
      bucket = &candidate;
      break;
    }
  }
  if (!bucket) {
    for (auto &candidate : r->query_buckets) {
      if (!candidate.used) {
        bucket = &candidate;
        break;
      }
    }
    if (!bucket) {
      // 容量不足时只记录溢出，不扩大内存、不改变 query 结果。
      ++r->query.overflow;
      return;
    }
    bucket->used = true;
    bucket->key = key;
    bucket->first = result;
    if (bomb)
      m.first_bomb_query = result;
    bucket->reads = 1;
    ++m.query_fresh_buckets;
    return;
  }
  ++bucket->reads;
  ++bucket->repeats;
  ++r->query.repeats;
  ++m.query_repeats;
  m.query_repeat_mask |= 1u << action;
  ++coverage.repeats;
  if (bucket->first == result)
    return;
  // 不一致只计一次与该键；首个 mismatch 记录准确键与值，其余只累计。
  ++bucket->mismatches;
  ++r->query.mismatches;
  ++coverage.mismatches;
  ++m.query_mismatches;
  if (bomb)
    ++m.bomb_query_mismatches;
  m.query_mismatch_mask |= 1u << action;
  if (!r->query.mismatch_known) {
    r->query.mismatch_known = true;
    r->query.mismatch_key = key;
    r->query.mismatch_first = bucket->first;
    r->query.mismatch_last = result;
    Trace("query_repeat",
          "event=mismatch ordinal=%u consumer=%u query=%u action=%d tick=%llu "
          "generation=%llu epoch=%llu first=%g last=%g loop=%llu "
          "session_generation=%llu participant_generation=%llu",
          key.ordinal, key.consumer, key.query, action, key.tick,
          key.generation, key.epoch, bucket->first, result, r->loop,
          SessionGeneration(*r), key.generation);
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
  ordinal = ControlledOrdinal(player);
  controlled = ordinal >= 0;
  tick = r.pipeline.SimulationTick();
  generation = controlled ? r.players[ordinal].Generation() : 0;
  if (controlled) {
    const auto before = WorldPlayerAnchor(unsigned(ordinal), player);
    world_device = reinterpret_cast<void *>(uintptr_t(before.device));
    world_key = before.key;
    world_session = before.session;
    world_valid = before.valid;
  }
  if (controlled && tick)
    ++r.metrics[ordinal].player_entered;
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
    ++r->metrics[outer_player->ordinal].native_reached;
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
      ++r->metrics[ordinal].chain_skipped;
    else if (Settings.diagnostics && r->players[ordinal].BoundValid() &&
             r->players[ordinal].Generation() == generation &&
             r->players[ordinal].Bound().player == player) {
      auto &m = r->metrics[ordinal];
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
  if (controlled && native_reached && tick && consumer_mask && !r->blocks &&
      !r->control_lifecycle_depth && r->players[ordinal].BoundValid() &&
      r->players[ordinal].Generation() == generation &&
      r->pipeline.SimulationTick() == tick) {
    // 只有本身体实际进入原生更新/consumer并正常返回才作活动见证，备份关系不因此变成输入身体。
    for (unsigned b = 0; b < r->topology[ordinal].body_count; ++b) {
      const auto &body = r->topology[ordinal].bodies[b];
      if (body.player != player || !body.native_activity_required)
        continue;
      r->body_activity[ordinal][b] = {player, generation, tick};
      Trace("body_activity",
            "ordinal=%u player=%p generation=%llu tick=%llu consumers=%u",
            unsigned(ordinal), player, generation, tick, consumer_mask);
    }
  }
  RefreshLifecycle(*r, false, player);
  if (controlled) {
    const world::PlayerAnchor before{
        static_cast<std::uint32_t>(uintptr_t(player)),
        static_cast<std::uint32_t>(uintptr_t(world_device)),
        world_key,
        world_session,
        generation,
        world_valid};
    WorldPlayerReturned(unsigned(ordinal), before, tick, normal,
                        native_reached);
  }
}
} // namespace cs::game
