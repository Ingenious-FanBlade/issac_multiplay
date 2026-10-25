#pragma once
#include "core/participant.h"
#include "game/bridge.h"
#include "game/topology.h"

namespace cs::game {
struct SessionRuntime;
enum class JoinState : unsigned { Waiting, Request, Confirm, Joined, Failed };
// 每个 pass 固定一个合成动作；只在正常 pass 返回后推进，不按查询次数消费。
struct JoinData {
  JoinState state = JoinState::Waiting;
  void *manager = nullptr;
  core::Binding host{};
  RoomIdentity room{};
  int character = 0, action = -1, host_character = -1;
  unsigned passes = 0, creation_calls = 0;
  uint64_t mode_calls = 0, baby_overrides = 0, query_count = 0;
  bool scope = false, complete = false, remove_attempted = false;
  void *joined_player = nullptr;
};
const char *JoinStateName(JoinState state) noexcept;
bool JoinQuery(void *device, int action, QueryKind query,
               float &result) noexcept;
bool JoinMode(bool original, void *manager, int key, void *row, unsigned site) noexcept;
bool JoinCreate(void *manager, void *row, int *role) noexcept;
bool RetireJoinedPlayer(SessionRuntime &runtime) noexcept;
} // namespace cs::game
