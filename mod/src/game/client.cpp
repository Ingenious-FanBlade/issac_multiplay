#include "game/client_entity_e2e.h"
#include "game/client.h"
#include "game/client_scene.h"
#include "game/client_resources.h"
#include "game/client_entity_draw_native.h"
#include "game/client_player_isolation.h"
#include "game/client_isolation.h"
#include "replica/stream_e2e.h"
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
#include <cstring>
#include <memory>
#include <thread>
#include <psapi.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
enum class State { Off, Connecting, AwaitingWorld, PreparingWorld, Recovering, Retiring, Failed };
constexpr const char *kIsolationOriginals[]={"start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"};
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
  replica::stream::RuntimeConfig config;
  // 连接级恢复账本跨Begin/route保留，重传/新目标不重置原生失败次数。
  ClientApplyResult first_scene_failure{},cleanup_failure{};
  std::vector<ClientOwnerFact> first_owners;
  bool owner_diagnostics_missing=false;
  bool have_scene_failure=false,terminal_frozen=false,recovery_requested=false,recovery_succeeded=false,stale=false,was_paused=false;
#ifdef ISAACCS_E2E
  bool visual_held=false;
#endif
  unsigned native_rebuilds=0,recoveries=0;
  std::uint64_t recovery_started=0,recovery_sequence=0,failed_revision=0,stale_origin=0;
  replica::Scope recovery_scope{};
  std::uint64_t recovery_route=0;
  std::string recovery_reason,stop_reason;

  ClientScene scene;
  State state=State::Connecting;
  replica::Scope scope{};
  net::protocol::Ack progress{};
  std::optional<net::live::Event> prepare,retire_control;
  bool prepare_sent=false;
  std::array<std::uint64_t,6> prepare_progress{};
  unsigned ordinal=0, roster=0;
  std::uint64_t route=1;
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
  bool focused=false,ui_blocked=false,input_ready=false,fixture=false;
  InputTiming timing;
  ClientPlayerControlView control;
  unsigned neutral_loops=0;
  unsigned fixture_cancel_stage=0;
  void *source=nullptr;
  std::string address, error;
  // I3 纯值分派诊断独立于观察记录，供最终整体验收区分预期跳过与漏链。
  std::array<std::uint64_t,unsigned(ClientEntry::Count)> dispatch_original{},dispatch_expected{},dispatch_fault{};
  unsigned dispatch_last_entry=0,dispatch_last_decision=0;
  std::uint64_t callback_skipped=0,callback_own=0,callback_fault=0;
  // 精确连接边界的原调用计数，避免把连接前最后一次Off调用混入Client验收窗口。
  std::array<std::uint64_t,9> original_begin{};
};
std::unique_ptr<ClientRuntime> client;
std::atomic<bool> client_active{false},foreign_client{false};
// 真实窗口关闭一旦始于Client，保护延续到Shutdown/Manager最后写入者。
// 场景安全退休可先交还Off，但不能因此重新开放同一次进程退出的进度写入。
std::atomic<bool> client_exit_progress_guard{false};
std::uint64_t connections=0;
std::uint64_t ui_seen_ms=0,ui_frames=0;
thread_local unsigned client_depth=0;
thread_local ClientBorrowToken render_observation{};
// I3 的单根原生用途帧仅在主线程存放；每帧票据阻止连接/scene 提前回收。
struct ClientDispatchFrame final {
  ClientScene *scene=nullptr;
  ClientBorrowToken borrow{};
  ClientOperation operation=ClientOperation::Build;
  ClientEntry root=ClientEntry::GameUpdate;
  ClientSceneState state=ClientSceneState::Waiting;
  Owner owner=Owner::None;
  std::uint64_t id=0,owner_token=0,expected=0;
  bool called=false,returned=false,fault=false;
};
thread_local ClientDispatchFrame dispatch_frame{};
std::uint64_t next_dispatch_frame=0;

constexpr std::uint64_t EntryBit(ClientEntry entry) noexcept {
  return ClientEntryMask(entry);
}
constexpr std::uint64_t kExpectedNestedMask=
    EntryBit(ClientEntry::GameUpdate)|EntryBit(ClientEntry::PostUpdate)|
    EntryBit(ClientEntry::RoomUpdate)|EntryBit(ClientEntry::EntityListUpdate)|
    EntryBit(ClientEntry::PlayerUpdate)|EntryBit(ClientEntry::OddHelper)|
    EntryBit(ClientEntry::DoorUpdate)|EntryBit(ClientEntry::BackdropEffects);
constexpr const char *kClientEntryNames[]={
  "game_update","post_update","room_update","entity_list","player_update",
  "odd_helper","lifecycle_fsm","door_update","door_nested","new_room",
  "room_init","game_render","room_prerender","room_render","render_sort","game_start",
  "native_build","native_apply","native_teardown","presentation_maintenance","presentation_draw","menu_render","backdrop_effects"};
void CountDispatch(ClientRuntime &c,ClientEntry entry,ClientEntryDecision decision) noexcept {
  const auto index=unsigned(entry);
  if(index>=c.dispatch_original.size())return;
  ClientIsolationObserve(decision==ClientEntryDecision::CallOriginal?ClientIsolationKind::NativeCall:
    decision==ClientEntryDecision::ExpectedSkip?ClientIsolationKind::ExpectedSkip:ClientIsolationKind::RejectedEntry,kClientEntryNames[index]);
  auto &counter=decision==ClientEntryDecision::CallOriginal?c.dispatch_original[index]:
                decision==ClientEntryDecision::ExpectedSkip?c.dispatch_expected[index]:
                c.dispatch_fault[index];
  if(counter!=UINT64_MAX)++counter;
  c.dispatch_last_entry=index;c.dispatch_last_decision=unsigned(decision);
}
// 只允许用途已证明的具名根；状态门与根入口共同约束原链资格。
bool DispatchRootAllowed(ClientOperation operation,ClientEntry root,
                         ClientSceneState state,Owner owner) noexcept {
  switch(operation){
    case ClientOperation::Build:
      return state==ClientSceneState::Preparing&&owner==Owner::Manager&&
          root==ClientEntry::NativeBuild;
    case ClientOperation::Apply:
      return state==ClientSceneState::Applying&&owner==Owner::Manager&&
          root==ClientEntry::NativeApply;
    case ClientOperation::PresentMaintenance:
      return state==ClientSceneState::Applying&&owner==Owner::Manager&&
          root==ClientEntry::PresentationMaintenance;
    case ClientOperation::PresentDraw:
      return state==ClientSceneState::Displaying&&owner==Owner::Render&&
          root==ClientEntry::PresentationDraw;
    case ClientOperation::Retire:
      return (state==ClientSceneState::Retiring||state==ClientSceneState::Failed)&&
          owner==Owner::Manager&&root==ClientEntry::NativeTeardown;
  }
  return false;
}

