#include "binding/offsets.h"
#include "game/bridge.h"
#include "game/context.h"
#include "game/lifecycle.h"
#include "game/persistence.h"
#include "game/state.h"
#include "game/tick.h"
#include "game/topology.h"
#include "runtime.h"
#include <algorithm>
#include <charconv>
#include <cstring>
#include <exception>
#include <mutex>
#include <sstream>
namespace cs {
namespace {
// 状态变更只排队，在下一次最外层 Manager 入口统一执行。
struct Mutation {
  uint64_t id;
  enum class Kind { Host, StopHost, SoloStart, SoloStop } kind;
  unsigned port;
  int character;
};
Ring<Mutation, 64> mutations;
Ring<PortRequest, 32> port_requests;
Ring<PortReply, 32> port_replies;
std::atomic<uint64_t> next_id{1}, queue_dropped{};
uint64_t frame = 0;
unsigned frame_depth = 0;
bool pending_host_start = false, pending_solo_start = false;
bool started = false;
std::mutex snapshot_lock;
std::string status_snapshot =
    "ok=1\nstatus_version=1\nstate=Disabled\nreason=initializing\n";
std::string counter_snapshot;
std::string last_error;
std::string state() {
  for (auto c : Capabilities)
    if (!c)
      return "Disabled";
  return game::StateName();
}
// 所有来源共用参数解析；变更入队，查询读取上一帧快照。
std::string Execute(std::string_view text, const char *source) {
  if (text.size() > 512 ||
      text.find_first_of("\r\n\0", 0, 3) != std::string_view::npos)
    return "ok=0\nerror=invalid_line\n";
  std::istringstream in{std::string(text)};
  std::string prefix, verb, arg, extra;
  in >> prefix >> verb;
  std::string result;
  if (prefix != "cs")
    result = "ok=0\nerror=unknown_prefix\n";
  else if (verb == "echo") {
    in >> arg >> extra;
    result = arg.empty() || !extra.empty() ? "ok=0\nerror=arguments\n"
                                           : "ok=1\necho=" + arg + "\n";
  } else if (verb == "query_checkpoint") {
    in >> arg >> extra;
    // 场景标签限制为短的单个词；只发布主线程已有统计，不发起原生查询、
    // Submit/Resolve，不清桶、不改变输入或参与者状态。
    const bool label_ok = !arg.empty() && arg.size() <= 64 && extra.empty() &&
        arg.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") == std::string::npos;
    if (!label_ok)
      result = "ok=0\nerror=arguments\n";
    else if (auto *r = game::Runtime()) {
      game::PublishQueryCoverage(*r, arg.c_str());
      result = "ok=1\nquery_checkpoint=" + arg + "\nquery_snapshot=" +
          std::to_string(r->query_snapshots) + "\nsession_generation=" +
          std::to_string(game::SessionGeneration(*r)) + "\n";
    } else
      result = "ok=0\nerror=no_session\n";
  } else if (verb == "host" || verb == "stop" || verb == "solo") {
    std::string trailing;
    in >> arg >> extra >> trailing;
    unsigned port = Settings.host_port;
    int character = 0;
    bool valid = trailing.empty();
    const bool solo = verb == "solo";
    const bool start_solo = solo && arg == "start";
    if (verb == "host") {
      auto parsed = std::from_chars(arg.data(), arg.data() + arg.size(), port);
      valid = valid && !arg.empty() && parsed.ec == std::errc{} &&
              parsed.ptr == arg.data() + arg.size() && port > 0 &&
              port <= 65535;
      if (!extra.empty()) {
        auto role = std::from_chars(extra.data(), extra.data() + extra.size(), character);
        // 可选性在 Join 的原生角色表中确认；这里仅拒绝负值、随机标记与溢出。
        valid = valid && role.ec == std::errc{} &&
                role.ptr == extra.data() + extra.size() && character >= 0;
      }
    } else if (solo)
      valid = valid && extra.empty() && (arg == "start" || arg == "stop");
    else
      valid = valid && arg.empty() && extra.empty();
    if (!valid)
      result = "ok=0\nerror=arguments\n";
    else if ((verb == "host" || start_solo) && state() == "Disabled")
      result = "ok=0\nerror=capabilities_unavailable\n";
    else if (((verb == "host" || start_solo) &&
              (game::Busy() || pending_host_start || pending_solo_start)) ||
             (solo && !start_solo && (game::HostBusy() || pending_host_start)))
      result = "ok=0\nerror=busy\n";
    else if (start_solo && !game::RunActive())
      result = "ok=0\nerror=not_in_run\n";
    else {
      const auto kind = solo ? (start_solo ? Mutation::Kind::SoloStart
                                           : Mutation::Kind::SoloStop)
                             : (verb == "host" ? Mutation::Kind::Host
                                               : Mutation::Kind::StopHost);
      Mutation mutation{next_id.fetch_add(1), kind, port, character};
      if (mutations.Push(mutation)) {
        if (kind == Mutation::Kind::Host)
          pending_host_start = true;
        if (kind == Mutation::Kind::SoloStart)
          pending_solo_start = true;
        result = "ok=1\nqueued=" + std::to_string(mutation.id) +
                 "\nframe=" + std::to_string(frame) + "\n";
      } else {
        ++queue_dropped;
        result = "ok=0\nerror=queue_full\n";
      }
    }
  } else {
    in >> extra;
    if (!extra.empty())
      result = "ok=0\nerror=arguments\n";
    else if (verb == "version")
      result = std::string("ok=1\ncommand_version=1\ndll_version=") + Version +
               "\nrepentogon_version=" + RepentogonVersion + "\n";
    else if (verb == "status" || verb == "counters") {
      std::lock_guard lock(snapshot_lock);
      result = verb == "status" ? status_snapshot : counter_snapshot;
    } else
      result = "ok=0\nerror=unknown_verb\n";
  }
  // 记录所有命令来源及 Q14 的 Game 指针；cs 不进入原生命令分支。
  // status/counters 的完整响应走控制口；trace 只保留响应长度与结果，避免
  // 大快照挤入单个 3500 字节事件被截断。其它命令继续保留原始结果。
  if (verb == "status" || verb == "counters")
    Trace("command", "source=%s text=%.*s result_logged=0 result_bytes=%zu "
          "result_ok=%u g_game=%p frame=%llu", source, int(text.size()), text.data(),
          result.size(), unsigned(result.rfind("ok=1\n", 0) == 0), GamePointer(), frame);
  else
    Trace("command", "source=%s text=%.*s result=%s g_game=%p frame=%llu", source,
          int(text.size()), text.data(), result.c_str(), GamePointer(), frame);
  return result;
}
} // namespace
bool OwnCommand(std::string_view text) noexcept {
  return text == "cs" || (text.size() > 2 && text.substr(0, 2) == "cs" &&
                          (text[2] == ' ' || text[2] == '\t'));
}
std::string Command(std::string_view text, const char *source) {
  if (!IsMain())
    return "ok=0\nerror=wrong_thread\n";
  return Execute(text, source);
}
bool QueuePort(const PortRequest &req) noexcept {
  if (port_requests.Push(req))
    return true;
  ++queue_dropped;
  return false;
}
bool TakeReply(PortReply &reply) noexcept { return port_replies.Pop(reply); }
// 在主线程复制只读状态，再一次性替换供命令查询的快照。
void PublishStatus() noexcept {
  try {
    std::string counters = "ok=1\n";
    size_t n;
    auto hooks = Hooks(n);
    unsigned registered = 0;
    uint64_t foreign = 0, total = 0;
    for (size_t i = 0; i < n; ++i) {
      auto &h = *hooks[i];
      registered += h.registered;
      foreign += h.foreign.load();
      total += h.calls.load();
      counters += "hook." + std::string(h.id) + "=" +
                  std::to_string(h.calls.load()) + "\nhook." + h.id +
                  ".foreign=" + std::to_string(h.foreign.load()) + "\nhook." +
                  h.id + ".completed=" + std::to_string(h.completed.load()) +
                  "\n";
    }
    counters += PointerCounters();
    auto *manager = *reinterpret_cast<unsigned char **>(Base + offset::Manager);
    const bool starting =
        manager && *reinterpret_cast<bool *>(manager + offset::Starting);
    auto *game = static_cast<unsigned char *>(GamePointer());
    const bool run =
        game && *reinterpret_cast<bool *>(game + offset::RunActive);
    std::string s =
        "ok=1\nstatus_version=1\nstate=" + state() +
        "\nframe=" + std::to_string(frame) + "\ng_game=" + (game ? "1" : "0") +
        "\nrun_active=" + (run ? "1" : "0") +
        "\nmanager_starting=" + (starting ? "1" : "0") +
        "\nregistered=" + std::to_string(registered) +
        "\npointers=" + std::to_string(PointerCount()) +
        "\nforeign=" + std::to_string(foreign) +
        "\ncalls=" + std::to_string(total) +
        "\nqueue_dropped=" + std::to_string(queue_dropped.load()) +
        "\ntrace_dropped=" + std::to_string(TraceDropped()) +
        "\ntrace_truncated=" + std::to_string(TraceTruncated()) +
        "\ncontext_faults=" + std::to_string(ContextFaults()) +
        "\ntrace=" + TracePath() + "\nlast_error=" + last_error + "\n";
    s += MenuStatus(run);
    s += game::StatusFields();
    for (size_t i = 0; i < Capabilities.size(); ++i) {
      s += "capability." + std::string(GroupNames[i]) + "=" +
           (Capabilities[i] ? "1" : "0") + "\n";
      s += "reason." + std::string(GroupNames[i]) + "=" + CapabilityReasons[i] +
           "\n";
    }
    std::lock_guard lock(snapshot_lock);
    status_snapshot.swap(s);
    counter_snapshot.swap(counters);
  } catch (...) {
    Trace("status_error", "exception");
  }
}
// Manager 帧只是观测编号，不代表 Authority tick。
void FrameBegin() noexcept {
  if (frame_depth++ != 0) {
    game::ContextFault("reentrant_manager");
    Trace("context_error", "reentrant_manager");
    return;
  }
  ++frame;
  if (frame <= 3)
    Trace("frame_begin", "frame=%llu", frame);
  if (!started) {
    started = true;
    RefreshVersion();
    InstallRuntimePatches();
    Trace("runtime_version", "repentogon=%s", RepentogonVersion.c_str());
  }
  game::BeginManager(frame);
  try {
    // N 帧收到的变更在 N+1 帧入口执行，下面取出的端口请求不能抢跑。
    Mutation m{};
    for (unsigned i = 0; i < 64 && mutations.Pop(m); ++i) {
      const std::string before = state();
      last_error.clear();
      switch (m.kind) {
      case Mutation::Kind::Host:
        pending_host_start = false;
        {
          net::WorkerConfig config;
          config.port = static_cast<uint16_t>(m.port);
          config.bind_address = Settings.host_bind_address;
          config.peer_timeout_ms = Settings.peer_timeout_ms;
          last_error = game::StartHost(config, m.character);
        }
        break;
      case Mutation::Kind::StopHost:
        game::StopSession();
        break;
      case Mutation::Kind::SoloStart:
        pending_solo_start = false;
        last_error = game::StartSolo();
        break;
      case Mutation::Kind::SoloStop:
        if (game::HostBusy())
          last_error = "busy";
        else
          game::StopSolo();
        break;
      }
      Trace("state", "id=%llu from=%s to=%s port=%u character=%d frame=%llu error=%s", m.id,
            before.c_str(), state().c_str(), m.port, m.character, frame, last_error.c_str());
    }
    PortRequest request{};
    for (unsigned i = 0; i < 32 && port_requests.Pop(request); ++i) {
      // 失焦或加载后迟到的请求不能在已超时后才执行，尤其不能迟到创建会话。
      const auto now = GetTickCount64();
      if (now >= request.deadline_ms) {
        Trace("control_expired", "id=%llu text=%s wait_ms=%llu frame=%llu",
              request.id, request.text, now - request.enqueued_ms, frame);
        continue;
      }
      Trace("control_execute", "id=%llu wait_ms=%llu frame=%llu", request.id,
            now - request.enqueued_ms, frame);
      PortReply reply{};
      reply.id = request.id;
      auto text = Execute(request.text, "port");
      // 有界控制口响应不能静默截断；超长时回显式错误并留下实际长度。
      if (text.size() >= sizeof(reply.text)) {
        Trace("command_error", "response_too_large id=%llu bytes=%zu capacity=%zu",
              request.id, text.size(), sizeof(reply.text));
        strcpy_s(reply.text, "ok=0\nerror=response_too_large\n");
      } else
        strcpy_s(reply.text, text.c_str());
      if (!port_replies.Push(reply)) {
        ++queue_dropped;
        Trace("command_error", "completion_queue_full");
      }
    }
  } catch (...) {
    Trace("command_error", "frame_exception");
  }
}
void FrameEnd(bool completed) noexcept {
  if (!frame_depth || --frame_depth != 0)
    return;
  // 原生异常展开只归还深度，不发布未完成帧的状态快照。
  game::EndManager(completed);
  if (!completed)
    return;
  PublishStatus();
  if (frame <= 3)
    Trace("frame_end", "frame=%llu", frame);
  if (frame % 60 == 0) {
    try {
      std::lock_guard lock(snapshot_lock);
      // 按换行分片，每片最多 2500 字节；frame/part/last/total_bytes 可重组快照。
      constexpr size_t chunk_limit = 2500;
      size_t offset = 0;
      unsigned part = 0;
      while (offset < counter_snapshot.size()) {
        size_t length = (std::min)(chunk_limit, counter_snapshot.size() - offset);
        if (offset + length < counter_snapshot.size()) {
          const auto newline = counter_snapshot.rfind('\n', offset + length - 1);
          if (newline != std::string::npos && newline >= offset)
            length = newline - offset + 1;
        }
        Trace("counters", "frame=%llu part=%u last=%u total_bytes=%zu\n%.*s",
              frame, ++part, unsigned(offset + length == counter_snapshot.size()),
              counter_snapshot.size(), int(length), counter_snapshot.data() + offset);
        offset += length;
      }
    } catch (...) {
      Trace("status_error", "counter_exception");
    }
  }
}
// 只清掉旧 run 的排队变更与 pending 标记，不改变已经生效的会话。
// Restore 的保留分支与真实退出共用这一个辅助函数。
void ClearPendingRunMutations() noexcept {
  Mutation ignored{};
  while (mutations.Pop(ignored)) {
  }
  pending_host_start = pending_solo_start = false;
}
void RunExit() noexcept {
  game::StopSession(core::SessionCause::RunTeardown);
  // 清掉旧 run 的排队变更，避免返回菜单后旧 host 命令复活会话。
  ClearPendingRunMutations();
  game::ClosePersistenceWindow(game::PersistenceWindowKind::RunExit,
                               "native_return", true);
  Trace("state", "to=%s reason=run_exit frame=%llu", game::StateName(), frame);
  Trace("run_exit", "state=%s pending_mutations=0 resident=1",
        game::StateName());
}
namespace game {
// tx-04-07：一次 Restore 决策同时处理两个停止点。保留条件必须同时满足，
// 不能只凭 Warmup 或 startGame 豁免已有 run。
RestoreScope::RestoreScope(void *game_object, void *state, bool start_game) noexcept
    : game_(game_object), state_(state), start_game_(start_game),
      main_(cs::IsMain()), exceptions_(std::uncaught_exceptions()) {
  if (!main_)
    return;
  auto *r = Runtime();
  if (!r || r->finished)
    return;
  active_ = true;
  NoteLoadingOwnerEntered(RestoreOwner);
  const bool host_mode = r->mode == core::SessionMode::Host;
  // ExecuteStartGame 在调用 RestoreState 之前就已把 Game+0x25B02 置 1，因此这里
  // 不能再用“当前 RunActive”判断首次加载；资格在外层 Start 进入时已经固定。
  const bool identified_start =
      r->start_qualified && HasOwner(cs::Owner::Start);
  const bool clean = !r->join.complete &&
                     r->device.State() == DeviceKState::Empty &&
                     !r->players[0].BoundValid() &&
                     !r->players[1].BoundValid() && !r->retiring && !r->failure;
  // 首次菜单加载：Host 从主菜单等待首个 run，尚未创建 K、未 Join、无旧绑定。
  preserve_ = host_mode && start_game_ && identified_start && clean &&
              !r->run_observed &&
              (!r->peer.established ||
               r->session.Status().state == core::SessionState::Warmup);
  if (preserve_) {
    // 本轮资格只用于这一次加载；保留 worker、握手/session identity 与菜单 Host。
    r->start_qualified = false;
    InvalidateBinding(*r, "restore_preserve", false);
    ClearPendingRunMutations();
    Trace("restore",
          "event=preserve game=%p state=%p start=1 run_observed=0 device=%s "
          "joined=0 loop=%llu",
          game_, state_, DeviceKStateName(r->device.State()), r->loop);
    return;
  }
  // 已有 run 的恢复与实际退出：两个停止点合并为一次退休请求。
  RunTeardownBeginning();
  ClearPendingRunMutations();
  Trace("restore",
        "event=retire game=%p state=%p start=%u run_observed=%u device=%s "
        "joined=%u retiring=%u loop=%llu",
        game_, state_, unsigned(start_game_), unsigned(r->run_observed),
        DeviceKStateName(r->device.State()), unsigned(r->join.complete),
        unsigned(r->retiring), r->loop);
}
RestoreScope::~RestoreScope() noexcept {
  if (!main_ || !active_)
    return;
  const bool normal = exceptions_ == std::uncaught_exceptions();
  auto *r = Runtime();
  // 记录 owner 正常/异常返回，并把稳定入口延后到后续 loop。
  NoteLoadingOwnerReturned(RestoreOwner, normal);
  Trace("restore", "event=return preserve=%u normal=%u game=%p state=%p loop=%llu",
        unsigned(preserve_), unsigned(normal), game_, state_,
        r ? r->loop : 0);
  if (!normal || !r || r->finished)
    return;
  // 只延后稳定入口；不在 Restore owner 仍在内层时做绑定/生命观察。
  if (r->defer_through < r->loop)
    r->defer_through = r->loop;
}
} // namespace game
} // namespace cs
