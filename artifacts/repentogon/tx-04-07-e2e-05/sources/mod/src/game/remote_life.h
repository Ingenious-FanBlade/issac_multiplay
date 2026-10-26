#pragma once
#include "game/topology.h"
#include <cstdint>

namespace cs::game {
struct SessionRuntime;
// B 的生命状态。None 表示尚无 B（Join 前或已退休）；Alive 只表示生命可用，
// 是否已绑定由 Participant 的绑定状态单独表达，不用它冒充“已绑定”。
enum class RemoteLifeStatus : unsigned { None, Alive, Lost, Revived };
const char *RemoteLifeStatusName(RemoteLifeStatus status) noexcept;
// 资格变化原因的稳定分类；trace 同时保留原始 reason，不能用分类丢掉事实。
const char *EligibilityReasonClass(const char *reason) noexcept;
// 当前确认的自有 B 归属。只在主线程写入；复制出的值可交给 trace/网络。
struct RemoteOwnership {
  void *player = nullptr;
  void *device = nullptr;
  int key = -1, character = -1;
  RoomIdentity room{};
  std::uint64_t generation = 0;
  bool confirmed = false;
};
// 退休快照：移出 K 之前保存的 Game/PlayerManager/A/B/controller 只读关联。
// 全部是值；移出后必须重新从列表确认才能调用原生移除。
struct RetirementPlayerSnapshot {
  void *game = nullptr;
  // Game 内嵌的 PlayerManager，原生 RemoveCoPlayer 的 this；不是全局 Manager。
  void *player_manager = nullptr;
  void *host = nullptr;
  void *remote = nullptr;
  void *device = nullptr;
  int host_key = -1, remote_key = -1;
  int host_character = -1, remote_character = -1;
  RoomIdentity room{};
  std::uint64_t generation = 0;
  bool confirmed = false;
};
// B 的生命记录、死亡窗口与重绑计数。所有字段由主线程修改。
struct RemoteLifeData {
  RemoteLifeStatus status = RemoteLifeStatus::None;
  std::uint64_t death_event = 0; // 当前死亡窗口编号；0 表示没有
  std::uint64_t deaths = 0, revives = 0, rebinds = 0;
  std::uint64_t lost_loop = 0, lost_tick = 0, revived_loop = 0,
                revived_tick = 0;
  std::uint64_t unreadable = 0; // 暂时不可读的观察次数，不冒充移除
  std::uint64_t conflicts = 0;  // 已确认移除/归属矛盾的次数
  RemoteOwnership current{};
  RetirementPlayerSnapshot retirement{};
  bool joined = false;          // 首次 Join 绑定已完成
  bool awaiting_rebind = false; // Revived 但尚未重绑
  bool retire_ready = false;    // 退休快照有效
  const char *pending_reason = "";  // 最近一次失效原因（重绑时作为 reason）
  const char *conflict_reason = ""; // 最近一次归属矛盾原因
};
// 非加载观察段的返回值：只请求中性或 PeerLost，Bind 由稳定入口单独执行。
enum class RemoteLifeAction : unsigned { None, Invalidate, PeerLost };
// 退休确认结果：只有 Ready 才允许调用原生移除；Absent 表示已确认不在列表，
// 跳过移除但视为清理完成；其余情况保留现场并报告残留。
enum class RetirementConfirm : unsigned { Unknown, Absent, Conflict, Ready };
// 主线程已读取的 A/B 拓扑；只更新生命状态与请求，不 Bind、不读队列、不写世界。
RemoteLifeAction ObserveRemoteLife(SessionRuntime &runtime, const Topology &host,
                                   const Topology &remote) noexcept;
// Join 正常返回并确认完整 B 后建立首次生命记录与当前归属。
void NoteRemoteJoined(SessionRuntime &runtime, const Topology &remote) noexcept;
// 稳定且未阻断的 Authority 入口：重新读取唯一 exact pair 后绑定同一 ordinal。
// after_revival 只影响计数与 trace 的原因，不放宽任何健康/归属条件。
bool BindRevivedRemote(SessionRuntime &runtime, bool after_revival) noexcept;
// 撤销 B 的绑定一次（Participant::Invalidate 已递增 generation），并保留
// 一次输入丢弃边界；重复调用在未绑定时不产生额外效果。
void InvalidateRemoteBinding(SessionRuntime &runtime, const char *reason) noexcept;
// 移出 K 之前保存当前 Game/PlayerManager/A/B/controller 的只读关联。
bool CaptureRetirementSnapshot(SessionRuntime &runtime) noexcept;
// K 已离开设备 vector 后用受限读取确认当前 slot 仍是快照中的自有 B。
RetirementConfirm ConfirmRetirementPlayer(SessionRuntime &runtime,
                                          void *&player) noexcept;
// 健康条件：ReadTopology 已校验 Type/Variant/index/controller/唯一设备，
// 这里再要求生命可用、角色与本轮指定角色一致。
bool RemoteHealthEligible(const SessionRuntime &runtime,
                          const Topology &remote) noexcept;
} // namespace cs::game