// 设备/代次/UI变化清除待发边沿，但保留此次物理held基线防止恢复时补发。
void Neutral(ClientRuntime &c,bool forget=false) noexcept {
  c.sample={};c.eligible=false;c.last_target=0;
  c.edge_head=c.edge_count=0;
  if(forget){c.source=nullptr;c.baseline=false;c.held=0;c.shooting={};}
}
// 诊断分配失败只标缺测；不能改变正式停止或恢复是否生效。
void RememberFailure(ClientRuntime &c) noexcept {
  if(c.have_scene_failure)return;
  c.first_scene_failure=c.scene.Result();c.have_scene_failure=true;
  try{c.first_owners=c.scene.RetainedFacts();}catch(...){c.owner_diagnostics_missing=true;}
}
// 所有终态先封输入/发布，再由Manager真实退休；首因与后续清理错误分开。
void Failure(ClientRuntime &c,const char *reason) noexcept {
  if(c.terminal_frozen)return;
  ClientIsolationObserve(ClientIsolationKind::Fault,reason);
  RememberFailure(c);
  c.terminal_frozen=true;c.scene.CancelRecovery();
  Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};
  c.prepare.reset();c.retire_control.reset();c.state=State::Retiring;c.neutral_loops=0;
  try {if(c.error.empty())c.error=reason;c.stop_reason=reason;}catch(...){}
  c.scene.Retire(reason);c.network.Stop();
  Trace("client","event=failed_retiring connection=%llu reason=%s",c.connection,reason);
}
// 只使用既有完整退休步骤；没有安全owner时保留隔离，不盲目重建旧T。
void Recover(ClientRuntime &c,const char *reason) noexcept {
  if(c.terminal_frozen||c.state==State::Recovering)return;
  ClientIsolationObserve(ClientIsolationKind::Recover,reason);
  const auto result=c.scene.Result();
  const bool written=c.scene.NetworkStream()?c.scene.StreamFailureWritten():(!c.scene.DisplayReady()&&(result.native_written||result.failure_executed_prefix||c.scene.HasNativePrefix()));
  const auto policy=replica::stream::ClassifyFailure(reason,written,!c.scene.StreamRecoverable());
  RememberFailure(c);
  if(!policy.recoverable||(written&&c.native_rebuilds>=1)){Failure(c,written?"native_recovery_exhausted_or_unsafe":reason);return;}
  if(written)++c.native_rebuilds;
  try {
    if(c.error.empty())c.error=reason;
    c.recovery_reason=(written?"native:":"state:")+std::string(reason);
    if(c.recovery_reason.size()>120)c.recovery_reason.resize(120);
  }catch(...){Failure(c,"recovery_reason_allocation");return;}
  ++c.recoveries;if(!c.recovery_started||c.recovery_succeeded)c.recovery_started=GetTickCount64();c.recovery_requested=false;c.recovery_succeeded=false;
  c.recovery_scope=c.scope;c.recovery_route=c.route;c.failed_revision=result.revision;
  Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.prepare.reset();
  if(!c.network.SuspendWorld()){Failure(c,"recovery_suspend_queue");return;}
  c.state=State::Recovering;c.scene.Retire("stream_failure_safe_retirement");
}
void SceneFailure(ClientRuntime &c) noexcept {
  if(c.scene.RetirementFault()){
    RememberFailure(c);c.cleanup_failure=c.scene.Result();
    Failure(c,"native_retirement_failed");c.state=State::Failed;return;
  }
  if(c.scene.State()!=ClientSceneState::Failed||c.state==State::Retiring||c.state==State::Failed)return;
  if(!c.fixture&&c.route>1)Recover(c,c.scene.Result().failure_reason[0]?c.scene.Result().failure_reason.data():c.scene.Result().reason.data());
  else if(!c.scene.RecoveryPending())Failure(c,c.scene.Result().reason.data());
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
// 正式连接与测试纯值来源共用启动前置；保护必须先于任何原生场景写入。
const char *ClientStartUnavailable() noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||client_depth)return "wrong_boundary";
  if(ClientBusy()||Busy())return "busy";
  if(!MenuReady())return "client_requires_stable_menu";
  if(PointerCount()!=8||!ContextHealthy()||!Capabilities[size_t(Group::Input)]||
     !Capabilities[size_t(Group::Loop)]||!Capabilities[size_t(Group::Control)]||
     !Capabilities[size_t(Group::Client)]||!WorldExitPatchInstalled())return "adaptation_unavailable";
  if(!ClientShutdownPatchInstalled())return "shutdown_isolation_unavailable";
  if(!ClientBackdropPatchInstalled())return "backdrop_isolation_unavailable";
  ClientGuardLuaBindings();
  if(!ClientLuaGuardReady())return "lua_persistence_guard_unavailable";
  if(!ClientDispatchIsolationInstalled())return "client_dispatch_isolation_unavailable";
  if(!ClientEntityInitIsolationInstalled()||!ClientEntityDrawIsolationInstalled())return "client_entity_isolation_unavailable";
  if(!ClientPlayerIsolationInstalled())return "client_player_isolation_unavailable";
  if(connections==UINT64_MAX)return "connection_exhausted";
  return "";
}
}
bool ClientBusy() noexcept {return client_active.load(std::memory_order_acquire);}
// 仅主线程复制现有Scene身份；外线程诊断保留未知，不读取该控制块。
ClientIsolationIdentity ClientIsolationCurrent() noexcept {
 if(!IsMain()||!client)return {};const auto &g=client->scene.Generations();
 return {client->connection,client->scope.connection,client->route,g.scene,g.instance,g.resource,g.lua,g.context,unsigned(client->scene.State())};
}
bool ClientPlayerBorrowAllowed() noexcept {
  return IsMain()&&ClientBusy()&&client&&(client->scene.State()==ClientSceneState::Applying||client->scene.DisplayReady());
}
std::shared_ptr<const ClientPlayerDisplayView> ClientPlayerDisplay() noexcept {
  return IsMain()&&ClientBusy()&&client?client->scene.PlayerDisplay():nullptr;
}
bool ClientPlayerDisplayCurrent(const std::shared_ptr<const ClientPlayerDisplayView> &view) noexcept {
  return IsMain()&&ClientBusy()&&client&&client->scene.PlayerDisplayCurrent(view);
}
// 检查真实 trampoline，不能拿清单行或能力组存在冒充已经安装。
bool ClientDispatchIsolationInstalled() noexcept {
  if(!IsMain()||!Capabilities[size_t(Group::Client)]||!ClientCallbackGuardReady())return false;
  static constexpr const char *required[]={"client_game_update","client_post_update",
    "client_room_update","client_entity_list","client_player_update","client_odd_helper",
    "client_room_lifecycle","client_door_update","client_door_nested","client_new_room",
    "room_init","game_render","room_prerender","room_render","entitylist_render_sort","client_game_start","client_menu_render","client_backdrop_effects","client_backdrop_wall","client_backdrop_graphics"};
  size_t count=0;auto hooks=Hooks(count);
  for(const char *id:required){
    bool installed=false;
    for(size_t i=0;i<count;++i)if(std::strcmp(hooks[i]->id,id)==0){
      installed=hooks[i]->registered&&hooks[i]->super&&*hooks[i]->super;break;
    }
    if(!installed)return false;
  }
  return true;
}
void ClientCallbackFault(const char *reason) noexcept {
  ClientIsolationObserve(ClientIsolationKind::Fault,reason);
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(client&&ClientBusy()){
    if(client->callback_fault!=UINT64_MAX)++client->callback_fault;
    Failure(*client,reason);
  }
}
void ClientCallbackSkipped() noexcept {
  ClientIsolationObserve(ClientIsolationKind::LuaSkip,"gameplay_callback");
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(client&&ClientBusy()&&client->callback_skipped!=UINT64_MAX)++client->callback_skipped;
}
void ClientCallbackOwnExecuted() noexcept {
  ClientIsolationObserve(ClientIsolationKind::OwnLua,"own_menu_ui");
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(client&&ClientBusy()&&client->callback_own!=UINT64_MAX)++client->callback_own;
}
bool ClientCallbackMenuAllowed() noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return false;}
  return ClientBusy()&&client&&!dispatch_frame.id&&TopOwner()==Owner::Render&&
         !InBlockedContext()&&ContextHealthy()&&MenuReady()&&
         render_observation.active&&client->scene.Valid(render_observation);
}
ClientDispatchScope::ClientDispatchScope(ClientOperation operation,ClientEntry root,
                                         std::uint64_t expected_skips) noexcept
    : exceptions_(std::uncaught_exceptions()) {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(!ClientBusy()||!client)return;
  auto &c=*client;
  const auto owner=TopOwner();
  if(dispatch_frame.id||!ContextHealthy()||InBlockedContext()||!ClientDispatchIsolationInstalled()||
     (expected_skips&~kExpectedNestedMask)||
     (expected_skips&EntryBit(root))||
     !DispatchRootAllowed(operation,root,c.scene.State(),owner)||
     next_dispatch_frame==UINT64_MAX){Failure(c,"client_dispatch_admission");return;}
  ClientBorrowToken borrow{};
  if(!c.scene.Borrow(ClientPurpose::Dispatch,borrow)){Failure(c,"client_dispatch_borrow");return;}
  const auto token=OwnerToken(owner);
  if(!token){c.scene.Release(borrow);Failure(c,"client_dispatch_owner");return;}
  frame_=++next_dispatch_frame;
  dispatch_frame={&c.scene,borrow,operation,root,c.scene.State(),owner,frame_,token,
                  expected_skips,false,false,false};
  active_=true;
}
bool ClientDispatchScope::Succeeded() const noexcept {
  if(!IsMain())return false;
  return active_&&dispatch_frame.id==frame_&&dispatch_frame.called&&
         dispatch_frame.returned&&!dispatch_frame.fault&&
         dispatch_frame.scene->State()==dispatch_frame.state&&
         dispatch_frame.scene->Valid(dispatch_frame.borrow)&&
         ContextHealthy()&&OwnerToken(dispatch_frame.owner)==dispatch_frame.owner_token&&
         exceptions_==std::uncaught_exceptions();
}
ClientDispatchScope::~ClientDispatchScope() noexcept {
  if(!active_)return;
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(dispatch_frame.id!=frame_||!dispatch_frame.scene){
    if(client)Failure(*client,"client_dispatch_unpaired");return;
  }
  auto frame=dispatch_frame;
  dispatch_frame={};
  if(std::uncaught_exceptions()!=exceptions_||!frame.returned||frame.fault||
     frame.scene->State()!=frame.state||!ContextHealthy()||
     !frame.scene->Valid(frame.borrow)||
     OwnerToken(frame.owner)!=frame.owner_token){
    if(client && (&client->scene==frame.scene))Failure(*client,"client_dispatch_incomplete");
  }
  frame.scene->Release(frame.borrow);
}
// 主动 Client 分派不计入观察 Hook 的 Returned；预期嵌套跳过必须由根用途显式声明。
ClientEntryDecision ClientDispatchBefore(ClientEntry entry) noexcept {
  if(!ClientBusy())return ClientEntryDecision::CallOriginal;
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return ClientEntryDecision::FaultSkip;}
  if(!client)return ClientEntryDecision::FaultSkip;
  const auto decide=[&](ClientEntryDecision decision) noexcept {
    CountDispatch(*client,entry,decision);return decision;
  };
  auto &frame=dispatch_frame;
  if(frame.id&&frame.scene==&client->scene&&ContextHealthy()&&
     OwnerToken(frame.owner)==frame.owner_token&&
     frame.borrow.generations==client->scene.Generations()&&
     frame.scene->State()==frame.state){
    if(entry==frame.root&&!frame.called&&!frame.fault){
      if(!frame.scene->Valid(frame.borrow)){
        frame.fault=true;Failure(*client,"client_dispatch_borrow_invalid");
        return decide(ClientEntryDecision::FaultSkip);
      }
      if(!frame.scene->NoteNativeDispatchEntry()){
        frame.fault=true;Failure(*client,"client_dispatch_prefix_budget");return decide(ClientEntryDecision::FaultSkip);
      }
      frame.called=true;return decide(ClientEntryDecision::CallOriginal);
    }
    if(frame.called&&!frame.returned&&(frame.expected&EntryBit(entry))&&
       !frame.fault)return decide(ClientEntryDecision::ExpectedSkip);
    frame.fault=true;
  }
  Failure(*client,"client_dispatch_unexpected_entry");
  return decide(ClientEntryDecision::FaultSkip);
}
void ClientDispatchReturned(ClientEntry entry) noexcept {
  if(!ClientBusy())return;
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(!client||!dispatch_frame.id||dispatch_frame.root!=entry||
     dispatch_frame.scene!=&client->scene||!dispatch_frame.called||dispatch_frame.returned||
     dispatch_frame.fault||!ContextHealthy()||
     !dispatch_frame.scene->Valid(dispatch_frame.borrow)||
     dispatch_frame.borrow.generations!=client->scene.Generations()||
     client->scene.State()!=dispatch_frame.state){
    if(client)Failure(*client,"client_dispatch_return_mismatch");return;
  }
  dispatch_frame.returned=true;
  ClientIsolationObserve(ClientIsolationKind::NativeReturn,kClientEntryNames[unsigned(entry)]);
}
const char *ClientStateName() noexcept {
  if(!client)return "Off";
  switch(client->state) {
    case State::Off:return "Off";case State::Connecting:return "Connecting";
    case State::AwaitingWorld:return "AwaitingWorld";case State::PreparingWorld:return "PreparingWorld";case State::Recovering:return "Recovering";case State::Retiring:return "Retiring";
    case State::Failed:return "Failed";
  }
  return "Failed";
}
const char *StartClient(const std::string &ipv4,unsigned port,int device_key) noexcept {
  const auto unavailable=ClientStartUnavailable();if(*unavailable)return unavailable;
  if(!ui_seen_ms||GetTickCount64()-ui_seen_ms>1000)return "client_ui_unavailable";
  if(port<1||port>65535||device_key<0||device_key>255)return "arguments";
  try {
    auto next=std::make_unique<ClientRuntime>();next->connection=++connections;
    next->address=ipv4;next->device_key=device_key;next->config=Settings.stream;
    const auto error=next->network.Start(ipv4,std::uint16_t(port),Settings.peer_timeout_ms,next->config);
    if(*error)return error;
    // 真实网络Client同样冻结原调用起点；不能只给离线fixture提供隔离见证。
    size_t count=0;auto hooks=Hooks(count);
    for(unsigned j=0;j<next->original_begin.size();++j)for(size_t i=0;i<count;++i)
      if(std::strcmp(hooks[i]->id,kIsolationOriginals[j])==0)next->original_begin[j]=hooks[i]->calls.load();
    ForgetInputDevice();client=std::move(next);foreign_client=false;
    client_active.store(true,std::memory_order_release);
    ClientIsolationObserve(ClientIsolationKind::Begin,"network_client");
    Trace("client","event=connecting connection=%llu address=%s port=%u key=%d local_run=0",connections,ipv4.c_str(),port,device_key);
    return "";
  } catch(...) {return "client_allocation";}
}
#ifdef ISAACCS_E2E
// 只从真实已提交网络范围派生负向纯值，不使用本地文件或替代Client。
std::string ClientTestStreamReject(const std::string &token,unsigned tx,const std::string &name) noexcept {
 if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!client||client->fixture||!client->scene.NetworkStream())return "ok=0\nerror=network_negative_context\n";
 return client->scene.TestStreamReject(tx,name);
}
// 使用本机收到的Authority锚点推算年龄，误差显式包含RTT与发布年龄。
void ClientStreamAgeWitness(std::uint64_t source_tick) noexcept {
 if(!IsMain()||!client||!client->timing.received_ms)return;const auto now=GetTickCount64();const auto &t=client->timing;
 replica::stream::E2EAge(client->progress.server_tick,source_tick,now>=t.received_ms?now-t.received_ms:0,t.round_trip_ms+t.publication_age_ms,client->stale);
}
// 只读后态借用仍由Scene唯一提交资格决定，不用DTO取代原生检查。
std::string ClientTestInspect(const std::string &token) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!client||(!client->fixture&&!client->scene.NetworkStream()))return "ok=0\nerror=inspect_context\n";
  return client->scene.TestInspect();
}
// 测试会话只替换目标来源；启动保护、原生应用、失败隔离及退休仍走同一正式 owner。
const char *ClientTestBegin(const std::string &token,const replica::Scope &scope) noexcept {
  if(Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token)return "client_test_disabled";
  const auto unavailable=ClientStartUnavailable();if(*unavailable)return unavailable;
  if(!scope.session||!scope.connection||!scope.run||!scope.scene)return "formal_scope_required";
  try {
    auto next=std::make_unique<ClientRuntime>();next->connection=++connections;
    next->fixture=true;next->address="e2e_value_fixture";next->scope=scope;
    next->scope.run=next->scope.floor=next->scope.dimension=next->scope.room=next->scope.scene=0;
    next->state=State::AwaitingWorld;next->scene.Begin(next->scope);
    if(next->scene.State()==ClientSceneState::Failed)return "client_scene_begin_failed";
    size_t count=0;auto hooks=Hooks(count);
    for(unsigned j=0;j<next->original_begin.size();++j)for(size_t i=0;i<count;++i)
      if(std::strcmp(hooks[i]->id,kIsolationOriginals[j])==0)next->original_begin[j]=hooks[i]->calls.load();
    ForgetInputDevice();client=std::move(next);foreign_client=false;
    client_active.store(true,std::memory_order_release);
    Trace("client","event=fixture_begin connection=%llu source=e2e_value_fixture session=%llu wire_connection=%llu",connections,scope.session,scope.connection);
    return "";
  }catch(...){return "client_allocation";}
}
// 在真实Manager阶段边界取消，避免IPC轮询跳过短暂阶段；只调用正式StopClient。
bool ClientTestCancelAfter(const std::string &token,unsigned stage) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!client||!client->fixture||stage<1||stage>3)return false;
  client->fixture_cancel_stage=stage;return true;
}
// 目标在 Manager 上复制为自有值；测试 token 不授予任意地址或额外 native 写入权限。
// 统一实机验收在已发布真实场景检验owner拒绝；worker只携带自有纯值模型。
std::string ClientTestExerciseOwnership(const std::string &token) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||
     !ClientBusy()||!client||(!client->fixture&&!client->scene.NetworkStream())||!client->scene.DisplayReady())return "ok=0\nerror=client_ownership_context\n";
  try {
    auto values=replica::Copy(*client->scene.Published());const auto scope=replica::ReadScope(values["scope"]);
    const auto revision=replica::Unsigned(values["revision"]);bool accepted=true;
    std::thread foreign([model=std::move(values),scope,revision,&accepted]{accepted=ClientAdmitTarget(scope,revision,model);});
    foreign.join();if(accepted)return "ok=0\nerror=foreign_target_accepted\n";
    ClientBorrowToken borrowed{};
    if(!client->scene.Borrow(ClientPurpose::Graphics,borrowed)||!client->scene.Valid(borrowed))return "ok=0\nerror=graphics_borrow_missing\n";
    const auto stale=borrowed;client->scene.Release(borrowed);
    if(client->scene.Valid(stale))return "ok=0\nerror=released_borrow_valid\n";
    // 暂撤本线程的实际WGL current，再验证正式图形借用在任何原生写入前拒绝。
    const auto dc=wglGetCurrentDC();const auto context=wglGetCurrentContext();
    if(!dc||!context||!wglMakeCurrent(nullptr,nullptr))return "ok=0\nerror=context_detach_failed\n";
    ClientBorrowToken invalid{};const bool wrong_context=client->scene.Borrow(ClientPurpose::Graphics,invalid);
    const bool restored=wglMakeCurrent(dc,context)!=FALSE;
    if(wrong_context)client->scene.Release(invalid);
    if(!restored||wrong_context)return "ok=0\nerror=context_rejection_or_restore_failed\n";
    return "ok=1\nownership_exercise=1\n";
  }catch(...){return "ok=0\nerror=ownership_exercise_exception\n";}
}
// VM不匹配是终止当前场景的故障，单独编排并由正式退休/重建恢复。
std::string ClientTestExerciseVm(const std::string &token) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!client||(!client->fixture&&!client->scene.NetworkStream())||!client->scene.DisplayReady())return "ok=0\nerror=vm_exercise_context\n";
  try {
    // 用真实独立Lua VM制造身份不匹配；保留游戏原根，并在同一栈释放旧借用和临时VM。
    void *vm=ClientTestCreateVm();if(!vm)return "ok=0\nerror=temporary_vm_allocation\n";
    void *original=ClientLuaStateIdentity();ClientBorrowToken lua{};
    if(!client->scene.Borrow(ClientPurpose::Lua,lua)){ClientTestDestroyVm(vm);return "ok=0\nerror=lua_borrow_missing\n";}
    client->scene.ObserveLuaState(vm);const bool stale_valid=client->scene.Valid(lua);
    client->scene.Release(lua);client->scene.ObserveLuaState(original);ClientTestDestroyVm(vm);
    if(stale_valid||client->scene.State()!=ClientSceneState::Failed)return "ok=0\nerror=vm_generation_not_rejected\n";
    return "ok=1\nvm_exercise=1\n";
  }catch(...){return "ok=0\nerror=vm_exercise_exception\n";}
}
#endif
bool ClientAdmitTarget(const replica::Scope &scope,std::uint64_t revision,const replica::Value &value) noexcept {
  ClientEntityTestMeasure measure("target_parse_plan");
  if(!IsMain()||TopOwner()!=Owner::Manager||client_depth||!ClientBusy()||!client||
     (client->state!=State::AwaitingWorld&&!(client->fixture&&client->state==State::Failed)))return false;
  const bool admitted=client->scene.AdmitTarget(scope,revision,value);
  if(admitted){
    Neutral(*client,true);client->input_ready=false;
    // 测试来源可交付新的完整T进行正式恢复；真实断连不借此恢复已停止的网络线程。
    if(client->fixture){client->state=State::AwaitingWorld;client->error.clear();}
  }
  return admitted;
}
void StopClient() noexcept {
  if(!IsMain()||!ClientBusy())return;
  if(client->terminal_frozen||client->state==State::Retiring)return;
  ClientIsolationObserve(ClientIsolationKind::Stop,"client_stop");
  client->terminal_frozen=true;client->stop_reason="client_stop";
  Neutral(*client,true);client->network.RevokeInput();client->input_ready=false;client->timing={};client->progress={};client->prepare.reset();client->retire_control.reset();client->state=State::Retiring;client->scene.Retire("client_stop");client->neutral_loops=0;client->network.Stop();
  Trace("client","event=retiring connection=%llu",client->connection);
}
bool ClientAdmitGridIndependent(const replica::Message &message,const char *&reason) noexcept {
  if(!IsMain()||!client||!ClientBusy()||client_depth){reason="grid_independent_client_unavailable";return false;}
  return client->scene.AdmitGridIndependent(message,reason);
}
bool ClientVisitGridIndependent(void *context,ClientGridIndependentVisitor visitor) noexcept {
  return IsMain()&&client&&ClientBusy()&&client->scene.VisitGridIndependent(context,visitor);
}
bool ClientCompleteGridIndependent(std::uint64_t sequence,std::uint64_t scope,const char *phase) noexcept {
  return IsMain()&&client&&ClientBusy()&&client->scene.CompleteGridIndependent(sequence,scope,phase);
}
bool ClientAdmitEntityIndependent(const replica::Message &message,const char *&reason) noexcept {
  if(!IsMain()||!client||!ClientBusy()||client_depth){reason="entity_independent_client_unavailable";return false;}
  return client->scene.AdmitEntityIndependent(message,reason);
}
bool ClientVisitEntityIndependent(void *context,ClientEntityEvents::Visitor visitor) noexcept {
  return IsMain()&&client&&ClientBusy()&&client->scene.VisitEntityIndependent(context,visitor);
}
bool ClientCompleteEntityIndependent(replica::Kind kind,std::uint64_t sequence,std::uint64_t source,const char *phase) noexcept {
  return IsMain()&&client&&ClientBusy()&&client->scene.CompleteEntityIndependent(kind,sequence,source,phase);
}
// 主线程再次确认当前路由；仅交付自有值，不改变Scene或Authority时钟。
bool ClientPollWorld(net::live::Event &event) noexcept {
  if(!IsMain()||!client||client->fixture||client->state!=State::AwaitingWorld)return false;
  for(unsigned n=0;n<72&&client->network.PollWorld(event);++n){
    if(event.scope==client->scope&&event.route==client->route&&event.ordinal==client->ordinal)return true;
    event={};
  }
  return false;
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
    SceneFailure(c);
    c.scene.Pump();
    if(c.fixture_cancel_stage){const auto r=c.scene.Result();const auto stage=c.fixture_cancel_stage;
      if((stage==1&&r.native_written)||(stage==2&&r.grid_prefix)||(stage==3&&c.scene.DisplayReady())){
        c.fixture_cancel_stage=0;StopClient();
      }
    }
    c.scene.DrainRetained();
    SceneFailure(c);
    if(foreign_client.exchange(false))Failure(c,"client_foreign_manager");
    ++c.manager_skipped;
    net::live::Event event;
    for(unsigned i=0;!c.fixture&&i<256&&c.network.Poll(event);++i) {
      if(c.terminal_frozen)continue;
      switch(event.type) {
        case net::live::Event::Type::Welcome:
          if(c.state!=State::Connecting)break;
          c.scope=event.scope;c.route=event.route;c.scene.Begin(event.scope);
          if(c.scene.State()==ClientSceneState::Failed){Failure(c,"client_scene_begin_failed");break;}
          c.ordinal=event.ordinal;c.state=State::AwaitingWorld;
          Trace("client","event=welcome connection=%llu ordinal=%u session=%llu wire_connection=%llu",c.connection,c.ordinal,c.scope.session,c.scope.connection);break;
        case net::live::Event::Type::Prepared:{
          if(c.state!=State::PreparingWorld||!c.prepare||!c.prepare_sent||!c.scene.CanReturnToMenu()||!event.world){Failure(c,"client_prepare_activation_state");break;}
          const auto &p=c.prepare->world->body;if(event.route!=replica::Unsigned(p["next_route"])||event.scope!=replica::stream::Connected({replica::Unsigned(p["world"]["session"]),replica::Unsigned(p["world"]["run"]),replica::Unsigned(p["world"]["floor"]),replica::Unsigned(p["world"]["dimension"]),replica::Unsigned(p["world"]["room"]),replica::Unsigned(p["world"]["scene"])},c.scope.connection)){Failure(c,"client_prepare_activation_identity");break;}
          Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.generation=0;c.control={};c.last_tick=c.last_target=0;c.scope=event.scope;c.route=event.route;c.scene.Begin(c.scope);
          if(c.scene.State()==ClientSceneState::Failed||!c.scene.BeginStream(c.scope,c.route,replica::Unsigned(p["manifest"]),replica::Unsigned(p["generation"]),c.config,c.prepare->received_ms)){Failure(c,"client_stream_begin");break;}
          c.prepare.reset();c.prepare_sent=false;c.state=State::AwaitingWorld;break;
        }
        case net::live::Event::Type::Progress:
        case net::live::Event::Type::TickProgress:
          if(c.state!=State::AwaitingWorld)break;
          if(c.scope!=event.scope||c.route!=event.route||c.ordinal!=event.ordinal){Failure(c,"client_scope_conflict");break;}
          if(event.progress.eligibility_revision<c.progress.eligibility_revision)break;
          if(c.generation!=event.progress.generation) {
            Neutral(c,true);c.network.RevokeInput();c.timing={};c.generation=event.progress.generation;c.last_tick=0;
            Trace("client","event=generation connection=%llu generation=%llu",c.connection,c.generation);
          }
          if(event.progress.server_tick<c.progress.server_tick)break;
          c.progress=event.progress;c.roster=event.progress.roster_mask;
          if(event.type==net::live::Event::Type::TickProgress){if(!c.blocked)c.timing.Observe(event);else c.timing={};}break;
        case net::live::Event::Type::InputAck:
          if(c.state!=State::AwaitingWorld||event.scope!=c.scope||event.route!=c.route||event.ordinal!=c.ordinal)break;
          if(event.progress.generation==c.generation&&event.progress.processed>=c.input_ack) {
            if(event.progress.processed>c.sequence){Failure(c,"client_ack_unsent");break;}
            c.input_ack=event.progress.processed;c.input_ack_tick=event.progress.processed_tick;
          }
          break;
        case net::live::Event::Type::World:
        case net::live::Event::Type::Control:Failure(c,"client_world_on_notification_queue");break;
        case net::live::Event::Type::Stopped:if(!c.terminal_frozen)Failure(c,event.reason);break;
        case net::live::Event::Type::Failed:if(!c.terminal_frozen)Failure(c,event.reason);break;
      }
    }
    // 产品世界只从网络自有值进入唯一Scene；文件/fixture入口不参与此路径。
    for(unsigned n=0;!c.fixture&&(c.state==State::AwaitingWorld||c.state==State::PreparingWorld||c.state==State::Recovering)&&n<16&&c.network.PollWorld(event);++n){
      if(event.scope!=c.scope||event.route!=c.route||event.ordinal!=c.ordinal||!event.world)continue;
      const auto &m=*event.world;
      if(m.kind==replica::stream::Kind::ScenePrepare){
        const auto &p=m.body;if(replica::Unsigned(p["world"]["session"])!=c.scope.session||replica::Unsigned(p["next_route"])!=c.route+1||replica::Unsigned(p["native_domains"])!=71||replica::Unsigned(p["retained_domains"])!=(2047u^71u)){Failure(c,"client_prepare_route_or_native_contract");break;}
        if(c.prepare){if(c.prepare->world->sequence!=m.sequence||c.prepare->world->body!=m.body)Failure(c,"client_prepare_conflict");continue;}
        Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.prepare=event;c.prepare_sent=false;c.prepare_progress={};c.state=State::PreparingWorld;
        if(!c.scene.CanReturnToMenu())c.scene.Retire("network_scene_prepare");
      }else if(m.kind==replica::stream::Kind::Retire){
        Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.prepare.reset();c.retire_control=event;c.prepare_progress={};c.scene.Retire("network_source_retired");
      }else if(m.kind==replica::stream::Kind::Range&&c.state==State::AwaitingWorld){const char *why="client_stream_admission";if(GetTickCount64()>=event.received_ms)c.scene.ObserveStreamQueueAge((GetTickCount64()-event.received_ms)*1000);if(!c.scene.AdmitStream(m,why)){Recover(c,why);break;}}
      else if(m.kind!=replica::stream::Kind::Cancel&&!(m.kind==replica::stream::Kind::Range&&c.state==State::Recovering)){Failure(c,"client_stream_control_kind");break;}
    }
    if(!c.fixture&&(c.prepare||c.retire_control)&&!c.scene.CanReturnToMenu()){
      const auto r=c.scene.Result();const std::array<std::uint64_t,6> step{r.attempt,r.executed_prefix,r.native_owners,r.grid_prefix,r.grid_owners,r.pending_owners};
      // 只报告真实完整退休步骤的变化；不以Manager心跳延长准备超时。
      if(step!=c.prepare_progress&&c.network.Complete(c.prepare?*c.prepare:*c.retire_control,net::live::OperationStage::Executing,"native_retirement_progress"))c.prepare_progress=step;
    }
    if(!c.fixture&&c.state==State::PreparingWorld&&c.prepare&&!c.prepare_sent&&c.scene.CanReturnToMenu()){
      if(!c.network.Prepare(*c.prepare))Failure(c,"client_prepare_queue");else c.prepare_sent=true;
    }
    if(!c.fixture&&c.retire_control&&c.scene.CanReturnToMenu()){
      if(c.network.Complete(*c.retire_control,net::live::OperationStage::Complete,"native_scene_retired")){c.retire_control.reset();c.scene.Begin(c.scope);} // 成功排队后才释放真实退休结果。
    }
    if(!c.fixture&&c.state==State::AwaitingWorld&&!c.retire_control){
      c.scene.PumpStream();SceneFailure(c);for(unsigned n=0;n<16;++n){auto m=c.scene.StreamControl();if(!m||!c.network.SendControl(m))break;c.scene.StreamControlSent();}
      SceneFailure(c);
    }
    if(!c.fixture&&c.recovery_started&&!c.recovery_succeeded&&!c.terminal_frozen){
      const auto now=GetTickCount64();
      if(now<c.recovery_started||now-c.recovery_started>=c.config.absolute_timeout_ms)Failure(c,"recovery_deadline");
      else if(c.state==State::Recovering&&c.scene.CanReturnToMenu()&&!c.recovery_requested){
        if(c.recovery_sequence>=UINT64_MAX-1){throw replica::ContractError("recovery_sequence_exhausted");}
        auto m=std::make_shared<replica::stream::Message>();m->kind=replica::stream::Kind::Resync;m->sequence=++c.recovery_sequence;m->connection=c.recovery_scope.connection;m->route=c.recovery_route;m->body.SetObject();auto &a=m->body.GetAllocator();
        replica::Put(m->body,"world",replica::stream::WorldValue(replica::stream::World(c.recovery_scope),a),a);replica::Put(m->body,"failed_revision",replica::Decimal(c.failed_revision,a),a);replica::Put(m->body,"reason",c.recovery_reason,a);
        if(c.network.SendControl(std::move(m)))c.recovery_requested=true;
      }
      else if(c.scene.DisplayReady()&&c.state==State::AwaitingWorld){c.recovery_succeeded=true;c.stale_origin=now;}
    }
    bool visual_hold=false;
#ifdef ISAACCS_E2E
    visual_hold=replica::stream::E2EVisualHeld();
    if(c.visual_held&&!visual_hold){c.stale_origin=GetTickCount64();c.stale=false;}
    c.visual_held=visual_hold;
#endif
    if(!c.fixture&&!visual_hold&&c.state==State::AwaitingWorld&&c.scene.DisplayReady()){
      const auto now=GetTickCount64();
      if(c.was_paused&&!c.progress.authority_paused)c.stale_origin=now;
      c.was_paused=c.progress.authority_paused;
      const auto last=(std::max)(c.scene.LastStreamCommit(),c.stale_origin);
      const auto age=last&&now>=last?now-last:0;
      c.stale=!c.progress.authority_paused&&age>=c.config.stale_ms;
      if(c.stale&&age>=c.config.recovery_ms)Recover(c,"client_state_stale");
    }
    if(c.state==State::Retiring||c.state==State::Failed){net::live::Event discarded;for(unsigned n=0;n<80&&c.network.PollWorld(discarded);++n){}}
    if(c.state==State::Retiring) {
      // 原生菜单可能在Input更新之前消费缓存；交还前刷新两轮真实松键，不能重放Client按键。
      c.sampled_this_loop=false;c.physical_neutral=false;c.blocked=true;ClientPollInput();
      if(!c.sampled_this_loop||c.physical_neutral)++c.neutral_loops;else c.neutral_loops=0;
      // 只有无原生执行前缀、无借用与在途使用时才能交还本地菜单。
      if((c.fixture||c.network.Finished())&&c.neutral_loops>=2&&MenuReady()&&c.scene.CanReturnToMenu()) {
        ClientIsolationObserve(ClientIsolationKind::Retired,"scene_and_worker_drained");
        c.state=State::Off;ForgetInputDevice();client_active.store(false,std::memory_order_release);
        Trace("client","event=off connection=%llu input_closed=1 worker_finished=1 local_run=0",c.connection);
      }
    } else if(c.state==State::Connecting||c.state==State::AwaitingWorld) {
      const bool was_blocked=c.blocked;c.sampled_this_loop=false;c.focused=Focused();c.ui_blocked=ClientUiBlocked();c.blocked=!c.focused||c.ui_blocked;
      if(c.blocked&&!was_blocked){c.network.RevokeInput();c.timing={};}
      std::uint64_t target=0;
      // 16 的等待场景仍能向 Authority 发输入；已有目标的构建/替换窗口则清边沿并保持中性。
      const bool input_scene=c.scene.DisplayReady();
      const auto control=c.scene.PlayerControl(c.ordinal);
      // 构建中的无视图不是新的绑定；停止发意图并清FIFO，保留旧身份用于重建后的比较。
      if(c.scene.DisplayReady()){
        if(!ClientPlayerControlSame(c.control,control)){Neutral(c);c.timing={};}
        c.control=control;
      }
      // 观测binding_revision与Authority participant generation出自同一绑定；旧A不得给新代次发边沿。
      const bool group_ready=!c.scene.HasPlayerControl()||
          (control.present&&control.consumers&&control.binding_revision==c.generation);
      c.input_ready=!c.fixture&&input_scene&&!c.blocked&&c.progress.input_enabled&&c.state==State::AwaitingWorld&&
                    group_ready&&c.progress.session_state==core::SessionState::Active&&c.generation&&
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
          net::live::Intent intent{sample,c.generation,c.sequence+1,target,c.scope,c.route};
          if(c.network.Send(intent)) {
            ++c.sequence;++c.sent;c.last_target=target;c.last_tick=c.progress.server_tick;c.sample.edge_flags=0;
#ifdef ISAACCS_E2E
            replica::stream::E2EInput("physical-input",{c.ordinal,0,0,sample.edge_flags,c.generation,c.sequence,target,c.progress.server_tick,{sample.move_x,sample.move_y,sample.aim_x,sample.aim_y}});
#endif
            c.sent_edge_mask|=sample.edge_flags;
            if(c.edge_count){c.edge_head=(c.edge_head+1)%unsigned(c.edges.size());--c.edge_count;}
            if(Settings.diagnostics)Trace("client_input","connection=%llu ordinal=%u generation=%llu sequence=%llu target=%llu move=%d,%d aim=%d,%d held=%u edge=%u",c.connection,c.ordinal,c.generation,c.sequence,target,intent.state.move_x,intent.state.move_y,intent.state.aim_x,intent.state.aim_y,intent.state.held_flags,intent.state.edge_flags);
          } else Failure(c,"client_input_queue_full");
        }
      }
      if(!c.fixture&&c.network.Finished()&&c.state!=State::Failed)Failure(c,"client_transport_finished");
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
// 观察窗口跨过Manager Render完整原链；具体原生展示在其菜单绘制子入口替代，保留清屏/Swap职责。
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
// 原生菜单绘制子入口中执行已发布场景，既不调用完整Game Render，也不在Swap之后补画。
bool ClientMenuRenderBoundary() noexcept {
  if(!ClientBusy())return false;
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return true;}
  if(!client)return true;
  if(TopOwner()!=Owner::Render||!render_observation.active||
     !client->scene.Valid(render_observation)||dispatch_frame.id){
    CountDispatch(*client,ClientEntry::MenuRender,ClientEntryDecision::FaultSkip);
    Failure(*client,"client_menu_render_phase");return true;
  }
  if(!client->scene.HasNativePrefix()&&!client->scene.DisplayReady()){
    CountDispatch(*client,ClientEntry::MenuRender,ClientEntryDecision::CallOriginal);
    return false;
  }
  CountDispatch(*client,ClientEntry::MenuRender,ClientEntryDecision::ExpectedSkip);
  if(client->scene.DisplayReady())client->scene.RenderPublished();
  return true;
}
// Lua VM 关闭先换代并撤销本方借用，不能把 unref 当作 userdata 析构。
void ClientLuaClosing() noexcept {
  ClientLuaGuardClosed();
  if(IsMain()&&ClientBusy())client->scene.LuaClosing();
}
// registry 捕获先取得 VM 借用与有界 owner 槽，再执行可能触发 GC 的 Lua API。
bool ClientLuaRegistryCapture(int stack_index,ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return false;}
  if(!ClientBusy()||ticket.id)return false;
  ClientBorrowToken borrow{};
  if(!client->scene.Borrow(ClientPurpose::Lua,borrow))return false;
  void *owner=ClientLuaRegistryMakeOwner();
  if(!owner){client->scene.Release(borrow);return false;}
  if(!client->scene.Retain(ClientRetainedKind::LuaRegistry,ClientReleasePhase::LuaState,
                           owner,ClientLuaRegistryReleaseRaw,ticket)){
    ClientLuaRegistryDiscardOwner(owner);client->scene.Release(borrow);return false;
  }
  const bool captured=ClientLuaRegistryCaptureRaw(owner,stack_index);
  if(!captured)client->scene.RetireRetained(ticket);
  client->scene.Release(borrow);
  if(!captured)ticket={};
  return captured;
}
bool ClientLuaRegistryVisit(const ClientRetainedTicket &ticket,
                            bool (*visitor)(void *,void *) noexcept,void *context) noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return false;}
  if(!ClientBusy()||!visitor)return false;
  ClientBorrowToken borrow{};
  if(!client->scene.Borrow(ClientPurpose::Lua,borrow))return false;
  void *owner=nullptr;
  const bool valid=client->scene.UseLuaRegistry(ticket,owner);
  const bool visited=valid&&ClientLuaRegistryVisitRaw(owner,visitor,context);
  if(valid&&!visited)client->scene.Fail("lua_registry_visit_failed");
  if(!client->scene.Valid(borrow))client->scene.Fail("lua_visit_invalidated");
  client->scene.Release(borrow);
  return visited;
}
void ClientLuaRegistryRetire(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(!ClientBusy())return;
  client->scene.RetireRetained(ticket);
}
void ClientShutdownBeginning() noexcept {
  if(!IsMain()||!ClientBusy())return;
  client_exit_progress_guard.store(true,std::memory_order_release);
  // 关闭阶段只保留 Options 明确的本地写入，不能用场景计数打开全部 progress writer。
  if(client->state!=State::Retiring)StopClient();
}
void ClientContextClosing() noexcept {
  if(IsMain()&&ClientBusy())client->scene.ContextClosing();
}
// 此分派独立于 PersistenceScope 的 original exactly-once 观察；外层 wrapper 可直接拒绝。
bool ClientPersistenceBlocked() noexcept {
  return client_active.load(std::memory_order_acquire)||client_exit_progress_guard.load(std::memory_order_acquire);
}
bool ClientRetainGraphics(void *owner,ClientRetainedRelease release,ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()||!ClientBusy()||!client)return false;
  return client->scene.Retain(ClientRetainedKind::Graphics,ClientReleasePhase::ManagerContext,owner,release,ticket);
}
void ClientRetireGraphics(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(ClientBusy()&&client)client->scene.RetireRetained(ticket);
}
// Shutdown 内部两段无 Hook 的直接 PGD 写在指令入口判定，保护寿命独立于 socket。
bool ClientShutdownProgressBlocked() noexcept {return ClientPersistenceBlocked();}
// 自带纯菜单UI实际运行的主线程心跳；未加载UI时拒绝进入不可见等待状态。
void ClientUiHeartbeat() noexcept {
  if(IsMain()){ui_seen_ms=GetTickCount64();if(ClientBusy())++ui_frames;}
}
// 原生主循环关闭前完成本方线程退休；不修改窗口关闭标志，也不调用Game::Exit。
bool ClientProcessExitDecision() noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return false;}
  if(!ClientBusy())return true;
  if(!client_exit_progress_guard.exchange(true,std::memory_order_acq_rel))
    Trace("client","event=process_exit_progress_guard connection=%llu",client->connection);
  if(client->state!=State::Retiring)StopClient();
  return (client->fixture||client->network.Finished())&&client->scene.CanReturnToMenu();
}
// 详细有界聚合只在显式诊断命令读取；普通每帧status不展开128个范围。
std::string ClientStreamFields(unsigned page) {
  if(!IsMain()||!client||page>=32)return "ok=0\nerror=stream_client_unavailable\n";
  const auto &c=*client;std::string out="ok=1\nstream_diagnostics_version=1\n";
  out+=Settings.stream.Fields("stream.requested.");out+=c.config.Fields("stream.effective.");
  out+=replica::stream::LimitsFields(c.network.EffectiveLimits(),"stream.negotiated.");out+=replica::stream::LimitsFields(c.network.EffectiveFastLimits(),"stream.negotiated_fast.");
  out+=c.network.TransmitMetrics().Fields("stream.transmit.",page);
  out+=c.network.Metrics().Fields("stream.receive.",page);out+=c.scene.StreamMetrics().Fields("stream.native.",page);
  const auto t=c.network.TransferStatus();
  const auto add=[&](const char *name,std::uint64_t value){out+="stream."+std::string(name)+"="+std::to_string(value)+"\n";};
  add("fragments",t.fragments);add("duplicates",t.duplicates);add("conflicts",t.conflicts);add("decoded",t.decoded);add("decoded_bytes",t.decoded_bytes);add("decode_us",t.decode_us);add("expired",t.expired);add("ack_windows",t.ack_windows);
  out+="stream.native_recovery_count_semantics=reserved_attempts_not_completed_rebuilds\n";
  add("native_rebuilds",c.native_rebuilds);add("recoveries",c.recoveries);add("recovery_succeeded",c.recovery_succeeded);add("state_stale",c.stale);add("authority_paused",c.progress.authority_paused);
  add("owner_diagnostics_missing",c.owner_diagnostics_missing);add("failure_owner_count",c.first_owners.size());
  for(unsigned i=page*34;i<(std::min)(unsigned(c.first_owners.size()),page*34+34);++i){const auto &f=c.first_owners[i];out+="stream.failure_owner."+std::to_string(i)+"="+std::to_string(f.ticket)+"/"+std::to_string(f.kind)+"/"+std::to_string(f.phase)+"/"+std::to_string(f.resource)+"/"+std::to_string(f.lua)+"/"+std::to_string(f.context)+"/"+std::to_string(f.cpu)+"/"+std::to_string(f.gpu)+"/"+std::to_string(f.failed)+"\n";}
  add("first_failure_native_owners",c.first_scene_failure.native_owners);add("first_failure_grid_owners",c.first_scene_failure.grid_owners);add("first_failure_pending_owners",c.first_scene_failure.pending_owners);add("first_failure_instance",c.first_scene_failure.generations.instance);add("first_failure_resource",c.first_scene_failure.generations.resource);
  add("first_failure_attempt",c.first_scene_failure.attempt);add("first_failure_prefix",c.first_scene_failure.failure_executed_prefix);add("first_failure_key_run",c.first_scene_failure.failed_key.run);add("first_failure_key_id",c.first_scene_failure.failed_key.id);add("first_failure_key_space",c.first_scene_failure.failed_key.space);
  out+="stream.first_reason="+c.error+"\nstream.first_phase="+c.first_scene_failure.failure_phase.data()+"\nstream.first_native_reason="+c.first_scene_failure.failure_reason.data()+"\nstream.cleanup_failure="+c.cleanup_failure.retirement_failure.data()+"\nstream.stop_reason="+c.stop_reason+"\n";
  const auto policy=replica::stream::ClassifyFailure(c.error.c_str(),c.native_rebuilds>0,!c.scene.StreamRecoverable());
  out+="stream.reason_class="+(c.error.empty()?std::string("none"):std::string(policy.name))+"\nstream.user_message="+(c.error.empty()?std::string{}:c.recovery_succeeded?std::string("场景已恢复"):std::string(policy.message))+"\n";add("native_recovery_available",c.scene.StreamRecoverable()&&c.native_rebuilds<1&&!c.terminal_frozen);add("state_recovery_available",c.scene.StreamRecoverable()&&!c.terminal_frozen);
  PROCESS_MEMORY_COUNTERS_EX pm{};pm.cb=sizeof(pm);if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pm),sizeof(pm))){add("process_private_bytes",pm.PrivateUsage);add("process_working_set_bytes",pm.WorkingSetSize);}else out+="stream.process_memory=unavailable\n";
  const auto pools=memory::ReplicationBudget()->Snapshot();for(unsigned i=0;i<3;++i){out+="stream.pool."+std::to_string(i)+"="+std::to_string(pools[i].current)+"/"+std::to_string(pools[i].high_water)+"/"+std::to_string(pools[i].limit)+"\n";}
  return out;
}
std::string ClientFields() {
  std::string s="role="+std::string(ClientBusy()?"Client":HostBusy()?"Host":"Off")+"\nclient_state="+ClientStateName()+"\n";
  s+="client_world_applied="+std::string(client&&client->scene.DisplayReady()?"1":"0")+"\n";
  s+="client_ui_available="+std::string(ui_seen_ms&&GetTickCount64()-ui_seen_ms<=1000?"1":"0")+"\n";
  s+="client.ui_frames="+std::to_string(ui_frames)+"\n";
  s+=ClientIsolationFields();
  if(!client)return s;
  const auto &c=*client;
  const auto result=c.scene.Result();
  s+="client.target_source="+std::string(c.fixture?"e2e_value_fixture":"network")+"\n";
  const auto add=[&](const char *name,std::uint64_t value){s+="client."+std::string(name)+"="+std::to_string(value)+"\n";};
  for(unsigned i=0;i<c.original_begin.size();++i)add((std::string("original_begin.")+kIsolationOriginals[i]).c_str(),c.original_begin[i]);
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
  add("instance_generation",c.scene.Generations().instance);
  add("resource_generation",c.scene.Generations().resource);
  add("lua_generation",c.scene.Generations().lua);
  add("context_generation",c.scene.Generations().context);
  add("native_attempt",result.attempt);
  add("scene_state",unsigned(c.scene.State()));
  add("native_result",unsigned(result.kind));
  add("native_revision",result.revision);
  add("player_status_version",1);add("target_revision",c.scene.TargetRevision());
  add("player_role_types",c.scene.PlayerRoleTypes());add("player_body_roles",c.scene.PlayerBodyRoles());
  add("applied_revision",c.scene.AppliedRevision());
  add("native_scope_run",result.scope.run);
  add("native_scope_scene",result.scope.scene);
  add("native_executed_prefix",result.executed_prefix);
  add("native_committed",result.native_committed);
  add("native_committed_domains",result.committed_domains);
  add("non_player_entity_applied",result.non_player_entity_applied);
  add("player_support_applied",result.player_support_applied);
  add("full_entity_applied",result.full_entity_applied);
  add("player_bodies_applied",result.player_bodies_applied);
  add("player_relations_applied",result.player_relations_applied);
  add("player_resources_applied",result.player_resources_applied);
  add("player_domain_applied",result.player_domain_applied);
  add("player_random_denied",ClientPlayerRandomDenials());
  add("failure_executed_prefix",result.failure_executed_prefix);
  add("stream_stale",c.stale);add("authority_paused",c.progress.authority_paused);
  add("stream_recoveries",c.recoveries);add("stream_native_rebuilds",c.native_rebuilds);add("stream_recovery_requested",c.recovery_requested);add("stream_recovery_succeeded",c.recovery_succeeded);
  s+="client.stop_reason="+c.stop_reason+"\n";
  add("recovery_pending",c.scene.RecoveryPending());
  s+="client.recovery_state="+std::string(c.scene.RecoveryState())+"\n";
  // 主线程帧尾复制稳定Key/字段/前缀；控制口和Rust只读字符串快照，不访问Player地址。
  const auto player_diagnostic=[&](const char *prefix,const ClientPlayerDiagnostics &d){
    const auto number=[&](const char *name,std::uint64_t value){add((std::string(prefix)+name).c_str(),value);};
    number("key_run",d.body.run);number("key_space",d.body.space);number("key_id",d.body.id);
    number("dependency_run",d.dependency.run);number("dependency_space",d.dependency.space);number("dependency_id",d.dependency.id);
    number("instance",d.instance);number("shape",d.shape);number("prefix",d.prefix);number("body_role",d.body_role);
    s+="client."+std::string(prefix)+"player_type="+std::to_string(d.player_type)+"\n";
    s+="client."+std::string(prefix)+"phase="+d.phase.data()+"\n";
    s+="client."+std::string(prefix)+"field="+d.field.data()+"\n";
    s+="client."+std::string(prefix)+"reason="+d.reason.data()+"\n";
  };
  player_diagnostic("player_step_",result.player_step);player_diagnostic("player_failure_",result.player_failure);
  s+="client.retirement_cause="+std::string(result.retirement_cause.data())+"\n";
  s+="client.retirement_failure="+std::string(result.retirement_failure.data())+"\n";
  const auto players=c.scene.PlayerBudget();
  add("player_participants",players.participants);add("player_group_nodes",players.group_nodes);
  add("player_bodies",players.bodies);add("player_roster",players.roster);add("player_non_roster",players.non_roster);add("player_backups",players.backups);
  add("player_inventory_entries",players.inventory);add("player_effects",players.effects);add("player_history",players.history);
  add("player_costumes",players.costumes);add("player_weapons",players.weapons);add("player_slots",players.slots);add("player_records",players.records);
  add("player_dynamic_hud_owners",players.hud_owners);
  add("player_references",players.references);add("player_resource_slots",players.resources);add("player_layers",players.layers);add("player_selectors",players.selectors);
  add("player_plan_cost",players.steps);add("player_source_bytes",players.source_bytes);add("player_source_nodes",players.source_nodes);add("player_owned_estimate_bytes",players.owned_bytes);
  // 只在显式status查询采进程实际内存；估算的模型成本与操作系统值分开，不写逐tick日志。
  PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);
  if(K32GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),sizeof(memory))){
    add("process_working_set_bytes",memory.WorkingSetSize);add("process_private_bytes",memory.PrivateUsage);
  }
  const auto entity_budget=c.scene.EntityBudget();
  add("entity_members",entity_budget.members);add("entity_players",entity_budget.players);add("entity_retired_rows",entity_budget.retired);
  add("entity_resource_slots",entity_budget.resources);add("entity_references",entity_budget.references);add("entity_curve_points",entity_budget.curve_points);
  add("entity_commands",entity_budget.commands);add("entity_commands_completed",entity_budget.completed);
  add("entity_independent_pending",c.scene.EntityIndependentPending());
  const auto independent=c.scene.EntityIndependentStatus();
  add("entity_independent_presented",independent.presented);add("entity_independent_expired",independent.expired);
  add("entity_independent_superseded",independent.superseded);add("entity_independent_bytes",independent.bytes);
  add("grid_prefix",result.grid_prefix);
  add("grid_owners",result.grid_owners);
  add("failure_key_run",result.failed_key.run);
  add("failure_key_space",result.failed_key.space);
  add("failure_key_id",result.failed_key.id);
  add("native_pending_owners",result.pending_owners);
  add("pending_owners",result.pending_owners);
  add("native_owners",result.native_owners);
  add("native_written",result.native_written);
  add("native_old_display_valid",result.old_display_valid);
  add("old_display_valid",result.old_display_valid);
  add("dispatch_last_entry",c.dispatch_last_entry);
  add("dispatch_last_decision",c.dispatch_last_decision);
  add("callback_skipped",c.callback_skipped);
  add("callback_own",c.callback_own);
  add("callback_fault",c.callback_fault);
  for(unsigned i=0;i<c.dispatch_original.size();++i)
    if(c.dispatch_original[i]||c.dispatch_expected[i]||c.dispatch_fault[i])
      s+="client.dispatch."+std::string(kClientEntryNames[i])+"="+
         std::to_string(c.dispatch_original[i])+","+
         std::to_string(c.dispatch_expected[i])+","+
         std::to_string(c.dispatch_fault[i])+"\n";
  const auto budget=c.scene.Budget();
  add("target_bytes",budget.bytes);add("target_nodes",budget.nodes);add("target_resources",budget.resources);
  add("target_layers",budget.layers);add("target_aliases",budget.aliases);add("target_fx",budget.fragments);
  add("unadopted_descriptor_grid_indices",budget.unadopted_grid_indices);
  if(Settings.diagnostics&&!Settings.world_e2e_token.empty()){
    void *dc=nullptr,*context=nullptr;
    if(ClientRendererHandles(dc,context))add("renderer_window",reinterpret_cast<std::uintptr_t>(WindowFromDC(static_cast<HDC>(dc))));
  }
  const auto resources=ClientResourcesStatistics();add("resource_unchanged",resources.unchanged);
  add("resource_graphics_reused",resources.reused_graphics);add("resource_candidate_loads",resources.candidate_loads);
  s+="client.native_phase="+std::string(result.phase.data())+"\n";
  s+="client.native_reason="+std::string(result.reason.data())+"\n";
  s+="client.failure_domain="+std::string(result.failed_domain.data())+"\n";
  s+="client.failure_phase="+std::string(result.failure_phase.data())+"\n";
  s+="client.failure_reason="+std::string(result.failure_reason.data())+"\n";
  s+="client.recovery="+std::string(result.recovery.data())+"\n";
  s+="client.error="+c.error+"\n";return s;
}
} // namespace cs::game
