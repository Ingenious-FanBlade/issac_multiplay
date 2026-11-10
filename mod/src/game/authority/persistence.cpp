#include "game/authority/persistence.h"
#include "game/client/isolation/client_isolation.h"
#include "binding/offsets.h"
#include "game/common/context.h"
#include "game/authority/lifecycle.h"
#include "game/participants/remote_life.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/participants/topology.h"
#include "runtime.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <atomic>

namespace cs::game {
namespace {
// 跨 loop 窗口与同步调用都只在主线程记录；固定容量，容量不足时显式报错。
constexpr std::size_t kWindowCapacity = 16;
// 一个窗口在没有观察结果时的上限（Manager loop 单位）；超时单列，不冒充完成。
constexpr std::uint64_t kWindowTimeoutLoops = 1800;
struct WindowRegistry {
  std::array<PersistenceWindow, kWindowCapacity> windows{};
  std::array<std::uint64_t, static_cast<std::size_t>(
                               PersistenceWindowKind::Count)>
      opens{}, closes{}, abnormal{};
  std::uint64_t next_id = 0, next_call = 0, current_call = 0;
  std::uint64_t entries = 0, returns = 0, return_errors = 0, overflow = 0;
  std::uint64_t load_reads = 0;
  std::int64_t last_load_slot = -1;
};
WindowRegistry registry;
// 外线程也只能累加纯值；不从拒绝路径访问 PGD、Lua 或当前场景对象。
std::atomic<std::uint64_t> client_denied_native{0},client_denied_lua{0};
// 最大八个窗口，各 id 最长二十位加分隔符，完整列表不能沿用旧 64 字节缓冲。
constexpr std::size_t kWindowIdsBytes = kWindowCapacity * 21 + 1;
// 预读可能发生在 Host 之前；无会话用 0，Hosting 用已分配的本轮代次。
uint64_t PersistenceSessionGeneration() noexcept {
  const auto *r = Runtime();
  return r && !r->finished ? SessionGeneration(*r) : 0;
}
// 线程局部同步 operation 标签：只围住一次原生同步调用，不跨帧存活。
thread_local const char *operation = "none";

int ControlledRemote(SessionRuntime &r, void *entity) noexcept {
  for (unsigned i = 1; i < core::kMaximumParticipants; ++i)
    if (r.session.HasParticipant(i) && r.Remote(i).join.complete && r.topology[i].Contains(entity, false)) return int(i);
  return -1;
}
// 有效窗口 id 列表；只用于把一次入口与它所属的跨 loop 窗口关联起来。
void WindowIds(char *out, std::size_t size) noexcept {
  if (!out || !size)
    return;
  out[0] = 0;
  std::size_t used = 0;
  for (const auto &window : registry.windows) {
    if (!window.active)
      continue;
    char item[32];
    const int n = _snprintf_s(item, sizeof(item), _TRUNCATE, "%s%llu",
                              used ? "," : "", window.id);
    if (n < 0 || used + std::size_t(n) + 1 > size)
      return;
    std::memcpy(out + used, item, std::size_t(n));
    used += std::size_t(n);
    out[used] = 0;
  }
}
PersistenceWindow *FindWindow(PersistenceWindowKind kind, unsigned ordinal = 0) noexcept {
  for (auto &window : registry.windows)
    if (window.active && window.kind == kind && window.ordinal == ordinal)
      return &window;
  return nullptr;
}
// 记录一次同步入口的正常/异常返回；不虚构异常路径的返回值。
void FinishCall(const char *entry, std::uint64_t call, std::uint64_t parent,
                bool normal, bool has_value, bool value) noexcept {
  if (normal)
    ++registry.returns;
  else
    ++registry.return_errors;
  // 本次同步入口对应的跨 loop 窗口记录“原生已返回”，窗口本身继续等结果。
  for (auto &window : registry.windows) {
    if (!window.active || window.caller != call)
      continue;
    window.call_returned = true;
    window.call_normal = normal;
  }
  char windows[kWindowIdsBytes];
  WindowIds(windows, sizeof(windows));
  Trace("persistent",
        "phase=return entry=%s call=%llu parent=%llu normal=%u has_value=%u "
        "value=%u operation=%s windows=%s loop=%llu session_generation=%llu",
        entry, call, parent, unsigned(normal), unsigned(has_value),
        unsigned(has_value && value), operation, windows, ManagerLoop(),
        PersistenceSessionGeneration());
}
} // namespace
void ObserveClientPersistenceDenied(const char *entry,bool lua) noexcept {
  ClientIsolationObserve(lua?ClientIsolationKind::LuaPersistence:ClientIsolationKind::NativePersistence,entry);
  auto &counter=lua?client_denied_lua:client_denied_native;
  counter.fetch_add(1,std::memory_order_relaxed);
  if(IsMain())Trace("client_persistence","event=denied source=%s entry=%s loop=%llu",
                    lua?"lua":"native",entry?entry:"unknown",ManagerLoop());
}
const char *PersistenceWindowKindName(PersistenceWindowKind kind) noexcept {
  switch (kind) {
  case PersistenceWindowKind::None:
    return "none";
  case PersistenceWindowKind::BDeath:
    return "b_death";
  case PersistenceWindowKind::BRevive:
    return "b_revive";
  case PersistenceWindowKind::StageChange:
    return "stage_change";
  case PersistenceWindowKind::Continue:
    return "continue";
  case PersistenceWindowKind::RunExit:
    return "run_exit";
  case PersistenceWindowKind::Count:
    break;
  }
  return "UNKNOWN";
}
uint64_t OpenPersistenceWindow(PersistenceWindowKind kind, const char *source,
                               uint64_t death_event, unsigned ordinal) noexcept {
  if (!IsMain() || kind == PersistenceWindowKind::None ||
      kind == PersistenceWindowKind::Count)
    return 0;
  // 同类窗口不重复打开：一次原生请求对应一个窗口。再次请求时保留 id 与
  // 起始 loop，但重置“调用已返回”标志——否则第二次原生调用会被误当成已返回。
  if (auto *existing = FindWindow(kind, ordinal)) {
    existing->source = source ? source : "";
    existing->caller = registry.current_call;
    existing->death_event = death_event;
    existing->call_returned = false;
    existing->call_normal = false;
    Trace("persist_window",
          "event=reopen id=%llu kind=%s source=%s caller=%llu death=%llu "
          "loop=%llu session_generation=%llu",
          existing->id, PersistenceWindowKindName(kind), existing->source,
          existing->caller, existing->death_event, ManagerLoop(),
          existing->session_generation);
    return existing->id;
  }
  PersistenceWindow *slot = nullptr;
  for (auto &window : registry.windows)
    if (!window.active) {
      slot = &window;
      break;
    }
  if (!slot) {
    ++registry.overflow;
    Trace("persist_window", "event=overflow kind=%s source=%s loop=%llu",
          PersistenceWindowKindName(kind), source ? source : "", ManagerLoop());
    return 0;
  }
  auto *r = Runtime();
  *slot = {};
  slot->id = ++registry.next_id;
  slot->kind = kind;
  slot->ordinal = ordinal;
  slot->caller = registry.current_call;
  slot->parent = 0;
  slot->source = source ? source : "";
  slot->active = true;
  slot->start_loop = ManagerLoop();
  slot->session_generation = PersistenceSessionGeneration();
  slot->loading_episode = r && r->loading.active ? r->loading.id : 0;
  slot->death_event = death_event;
  ++registry.opens[static_cast<std::size_t>(kind)];
  Trace("persist_window",
        "event=open id=%llu kind=%s source=%s caller=%llu loading_episode=%llu "
        "death=%llu generation=%llu loop=%llu session_generation=%llu",
        slot->id, PersistenceWindowKindName(kind), slot->source, slot->caller,
        slot->loading_episode, slot->death_event, slot->session_generation,
        slot->start_loop, slot->session_generation);
  return slot->id;
}
bool ClosePersistenceWindow(PersistenceWindowKind kind, const char *reason,
                            bool normal, unsigned ordinal) noexcept {
  auto *window = FindWindow(kind, ordinal);
  if (!window)
    return false;
  window->active = false;
  window->end_loop = ManagerLoop();
  window->end_reason = reason ? reason : "closed";
  window->normal = normal;
  ++registry.closes[static_cast<std::size_t>(kind)];
  if (!normal)
    ++registry.abnormal[static_cast<std::size_t>(kind)];
  Trace("persist_window",
        "event=close id=%llu kind=%s reason=%s normal=%u call_returned=%u "
        "call_normal=%u start_loop=%llu end_loop=%llu death=%llu "
        "session_generation=%llu caller=%llu loading_episode=%llu",
        window->id, PersistenceWindowKindName(kind), window->end_reason,
        unsigned(normal), unsigned(window->call_returned),
        unsigned(window->call_normal), window->start_loop, window->end_loop,
        window->death_event, window->session_generation, window->caller,
        window->loading_episode);
  return true;
}
bool PersistenceWindowActive(PersistenceWindowKind kind, unsigned ordinal) noexcept {
  return FindWindow(kind, ordinal) != nullptr;
}
bool PersistenceWindowCallNormal(PersistenceWindowKind kind, unsigned ordinal) noexcept {
  const auto *window = FindWindow(kind, ordinal);
  return window && window->call_returned && window->call_normal;
}
bool PersistenceWindowCallReturned(PersistenceWindowKind kind, unsigned ordinal) noexcept {
  const auto *window = FindWindow(kind, ordinal);
  return window && window->call_returned;
}
void CloseAllPersistenceWindows(const char *reason, bool normal) noexcept {
  for (std::size_t i = 1; i < static_cast<std::size_t>(PersistenceWindowKind::Count);
       ++i)
    for (unsigned ordinal = 0; ordinal < core::kMaximumParticipants; ++ordinal)
      ClosePersistenceWindow(static_cast<PersistenceWindowKind>(i), reason, normal, ordinal);
}
PersistenceCallScope::PersistenceCallScope(const char *entry,
                                           void *object, bool enabled) noexcept
    : entry_(entry ? entry : ""), object_(object), main_(IsMain() && enabled),
      exceptions_(std::uncaught_exceptions()) {
  if (!main_)
    return;
  call_ = ++registry.next_call;
  parent_ = registry.current_call;
  registry.current_call = call_;
  ++registry.entries;
  char windows[kWindowIdsBytes];
  WindowIds(windows, sizeof(windows));
  Trace("persistent",
        "phase=enter entry=%s call=%llu parent=%llu object=%p operation=%s "
        "windows=%s loop=%llu session_generation=%llu",
        entry_, call_, parent_, object_, operation, windows, ManagerLoop(),
        PersistenceSessionGeneration());
}
PersistenceCallScope::~PersistenceCallScope() noexcept {
  if (!main_)
    return;
  if (!recorded_) {
    recorded_ = true;
    const bool normal = exceptions_ == std::uncaught_exceptions();
    FinishCall(entry_, call_, parent_, normal, false, false);
  }
  registry.current_call = parent_;
}
void PersistenceCallScope::Returned(bool value) noexcept {
  if (!main_ || recorded_)
    return;
  recorded_ = true;
  FinishCall(entry_, call_, parent_, true, true, value);
}
void PersistenceCallScope::Returned() noexcept {
  if (!main_ || recorded_)
    return;
  recorded_ = true;
  FinishCall(entry_, call_, parent_, true, false, false);
}
// Kill 只对当前归属 B 开启跨 loop 死亡窗口；不修改原生死亡结果。
void ObserveEntityKill(void *entity, void *source) noexcept {
  if (!IsMain() || !entity)
    return;
  auto *r = Runtime();
  if (!r || r->finished || r->mode != core::SessionMode::Host)
    return;
  const int ordinal = ControlledRemote(*r, entity);
  if (ordinal < 1) return;
  const auto window = OpenPersistenceWindow(PersistenceWindowKind::BDeath,
                                            "native_kill", r->Remote(unsigned(ordinal)).life.death_event, unsigned(ordinal));
  Trace("life",
        "ordinal=%u event=kill_request player=%p source=%p generation=%llu death=%llu "
        "window=%llu loop=%llu",
        unsigned(ordinal), entity, source, r->players[ordinal].Generation(), r->Remote(unsigned(ordinal)).life.death_event,
        window, r->loop);
}
// RevivePlayerGhost 只对当前归属 B 开启复活窗口；bool 由 Lua 端记录，不由这里伪造。
void ObserveReviveGhost(void *player) noexcept {
  if (!IsMain() || !player)
    return;
  auto *r = Runtime();
  if (!r || r->finished || r->mode != core::SessionMode::Host)
    return;
  const int ordinal = ControlledRemote(*r, player);
  if (ordinal < 1) return;
  const auto window = OpenPersistenceWindow(PersistenceWindowKind::BRevive,
                                            "native_revive", r->Remote(unsigned(ordinal)).life.death_event, unsigned(ordinal));
  Trace("life",
        "ordinal=%u event=revive_request player=%p status=%s death=%llu window=%llu "
        "loop=%llu",
        unsigned(ordinal), player, RemoteLifeStatusName(r->Remote(unsigned(ordinal)).life.status), r->Remote(unsigned(ordinal)).life.death_event,
        window, r->loop);
}
// 换层请求在 original 之前开始观察；真正进入 FSM 由后续字段观察决定。
void ObserveStageTransition(void *game, bool same_stage, int param2,
                            void *player) noexcept {
  if (!IsMain() || !game || game != GamePointer())
    return;
  const auto window =
      OpenPersistenceWindow(PersistenceWindowKind::StageChange, "native_request");
  auto *r = Runtime();
  Trace("stage",
        "event=request same_stage=%u param=%d player=%p window=%llu "
        "starting=%u stage=%d room_transition=%d loop=%llu",
        unsigned(same_stage), param2, player, window,
        r ? unsigned(r->topology[0].starting) : 0u,
        r ? r->topology[0].stage_transition : 0,
        r ? r->topology[0].room_transition : 0, ManagerLoop());
}
// 菜单预读存档用独立 read 记录关联，不提前伪造 continue 窗口。
void ObserveLoadGameState(void *manager, int saveslot) noexcept {
  if (!IsMain() || !manager)
    return;
  ++registry.load_reads;
  registry.last_load_slot = saveslot;
  char windows[kWindowIdsBytes];
  WindowIds(windows, sizeof(windows));
  auto *r = Runtime();
  Trace("persist_read",
        "event=load_game_state slot=%d call=%llu windows=%s run=%u "
        "run_observed=%u loop=%llu session_generation=%llu",
        saveslot, registry.current_call, windows,
        r ? unsigned(RunActive()) : 0u, r ? unsigned(r->run_observed) : 0u,
        ManagerLoop(), PersistenceSessionGeneration());
}
// 每次稳定观察后收口：只读已复制的 runtime 值，不调用原生函数。
void ObservePersistenceWindows(SessionRuntime &r) noexcept {
  const bool stable = !r.blocks && !r.join_ordinal && !r.retiring;
  if (auto *window = FindWindow(PersistenceWindowKind::Continue)) {
    if (r.retiring || r.failure) {
      ClosePersistenceWindow(PersistenceWindowKind::Continue,
                             r.failure ? "session_failed" : "session_retiring",
                             false);
    } else if (!r.loading.active && stable && r.players[0].BoundValid()) {
      // 首次菜单加载已稳定：A 重新唯一绑定且加载窗口结束后才收口。
      ClosePersistenceWindow(PersistenceWindowKind::Continue, "stabilized",
                             true);
    } else if (r.loop > window->start_loop + kWindowTimeoutLoops) {
      ClosePersistenceWindow(PersistenceWindowKind::Continue, "timeout", false);
    }
  }
  for (unsigned ordinal = 1; ordinal < core::kMaximumParticipants; ++ordinal) {
  if (auto *window = FindWindow(PersistenceWindowKind::BRevive, ordinal)) {
    if (r.retiring || r.failure) {
      ClosePersistenceWindow(PersistenceWindowKind::BRevive,
                             r.failure ? "session_failed" : "session_retiring",
                             false, ordinal);
    } else if (r.Remote(ordinal).life.status == RemoteLifeStatus::Alive &&
               r.players[ordinal].BoundValid() && r.Remote(ordinal).life.revived_loop != 0) {
      // 正常情况下已在观察到健康时收口；这里只兜底重绑成功仍未关闭的情况。
      ClosePersistenceWindow(PersistenceWindowKind::BRevive, "rebound", true, ordinal);
    } else if (r.loop > window->start_loop + kWindowTimeoutLoops) {
      ClosePersistenceWindow(PersistenceWindowKind::BRevive, "timeout", false, ordinal);
    }
  }
  }
  if (auto *window = FindWindow(PersistenceWindowKind::StageChange)) {
    const bool stage_clear = r.topology[0].known && r.topology[0].run &&
                             r.topology[0].stage_transition == 0;
    if (r.retiring || r.failure) {
      ClosePersistenceWindow(PersistenceWindowKind::StageChange,
                             r.failure ? "session_failed" : "session_retiring",
                             false);
    } else if (stage_clear && stable && r.loop > window->start_loop) {
      // 未进入转场记 rejected：请求后字段从未置位、窗口外已经稳定。
      const bool entered = window->call_returned && window->call_normal;
      ClosePersistenceWindow(PersistenceWindowKind::StageChange,
                             entered ? "stabilized" : "rejected", entered);
    } else if (r.loop > window->start_loop + kWindowTimeoutLoops) {
      ClosePersistenceWindow(PersistenceWindowKind::StageChange, "timeout",
                             false);
    }
  }
  // b_death 的正常收口在观察到 ghost 时完成；这里只处理会话结束与超时，
  // 不把“未观察到结果”冒充成功。超时时 B 仍为 Alive 的按意外自动复活单列。
  for (unsigned ordinal = 1; ordinal < core::kMaximumParticipants; ++ordinal) {
  if (auto *window = FindWindow(PersistenceWindowKind::BDeath, ordinal)) {
    if (r.retiring || r.failure) {
      ClosePersistenceWindow(PersistenceWindowKind::BDeath,
                             r.failure ? "session_failed" : "session_retiring",
                             false, ordinal);
    } else if (r.loop > window->start_loop + kWindowTimeoutLoops) {
      const bool still_alive = r.Remote(ordinal).life.status == RemoteLifeStatus::Alive;
      ClosePersistenceWindow(PersistenceWindowKind::BDeath,
                             still_alive ? "unexpected_revive" : "timeout",
                             false, ordinal);
    }
  }
  }
}
void FinishPersistenceWindows(const char *reason) noexcept {
  CloseAllPersistenceWindows(reason, false);
}
std::string PersistenceFields() {
  std::string out;
  const auto n = [&](const char *key, uint64_t value) {
    out += key;
    out += '=';
    out += std::to_string(value);
    out += '\n';
  };
  n("persist_entries", registry.entries);
  n("persist_client_denied_native",client_denied_native.load(std::memory_order_relaxed));
  n("persist_client_denied_lua",client_denied_lua.load(std::memory_order_relaxed));
  n("persist_returns", registry.returns);
  n("persist_return_errors", registry.return_errors);
  n("persist_window_overflow", registry.overflow);
  n("persist_load_reads", registry.load_reads);
  n("persist_last_load_slot",
    registry.last_load_slot < 0 ? 0 : uint64_t(registry.last_load_slot));
  n("persist_load_slot_known", registry.last_load_slot >= 0);
  n("persist_current_call", registry.current_call);
  unsigned open = 0;
  for (const auto &window : registry.windows)
    open += window.active;
  n("persist_open_windows", open);
  for (std::size_t i = 1;
       i < static_cast<std::size_t>(PersistenceWindowKind::Count); ++i) {
    const auto kind = static_cast<PersistenceWindowKind>(i);
    const std::string name = PersistenceWindowKindName(kind);
    n(("persist." + name + ".open").c_str(), registry.opens[i]);
    n(("persist." + name + ".close").c_str(), registry.closes[i]);
    n(("persist." + name + ".abnormal").c_str(), registry.abnormal[i]);
  }
  return out;
}
std::string PersistenceWindowFields() {
  std::string out;
  for (const auto &window : registry.windows) {
    if (!window.active)
      continue;
    const std::string prefix = "persist_window." + std::to_string(window.id);
    out += prefix + ".ordinal=" + std::to_string(window.ordinal) + "\n";
    out += prefix + ".kind=" + PersistenceWindowKindName(window.kind) + "\n";
    out += prefix + ".source=" + window.source + "\n";
    out += prefix + ".start_loop=" + std::to_string(window.start_loop) + "\n";
    out += prefix + ".call_returned=" +
           std::to_string(unsigned(window.call_returned)) + "\n";
    out += prefix + ".call_normal=" +
           std::to_string(unsigned(window.call_normal)) + "\n";
    out += prefix + ".session_generation=" + std::to_string(window.session_generation) + "\n";
    out += prefix + ".caller=" + std::to_string(window.caller) + "\n";
    out += prefix + ".loading_episode=" + std::to_string(window.loading_episode) + "\n";
    out += prefix + ".death_event=" + std::to_string(window.death_event) + "\n";
  }
  return out;
}
PersistenceOperation::PersistenceOperation(const char *name) noexcept
    : previous_(operation) {
  operation = name;
}
PersistenceOperation::~PersistenceOperation() noexcept {
  operation = previous_;
}
namespace {
// 仅读取 ST8 与官方结构均已确认的字段；无效下标不解引用。
bool Value(const char *entry, void *data, int index, int &value) noexcept {
  if (std::strcmp(entry, "pgd_unlock") == 0) {
    unsigned char unlocked = 0;
    if (index < 0 || index >= 641 || !ReadAt(data, 0x38 + index, unlocked))
      return false;
    value = unlocked;
    return true;
  }
  if (std::strcmp(entry, "pgd_counter") == 0)
    return index >= 0 && index < 523 && ReadAt(data, 0x2BC + index * 4, value);
  value = 0;
  return true;
}
} // namespace
PersistenceScope::PersistenceScope(const char *entry, void *data, int index,
                                   int amount) noexcept
    : entry_(entry), operation_(operation), data_(data), index_(index),
      amount_(amount), exceptions_(std::uncaught_exceptions()) {
  if (!IsMain())
    return;
  call_ = ++registry.next_call;
  parent_ = registry.current_call;
  registry.current_call = call_;
  ++registry.entries;
  // Join 原生 Init/官方回调发生在 Join owner 内，也覆盖嵌套的 PGD 调用。
  if (std::strcmp(operation_, "none") == 0 && HostBusy() &&
      HasOwner(Owner::Join))
    operation_ = "join";
  char windows[kWindowIdsBytes];
  WindowIds(windows, sizeof(windows));
  if (std::strcmp(entry_, "button_maps") == 0) {
    known_ = true;
    Trace("persistent",
          "phase=enter entry=%s call=%llu parent=%llu operation=%s known=1 "
          "windows=%s loop=%llu session_generation=%llu",
          entry_, call_, parent_, operation_, windows, ManagerLoop(),
          PersistenceSessionGeneration());
    return;
  }
  known_ = ReadAt(data_, 0, dirty_) && ReadAt(data_, 1, readonly_) &&
           ReadAt(data_, 0xF8C, loaded_) && Value(entry_, data_, index_, before_);
  Trace("persistent",
        "phase=enter entry=%s call=%llu parent=%llu operation=%s known=%u "
        "dirty=%u readonly=%u loaded=%u index=%d amount=%d value=%d windows=%s "
        "loop=%llu session_generation=%llu",
        entry_, call_, parent_, operation_, known_, dirty_, readonly_, loaded_,
        index_, amount_, before_, windows, ManagerLoop(), PersistenceSessionGeneration());
}
PersistenceScope::~PersistenceScope() noexcept {
  if (!IsMain())
    return;
  unsigned char dirty = 0;
  int after = 0;
  const bool maps = std::strcmp(entry_, "button_maps") == 0;
  const bool known =
      known_ && (maps || (ReadAt(data_, 0, dirty) && Value(entry_, data_, index_, after)));
  if (!recorded_) {
    recorded_ = true;
    const bool normal = exceptions_ == std::uncaught_exceptions();
    if (normal)
      ++registry.returns;
    else
      ++registry.return_errors;
    char windows[kWindowIdsBytes];
    WindowIds(windows, sizeof(windows));
    Trace("persistent",
          "phase=return entry=%s call=%llu parent=%llu operation=%s known=%u "
          "normal=%u dirty_before=%u dirty_after=%u readonly=%u loaded=%u "
          "index=%d amount=%d before=%d after=%d windows=%s loop=%llu "
          "session_generation=%llu has_value=0",
          entry_, call_, parent_, operation_, known, unsigned(normal), dirty_,
          dirty, readonly_, loaded_, index_, amount_, before_, after, windows,
          ManagerLoop(), PersistenceSessionGeneration());
  } else {
    char windows[kWindowIdsBytes];
    WindowIds(windows, sizeof(windows));
    Trace("persistent",
          "phase=observe entry=%s call=%llu parent=%llu known=%u dirty_before=%u "
          "dirty_after=%u readonly=%u loaded=%u index=%d amount=%d before=%d "
          "after=%d windows=%s loop=%llu session_generation=%llu",
          entry_, call_, parent_, known, dirty_, dirty, readonly_, loaded_,
          index_, amount_, before_, after, windows, ManagerLoop(), PersistenceSessionGeneration());
  }
  registry.current_call = parent_;
}
// TryUnlock 等 bool 入口的正常返回值：区分“调用过”“返回 true”“字段变化”。
void PersistenceScope::Returned(bool value) noexcept {
  if (!IsMain() || recorded_)
    return;
  recorded_ = true;
  ++registry.returns;
  int after = 0;
  const bool after_known = Value(entry_, data_, index_, after);
  const bool observed = known_ && after_known && after != before_;
  char windows[kWindowIdsBytes];
  WindowIds(windows, sizeof(windows));
  Trace("persistent",
        "phase=return entry=%s call=%llu parent=%llu operation=%s normal=1 "
        "result=%u field_changed=%u before=%d after=%d windows=%s loop=%llu "
        "session_generation=%llu has_value=1 value=%u known=%u",
        entry_, call_, parent_, operation_, unsigned(value),
        unsigned(observed), before_, after, windows, ManagerLoop(),
        PersistenceSessionGeneration(), unsigned(value), unsigned(known_ && after_known));
}
} // namespace cs::game
