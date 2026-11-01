#include "game/client.h"
#include "game/client_scene.h"
#include "binding/offsets.h"
#include "game/context.h"
#include "game/input_a.h"
#include "game/tick.h"
#include "game/topology.h"
#include "net/live.h"
#include "ops/world_commands.h"
#include "runtime.h"
#include <algorithm>
#include <exception>
#include <memory>

namespace cs::game {
namespace {
enum class State { Off, Connecting, AwaitingWorld, Retiring, Failed };
// 输入目标只投影到预计抵达服务器的时刻；不推进本地/Authority时钟，不放行过期命令。
// 速率由服务器实际提交tick与同机单调时间的差值建立；暂停/换代立即丢弃旧估计。
struct InputTiming final {
  std::uint64_t anchor_ms=0,anchor_tick=0,rate_ticks=0,rate_ms=0,
                received_ms=0,round_trip_ms=0,publication_age_ms=0;
  void Observe(const net::live::Event &event) noexcept {
    const auto &p=event.progress;
    if(!p.input_enabled||p.session_state!=core::SessionState::Active||!p.generation) {
      *this={};return;
    }
    received_ms=event.received_ms;round_trip_ms=event.round_trip_ms;
    publication_age_ms=event.publication_age_ms;
    if(!anchor_ms||p.server_sample_ms<anchor_ms||p.server_tick<anchor_tick||p.server_sample_ms-anchor_ms>2000) {
      anchor_ms=p.server_sample_ms;anchor_tick=p.server_tick;rate_ms=rate_ticks=0;return;
    }
    const auto elapsed=p.server_sample_ms-anchor_ms,delta=p.server_tick-anchor_tick;
    if(elapsed>=250) {
      // 不猜固定60Hz；异常/长停顿样本不能恢复输入资格。
      if(delta&&delta<=240&&delta*1000<=elapsed*120) {rate_ticks=delta;rate_ms=elapsed;}
      else rate_ms=rate_ticks=0;
      if(elapsed>=1000){anchor_ms=p.server_sample_ms;anchor_tick=p.server_tick;}
    }
  }
  bool Target(std::uint64_t tick,std::uint64_t now,std::uint64_t &target) const noexcept {
    if(!rate_ms||!rate_ticks||now<received_ms||now-received_ms>1000||
       round_trip_ms>1500||publication_age_ms>1000)return false;
    const auto age=now-received_ms+round_trip_ms+publication_age_ms;
    // 使用完整往返时间覆盖下行旧样本和本次上行，不把半程时间误当总时差。
    const auto projection=(age*rate_ticks+rate_ms-1)/rate_ms;
    if(projection>core::input::kMaximumFutureTicks-core::input::kInputLeadTicks||tick>UINT64_MAX-projection)return false;
    return core::NextInputTarget(tick+projection,core::input::InputRuntimeConfig{},target);
  }
};
// 主线程唯一持有的Client控制块；只保存自有输入值，设备地址仅用于同轮临时确认。
struct ClientRuntime final {
  net::live::Client network;
  ClientScene scene;
  State state=State::Connecting;
  replica::Scope scope{};
  net::protocol::Ack progress{};
  unsigned ordinal=0, roster=0;
  int device_key=0;
  std::uint64_t connection=0, sequence=0, input_ack=0, input_ack_tick=0,
                last_target=0, last_tick=0, generation=0, sampled=0, sent=0,
                manager_skipped=0, device_missing=0, render_calls=0;
  unsigned sent_edge_mask=0;
  core::ShootingDirections shooting;
  core::InputStateDraftV2 sample{};
  std::array<unsigned,16> edges{};
  unsigned edge_head=0,edge_count=0;
  unsigned held=0;
  bool baseline=false, eligible=false, sampled_this_loop=false, blocked=true;
  bool physical_neutral=false;
  bool focused=false,ui_blocked=false,input_ready=false;
  InputTiming timing;
  unsigned neutral_loops=0;
  void *source=nullptr;
  std::string address, error;
};
std::unique_ptr<ClientRuntime> client;
std::atomic<bool> client_active{false},foreign_client{false};
// 无服务器原生写入前缀的进程关闭仍属于访客原有本地存档，正常 Shutdown 可透传。
std::atomic<bool> local_shutdown_passthrough{false};
std::uint64_t connections=0;
std::uint64_t ui_seen_ms=0,ui_frames=0;
thread_local unsigned client_depth=0;
thread_local ClientBorrowToken render_observation{};

// 设备/代次/UI变化清除待发边沿，但保留此次物理held基线防止恢复时补发。
void Neutral(ClientRuntime &c,bool forget=false) noexcept {
  c.sample={};c.eligible=false;c.last_target=0;
  c.edge_head=c.edge_count=0;
  if(forget){c.source=nullptr;c.baseline=false;c.held=0;c.shooting={};}
}
void Failure(ClientRuntime &c,const char *reason) noexcept {
  Neutral(c,true);c.input_ready=false;c.state=State::Failed;
  if(c.scene.State()!=ClientSceneState::Failed)c.scene.Fail(reason);
  c.network.Stop();
  try {c.error=reason;Trace("client","event=failed connection=%llu reason=%s",c.connection,reason);}catch(...){}
}
bool Focused() noexcept {
  auto window=GetForegroundWindow();DWORD pid=0;
  if(!window||!GetWindowThreadProcessId(window,&pid))return false;
  return pid==GetCurrentProcessId();
}
// 当前Client角色仅接管稳定的菜单，不触碰访客自己的run或Continue。
bool MenuReady() noexcept {
  auto *manager=*reinterpret_cast<unsigned char **>(Base+offset::Manager);
  bool starting=false;
  return manager&&ClientMenuAvailable()&&!RunActive()&&ReadAt(manager,offset::Starting,starting)&&!starting;
}
}
bool ClientBusy() noexcept {return client_active.load(std::memory_order_acquire);}
const char *ClientStateName() noexcept {
  if(!client)return "Off";
  switch(client->state) {
    case State::Off:return "Off";case State::Connecting:return "Connecting";
    case State::AwaitingWorld:return "AwaitingWorld";case State::Retiring:return "Retiring";
    case State::Failed:return "Failed";
  }
  return "Failed";
}
const char *StartClient(const std::string &ipv4,unsigned port,int device_key) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||client_depth)return "wrong_boundary";
  if(ClientBusy()||Busy())return "busy";
  if(!MenuReady())return "client_requires_stable_menu";
  if(!ui_seen_ms||GetTickCount64()-ui_seen_ms>1000)return "client_ui_unavailable";
  if(port<1||port>65535||device_key<0||device_key>255)return "arguments";
  if(PointerCount()!=8||!ContextHealthy()||!Capabilities[size_t(Group::Input)]||
     !Capabilities[size_t(Group::Loop)]||!Capabilities[size_t(Group::Control)]||
     !Capabilities[size_t(Group::Client)]||!WorldExitPatchInstalled())return "adaptation_unavailable";
  if(connections==UINT64_MAX)return "connection_exhausted";
  try {
    auto next=std::make_unique<ClientRuntime>();next->connection=++connections;
    next->address=ipv4;next->device_key=device_key;
    const auto error=next->network.Start(ipv4,std::uint16_t(port),Settings.peer_timeout_ms);
    if(*error)return error;
    ForgetInputDevice();client=std::move(next);foreign_client=false;
    local_shutdown_passthrough.store(false,std::memory_order_release);
    client_active.store(true,std::memory_order_release);
    Trace("client","event=connecting connection=%llu address=%s port=%u key=%d local_run=0",connections,ipv4.c_str(),port,device_key);
    return "";
  } catch(...) {return "client_allocation";}
}
void StopClient() noexcept {
  if(!IsMain()||!ClientBusy())return;
  if(client->state==State::Retiring)return;
  Neutral(*client,true);client->input_ready=false;client->state=State::Retiring;client->scene.Retire("client_stop");client->neutral_loops=0;client->network.Stop();
  Trace("client","event=retiring connection=%llu",client->connection);
}
// 该入口在FrameScope之后、完整Manager原链之前；从不推进Authority时钟。
bool ClientManagerBoundary() noexcept {
  if(!ClientBusy())return false;
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return true;}
  auto &c=*client;
  if(client_depth){Failure(c,"client_manager_reentrant");return true;}
  ++client_depth;
  try {
    if(c.scene.ConsumeForeignFault())Failure(c,"client_foreign_scene");
    c.scene.ObserveLuaState(ClientLuaStateIdentity());
    if(c.scene.State()==ClientSceneState::Failed&&c.state!=State::Failed)Failure(c,"client_scene_failed");
    if(foreign_client.exchange(false))Failure(c,"client_foreign_manager");
    ++c.manager_skipped;
    net::live::Event event;
    for(unsigned i=0;i<256&&c.network.Poll(event);++i) {
      switch(event.type) {
        case net::live::Event::Type::Welcome:
          if(c.state!=State::Connecting)break;
          c.scope=event.scope;c.scene.Begin(event.scope);
          if(c.scene.State()==ClientSceneState::Failed){Failure(c,"client_scene_begin_failed");break;}
          c.ordinal=event.ordinal;c.state=State::AwaitingWorld;
          Trace("client","event=welcome connection=%llu ordinal=%u session=%llu wire_connection=%llu",c.connection,c.ordinal,c.scope.session,c.scope.connection);break;
        case net::live::Event::Type::Progress:
          if(c.state!=State::AwaitingWorld)break;
          if(c.scope!=event.scope||c.ordinal!=event.ordinal){Failure(c,"client_scope_conflict");break;}
          if(c.generation!=event.progress.generation) {
            Neutral(c,true);c.timing={};c.generation=event.progress.generation;c.last_tick=0;
            Trace("client","event=generation connection=%llu generation=%llu",c.connection,c.generation);
          }
          c.progress=event.progress;c.roster=event.progress.roster_mask;c.timing.Observe(event);break;
        case net::live::Event::Type::InputAck:
          if(c.state!=State::AwaitingWorld||event.scope!=c.scope||event.ordinal!=c.ordinal)break;
          if(event.progress.generation==c.generation&&event.progress.processed>=c.input_ack) {
            if(event.progress.processed>c.sequence){Failure(c,"client_ack_unsent");break;}
            c.input_ack=event.progress.processed;c.input_ack_tick=event.progress.processed_tick;
          }
          break;
        case net::live::Event::Type::Stopped:Failure(c,event.reason);break;
        case net::live::Event::Type::Failed:Failure(c,event.reason);break;
      }
    }
    if(c.state==State::Retiring) {
      // 原生菜单可能在Input更新之前消费缓存；交还前刷新两轮真实松键，不能重放Client按键。
      c.sampled_this_loop=false;c.physical_neutral=false;c.blocked=true;ClientPollInput();
      if(!c.sampled_this_loop||c.physical_neutral)++c.neutral_loops;else c.neutral_loops=0;
      // 只有无原生执行前缀、无借用与在途使用时才能交还本地菜单。
      if(c.network.Finished()&&c.neutral_loops>=2&&MenuReady()&&c.scene.CanReturnToMenu()) {
        c.state=State::Off;ForgetInputDevice();client_active.store(false,std::memory_order_release);
        local_shutdown_passthrough.store(false,std::memory_order_release);
        Trace("client","event=off connection=%llu input_closed=1 worker_finished=1 local_run=0",c.connection);
      }
    } else if(c.state==State::Connecting||c.state==State::AwaitingWorld) {
      c.sampled_this_loop=false;c.focused=Focused();c.ui_blocked=ClientUiBlocked();c.blocked=!c.focused||c.ui_blocked;
      std::uint64_t target=0;
      c.input_ready=!c.blocked&&c.progress.input_enabled&&c.state==State::AwaitingWorld&&
                    c.progress.session_state==core::SessionState::Active&&c.generation&&
                    c.timing.Target(c.progress.server_tick,GetTickCount64(),target);
      ClientPollInput();
      if(!c.sampled_this_loop){++c.device_missing;Neutral(c,true);}
      if(!c.input_ready)
        Neutral(c);
      if(c.eligible&&c.sampled_this_loop&&c.progress.server_tick!=c.last_tick&&target>c.last_target) {
        if(c.sequence==UINT64_MAX)
          Failure(c,"client_input_exhausted");
        else {
          auto sample=c.sample;sample.edge_flags=c.edge_count?c.edges[c.edge_head]:0;
          net::live::Intent intent{sample,c.generation,c.sequence+1,target};
          if(c.network.Send(intent)) {
            ++c.sequence;++c.sent;c.last_target=target;c.last_tick=c.progress.server_tick;c.sample.edge_flags=0;
            c.sent_edge_mask|=sample.edge_flags;
            if(c.edge_count){c.edge_head=(c.edge_head+1)%unsigned(c.edges.size());--c.edge_count;}
            if(Settings.diagnostics)Trace("client_input","connection=%llu ordinal=%u generation=%llu sequence=%llu target=%llu move=%d,%d aim=%d,%d held=%u edge=%u",c.connection,c.ordinal,c.generation,c.sequence,target,intent.state.move_x,intent.state.move_y,intent.state.aim_x,intent.state.aim_y,intent.state.held_flags,intent.state.edge_flags);
          } else Failure(c,"client_input_queue_full");
        }
      }
      if(c.network.Finished()&&c.state!=State::Failed)Failure(c,"client_transport_finished");
    }
  } catch(const std::exception &error){Failure(c,error.what());}
  catch(...){Failure(c,"client_main_exception");}
  --client_depth;
  // 即使本轮刚退休，也到下一轮才允许原生链，避免同轮混合owner。
  return true;
}
bool ClientDeviceReturned(void *device,bool success,unsigned polls,bool poll_ok) {
  if(!ClientBusy())return false;
  if(!client_depth||!IsMain())return true;
  auto &c=*client;int key=-1;
  if(!ReadAt(device,offset::DeviceId,key)||key!=c.device_key)return true;
  if(!success||polls!=1||!poll_ok||DeviceIdOccurrences(key)!=1||DeviceOccurrences(device)!=1) {
    Neutral(c,true);return true;
  }
  PhysicalActions actions{};
  if(!ReadPhysicalActions(device,actions)){Neutral(c,true);return true;}
  if(c.sampled_this_loop){Failure(c,"client_duplicate_device_sample");return true;}
  c.sampled_this_loop=true;++c.sampled;
  c.physical_neutral=true;
  for(unsigned i=0;i<12;++i)if(actions.values[i]!=0||actions.pressed[i]||actions.triggered[i])c.physical_neutral=false;
  const bool same=c.baseline&&c.source==device;
  if(!same){Neutral(c,true);c.shooting={};}
  unsigned held=0;for(unsigned i=0;i<4;++i)if(actions.pressed[8+i])held|=1u<<i;
  core::input::RawInputSample raw{};raw.move_x=actions.values[1]-actions.values[0];raw.move_y=actions.values[3]-actions.values[2];
  raw.held_flags=((held>>1)&7u)<<1;raw.edge_flags=same&&!c.blocked?held&~c.held:0;
  core::InputStateDraftV2 sample{};
  if(!core::CanonicalizeShooting(raw,{actions.values[4],actions.values[5],actions.values[6],actions.values[7]},c.shooting,sample)) {
    Failure(c,"client_noncanonical_sample");return true;
  }
  c.source=device;c.baseline=true;c.held=held;
  // 与Host物理源相同：真实上升沿按轮次进入有界FIFO，重复同键也不位或合并。
  const bool eligible=c.input_ready;
  if(!eligible) {c.edge_head=c.edge_count=0;sample.edge_flags=0;}
  else if(sample.edge_flags) {
    if(c.edge_count==c.edges.size()){Failure(c,"client_edge_queue_full");return true;}
    c.edges[(c.edge_head+c.edge_count)%c.edges.size()]=sample.edge_flags;++c.edge_count;
  }
  c.sample=sample;c.eligible=eligible;return true;
}
void ClientInputFinished() noexcept {
  if(ClientBusy()&&client_depth&&!client->sampled_this_loop)Neutral(*client,true);
}
void ClientInputUnavailable() noexcept {
  if(IsMain()&&ClientBusy())Failure(*client,"client_input_backend_unavailable");
}
void ClientMenuRendered() noexcept {
  if(IsMain()&&ClientBusy())++client->render_calls;
}
// 观察借用跨过完整 Manager::Render super，异常由析构收口；它不允许 Client 原生世界绘制。
ClientRenderScope::ClientRenderScope(bool enabled) noexcept
  :exceptions_(std::uncaught_exceptions()) {
  if(!enabled||!IsMain()||!ClientBusy())return;
  if(render_observation.active){client->scene.Fail("client_render_reentrant");return;}
  client->scene.RenderWindow();
  active_=client->scene.Borrow(ClientPurpose::RenderObservation,render_observation);
}
ClientRenderScope::~ClientRenderScope() noexcept {
  if(!active_||!client)return;
  if(std::uncaught_exceptions()!=exceptions_)client->scene.Fail("client_render_exception");
  client->scene.Release(render_observation);
}
// Lua VM 关闭先换代并撤销本方借用，不能把 unref 当作 userdata 析构。
void ClientLuaClosing() noexcept {
  if(IsMain()&&ClientBusy())client->scene.LuaClosing();
}
void ClientShutdownBeginning() noexcept {
  if(!IsMain()||!ClientBusy())return;
  // 当前仅等待菜单且无原生写入前缀时，Shutdown 的直接 PGD 写仍是访客本地生命周期。
  // 一旦有服务器原生前缀，不能凭下层 PGD Hook 认为 Shutdown 安全。
  local_shutdown_passthrough.store(client->scene.CanReturnToMenu(),std::memory_order_release);
  if(client->state!=State::Retiring)StopClient();
}
void ClientContextClosing() noexcept {
  if(IsMain()&&ClientBusy())client->scene.ContextClosing();
}
// 此分派独立于 PersistenceScope 的 original exactly-once 观察；外层 wrapper 可直接拒绝。
bool ClientPersistenceBlocked() noexcept {
  return client_active.load(std::memory_order_acquire)&&
         !(IsMain()&&local_shutdown_passthrough.load(std::memory_order_acquire));
}
// 自带纯菜单UI实际运行的主线程心跳；未加载UI时拒绝进入不可见等待状态。
void ClientUiHeartbeat() noexcept {
  if(IsMain()){ui_seen_ms=GetTickCount64();if(ClientBusy())++ui_frames;}
}
// 原生主循环关闭前完成本方线程退休；不修改窗口关闭标志，也不调用Game::Exit。
bool ClientProcessExitDecision() noexcept {
  if(!ClientBusy())return true;
  if(client->state!=State::Retiring)StopClient();
  return client->network.Finished()&&client->scene.CanReturnToMenu();
}
std::string ClientFields() {
  std::string s="role="+std::string(ClientBusy()?"Client":HostBusy()?"Host":"Off")+"\nclient_state="+ClientStateName()+"\nclient_world_applied=0\n";
  s+="client_ui_available="+std::string(ui_seen_ms&&GetTickCount64()-ui_seen_ms<=1000?"1":"0")+"\n";
  s+="client.ui_frames="+std::to_string(ui_frames)+"\n";
  if(!client)return s;
  const auto &c=*client;
  const auto add=[&](const char *name,std::uint64_t value){s+="client."+std::string(name)+"="+std::to_string(value)+"\n";};
  add("connection",c.connection);add("participant",c.ordinal);add("roster",c.roster);add("generation",c.generation);
  add("wire_session",c.scope.session);add("wire_connection",c.scope.connection);
  add("server_tick",c.progress.server_tick);add("sent",c.sent);add("sequence",c.sequence);
  add("accepted",c.progress.accepted);add("processed",c.input_ack);add("processed_tick",c.input_ack_tick);
  add("sampled",c.sampled);add("manager_skipped",c.manager_skipped);add("device_missing",c.device_missing);
  // 纯值诊断明确区分焦点/UI屏蔽、服务器资格和输入时序，避免再次仅凭收包猜测。
  add("focused",c.focused);add("ui_blocked",c.ui_blocked);add("input_enabled",c.progress.input_enabled);
  add("input_ready",c.input_ready);add("round_trip_ms",c.timing.round_trip_ms);
  add("clock_rate_ticks",c.timing.rate_ticks);add("clock_rate_ms",c.timing.rate_ms);add("last_target",c.last_target);
  // 采样/发包动作的纯值证据；验收不能拿中性心跳的处理序号冒充物理按键已生效。
  s+="client.sample_move="+std::to_string(c.sample.move_x)+","+std::to_string(c.sample.move_y)+"\n";
  s+="client.sample_aim="+std::to_string(c.sample.aim_x)+","+std::to_string(c.sample.aim_y)+"\n";
  add("sent_edge_mask",c.sent_edge_mask);
  add("render_calls",c.render_calls);
  add("scene_generation",c.scene.Generations().scene);
  add("lua_generation",c.scene.Generations().lua);
  add("context_generation",c.scene.Generations().context);
  add("native_attempt",c.scene.Result().attempt);
  add("native_executed_prefix",c.scene.Result().executed_prefix);
  add("native_committed",c.scene.Result().native_committed);
  s+="client.native_reason="+std::string(c.scene.Result().reason.data())+"\n";
  s+="client.error="+c.error+"\n";return s;
}
} // namespace cs::game
