#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/client/session/client.h"
#include "game/client/scene/client_scene.h"
#include "game/client/isolation/client_isolation.h"
#include "game/common/context.h"
#include "game/authority/lifecycle.h"
#include "game/authority/lobby.h"
#include "game/authority/persistence.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/participants/topology.h"
#include "game/observation/world_bridge.h"
#include "ops/world_commands.h"
#include "core/capture_cost.h"
#include "world/common/frame_read_regions.h"
#ifdef ISAACCS_E2E
#include "net/live.h"
#include "ops/e2e_ui.h"
#include "replica/stream_e2e.h"
#include "game/tests/client_entity_e2e.h"
#include "game/tests/client_player_e2e.h"
#include "game/tests/e09_compare.h"
#endif
#include "replica/codec.h"
#include "runtime.h"
#include <algorithm>
#include <charconv>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
namespace cs {
namespace {
// 状态变更只排队，在下一次最外层 Manager 入口统一执行。
struct Mutation {
  uint64_t id;
  enum class Kind {
    Host,
    StopHost,
    SoloStart,
    SoloStop,
    Client,
    LobbyRole,
    LobbyReady,
    LobbyStart,
    Role,
    WorldStart,
    WorldStop,
    WorldCheckpoint
  } kind;
  unsigned port;
  int character;
  std::uint32_t generation = 0;
  char label[65]{};
  char address[16]{};
  bool lobby = false;
};
Ring<Mutation, 64> mutations;
Ring<PortRequest, 32> port_requests;
Ring<PortReply, 32> port_replies;
std::atomic<uint64_t> next_id{1}, queue_dropped{};
uint64_t frame = 0;
unsigned frame_depth = 0;
bool pending_host_start = false, pending_solo_start = false;
bool pending_client_start = false, native_frame = true;
bool pending_world_start = false;
std::uint32_t world_generation = 0;
std::uint64_t pending_world_request = 0;
bool started = false;
std::mutex snapshot_lock;
std::string status_snapshot =
    "ok=1\nstatus_version=1\nstate=Disabled\nreason=initializing\n";
std::string counter_snapshot;
std::string last_error;
// 界面按排队编号领取最近一次大厅操作的结果，避免被无关命令清空通用错误。
std::uint64_t lobby_completed = 0;
std::string lobby_error;
replica::stream::Metrics command_metrics;
std::string SessionStateName() {
  for (auto c : Capabilities)
    if (!c)
      return "Disabled";
  if (game::ClientBusy()) return game::ClientStateName();
  return game::StateName();
}
// 所有来源共用参数解析；变更入队，status/counters 读帧尾快照，witness
// 只读当前主线程边界。
std::string Execute(std::string_view text, const char *source) {
#ifdef ISAACCS_E2E
  // 本方面板的固定UI前缀不计正式IPC；其余命令仍从校验/解析之前完整计时。
  constexpr std::string_view ui_prefix="cs e2e_ui";
  const bool ui_call=text.substr(0,ui_prefix.size())==ui_prefix&&
      (text.size()==ui_prefix.size()||text[ui_prefix.size()]==' '||text[ui_prefix.size()]=='\t');
  std::optional<replica::stream::Measure> command_measure;
  if(!ui_call)command_measure.emplace(command_metrics,replica::stream::CpuStage::Ipc);
#else
  replica::stream::Measure command_measure(command_metrics,replica::stream::CpuStage::Ipc);
#endif
  if (text.size() > 512 ||
      text.find_first_of("\r\n\0", 0, 3) != std::string_view::npos)
    return "ok=0\nerror=invalid_line\n";
  std::istringstream in{std::string(text)};
  std::string prefix, verb, arg, extra;
  in >> prefix >> verb;
#ifdef ISAACCS_E2E
  // UI刷新只有自有值/队列，不计入正式IPC阶段，也不从Render执行世界命令。
  if(prefix=="cs"&&verb=="e2e_ui")return E2eUiCommand(in);
#endif
  std::string result;
  if (prefix != "cs")
    result = "ok=0\nerror=unknown_prefix\n";
  else if (verb == "echo") {
    in >> arg >> extra;
    result = arg.empty() || !extra.empty() ? "ok=0\nerror=arguments\n"
                                           : "ok=1\necho=" + arg + "\n";
  } else if (verb == "observe") {
    in >> arg >> extra;
    result=extra.empty()?ObservationCommand(arg):"ok=0\nerror=arguments\n";
  }
#ifdef ISAACCS_E2E
  else if(verb=="stream_test"){
    try {
    std::string token,mode,role,tail;std::uint64_t x=0,y=0;in>>token>>mode;
    if(!IsMain()||game::TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token)result="ok=0\nerror=stream_test_token_or_context\n";
    else if(mode=="on"||mode=="off"){in>>tail;if(!tail.empty())result="ok=0\nerror=arguments\n";else{replica::stream::E2ETrace(mode=="on");result="ok=1\n";}}
    else if(mode=="e09"&&(in>>role)&&(role=="on"||role=="off")&&!(in>>tail)){
      replica::stream::E2ECompareTrace(role=="on");
      game::E09Enable(role=="on");
      result="ok=1\n";
    }
    else if(mode=="trace"&&(in>>x)&&!(in>>tail))result=replica::stream::E2ETracePage(x);
    else if(mode=="clock"&&!(in>>tail))result="ok=1\nlocal_us="+std::to_string(replica::stream::MetricMicros())+"\n"+E2eUiFields();
    else if(mode=="visual"&&(in>>role)&&(role=="on"||role=="off")&&!(in>>tail)){replica::stream::E2EVisual(role=="on");result="ok=1\nvisual_neutral_only=1\n";}
    else if(mode=="wire"&&(in>>role)&&!(in>>tail)){replica::stream::E2EWireArm(role);result="ok=1\n";}
    else if(mode=="wire-status"&&!(in>>tail))result=replica::stream::E2EWireStatus();
    else if(mode=="hello"&&(in>>x)&&!(in>>tail))result=replica::stream::E2EHelloPackets(x,net::live::E2EHandshakeCapabilities());
    else if(mode=="observation-loss"&&!(in>>tail)){
      // 只损伤诊断，不撤销来源、Scene或网络；E18仍须看到后续真实提交推进。
      replica::stream::E2EObservationLoss();game::ClientIsolationObserve(game::ClientIsolationKind::ObservationLost,"e2e_observation_loss");result="ok=1\nobservation_loss_notified=1\n";
    }
    else result="ok=0\nerror=stream_test_arguments\n";
    }catch(const std::exception &e){result="ok=0\nerror="+std::string(e.what())+"\n";}catch(...){result="ok=0\nerror=stream_test_diagnostic_allocation\n";}
  }
  else if (verb == "world_test") {
    std::string token, mode, generation_text, value_text, trailing;
    in >> token >> generation_text >> mode >> value_text >> trailing;
    std::uint32_t generation = 0, value = 0;
    const auto parse = [](const std::string &s, std::uint32_t &v) {
      const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
      return !s.empty() && r.ec == std::errc{} && r.ptr == s.data() + s.size();
    };
    result = !trailing.empty() || !parse(generation_text, generation) ||
                     !parse(value_text, value)
                 ? "ok=0\nerror=arguments\n"
                 : TestWorldCommand(source, token, generation, mode, value);
  } else if (verb == "client_test") {
    std::string token,mode,label,trailing;in>>token>>mode>>label>>trailing;
    if(mode=="fault"&&trailing.empty())result=game::ClientEntityTestArm(token,label);
    else if(mode=="player-fault"&&trailing.empty())result=game::ClientPlayerTestArm(token,label);
    else if(mode=="player-fault-status"&&label.empty())result=game::ClientPlayerTestStatus(token);
    else if(mode=="fault-status"&&label.empty())result=game::ClientEntityTestStatus(token);
    else if(mode=="exercise"&&label=="native"&&trailing.empty())result=game::ClientTestExerciseNative(token);
    else if(mode=="exercise"&&label=="ownership"&&trailing.empty())result=game::ClientTestExerciseOwnership(token);
    else if(mode=="exercise"&&label=="vm"&&trailing.empty())result=game::ClientTestExerciseVm(token);
    else result="ok=0\nerror=client_test_arguments\n";
  }
#endif
  else if (verb == "client" || verb == "lobby_join") {
    // 地址和设备key只作连接配置；Client不在本机创建Player或载入Continue。
    std::string address, port_text, key_text, trailing;
    in >> address >> port_text >> key_text >> trailing;
    unsigned port=0;int key=0;
    const auto p=std::from_chars(port_text.data(),port_text.data()+port_text.size(),port);
    bool valid=!port_text.empty()&&p.ec==std::errc{}&&p.ptr==port_text.data()+port_text.size()&&port>0&&port<=65535&&
               !address.empty()&&address.size()<16&&address.find_first_not_of("0123456789.")==std::string::npos&&trailing.empty();
    if(!key_text.empty()) {const auto k=std::from_chars(key_text.data(),key_text.data()+key_text.size(),key);
      valid=valid&&k.ec==std::errc{}&&k.ptr==key_text.data()+key_text.size()&&key>=0&&key<=255;}
    if(!valid)result="ok=0\nerror=arguments\n";
    else if(SessionStateName()=="Disabled")result="ok=0\nerror=capabilities_unavailable\n";
    else if(game::Busy()||game::ClientBusy()||pending_host_start||pending_solo_start||pending_client_start||WorldCommandBusy()||pending_world_start)
      result="ok=0\nerror=busy\n";
    else if(game::RunActive())result="ok=0\nerror=client_requires_stable_menu\n";
    else {
      Mutation m{next_id.fetch_add(1),Mutation::Kind::Client,port,key};strcpy_s(m.address,address.c_str());
      m.lobby = verb == "lobby_join";
      if(mutations.Push(m)){pending_client_start=true;result="ok=1\nqueued="+std::to_string(m.id)+"\n";}
      else {++queue_dropped;result="ok=0\nerror=queue_full\n";}
    }
  } else if (verb == "world") {
    std::string action, label, trailing;
    in >> action >> label >> trailing;
    const bool labeled = action == "start" || action == "checkpoint";
    // label 是目录叶子/场景名，拒绝路径、控制字符及 Windows 尾部点别名。
    const bool label_ok = world::ValidWorldLabel(label);
    if (!trailing.empty() || (labeled ? !label_ok : !label.empty()))
      result = "ok=0\nerror=arguments\n";
    else if (action == "status") {
      result = pending_world_start
                   ? "ok=1\nworld_status_version=1\nworld_state="
                     "Preparing\nphase=Queued\nrequest=" +
                         std::to_string(pending_world_request) +
                         "\ngeneration=" + std::to_string(world_generation) +
                         "\nrecording=" + WorldRecordingId(world_generation) +
                         "\n"
                   : WorldCommandStatus();
    } else if (action != "start" && action != "stop" && action != "checkpoint")
      result = "ok=0\nerror=unknown_world_verb\n";
    else if (action == "start" && (WorldCommandBusy() || pending_world_start || game::ClientBusy() || pending_client_start))
      result = "ok=0\nerror=busy\n";
    else if (action == "start" && WorldStartError())
      result = "ok=0\nerror=" + std::string(WorldStartError()) + "\n";
    else if (action == "start" &&
             world_generation == (std::numeric_limits<std::uint32_t>::max)())
      result = "ok=0\nerror=recording_generation_exhausted\n";
    else if (action == "checkpoint" && !WorldCanCheckpoint())
      result = "ok=0\nerror=world_not_recording\n";
    else {
      Mutation mutation{};
      mutation.id = next_id.fetch_add(1);
      mutation.kind = action == "start"  ? Mutation::Kind::WorldStart
                      : action == "stop" ? Mutation::Kind::WorldStop
                                         : Mutation::Kind::WorldCheckpoint;
      mutation.generation = world_generation + (action == "start" ? 1u : 0u);
      if (labeled)
        strcpy_s(mutation.label, label.c_str());
      if (mutations.Push(mutation)) {
        if (action == "start") {
          pending_world_start = true;
          world_generation = mutation.generation;
          pending_world_request = mutation.id;
        }
        // queued 只是 admission；录制启动/标记落盘必须从后续快照与 artifact
        // 确认。
        result = "ok=1\nqueued=" + std::to_string(mutation.id) +
                 "\nrequest=" + std::to_string(mutation.id) +
                 "\ngeneration=" + std::to_string(mutation.generation) +
                 "\nrecording=" + WorldRecordingId(mutation.generation) +
                 "\nworld_state=" +
                 (action == "start" ? std::string("Preparing")
                  : action == "stop"
                      ? std::string(WorldCommandBusy() || pending_world_start
                                        ? "Closing"
                                        : "Closed")
                      : std::string("Recording")) +
                 "\n";
      } else {
        ++queue_dropped;
        result = "ok=0\nerror=queue_full\n";
      }
    }
  } else if (verb == "query_checkpoint") {
    in >> arg >> extra;
    // 场景标签限制为短的单个词；只发布主线程已有统计，不发起原生查询、
    // Submit/Resolve，不清桶、不改变输入或参与者状态。
    const bool label_ok =
        !arg.empty() && arg.size() <= 64 && extra.empty() &&
        arg.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV"
                              "WXYZ0123456789_.-") == std::string::npos;
    if (!label_ok)
      result = "ok=0\nerror=arguments\n";
    else if (auto *r = game::Runtime()) {
      game::PublishQueryCoverage(*r, arg.c_str());
      result = "ok=1\nquery_checkpoint=" + arg +
               "\nquery_snapshot=" + std::to_string(r->query_snapshots) +
               "\nsession_generation=" +
               std::to_string(game::SessionGeneration(*r)) + "\n";
    } else
      result = "ok=0\nerror=no_session\n";
  } else if (verb == "role") {
    // 角色只能在该远端第一次Join之前选择；仍在Manager队列执行，不绕过原生工厂。
    in >> arg >> extra;
    std::string trailing;
    in >> trailing;
    unsigned ordinal = 0;
    int character = -1;
    const auto a =
        std::from_chars(arg.data(), arg.data() + arg.size(), ordinal);
    const auto b =
        std::from_chars(extra.data(), extra.data() + extra.size(), character);
    if (arg.empty() || extra.empty() || !trailing.empty() ||
        a.ec != std::errc{} || b.ec != std::errc{} ||
        a.ptr != arg.data() + arg.size() ||
        b.ptr != extra.data() + extra.size() || ordinal < 1 ||
        ordinal >= core::kMaximumParticipants || character < 0)
      result = "ok=0\nerror=arguments\n";
    else {
      Mutation mutation{next_id.fetch_add(1), Mutation::Kind::Role, ordinal,
                        character};
      result = mutations.Push(mutation)
                   ? "ok=1\nqueued=" + std::to_string(mutation.id) + "\n"
                   : "ok=0\nerror=queue_full\n";
    }
  } else if (verb == "lobby_role" || verb == "lobby_ready" || verb == "lobby_start") {
    in >> arg >> extra;
    unsigned value = 0;
    const auto parsed = std::from_chars(arg.data(), arg.data()+arg.size(), value);
    const bool number = !arg.empty() && parsed.ec == std::errc{} && parsed.ptr == arg.data()+arg.size();
    const bool valid = extra.empty() && (verb == "lobby_start" ? arg.empty() :
        number && (verb == "lobby_ready" ? value <= 1 : core::LobbyCharacter(value)));
    if (!valid) result = "ok=0\nerror=arguments\n";
    else {
      const auto kind = verb == "lobby_start" ? Mutation::Kind::LobbyStart :
          verb == "lobby_ready" ? Mutation::Kind::LobbyReady : Mutation::Kind::LobbyRole;
      const Mutation mutation{next_id.fetch_add(1), kind, value, 0};
      if (mutations.Push(mutation)) result="ok=1\nqueued="+std::to_string(mutation.id)+"\n";
      else {++queue_dropped;result="ok=0\nerror=queue_full\n";}
    }
  } else if (verb == "host" || verb == "lobby_host" || verb == "stop" || verb == "solo") {
    std::string trailing;
    in >> arg >> extra >> trailing;
    unsigned port = Settings.host_port;
    int character = 0;
    bool valid = trailing.empty();
    const bool solo = verb == "solo";
    const bool host = verb == "host" || verb == "lobby_host";
    const bool start_solo = solo && arg == "start";
    if (host && (verb != "lobby_host" || !arg.empty())) {
      auto parsed = std::from_chars(arg.data(), arg.data() + arg.size(), port);
      valid = valid && !arg.empty() && parsed.ec == std::errc{} &&
              parsed.ptr == arg.data() + arg.size() && port > 0 &&
              port <= 65535;
      if (!extra.empty() && verb == "lobby_host") valid = false;
      else if (!extra.empty()) {
        auto role = std::from_chars(extra.data(), extra.data() + extra.size(),
                                    character);
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
    else if ((host || start_solo) && SessionStateName() == "Disabled")
      result = "ok=0\nerror=capabilities_unavailable\n";
    else if (((host || start_solo) &&
              (game::Busy() || game::ClientBusy() || pending_client_start || pending_host_start || pending_solo_start)) ||
             (solo && !start_solo && (game::HostBusy() || pending_host_start)))
      result = "ok=0\nerror=busy\n";
    else if (start_solo && !game::RunActive())
      result = "ok=0\nerror=not_in_run\n";
    else {
      const auto kind = solo ? (start_solo ? Mutation::Kind::SoloStart
                                           : Mutation::Kind::SoloStop)
                             : (host ? Mutation::Kind::Host
                                               : Mutation::Kind::StopHost);
      Mutation mutation{next_id.fetch_add(1), kind, port, character};
      mutation.lobby = verb == "lobby_host";
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
  } else if(verb=="stream") {
    std::string target;unsigned page=0;in>>target;
    if(!(in>>page)&&!in.eof())result="ok=0\nerror=stream_page\n";
    else if(page>=unsigned(replica::stream::CpuStage::Count)||(in.clear(),bool(in>>extra)))result="ok=0\nerror=arguments\n";
    else if(target=="reload"){
      // 只更新下次会话的流配置；运行中的输入、Hook和其他设置不热替换。
      if(game::Busy()||game::ClientBusy()||game::TopOwner()!=Owner::Manager)result="ok=0\nerror=stream_configuration_in_use\n";
      else{
        // 归还进程共享缓存的租约，让空闲重载仍可配置三池预算。
        const auto next=[]{
          world::SetFrameReadRegionCache(false);
          struct RestoreReadCache {~RestoreReadCache(){world::SetFrameReadRegionCache(true);}} restore;
          return ReadConfig();
        }();
        if(!next.enabled)result="ok=0\nerror="+next.reason+"\n";
        else{Settings.stream=next.stream;Settings.peer_timeout_ms=next.peer_timeout_ms;result="ok=1\n"+Settings.stream.Fields("stream.requested.");}
      }
    }
    else if(target=="client")result=game::ClientStreamFields(page);
    else if(target=="source")result=game::SourceStreamFields(page);
    else if(target=="main")result="ok=1\n"+capture::CostFields(page)+(page?std::string{}:world::FrameReadRegionFields());
    else if(target=="ipc")result="ok=1\n"+command_metrics.Fields("stream.ipc.",page);
    else if(target=="config")result="ok=1\n"+Settings.stream.Fields("stream.requested.");
    else if(target=="1"||target=="2"||target=="3")result=game::HostStreamFields(unsigned(target[0]-'0'),page);
    else result="ok=0\nerror=stream_target\n";
  } else {
    in >> extra;
    if (!extra.empty())
      result = "ok=0\nerror=arguments\n";
    else if (verb == "version")
      result = std::string("ok=1\ncommand_version=1\ndll_version=") + Version +
               "\nrepentogon_version=" + RepentogonVersion + "\n"
#ifdef ISAACCS_E2E
               +"e2e_ui_available="+std::to_string(Settings.e2e_ui_enabled)+"\n"
#endif
               ;
    else if (verb == "ui_perf") {
      // 游戏内 HUD 读取固定大小的累计值，不访问游戏对象或启用详细诊断。
      const auto host=game::LiveCaptureSession();
      if(game::ClientBusy())result=game::ClientHudTimingFields();
      else if(host){
        const auto capture_timing=capture::ReadCaptureTiming();
        result="ok=1\nrole=Host\nepoch="+std::to_string(host)+"\ncapture_n="+
               std::to_string(capture_timing.count)+"\ncapture_us="+std::to_string(capture_timing.total_us)+"\n";
      }else result="ok=1\nrole=Off\n";
    } else if (verb == "lobby_status") {
      game::ClientUiHeartbeat();
      result = game::LobbyFields()+"default_port="+std::to_string(Settings.host_port)+
          "\nstate="+SessionStateName()+"\ncommand_error="+last_error+
          "\nlobby_completed="+std::to_string(lobby_completed)+"\nlobby_error="+lobby_error+"\n";
    } else if (verb == "status" || verb == "counters" || verb == "ui_status") {
      if(verb=="ui_status")game::ClientUiHeartbeat();
      std::lock_guard lock(snapshot_lock);
      result = verb == "counters" ? counter_snapshot : status_snapshot;
    } else if (verb == "witness") {
      // Lua 在 Shooting 内观测激光时不能使用上一轮 status_snapshot 的 frame。
      // 此接口只复制当前主线程的编号/代次/阻断值，不读取蓄力或写入任何游戏状态。
      const auto *r = game::Runtime();
      const auto owner = game::TopOwner();
      result = "ok=1\nwitness_version=1\nframe=" + std::to_string(frame) +
               "\nsimulation_tick=" +
               std::to_string(r ? r->pipeline.SimulationTick() : 0) +
               "\nsession_generation=" +
               std::to_string(r ? game::SessionGeneration(*r) : 0) +
               "\na_generation=" +
               std::to_string(r ? r->players[0].Generation() : 0) +
               "\nb_generation=" +
               std::to_string(r ? r->players[1].Generation() : 0) +
               "\nblocks=" + std::to_string(r ? r->blocks : 0) +
               "\nowner=" + std::to_string(unsigned(owner)) +
               "\nowner_token=" + std::to_string(game::OwnerToken(owner)) +
               "\n";
    } else
      result = "ok=0\nerror=unknown_verb\n";
  }
  // 记录所有命令来源及 Q14 的 Game 指针；cs 不进入原生命令分支。
  // status/counters 的完整响应走控制口；trace 只保留响应长度与结果，避免
  // 大快照挤入单个 3500 字节事件被截断。其它命令继续保留原始结果。
  if (verb == "ui_status" || verb == "ui_perf" || verb == "lobby_status" || verb == "e2e_ui") {
    // UI只读刷新不是逐tick诊断，不向生产日志写每帧状态查询明细。
  } else if (verb == "status" || verb == "counters" || verb == "stream")
    Trace("command",
          "source=%s text=%.*s result_logged=0 result_bytes=%zu "
          "result_ok=%u g_game=%p frame=%llu",
          source, int(text.size()), text.data(), result.size(),
          unsigned(result.rfind("ok=1\n", 0) == 0), GamePointer(), frame);
  else
    Trace("command", "source=%s text=%.*s result=%s g_game=%p frame=%llu",
          source, int(text.size()), text.data(), result.c_str(), GamePointer(),
          frame);
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
    std::string counters =
        Settings.diagnostics
            ? "ok=1\ndiagnostics_enabled=1\n"
            : "ok=1\ndiagnostics_enabled=0\ncounters_available=0\n";
    size_t n;
    auto hooks = Hooks(n);
    unsigned registered = 0;
    uint64_t foreign = 0, total = 0;
    for (size_t i = 0; i < n; ++i) {
      auto &h = *hooks[i];
      registered += h.registered;
      foreign += h.foreign.load();
      total += h.calls.load();
      if (Settings.diagnostics)
        counters += "hook." + std::string(h.id) + "=" +
                    std::to_string(h.calls.load()) + "\nhook." + h.id +
                    ".foreign=" + std::to_string(h.foreign.load()) + "\nhook." +
                    h.id + ".completed=" + std::to_string(h.completed.load()) +
                    "\n";
    }
    if (Settings.diagnostics)
      counters += PointerCounters();
    auto *manager = *reinterpret_cast<unsigned char **>(Base + offset::Manager);
    const bool starting =
        manager && *reinterpret_cast<bool *>(manager + offset::Starting);
    auto *game = static_cast<unsigned char *>(GamePointer());
    const bool run =
        game && *reinterpret_cast<bool *>(game + offset::RunActive);
    std::string s =
        "ok=1\nstatus_version=1\nstate=" + SessionStateName() +
        "\nframe=" + std::to_string(frame) + "\ng_game=" + (game ? "1" : "0") +
        "\nrun_active=" + (run ? "1" : "0") +
        "\ndiagnostics_enabled=" + (Settings.diagnostics ? "1" : "0") +
        "\nworld_record_enabled=" +
        (Settings.world_record_enabled ? "1" : "0") +
        "\nrepeat_topology_reads=" +
        (Settings.repeat_topology_reads ? "1" : "0") +
        "\nmotion_interpolation=" + (Settings.motion.interpolation ? "1" : "0") +
        "\nmotion_extrapolation=" + (Settings.motion.extrapolation ? "1" : "0") +
        "\nmotion_prediction=" + (Settings.motion.prediction ? "1" : "0") +
        "\nmotion_projectile_lead=" + (Settings.motion.projectile_lead ? "1" : "0") +
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
    s += TraceFields();
    s += MenuStatus(run);
    s += game::StatusFields();
    s += game::ClientFields();
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
  world::InvalidateFrameReadRegions(world::FrameRegionReset::Frame);
  ++frame;
  if (frame <= 3)
    Trace("frame_begin", "frame=%llu", frame);
  if (!started) {
    started = true;
    RefreshVersion();
    InstallRuntimePatches();
    Trace("runtime_version", "repentogon=%s", RepentogonVersion.c_str());
  }
  // world 窗口必须先于 BeginManager 的网络/lifecycle 和排队命令建立。
  native_frame = !game::ClientBusy();
  if (native_frame) {
    game::WorldFrameBegin(frame);
    game::BeginManager(frame);
  } else game::BeginContextFrame();
  PumpWorldCommands();
  try {
    // N 帧收到的变更在 N+1 帧入口执行，下面取出的端口请求不能抢跑。
    Mutation m{};
    for (unsigned i = 0; i < 64 && mutations.Pop(m); ++i) {
      const std::string before = SessionStateName();
      last_error.clear();
      const bool lobby_action = m.lobby || m.kind == Mutation::Kind::LobbyStart ||
          m.kind == Mutation::Kind::StopHost;
      // 关闭请求已在 main 捕获后，旧会话启动命令不能在延后的原生循环中复活。
      if (WorldProcessExitPending() && (m.kind == Mutation::Kind::Host ||
                                        m.kind == Mutation::Kind::Client ||
                                        m.kind == Mutation::Kind::LobbyStart ||
                                        m.kind == Mutation::Kind::SoloStart)) {
        if (m.kind == Mutation::Kind::Host) pending_host_start = false;
        if (m.kind == Mutation::Kind::Client) pending_client_start = false;
        if (m.kind == Mutation::Kind::SoloStart) pending_solo_start = false;
        if (lobby_action) {
          lobby_completed = m.id;
          lobby_error = "process_exiting";
        }
        continue;
      }
      switch (m.kind) {
      case Mutation::Kind::Host:
        pending_host_start = false;
        {
          net::WorkerConfig config;
          config.port = static_cast<uint16_t>(m.port);
          config.bind_address = Settings.host_bind_address;
          config.peer_timeout_ms = Settings.peer_timeout_ms;
          config.stream = Settings.stream;
          config.heartbeat_ms = unsigned(config.stream.heartbeat_ms);
          config.lobby = m.lobby;
          last_error = game::ClientBusy() ? "busy" : game::StartHost(config, m.character);
        }
        break;
      case Mutation::Kind::Role:
        if (auto *r = game::Runtime();
            r && r->mode == core::SessionMode::Host && !r->retiring &&
            r->lobby.roster.phase == core::LobbyPhase::Off &&
            !r->Remote(m.port).retired && !r->Remote(m.port).retiring &&
            !r->Remote(m.port).join.passes && !r->Remote(m.port).join.scope)
          r->Remote(m.port).join.character = m.character;
        else
          last_error = "role_join_already_started_or_no_host";
        break;
      case Mutation::Kind::StopHost:
        if(game::ClientBusy())game::StopClient();else game::StopSession();
        break;
      case Mutation::Kind::Client:
        pending_client_start=false;
        last_error=WorldCommandBusy()?"busy":game::StartClient(m.address,m.port,m.character,m.lobby);
        break;
      case Mutation::Kind::LobbyRole:
        last_error=game::SelectLobbyCharacter(m.port);
        break;
      case Mutation::Kind::LobbyReady:
        last_error=game::SetLobbyReady(m.port != 0);
        break;
      case Mutation::Kind::LobbyStart:
        last_error=game::StartLobbyGame();
        break;
      case Mutation::Kind::SoloStart:
        pending_solo_start = false;
        last_error = game::ClientBusy() ? "busy" : game::StartSolo();
        break;
      case Mutation::Kind::SoloStop:
        if (game::HostBusy())
          last_error = "busy";
        else
          game::StopSolo();
        break;
      case Mutation::Kind::WorldStart:
        pending_world_start = false;
        StartWorldCommand(m.id, m.generation, m.label);
        break;
      case Mutation::Kind::WorldStop:
        StopWorldCommand(m.id, m.generation);
        break;
      case Mutation::Kind::WorldCheckpoint:
        CheckpointWorldCommand(m.id, m.generation, m.label);
        break;
      }
      if (lobby_action) {
        lobby_completed = m.id;
        lobby_error = last_error;
        if (m.lobby && m.kind == Mutation::Kind::Host && !lobby_error.empty())
          Trace("lobby_open_error", "id=%llu port=%u reason=%s", m.id, m.port, lobby_error.c_str());
      }
      Trace("state",
            "id=%llu from=%s to=%s port=%u character=%d frame=%llu error=%s",
            m.id, before.c_str(), SessionStateName().c_str(), m.port, m.character, frame,
            last_error.c_str());
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
        Trace("command_error",
              "response_too_large id=%llu bytes=%zu capacity=%zu", request.id,
              text.size(), sizeof(reply.text));
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
  game::PumpLobbyStart();
}
void FrameEnd(bool completed) noexcept {
  if (!frame_depth || --frame_depth != 0)
    return;
  // 原生异常展开只归还深度，不发布未完成帧的状态快照。
  if(native_frame) {
    // 根尾证言只覆盖本次 Manager 尾部，封存和时钟提交之间不能调用原生更新、Lua 或控制命令。
    game::WorldPreparePostState(completed);
    game::EndManager(completed);
    game::WorldFrameEnd();
  }
  PumpWorldCommands();
  if (!completed)
    return;
#ifdef ISAACCS_E2E
  E2eUiManagerReturned();
#endif
  PublishStatus();
  if (frame <= 3)
    Trace("frame_end", "frame=%llu", frame);
  if (Settings.diagnostics && frame % 60 == 0) {
    try {
      std::lock_guard lock(snapshot_lock);
      // 按换行分片，每片最多 2500 字节；frame/part/last/total_bytes
      // 可重组快照。
      constexpr size_t chunk_limit = 2500;
      size_t offset = 0;
      unsigned part = 0;
      while (offset < counter_snapshot.size()) {
        size_t length =
            (std::min)(chunk_limit, counter_snapshot.size() - offset);
        if (offset + length < counter_snapshot.size()) {
          const auto newline =
              counter_snapshot.rfind('\n', offset + length - 1);
          if (newline != std::string::npos && newline >= offset)
            length = newline - offset + 1;
        }
        Trace("counters", "frame=%llu part=%u last=%u total_bytes=%zu\n%.*s",
              frame, ++part,
              unsigned(offset + length == counter_snapshot.size()),
              counter_snapshot.size(), int(length),
              counter_snapshot.data() + offset);
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
  // world 独立于 run/Host；保存其队列顺序，run teardown 只清会话命令。
  std::array<Mutation, 64> world_pending{};
  size_t count = 0;
  Mutation ignored{};
  while (mutations.Pop(ignored)) {
    if (ignored.kind == Mutation::Kind::WorldStart ||
        ignored.kind == Mutation::Kind::WorldStop ||
        ignored.kind == Mutation::Kind::WorldCheckpoint)
      world_pending[count++] = ignored;
  }
  for (size_t i = 0; i < count; ++i)
    mutations.Push(world_pending[i]);
  pending_host_start = pending_solo_start = pending_client_start = false;
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
namespace {
thread_local bool hourglass_room=false;
}
HourglassRoomScope::HourglassRoomScope(const void *transition) noexcept:previous_(hourglass_room) {
  const auto *r=Runtime();
  int animation=0;
  if(IsMain()&&r&&r->mode==core::SessionMode::Host&&r->run_observed&&!r->retiring&&!r->finished&&!r->failure&&
     HasOwner(Owner::Door)&&transition&&ReadMemory(static_cast<const std::byte *>(transition)+0x1c,&animation,sizeof(animation))&&animation==12)
    hourglass_room=true;
}
HourglassRoomScope::~HourglassRoomScope() noexcept {hourglass_room=previous_;}
bool HourglassRoomActive() noexcept {return hourglass_room;}
bool HourglassRestoreActive() noexcept {return hourglass_room&&HasOwner(Owner::Restore);}

// tx-04-07：一次 Restore 决策同时处理两个停止点。保留条件必须同时满足，
// 不能只凭 Warmup 或 startGame 豁免已有 run。
RestoreScope::RestoreScope(void *game_object, void *state,
                           bool start_game) noexcept
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
  if(host_mode&&state_&&HourglassRoomActive()){
    // 这是原生同局回溯，远端身份保持，旧输入／玩家绑定照常撤销。
    preserve_=true;
    InvalidateBinding(*r,"hourglass_restore",true);
    ClearPendingRunMutations();
#ifdef ISAACCS_E2E
    if(replica::stream::E2ETraceEnabled())Trace("e39-restore","phase=enter loop=%llu",r->loop);
#endif
    return;
  }
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
    // 本轮资格只用于这一次加载；保留 worker、握手/session identity 与菜单
    // Host。
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
  Trace("restore",
        "event=return preserve=%u normal=%u game=%p state=%p loop=%llu",
        unsigned(preserve_), unsigned(normal), game_, state_, r ? r->loop : 0);
  if (!normal || !r || r->finished)
    return;
  // 只延后稳定入口；不在 Restore owner 仍在内层时做绑定/生命观察。
  if (r->defer_through < r->loop)
    r->defer_through = r->loop;
}
} // namespace game
} // namespace cs
