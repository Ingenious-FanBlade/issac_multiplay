#pragma once
#include "core/tick_pipeline.h"
#include "game/participants/device_k.h"
#include "game/participants/join.h"
#include "game/authority/lifecycle.h"
#include "game/authority/lobby.h"
#include "game/participants/peer.h"
#include "game/input/query.h"
#include "game/participants/remote_life.h"
#include "game/participants/topology.h"

namespace cs::game {
// 用户采纳的重绑射击恢复窗口：最多六个有效 Authority tick，不缓存或补写旧
// held。
inline constexpr uint64_t kShootingRecoveryTicks = 6;
struct ShootingRecovery {
  uint64_t generation = 0, first_tick = 0;
  bool source_seen =
      false; // 首条可消费的新命令（包括中性）出现后，本代次永久退出窗口。
};
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
  uint32_t query_read_mask = 0, query_repeat_mask = 0, query_mismatch_mask = 0;
  uint64_t player_entered = 0, native_reached = 0, chain_skipped = 0,
           shooting_suppressed = 0, shooting_startup_deferred = 0;
  int move_direction = -1;
  float velocity_x = 0, velocity_y = 0, shoot_x = 0, shoot_y = 0;
  unsigned char shoot_flag = 0;
  bool player_fields_known = false;
};
// 非roster普通Forgotten关联体只在本代次实际更新并进入consumer后取得输入活动资格。
struct BodyActivityWitness {
  void *player = nullptr;
  uint64_t generation = 0, tick = 0;
};
// 每个远端独立拥有一次性 K、Join、生命记录和收发进度；单 Peer
// 退休不关闭其他人。
struct RemoteRuntime {
  PeerBridge peer;
  DeviceK device;
  JoinData join;
  RemoteLifeData life;
  core::SessionCause retirement_cause = core::SessionCause::None;
  bool retiring = false, retired = false, cleanup_failed = false;
};
// 主线程单 owner；参与者槽与身体集合分开，网络线程只持有值队列。
struct SessionRuntime {
  core::Session session;
  core::TickClock clock;
  std::array<core::Participant, core::kMaximumParticipants> players;
  core::TickPipeline pipeline{session, clock, players};
  core::TickBoundary boundary{};
  std::uint64_t committed_sample_ms=0;
  unsigned committed_kind=0;
  std::array<core::InputStateDraftV2, core::kMaximumParticipants> tick_input{};
  std::array<TickMetrics, core::kMaximumParticipants> metrics{};
  // 只记录恢复时序；不保存旧输入，不读取或写入武器蓄力，A/B 使用相同上限。
  std::array<ShootingRecovery, core::kMaximumParticipants> shooting_recovery{};
  TopologyGroups topology{};
  std::array<void *, core::kMaximumParticipants> controlled_player{};
  std::array<std::array<BodyActivityWitness, kMaximumGroupBodies>,
             core::kMaximumParticipants>
      body_activity{};
  core::SessionMode mode = core::SessionMode::None;
  net::Worker network;
  HostLobby lobby;
  std::array<RemoteRuntime, core::kMaximumParticipants - 1> remotes{};
  // 旧 a/b 诊断字段保留为首个远端的兼容视图；控制逻辑必须按 ordinal 访问。
  PeerBridge &peer = remotes[0].peer;
  DeviceK &device = remotes[0].device;
  JoinData &join = remotes[0].join;
  RemoteLifeData &life = remotes[0].life;
  RemoteRuntime &Remote(unsigned ordinal) noexcept {
    return remotes[ordinal - 1];
  }
  const RemoteRuntime &Remote(unsigned ordinal) const noexcept {
    return remotes[ordinal - 1];
  }
  unsigned join_ordinal =
      0; // 一个原生 pass 只驱动一个 Join，不并发控制原生菜单行。
  bool core_started = false;
  unsigned control_lifecycle_depth =
      0; // 构造/Init/Restore原生栈未返回时禁止重新绑定。
  std::array<bool, core::kMaximumParticipants> control_quarantined{};
  std::array<uint64_t, core::kMaximumParticipants> binding_defer_through{};
  // 查询重复读取诊断：固定容量分桶 + 场景累计，均只作 trace/evidence。
  std::array<QueryRepeatBucket, kQueryRepeatCapacity> query_buckets{};
  QueryRepeatStats query{};
  LoadingEpisode loading{};
  core::SessionCause retirement_cause = core::SessionCause::None;
  uint64_t loop = 0, defer_through = 0, odd_scope = 0;
  uint64_t ticks_standard = 0, ticks_odd = 0, overrides = 0, suppressed = 0;
  // 资格变化段编号：任何绑定失效/重绑都递增，避免跨段比较查询结果。
  uint64_t query_epoch = 1;
  // 快照仅复用在同一Manager轮次和资格段；生命周期变化立即失效。
  uint64_t lifecycle_loop = 0, lifecycle_epoch = 0;
  uint64_t lifecycle_refreshes = 0, lifecycle_reuses = 0;
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
  std::array<bool, core::kMaximumParticipants> last_dispatch_suppressed{};
  core::TickDecision last_decision = core::TickDecision::Idle;
  const char *last_error = "";
};
// Hosting 时 core 尚无 identity，诊断仍使用本轮已分配的 session generation。
inline uint64_t SessionGeneration(const SessionRuntime &r) noexcept {
  return r.mode == core::SessionMode::Host ? r.peer.session_generation
                                           : r.session.Identity().generation;
}
} // namespace cs::game
