#pragma once
#include "core/tick_pipeline.h"
#include "game/device_k.h"
#include "game/join.h"
#include "game/peer.h"
#include "game/topology.h"

namespace cs::game {
// 每 tick 汇总查询及实际 Player 调用链，不用 consumer 缺席推断 Lua 跳过。
struct TickMetrics {
  // E2E 逐窗口顺序及炸弹查询幂等性证据，不参与输入判定。
  uint64_t authority_pre = 0, native_commit = 0, bomb_queries = 0,
           bomb_query_mismatches = 0;
  uint64_t input_sequence = 0;
  float first_bomb_query = 0;
  std::array<std::array<uint64_t, 3>, 6> reads{}, nonzero{};
  std::array<uint64_t, 6> consumers{};
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
  core::SessionCause retirement_cause = core::SessionCause::None;
  uint64_t loop = 0, defer_through = 0, odd_scope = 0;
  uint64_t ticks_standard = 0, ticks_odd = 0, overrides = 0, suppressed = 0;
  unsigned blocks = 0;
  bool observed = false, console_blocked = false, odd_started = false;
  bool retiring = false, failure = false, finished = false,
       run_torn_down = false, cleanup_failed = false, remote_life_lost = false,
       run_observed = false;
  std::array<bool, 2> last_dispatch_suppressed{};
  core::TickDecision last_decision = core::TickDecision::Idle;
  const char *last_error = "";
};
} // namespace cs::game
