#pragma once
#include "core/participant.h"
#include "game/bridge.h"
#include "game/topology.h"

namespace cs::game {
struct SessionRuntime;
// None 表示退休完成后已复位；最近一次业务结果另存 JoinData::last。
enum class JoinState : unsigned { None, Waiting, Request, Confirm, Joined, Failed };
// 每个 pass 固定一个合成动作；只在正常 pass 返回后推进，不按查询次数消费。
struct JoinData {
  JoinState state = JoinState::Waiting;
  JoinState last = JoinState::None; // 最近一次 Join 的业务结果，退休后保留
  void *manager = nullptr;
  core::Binding host{};
  RoomIdentity room{};
  int character = 0, action = -1, host_character = -1;
  unsigned passes = 0, creation_calls = 0;
  uint64_t mode_calls = 0, baby_overrides = 0, query_count = 0;
  uint64_t last_loop = 0;
  bool scope = false, complete = false, remove_attempted = false;
  // 只保留首次 Join 的历史地址；退休使用当前已确认归属的只读快照。
  void *joined_player = nullptr;
};
const char *JoinStateName(JoinState state) noexcept;
bool JoinQuery(void *device, int action, QueryKind query,
               float &result) noexcept;
bool JoinMode(bool original, void *manager, int key, void *row, unsigned site) noexcept;
bool JoinCreate(void *manager, void *row, int *role) noexcept;
// 只按当前已确认归属调用一次原生移除；已确认被移除时跳过，身份矛盾时报告残留。
bool RetireJoinedPlayer(SessionRuntime &runtime) noexcept;
// 退休完成后复位 join_state，并把最终业务结果保存到 join_last。
void ResetJoinStateAfterRetirement(SessionRuntime &runtime) noexcept;
} // namespace cs::game
