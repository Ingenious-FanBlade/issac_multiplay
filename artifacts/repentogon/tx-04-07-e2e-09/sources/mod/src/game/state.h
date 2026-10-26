#pragma once
#include "core/tick_pipeline.h"
#include "game/device_k.h"
#include "game/join.h"
#include "game/lifecycle.h"
#include "game/peer.h"
#include "game/query.h"
#include "game/remote_life.h"
#include "game/topology.h"

namespace cs::game {
// 每 tick 汇总查询及实际 Player 调用链，不用 consumer 缺席推断 Lua 跳过。
struct TickMetrics {
  // E2E 逐窗口顺序及炸弹查询幂等性证据，不参与输入判定。
  uint64_t authority_pre = 0, native_commit = 0, bomb_queries = 0,
           bomb_query_mismatches = 0;
  uint64_t input_sequence = 0;
  // 保存 Authority-pre 的来源代次，tick 中途失效不能改写已合法发生的输入归属。
  uint64_t input_generation = 0, input_epoch = 0;
  float first_bomb_query = 0;
  std::array<std::array<uint64_t, 3>, 6> reads{}, nonzero{};
  std::array<uint64_t, 6> consumers{};
  // tx-04-07：本 tick 的完整键重复读取汇总；只诊断，不 Submit/Resolve。
  uint64_t query_reads = 0, query_repeats = 0, query_mismatches = 0,
           query_fresh_buckets = 0;
  uint32_t query_read_mask = 0, query_repeat_mask = 0,
           query_mismatch_mask = 0;
  uint64_t player_entered = 0, native_reached = 0, chain_skipped = 0,
           shooting_suppressed = 0;
  int move_direction = -1;
  float velocity_x = 0, velocity_y = 0, shoot_x = 0, shoot_y = 0;
  unsigned char shoot_flag = 0;
  bool player_fields_known = false;
};
// 主线程单 owner，在堆上稳定存储；Solo 仅初始化 A，Host 冻结两个 ordinal。
struct SessionRuntime {
  core::Session session;
  core::TickClock clock;
  std::array<core::Participant, 2> players;
  core::TickPipeline pipeline{session, clock, players};
  core::TickBoundary boundary{};
  std::array<core::InputStateDraftV2, 2> tick_input{};
  std::array<TickMetrics, 2> metrics{};
  std::array<Topology, 2> topology{};
  std::array<void *, 2> controlled_player{};
  core::SessionMode mode = core::SessionMode::None;
  PeerBridge peer;
  DeviceK device;
  JoinData join;
  // B 的生命状态与当前确认归属；死亡只使 B 失效，Active 的 Authority 继续。
  RemoteLifeData life;
  // 查询重复读取诊断：固定容量分桶 + 场景累计，均只作 trace/evidence。
  std::array<QueryRepeatBucket, kQueryRepeatCapacity> query_buckets{};
  QueryRepeatStats query{};
  LoadingEpisode loading{};
  core::SessionCause retirement_cause = core::SessionCause::None;
  uint64_t loop = 0, defer_through = 0, odd_scope = 0;
  uint64_t ticks_standard = 0, ticks_odd = 0, overrides = 0, suppressed = 0;
  // 资格变化段编号：任何绑定失效/重绑都递增，避免跨段比较查询结果。
  uint64_t query_epoch = 1;
  // 分桶只覆盖当前 tick；新 tick 开始时释放旧键，场景累计仍保留。
  uint64_t query_bucket_tick = 0;
  uint64_t query_snapshots = 0; // 场景/会话覆盖快照编号，只增诊断值
  // 分键覆盖只在会话结束时输出一次，避免重复事件。
  bool query_coverage_published = false;
  uint64_t loading_episodes = 0;
  unsigned blocks = 0;
  bool observed = false, console_blocked = false, odd_started = false;
  bool retiring = false, failure = false, finished = false,
       run_torn_down = false, cleanup_failed = false, run_observed = false;
  // 首次菜单加载资格：在外层 Start scope 进入时固定，同一加载窗口内不反复变化。
  bool start_qualified = false;
  std::array<bool, 2> last_dispatch_suppressed{};
  core::TickDecision last_decision = core::TickDecision::Idle;
  const char *last_error = "";
};
// Hosting 时 core 尚无 identity，诊断仍使用本轮已分配的 session generation。
inline uint64_t SessionGeneration(const SessionRuntime &r) noexcept {
  return r.mode == core::SessionMode::Host ? r.peer.session_generation
                                          : r.session.Identity().generation;
}
} // namespace cs::game
