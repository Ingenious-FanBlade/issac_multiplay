#include "game/input/query.h"
#include "binding/offsets.h"
#include "game/input/consumers.h"
#include "game/common/context.h"
#include "game/authority/lifecycle.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include <cstddef>
#include <cstdio>
#include <cmath>

namespace cs::game {
// 返回 true 表示本次属于受控 A/B gameplay 查询，即使资格不足也必须输出中性。
bool OverrideQuery(void *device, int action, QueryKind kind,
                   float &result) noexcept {
  if (!IsMain() || !Armed() || action < 0 || action >= 12)
    return false;
  auto *runtime = Runtime();
  if (!runtime)
    return false;
  auto *context = CurrentConsumer();
  const auto ordinal = context ? ControlledOrdinal(context->player) : -1;
  if (ordinal < 0) {
    // 没有受控consumer时保持原值；输入参数已是物理wrapper的original结果，或K的中性回退值。
    // 上下文仍未知，但返回值可直接观察，不能把默认false当作已证明的中性。
    if (!runtime->pipeline.SimulationTick() && runtime->loading.active) {
      const bool finite = std::isfinite(result);
      RecordLoadingQuery(!finite || result != 0, false);
      Trace("loading_unscoped_query",
            "episode=%llu session_generation=%llu loop=%llu action=%d kind=%u "
            "consumer_present=%u context_known=0 overridden=0 value=%g finite=%u zero=%u",
            runtime->loading.id, SessionGeneration(*runtime), runtime->loop,
            action, unsigned(kind), unsigned(context != nullptr), result,
            unsigned(finite), unsigned(finite && result == 0));
    }
    return false;
  }
  auto &r = *runtime;
  RuntimeHealthy();
  ++r.overrides;
  result = 0;
  const auto tick = r.pipeline.SimulationTick();
  auto &p = r.players[ordinal];
  int device_id = -1, controller = -1;
  if (tick && !r.blocks && !r.failure && ContextHealthy() &&
      !InBlockedContext() && p.BoundValid() &&
      r.topology[ordinal].Contains(context->player) && p.Bound().device == device &&
      ReadAt(device, offset::DeviceId, device_id) &&
      ReadAt(context->player, offset::ControllerIndex, controller) &&
      device_id == p.Bound().key && controller == p.Bound().key) {
    result =
        p.Read(unsigned(action), static_cast<core::Query>(kind),
               static_cast<core::Consumer>(context->kind), p.Bound(), tick);
  }
  RecordQuery(unsigned(ordinal), context->kind, kind, action, result);
  return true;
}
// 会话累计按 ordinal/consumer 分行输出，零值也保留。每行最多 36 个键，
// 三项 uint64 计数的最坏文本长度小于 2800，连同元数据不超过 trace 上限。
void PublishQueryCoverage(SessionRuntime &r, const char *checkpoint) noexcept {
  if (!checkpoint && r.query_coverage_published)
    return;
  if (!checkpoint)
    r.query_coverage_published = true;
  const auto snapshot = ++r.query_snapshots;
  const auto *scope = checkpoint ? "checkpoint" : "session";
  const auto *label = checkpoint ? checkpoint : "session_end";
  const unsigned count = r.session.ParticipantLimit();
  for (unsigned ordinal = 0; ordinal < count; ++ordinal) {
    for (unsigned consumer = 0; consumer < kQueryConsumerCount; ++consumer) {
      char keys[2800]{};
      std::size_t used = 0;
      unsigned observed = 0, repeated = 0;
      uint64_t nonfinite = 0;
      bool complete = true;
      for (unsigned action = 0; action < kQueryActionCount; ++action) {
        for (unsigned kind = 0; kind < kQueryKindCount; ++kind) {
          const auto &value = r.query.coverage[ordinal][consumer][action][kind];
          observed += value.reads != 0;
          repeated += value.repeats != 0;
          nonfinite += value.nonfinite;
          const int n = _snprintf_s(keys + used, sizeof(keys) - used, _TRUNCATE,
                                   "%sa%u.k%u=%llu/%llu/%llu",
                                   used ? "," : "", action, kind + 1,
                                   value.reads, value.repeats, value.mismatches);
          if (n < 0) {
            complete = false;
            break;
          }
          used += std::size_t(n);
        }
        if (!complete)
          break;
      }
      Trace("query_coverage",
            "session_generation=%llu ordinal=%u consumer=%u loop=%llu "
            "scope=%s checkpoint=%s snapshot=%llu observed=%u repeated=%u complete=%u nonfinite=%llu "
            "values=reads/repeats/mismatches keys=%s",
            SessionGeneration(r), ordinal, consumer + 1, r.loop, scope, label, snapshot, observed,
            repeated, unsigned(complete), nonfinite, keys);
    }
  }
  Trace("query_summary",
        "session_generation=%llu scope=%s checkpoint=%s snapshot=%llu reads=%llu repeats=%llu mismatches=%llu "
        "overflow=%llu nonfinite=%llu epoch=%llu rows=%u loop=%llu "
        "mismatch_known=%u mismatch_ordinal=%u mismatch_consumer=%u "
        "mismatch_query=%u mismatch_action=%u mismatch_tick=%llu "
        "mismatch_generation=%llu mismatch_epoch=%llu first=%g last=%g",
        SessionGeneration(r), scope, label, snapshot, r.query.reads, r.query.repeats, r.query.mismatches,
        r.query.overflow, r.query.nonfinite, r.query_epoch,
        count * kQueryConsumerCount, r.loop, unsigned(r.query.mismatch_known),
        r.query.mismatch_key.ordinal, r.query.mismatch_key.consumer,
        r.query.mismatch_key.query, r.query.mismatch_key.action,
        r.query.mismatch_key.tick, r.query.mismatch_key.generation,
        r.query.mismatch_key.epoch, r.query.mismatch_first, r.query.mismatch_last);
}
} // namespace cs::game
