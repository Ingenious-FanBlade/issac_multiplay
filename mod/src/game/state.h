#pragma once
#include "core/tick_pipeline.h"
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
// 主线程单 owner，在堆上稳定存储；B 的存储不初始化、不参与 tick。
struct SoloRuntime {
  core::Session session;
  core::TickClock clock;
  std::array<core::Participant, 2> players;
  core::TickPipeline pipeline{session, clock, players};
  core::TickBoundary boundary{};
  core::InputStateDraftV2 tick_input{};
  TickMetrics metrics{};
  Topology topology{};
  void *controlled_player = nullptr;
  uint64_t loop = 0, defer_through = 0, odd_scope = 0;
  uint64_t ticks_standard = 0, ticks_odd = 0, overrides = 0, suppressed = 0;
  unsigned blocks = 0;
  bool observed = false, console_blocked = false, odd_started = false;
  bool retiring = false, failure = false, finished = false,
       last_dispatch_suppressed = false;
  core::TickDecision last_decision = core::TickDecision::Idle;
  const char *last_error = "";
};
} // namespace cs::game
