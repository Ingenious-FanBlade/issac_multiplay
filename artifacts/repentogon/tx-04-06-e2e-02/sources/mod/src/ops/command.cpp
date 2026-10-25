#include "binding/offsets.h"
#include "game/context.h"
#include "game/tick.h"
#include "game/topology.h"
#include "runtime.h"
#include <charconv>
#include <cstring>
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
      strncpy_s(reply.text, text.c_str(), _TRUNCATE);
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
      Trace("counters", "%s", counter_snapshot.c_str());
    } catch (...) {
      Trace("status_error", "counter_exception");
    }
  }
}
void RunExit() noexcept {
  game::StopSession(core::SessionCause::RunTeardown);
  // 清掉旧 run 的排队变更，避免返回菜单后旧 host 命令复活会话。
  Mutation ignored{};
  while (mutations.Pop(ignored)) {
  }
  pending_host_start = pending_solo_start = false;
  Trace("state", "to=%s reason=run_exit frame=%llu", game::StateName(), frame);
  Trace("run_exit", "state=%s pending_mutations=0 resident=1",
        game::StateName());
}
} // namespace cs
