#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace cs::game {
struct SessionRuntime;
enum Block : unsigned { Pause = 1, Loading = 2, Lifecycle = 4 };
// 加载相关 owner 位：Start/Restore/Lifecycle 都覆盖在加载窗口内。
enum OwnerEvent : unsigned {
  StartOwner = 1,
  RestoreOwner = 2,
  LifecycleOwner = 4
};
// 加载窗口：覆盖 starting/room_transition/stage_transition 标志及其
// Start/Restore/Lifecycle owner。只记录值，不驱动任何原生调用，也不把
// 内部 Room/Player 更新当成 Authority tick。
struct LoadingEpisode {
  std::uint64_t id = 0; // 0 表示当前没有加载窗口
  std::uint64_t start_loop = 0, end_loop = 0;
  std::uint64_t start_completed = 0, end_completed = 0;
  std::uint64_t overrides_at_start = 0, overrides_at_end = 0;
  // admission 计数只作对照：加载窗口内不应有 Submit/Resolve 推进。
  std::uint64_t admitted_at_start = 0, admitted_at_end = 0;
  std::array<std::uint64_t, 4> queue_at_start{}, generation_at_start{};
  // 结束快照冻结；加载中的 status 另显示当前值，不用默认 0 冒充结束值。
  std::array<std::uint64_t, 4> queue_at_end{}, generation_at_end{};
  std::array<std::uint64_t, 4> submit_at_start{}, submit_at_end{},
      resolved_tick_at_start{}, resolved_tick_at_end{};
  std::uint64_t invalidations = 0;
  std::uint64_t queries = 0, queries_nonzero = 0, queries_unknown = 0;
  unsigned reasons = 0; // 进入时的 starting/room_transition/stage_transition
  unsigned owners = 0;  // 窗口内进入过的加载相关 owner 位
  const char *reason = "loading_enter"; // 进入窗口的具体来源
  const char *end_reason = "open";      // 退休收尾与稳定结束分别保留真实原因
  bool active = false, first_menu = false;
};
void RefreshLifecycle(SessionRuntime &runtime, bool authority,
                      void *consumer_player = nullptr) noexcept;
void InvalidateBinding(SessionRuntime &runtime, const char *reason,
                       bool defer) noexcept;
bool ReadyForAuthority(SessionRuntime &runtime) noexcept;
// 加载相关 owner 的有界 enter/return 事件：在 Start 进入时固定首次菜单加载
// 资格，并在正常返回后把稳定入口延后到后续 loop。
void NoteLoadingOwnerEntered(unsigned bit) noexcept;
void NoteLoadingOwnerReturned(unsigned bit, bool normal) noexcept;
// 无 tick 的加载查询只累加窗口汇总，不改变 query 返回值。
void RecordLoadingQuery(bool nonzero, bool known_context) noexcept;
// 进入/结束加载窗口时记录 snapshot 边界；由 RefreshLifecycle 调用。
void BeginLoadingEpisode(SessionRuntime &runtime, unsigned reasons,
                         const char *source, bool first_menu) noexcept;
void EndLoadingEpisode(SessionRuntime &runtime, const char *reason) noexcept;
// status 的加载窗口摘要（窗口数、当前窗口、无 tick 查询汇总）。
std::string LoadingFields(const SessionRuntime *runtime);
} // namespace cs::game
