#include "game/tests/client_entity_e2e.h"
#include "game/client/session/client.h"
#include "game/mods/mod_api.h"
#include "game/mods/mod_native.h"
#include "game/client/scene/client_scene.h"
#include "game/client/scene/client_binary_scene.h"
#include "game/client/player/client_player_control.h"
#include "state/client_memory.h"
#include "game/client/display/client_display_bridge.h"
#include "game/client/display/client_pause_menu.h"
#include "state/display_contract.h"
#include "game/client/isolation/client_player_isolation.h"
#include "game/client/isolation/client_isolation.h"
#include "replica/stream_e2e.h"
#include "game/tests/e09_compare.h"
#include "binding/offsets.h"
#include "game/common/context.h"
#include "game/common/binary_effect_e2e.h"
#include "game/common/display_paths.h"
#include "game/input/input_a.h"
#include "game/authority/tick.h"
#include "game/participants/topology.h"
#include "net/live.h"
#include "net/timing_clock.h"
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
enum class State { Off, Connecting, AwaitingWorld, PreparingWorld, Retiring, Failed };
constexpr const char *kIsolationOriginals[]={"start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"};
// 输入目标只投影到预计抵达服务器的时刻；不推进本地/Authority时钟，不放行过期命令。
// 速率由服务器实际提交tick与同机单调时间的差值建立；暂停/换代立即丢弃旧估计。
struct InputTiming final {
  std::uint64_t anchor_ms=0,anchor_tick=0,rate_ticks=0,rate_ms=0,
                received_ms=0,round_trip_ms=0,publication_age_ms=0,timing_uncertainty_ms=0;
  void Observe(const net::live::Event &event) noexcept {
    const auto &p=event.progress;
    if(!p.input_enabled||p.session_state!=core::SessionState::Active||!p.generation) {
      *this={};return;
    }
    received_ms=event.received_ms;round_trip_ms=event.round_trip_ms;
    publication_age_ms=event.publication_age_ms;
    timing_uncertainty_ms=event.timing_uncertainty_ms;
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
    const auto age=now-received_ms+round_trip_ms+publication_age_ms+timing_uncertainty_ms;
    // 使用完整往返时间覆盖下行旧样本和本次上行，不把半程时间误当总时差。
    const auto projection=(age*rate_ticks+rate_ms-1)/rate_ms;
    if(projection>core::input::kMaximumFutureTicks-core::input::kInputLeadTicks||tick>UINT64_MAX-projection)return false;
    return core::NextInputTarget(tick+projection,core::input::InputRuntimeConfig{},target);
  }
};
// 预测插值阶段0的端到端延迟计时，只做客机诊断。
// 不改线格式、输入、目标tick或显示。Host的Body/World不带发送时刻，
// 所以“已生效”以ACK覆盖该序号为准，“已显示”以随后第一次Body发布进入原生为准。
// 后者是上界近似：同一Manager返回窗口的ACK与Body是两条独立消息，Body也不标注输入序号。
struct InputLatencyProbe final {
  // 已发意图的发送记录，按序号取模存放；被覆盖或代次变化后的旧序号不再计时。
  struct Sent final {std::uint64_t sequence=0,target=0,server_tick=0,sent_us=0;};
  std::array<Sent,128> ring{};
  // 发出到ACK覆盖该序号；起步（中性→非零移动）到ACK、到随后第一次Body发布。
  replica::stream::Duration ack_us,onset_ack_us,onset_body_us;
  // 发送时的提前量：target-server_tick（tick数）及按观测节拍折算的毫秒。
  replica::stream::Duration lead_ticks,lead_ms;
  std::uint64_t acked_through=0,ack_ring_miss=0;
  // ACK的processed_tick相对该序号target：早于、恰好、晚于（晚于表示Host在更晚的tick才处理）。
  std::uint64_t processed_early=0,processed_on_time=0,processed_late=0,processed_late_max=0;
  // 起步检测：只跟踪一个未完成的起步；新起步顶替旧的时计入dropped，超过2秒未完成计入expired。
  bool last_moving=false,onset_pending=false,onset_acked=false;
  std::uint64_t onset_sequence=0,onset_us=0,onset_body_base=0;
  std::uint64_t onsets=0,onsets_dropped=0,onsets_expired=0;
  // 记录一次成功发送；rate为InputTiming观测到的Host节拍，未知时不记毫秒提前量。
  void Record(std::uint64_t sequence,std::uint64_t target,std::uint64_t server_tick,bool moving,
             std::uint64_t rate_ticks,std::uint64_t rate_ms) noexcept {
    const auto now=replica::stream::MetricMicros();
    ring[sequence%ring.size()]={sequence,target,server_tick,now};
    const auto lead=target>server_tick?target-server_tick:0;
    lead_ticks.Observe(lead);
    if(rate_ticks&&rate_ms)lead_ms.Observe(lead*rate_ms/rate_ticks);
    if(moving&&!last_moving){
      if(onset_pending)++onsets_dropped;
      onset_pending=true;onset_acked=false;onset_sequence=sequence;onset_us=now;++onsets;
    }
    last_moving=moving;
  }
  // ACK推进到processed；body_count为当时已发布的Body批次数，用来判定随后的第一次Body。
  void Acked(std::uint64_t processed,std::uint64_t processed_tick,std::uint64_t body_count) noexcept {
    if(processed<=acked_through)return;
    acked_through=processed;
    const auto now=replica::stream::MetricMicros();
    const auto &s=ring[processed%ring.size()];
    if(s.sequence!=processed){++ack_ring_miss;}
    else {
      ack_us.Observe(now-s.sent_us);
      if(processed_tick<s.target)++processed_early;
      else if(processed_tick==s.target)++processed_on_time;
      else {++processed_late;processed_late_max=(std::max)(processed_late_max,processed_tick-s.target);}
    }
    if(onset_pending&&!onset_acked&&processed>=onset_sequence){
      onset_ack_us.Observe(now-onset_us);onset_acked=true;onset_body_base=body_count;
    }
  }
  // 本轮安装与显示推进之后调用；ACK之后出现的第一次Body发布即视为该起步首次可见。
  void BodyObserved(std::uint64_t body_count) noexcept {
    if(!onset_pending)return;
    const auto now=replica::stream::MetricMicros();
    if(onset_acked&&body_count>onset_body_base){onset_body_us.Observe(now-onset_us);onset_pending=false;return;}
    if(now-onset_us>2000000){++onsets_expired;onset_pending=false;}
  }
  // 代次或绑定变化后旧序号与旧起步作废；直方图累计保留。
  void Forget() noexcept {ring={};onset_pending=false;onset_acked=false;last_moving=false;}
  // 诊断文本：计数、p50、p95、最大值（微秒或tick），前缀为stream.latency.。
  std::string Fields() const {
    std::string out;
    const auto add=[&](const char *name,std::uint64_t value){out+="stream.latency."+std::string(name)+"="+std::to_string(value)+"\n";};
    const auto hist=[&](const char *name,const replica::stream::Duration &d){
      const std::string n(name);
      add((n+"_count").c_str(),d.count);add((n+"_p50").c_str(),d.Percentile(50));
      add((n+"_p95").c_str(),d.Percentile(95));add((n+"_max").c_str(),d.max_us);
    };
    hist("input_ack_us",ack_us);hist("lead_ticks",lead_ticks);hist("lead_ms",lead_ms);
    hist("onset_ack_us",onset_ack_us);hist("onset_body_us",onset_body_us);
    add("ack_ring_miss",ack_ring_miss);add("processed_early",processed_early);add("processed_on_time",processed_on_time);
    add("processed_late",processed_late);add("processed_late_max",processed_late_max);
    add("onsets",onsets);add("onsets_dropped",onsets_dropped);add("onsets_expired",onsets_expired);
    return out;
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
  bool have_scene_failure=false,terminal_frozen=false,stale=false,was_paused=false;
#ifdef ISAACCS_E2E
  bool visual_held=false;
#endif
  std::uint64_t stale_origin=0;
  std::string stop_reason;

  ClientScene scene;
  std::unique_ptr<ClientBinaryScene> binary;
  State state=State::Connecting;
  replica::Scope scope{};
  net::protocol::Ack progress{};
  replica::NightmareFrameRef nightmare;
  std::uint64_t nightmare_route=0,nightmare_received=0;
  std::optional<net::live::Event> prepare,retire_control;
  bool prepare_sent=false;
  std::array<std::uint64_t,6> prepare_progress{};
  unsigned ordinal=0, roster=0;
  // 本地选角和准备位使用最新完整值，Host 快照回显实际已接收的选择。
  unsigned lobby_selection=0, lobby_port=0;
  bool lobby_mode=false;
  std::uint64_t route=1;
  int device_key=0;
  std::uint64_t connection=0, sequence=0, input_ack=0, input_ack_tick=0,
                last_target=0, last_tick=0, generation=0, sampled=0, sent=0,
                manager_skipped=0, device_missing=0, render_calls=0;
  unsigned sent_edge_mask=0;
  core::ShootingDirections shooting;
  core::InputStateDraftV2 sample{};
  // 最近一条已发命令的采样（不含边沿），用于判断同一目标是否需要以更高序号重发。
  core::InputStateDraftV2 last_sent{};
  // 补发跳过 tick 的命令数、同一目标重发数（诊断）。
  std::uint64_t fill_sent=0,resent=0;
  struct InputEdge { unsigned actions; std::uint64_t keys; };
  std::array<InputEdge,16> edges{};
  unsigned edge_head=0,edge_count=0;
  unsigned held=0;
  bool baseline=false, eligible=false, sampled_this_loop=false, blocked=true;
  bool physical_neutral=false;
  bool map_pressed=false;
  bool focused=false,ui_blocked=false,input_ready=false;
  // 本地暂停菜单选择了“退出”：本轮显示推进完成后再离开会话，不在显示推进前撤场景。
  bool pause_exit=false;
  InputTiming timing;
  InputLatencyProbe latency; // 阶段0纯诊断计时，不参与任何输入或显示判定
  ClientPlayerControlView control;
  unsigned neutral_loops=0;
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
thread_local unsigned minimap_input_depth=0;
thread_local ClientBorrowToken render_observation{};
thread_local ClientBorrowToken native_render_graphics{};
thread_local std::optional<ClientDispatchScope> native_render_dispatch;
thread_local bool native_render_routed=false;
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
  std::array<unsigned,unsigned(ClientEntry::Count)> nested{};
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
  "native_build","native_apply","native_teardown","presentation_maintenance","presentation_draw","menu_render","backdrop_effects",
  "display_hud_update","display_hud_post","display_minimap_update","display_minimap_refresh","display_sort","display_fx",
  "display_background","display_transition","display_camera","display_sfx_refresh","display_music","display_sfx_replay","display_pause","display_cursor","transition_change_room","binary_native_preparation","binary_native_atomic_install","mod_state_install"};
static_assert(std::size(kClientEntryNames)==unsigned(ClientEntry::Count));
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
    case ClientOperation::ModInstall:
      return owner==Owner::Manager&&root==ClientEntry::ModStateInstall;
    case ClientOperation::BinaryInstall:
    case ClientOperation::BinaryInstallRetire:
      return owner==Owner::Manager&&root==ClientEntry::NativeAtomicInstall&&
        ClientBinaryInstallationScopeActive(operation==ClientOperation::BinaryInstallRetire);
    case ClientOperation::BinaryPreparation:
    case ClientOperation::BinaryPreparationRetire:
      return owner==Owner::Manager&&root==ClientEntry::NativePreparation&&
        ClientBinaryPreparationScopeActive(operation==ClientOperation::BinaryPreparationRetire);
    case ClientOperation::Build:
      return state==ClientSceneState::Preparing&&owner==Owner::Manager&&
          root==ClientEntry::NativeBuild;
    case ClientOperation::Apply:
      return state==ClientSceneState::Applying&&owner==Owner::Manager&&
          root==ClientEntry::NativeApply;
    case ClientOperation::PresentMaintenance:
      return (state==ClientSceneState::Applying||state==ClientSceneState::Displaying)&&owner==Owner::Manager&&
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
  c.terminal_frozen=true;c.pause_exit=false;
  // 本地暂停菜单属于当前会话；失败退休时立即关闭，不把打开的菜单留给本机菜单或下一局。
  ClientPauseMenuReset();
  Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};
  c.prepare.reset();c.retire_control.reset();c.state=State::Retiring;c.neutral_loops=0;
  try {if(c.error.empty())c.error=reason;c.stop_reason=reason;}catch(...){}
  if(c.binary)c.binary->Close();
  c.scene.Retire(reason);c.network.Stop();
  Trace("client_error","event=failed_retiring connection=%llu reason=%s",c.connection,reason);
}
void SceneFailure(ClientRuntime &c) noexcept {
  if(c.scene.RetirementFault()){
    RememberFailure(c);c.cleanup_failure=c.scene.Result();
    Failure(c,"native_retirement_failed");c.state=State::Failed;return;
  }
  if(c.scene.State()==ClientSceneState::Failed&&c.state!=State::Retiring&&c.state!=State::Failed)
    Failure(c,c.scene.Result().reason.data());
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
  if(!ClientEntityInitIsolationInstalled())return "client_entity_isolation_unavailable";
  if(!ClientPlayerIsolationInstalled())return "client_player_isolation_unavailable";
  if(connections==UINT64_MAX)return "connection_exhausted";
  return "";
}
}
bool ClientBusy() noexcept {return client_active.load(std::memory_order_acquire);}
bool ClientModIntentContext(net::ModIntent &intent) noexcept {
  return IsMain()&&ClientBusy()&&client&&client->binary&&client->scene.DisplayReady()&&
    !client->terminal_frozen&&client->binary->ModIntentWorld(intent);
}
bool SubmitClientModIntent(const net::ModIntent &intent) noexcept {
  return client&&client->network.SendModIntent(intent);
}
std::uint16_t ClientModIntentMember(const void *object) noexcept {
  return client&&client->binary?client->binary->ModIntentMember(object):state::kModGlobal;
}
bool ClientLocalOrdinal(unsigned &ordinal) noexcept {
  if(!IsMain()||!ClientBusy()||!client||!client->scope.session)return false;
  ordinal=client->ordinal;
  return true;
}
bool ClientLinkQuality(ClientLinkStats &stats) noexcept {
  if(!IsMain()||!ClientBusy()||!client||!client->scope.session)return false;
  const auto q=client->network.Quality();
  stats={q.rtt_valid,q.loss_valid,q.rtt_ms,q.loss_permille,q.loss_window};
  return true;
}
ClientMinimapInputScope::ClientMinimapInputScope() noexcept {++minimap_input_depth;}
ClientMinimapInputScope::~ClientMinimapInputScope() noexcept {--minimap_input_depth;}
// ZHL 在原生调用者和 C++ Hook 之间插入调用层，因此用小地图更新作用域限定 ACTION_MAP。
bool ClientMinimapMapPressed(int action,bool &pressed) noexcept {
  if(action!=13||!minimap_input_depth||!client_depth||!IsMain()||!ClientBusy())return false;
  const auto &c=*client;
  pressed=c.state==State::AwaitingWorld&&c.sampled_this_loop&&!c.blocked&&c.map_pressed;
  return true;
}
// 仅主线程复制现有Scene身份；外线程诊断保留未知，不读取该控制块。
ClientIsolationIdentity ClientIsolationCurrent() noexcept {
 if(!IsMain()||!client)return {};const auto &g=client->scene.Generations();
 return {client->connection,client->scope.connection,client->route,g.scene,g.instance,g.resource,g.lua,g.context,unsigned(client->scene.State())};
}
// 检查真实 trampoline，不能拿清单行或能力组存在冒充已经安装。
bool ClientDispatchIsolationInstalled() noexcept {
  if(!IsMain()||!Capabilities[size_t(Group::Client)]||!ClientCallbackGuardReady())return false;
  static constexpr const char *required[]={"client_game_update","client_post_update",
    "client_room_update","client_entity_list","client_player_update","client_odd_helper",
    "client_room_lifecycle","client_door_update","client_door_nested","client_new_room",
    "room_init","game_render","room_prerender","room_render","entitylist_render_sort","client_game_start","client_entity_draw_paused","client_entity_draw_projectile","client_backdrop_effects","client_backdrop_wall","client_backdrop_graphics"};
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
// 主菜单输入和绘制共用稳定菜单检查；绘制另须持有本帧的场景借用。
bool ClientCallbackInStableMenu() noexcept {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return false;}
  return ClientBusy()&&client&&!dispatch_frame.id&&!InBlockedContext()&&ContextHealthy()&&MenuReady();
}
bool ClientCallbackMenuAllowed() noexcept {
  return ClientCallbackInStableMenu()&&TopOwner()==Owner::Render&&
         render_observation.active&&client->scene.Valid(render_observation);
}
ClientDispatchScope::ClientDispatchScope(ClientOperation operation,ClientEntry root,
                                         std::uint64_t expected_skips) noexcept
    : exceptions_(std::uncaught_exceptions()) {
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(!ClientBusy()||!client)return;
  auto &c=*client;
  const auto owner=TopOwner();
  const bool root_allowed=DispatchRootAllowed(operation,root,c.scene.State(),owner);
  if(dispatch_frame.id||!ContextHealthy()||InBlockedContext()||!ClientDispatchIsolationInstalled()||
     (expected_skips&~kExpectedNestedMask)||
     (expected_skips&EntryBit(root))||
     !root_allowed||next_dispatch_frame==UINT64_MAX){
    // 分派首因保留具体门槛，区分票据资格撤销、嵌套分派与主线程上下文错误。
    Trace("client_error","event=dispatch_admission operation=%u root=%u owner=%u state=%u active_frame=%llu healthy=%u blocked=%u installed=%u root_allowed=%u expected_skips=%llu",
      unsigned(operation),unsigned(root),unsigned(owner),unsigned(c.scene.State()),dispatch_frame.id,
      unsigned(ContextHealthy()),unsigned(InBlockedContext()),unsigned(ClientDispatchIsolationInstalled()),unsigned(root_allowed),expected_skips);
    Failure(c,"client_dispatch_admission");return;
  }
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
      // 私有准备前缀归二进制 owner，不挂到旧 JSON 世界的退休位。
      if(frame.operation!=ClientOperation::BinaryPreparation&&frame.operation!=ClientOperation::BinaryPreparationRetire&&
         frame.operation!=ClientOperation::BinaryInstall&&frame.operation!=ClientOperation::BinaryInstallRetire&&
         frame.operation!=ClientOperation::ModInstall&&
         !frame.scene->NoteNativeDispatchEntry()){
        frame.fault=true;Failure(*client,"client_dispatch_prefix_budget");return decide(ClientEntryDecision::FaultSkip);
      }
      frame.called=true;return decide(ClientEntryDecision::CallOriginal);
    }
    if(frame.operation==ClientOperation::ModInstall){
      ModInstallBlocked();
      ClientIsolationObserve(ClientIsolationKind::ModInstall,kClientEntryNames[unsigned(entry)]);
      return decide(ClientEntryDecision::FaultSkip);
    }
    // 原生显示树只允许明确登记的显示入口；玩法Update仍走原先ExpectedSkip／拒绝。
    const bool display_entry=NativeDisplayPath(entry);
    if(frame.called&&!frame.returned&&!frame.fault&&display_entry&&ClientNativeDisplayCallbacks()){
      if(frame.nested[unsigned(entry)]++){frame.fault=true;Failure(*client,"client_display_entry_reentrant");return decide(ClientEntryDecision::FaultSkip);}
      return decide(ClientEntryDecision::CallOriginal);
    }
    if(entry==ClientEntry::TransitionChangeRoom)return decide(ClientEntryDecision::ExpectedSkip);
    if(frame.called&&!frame.returned&&(frame.expected&EntryBit(entry))&&
       !frame.fault)return decide(ClientEntryDecision::ExpectedSkip);
    if(unsigned(entry)<std::size(kClientEntryNames))
      Trace("client_error","event=unexpected_native_entry root=%s entry=%s operation=%u",
            kClientEntryNames[unsigned(frame.root)],kClientEntryNames[unsigned(entry)],unsigned(frame.operation));
    frame.fault=true;
  }
  Failure(*client,"client_dispatch_unexpected_entry");
  return decide(ClientEntryDecision::FaultSkip);
}
void ClientDispatchReturned(ClientEntry entry) noexcept {
  if(!ClientBusy())return;
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(client&&dispatch_frame.id&&entry!=dispatch_frame.root&&unsigned(entry)<dispatch_frame.nested.size()&&
     dispatch_frame.nested[unsigned(entry)]==1&&!dispatch_frame.fault&&ContextHealthy()&&
     dispatch_frame.scene==&client->scene&&dispatch_frame.called&&!dispatch_frame.returned&&
     dispatch_frame.scene->Valid(dispatch_frame.borrow)&&dispatch_frame.scene->State()==dispatch_frame.state&&
     dispatch_frame.borrow.generations==client->scene.Generations()&&OwnerToken(dispatch_frame.owner)==dispatch_frame.owner_token){
    dispatch_frame.nested[unsigned(entry)]=0;
    ClientIsolationObserve(ClientIsolationKind::NativeReturn,kClientEntryNames[unsigned(entry)]);return;
  }
  if(!client||!dispatch_frame.id||dispatch_frame.root!=entry||
     dispatch_frame.scene!=&client->scene||!dispatch_frame.called||dispatch_frame.returned||
     dispatch_frame.fault||!ContextHealthy()||
     !dispatch_frame.scene->Valid(dispatch_frame.borrow)||
     dispatch_frame.borrow.generations!=client->scene.Generations()||
     client->scene.State()!=dispatch_frame.state){
    if(client){
      // 保留首个不配对的实际入口与作用域状态，避免退休后的次生错误掩盖显示失败来源。
      Trace("client_error","event=dispatch_return_mismatch entry=%u root=%u id=%llu called=%u returned=%u fault=%u nested=%u state=%u expected_state=%u valid=%u context=%u",
        unsigned(entry),unsigned(dispatch_frame.root),static_cast<unsigned long long>(dispatch_frame.id),
        unsigned(dispatch_frame.called),unsigned(dispatch_frame.returned),unsigned(dispatch_frame.fault),
        unsigned(entry)<dispatch_frame.nested.size()?dispatch_frame.nested[unsigned(entry)]:0u,
        unsigned(client->scene.State()),unsigned(dispatch_frame.state),unsigned(client->scene.Valid(dispatch_frame.borrow)),unsigned(ContextHealthy()));
      Failure(*client,"client_dispatch_return_mismatch");
    }return;
  }
  if(std::any_of(dispatch_frame.nested.begin(),dispatch_frame.nested.end(),[](unsigned n){return n!=0;})){
    dispatch_frame.fault=true;Failure(*client,"client_display_nested_incomplete");return;}
  dispatch_frame.returned=true;
  ClientIsolationObserve(ClientIsolationKind::NativeReturn,kClientEntryNames[unsigned(entry)]);
}
const char *ClientStateName() noexcept {
  if(!client)return "Off";
  switch(client->state) {
    case State::Off:return "Off";case State::Connecting:return "Connecting";
    case State::AwaitingWorld:return "AwaitingWorld";case State::PreparingWorld:return "PreparingWorld";case State::Retiring:return "Retiring";
    case State::Failed:return "Failed";
  }
  return "Failed";
}
const char *StartClient(const std::string &ipv4,unsigned port,int device_key,bool lobby) noexcept {
  const auto unavailable=ClientStartUnavailable();if(*unavailable)return unavailable;
  if(!BinaryDisplayCaptureHooksInstalled()||!ClientDisplayPatchInstalled())return "client_binary_display_hooks_unavailable";
  if(!ui_seen_ms||GetTickCount64()-ui_seen_ms>1000)return "client_ui_unavailable";
  if(port<1||port>65535||device_key<0||device_key>255)return "arguments";
  try {
    net::ModManifestRef mods;
    if(const auto *error=CaptureModManifest(mods);*error)return error;
    auto next=std::make_unique<ClientRuntime>();next->connection=++connections;
    next->address=ipv4;next->device_key=device_key;next->config=Settings.stream;
    next->lobby_mode=lobby;next->lobby_port=port;
    const auto memory=state::ClientStateMemory::Create();
    next->binary=std::make_unique<ClientBinaryScene>(next->scene,next->network,memory);
    const auto error=next->network.Start(ipv4,std::uint16_t(port),Settings.peer_timeout_ms,next->config,
      ClientBinaryScene::Preparation(),memory,lobby,std::move(mods),FrozenModSchema());
    if(*error)return error;
    // 连接开始时冻结原生调用计数起点。
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
const char *UpdateClientLobby(int character, bool ready) noexcept {
  if (!IsMain() || TopOwner()!=Owner::Manager || !ClientBusy() || !client || !client->lobby_mode ||
      client->state==State::Retiring || client->state==State::Failed ||
      core::LobbyRoster::Unpack(client->network.LobbyRoster()).phase!=core::LobbyPhase::Waiting)
    return "lobby_not_open";
  auto &c=*client;
  const auto selected=character<0 ? c.lobby_selection&63 : unsigned(character);
  if (!core::LobbyCharacter(selected)) return "lobby_character";
  // 客机只能选房主存档已解锁的角色；位图随房主名单下发。
  if (!core::LobbyCharacterUnlocked(c.network.LobbyUnlocked(), selected)) return "lobby_character_locked";
  const auto selection=selected | (ready ? core::kLobbyReady : 0);
  if (!c.network.SetLobbySelection(selection)) return "lobby_connection_closed";
  c.lobby_selection=selection;
  return "";
}

core::LobbyRoster ClientLobbyRoster(unsigned &ordinal, unsigned &desired, std::string &details) {
  if (!client || !client->lobby_mode) return {};
  const auto &c=*client;
  ordinal=c.ordinal;
  desired=c.lobby_selection;
  details="addresses="+c.address+"\nport="+std::to_string(c.lobby_port)+
          "\nretiring="+std::to_string(c.state==State::Retiring || c.state==State::Failed)+
          "\nsession_error="+c.error+"\n";
  return core::LobbyRoster::Unpack(c.network.LobbyRoster());
}

std::uint64_t ClientLobbyUnlocked() noexcept {
  if (!client || !client->lobby_mode) return 0;
  return client->network.LobbyUnlocked();
}

std::string ClientLobbyFailure() {
  return client && client->lobby_mode && client->state == State::Off ? client->error : std::string{};
}
#ifdef ISAACCS_E2E
// 统一实机验收在已发布真实场景检验owner拒绝；worker只携带自有纯值模型。
std::string ClientTestExerciseOwnership(const std::string &token) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||
     !ClientBusy()||!client||!client->scene.DisplayReady())return "ok=0\nerror=client_ownership_context\n";
  try {
    bool accepted=true;
    std::thread foreign([&]{ClientBorrowToken token;accepted=client->scene.Borrow(ClientPurpose::Graphics,token);});
    foreign.join();
    if(accepted)return "ok=0\nerror=foreign_graphics_borrow_accepted\n";
    client->scene.ConsumeForeignFault();
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
  if(!IsMain()||TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||!client||!client->scene.DisplayReady())return "ok=0\nerror=vm_exercise_context\n";
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
void StopClient() noexcept {
  if(!IsMain()||!ClientBusy())return;
  if(client->terminal_frozen||client->state==State::Retiring)return;
  if(client->binary)client->binary->Close();
  ClientIsolationObserve(ClientIsolationKind::Stop,"client_stop");
  client->terminal_frozen=true;client->stop_reason="client_stop";client->pause_exit=false;
  ClientPauseMenuReset(); // 正常离开同样立即关闭本地暂停菜单
  Neutral(*client,true);client->network.RevokeInput();client->input_ready=false;client->timing={};client->progress={};client->prepare.reset();client->retire_control.reset();client->state=State::Retiring;client->scene.Retire("client_stop");client->neutral_loops=0;client->network.Stop();
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
    SceneFailure(c);
    c.scene.Pump();
#ifdef ISAACCS_E2E
    if(c.scene.State()==ClientSceneState::Retiring){const auto r=c.scene.Result();
      replica::stream::E2EStreamProgress("client-boundary-retiring",{r.revision,0,unsigned(c.state),r.executed_prefix,0,GetTickCount64(),0,0});}
#endif
    c.scene.DrainRetained();
    SceneFailure(c);
    if(foreign_client.exchange(false))Failure(c,"client_foreign_manager");
    ++c.manager_skipped;
    const auto deliver_mod_results=[&]{
      net::ModIntentResult result;
      while(c.network.PollModResult(result)){
        DispatchModIntentResult(result,true);c.network.FinishModIntent(result.id);
      }
    };
    net::live::Event event;
    for(unsigned i=0;i<256&&c.network.Poll(event);++i) {
      if(c.terminal_frozen)continue;
      switch(event.type) {
        case net::live::Event::Type::Welcome:
          if(c.state!=State::Connecting)break;
          c.scope=event.scope;c.route=event.route;c.scene.Begin(event.scope);
          if(c.scene.State()==ClientSceneState::Failed){Failure(c,"client_scene_begin_failed");break;}
          c.ordinal=event.ordinal;c.state=State::AwaitingWorld;
          Trace("client","event=welcome connection=%llu ordinal=%u session=%llu wire_connection=%llu",c.connection,c.ordinal,c.scope.session,c.scope.connection);break;
        case net::live::Event::Type::Prepared:Failure(c,"client_prepare_on_notification_queue");break;
        case net::live::Event::Type::Nightmare:
          if(event.scope==c.scope&&event.route==c.route){
            c.nightmare=std::move(event.nightmare);
            c.nightmare_route=event.route;c.nightmare_received=event.received_ms;
            // 演出期间没有玩法快照，收到当前路由持续推进的演出就刷新接收时限。
            c.stale_origin=(std::max)(c.stale_origin,event.received_ms);
            if(!c.nightmare)ReleaseNightmareSprites();
          }
          break;
        case net::live::Event::Type::Progress:
        case net::live::Event::Type::TickProgress:
          if(c.state!=State::AwaitingWorld)break;
          if(c.scope!=event.scope||c.route!=event.route||c.ordinal!=event.ordinal){Failure(c,"client_scope_conflict");break;}
          if(event.progress.eligibility_revision<c.progress.eligibility_revision)break;
          if(c.generation!=event.progress.generation) {
            Neutral(c,true);c.network.RevokeInput();c.timing={};c.latency.Forget();c.generation=event.progress.generation;c.last_tick=0;
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
            c.latency.Acked(c.input_ack,c.input_ack_tick,c.scene.AppliedBodyCount());
          }
          break;
        case net::live::Event::Type::Control:Failure(c,"client_world_on_notification_queue");break;
        case net::live::Event::Type::Stopped:if(!c.terminal_frozen)Failure(c,event.reason);break;
        case net::live::Event::Type::Failed:if(!c.terminal_frozen)Failure(c,event.reason);break;
      }
    }
    // 控制信封只准备和退休路由，世界批次由二进制邮箱交给原生协调器。
    for(unsigned n=0;(c.state==State::AwaitingWorld||c.state==State::PreparingWorld)&&n<16&&c.network.PollWorld(event);++n){
      if(event.type==net::live::Event::Type::Prepared){
        if(c.state!=State::PreparingWorld||!c.prepare||!c.prepare_sent||!c.scene.CanReturnToMenu()||!event.world){Failure(c,"client_prepare_activation_state");break;}
        const auto &p=c.prepare->world->body;if(event.route!=replica::Unsigned(p["next_route"])||event.scope!=replica::stream::Connected({replica::Unsigned(p["world"]["session"]),replica::Unsigned(p["world"]["run"]),replica::Unsigned(p["world"]["floor"]),replica::Unsigned(p["world"]["dimension"]),replica::Unsigned(p["world"]["room"]),replica::Unsigned(p["world"]["scene"])},c.scope.connection)){Failure(c,"client_prepare_activation_identity");break;}
        Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.generation=0;c.control={};c.last_tick=c.last_target=0;c.scope=event.scope;c.route=event.route;c.scene.Begin(c.scope);
        const auto &scope=c.scope;
        const bool opened=c.scene.State()!=ClientSceneState::Failed&&c.binary&&
            c.binary->Open({scope.connection,c.route,{scope.session,scope.run,scope.floor,scope.dimension,scope.room,scope.scene}});
        if(!opened){Failure(c,"client_stream_begin");break;}
        c.prepare.reset();c.prepare_sent=false;c.state=State::AwaitingWorld;continue;
      }
      if(event.scope!=c.scope||event.route!=c.route||event.ordinal!=c.ordinal||!event.world)continue;
      const auto &m=*event.world;
      if(m.kind==replica::stream::Kind::ScenePrepare){
        const auto &p=m.body;
        if(replica::Unsigned(p["world"]["session"])!=c.scope.session||replica::Unsigned(p["next_route"])!=c.route+1||
           replica::Unsigned(p["native_domains"])!=2047||replica::Unsigned(p["retained_domains"])||!c.binary){
          Failure(c,"client_prepare_route_or_native_contract");break;
        }
        if(c.prepare){if(c.prepare->world->sequence!=m.sequence||c.prepare->world->body!=m.body)Failure(c,"client_prepare_conflict");continue;}
        Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.prepare=event;c.prepare_sent=false;c.prepare_progress={};c.state=State::PreparingWorld;
        const bool same_run=replica::Unsigned(p["world"]["run"])==c.scope.run;
        const bool same_mod_scene=same_run&&replica::Unsigned(p["world"]["floor"])==c.scope.floor;
        if(!same_mod_scene){
          deliver_mod_results();
          c.network.FinishModIntent(CancelModIntents(true));
          RetireModWorld();
        }
        if(c.binary)c.binary->Close(same_run,same_mod_scene);
        if(!c.scene.CanReturnToMenu())c.scene.Retire("network_scene_prepare");
      }else if(m.kind==replica::stream::Kind::Retire){
        const auto cutscene_serial=replica::Unsigned(m.body["cutscene_serial"]);
        if(cutscene_serial)QueueClientModCutscene({cutscene_serial,unsigned(replica::Unsigned(m.body["cutscene_id"])),true});
        const std::string_view reason=m.body.HasMember("reason")?m.body["reason"].GetString():"";
        // 原生换房先撤旧来源时，新 World 可能尚未交给网络。此时也保存本地图，等下一份 Prepare 核对 run。
        const bool room_change=reason=="binary_host_room_source_retired"||reason=="binary_scope_changed";
        const bool preserve_room=room_change||reason=="binary_host_source_retired";
        if(!room_change){
          deliver_mod_results();
          c.network.FinishModIntent(CancelModIntents(true));RetireModWorld();
        }
        if(c.binary)c.binary->Close(preserve_room,room_change);
        Neutral(c,true);c.network.RevokeInput();c.input_ready=false;c.timing={};c.progress={};c.prepare.reset();c.retire_control=event;c.prepare_progress={};c.state=State::PreparingWorld;c.scene.Retire("network_source_retired");
      }else if(m.kind!=replica::stream::Kind::Cancel){Failure(c,"client_stream_control_kind");break;}
    }
    const auto recover_graphics=[&]{
      if(!c.binary||!c.scene.DisplayGraphicsChanged())return false;
      // resize后的旧快照已失效。完整基线在当前图形代次重新建图并恢复输入资格。
      Neutral(c,true);c.network.RevokeInput();c.binary->Recover(state::StateResyncReason::Apply);
      return true;
    };
    recover_graphics();
    if(c.binary)c.binary->Pump();
    if(c.binary)c.binary->InstallMods();
    deliver_mod_results();
    if(c.terminal_frozen)c.network.FinishModIntent(CancelModIntents(true));
    if((c.prepare||c.retire_control)&&!c.scene.CanReturnToMenu()){
      const auto r=c.scene.Result();const std::array<std::uint64_t,6> step{r.attempt,r.executed_prefix,r.native_owners,r.grid_prefix,r.grid_owners,r.pending_owners};
      // 只报告真实完整退休步骤的变化；不以Manager心跳延长准备超时。
      if(step!=c.prepare_progress&&c.network.Complete(c.prepare?*c.prepare:*c.retire_control,net::live::OperationStage::Executing,"native_retirement_progress"))c.prepare_progress=step;
    }
    if(c.state==State::PreparingWorld&&c.prepare&&!c.prepare_sent&&c.scene.CanReturnToMenu()){
      if(!c.network.Prepare(*c.prepare))Failure(c,"client_prepare_queue");else c.prepare_sent=true;
    }
    if(c.retire_control&&c.scene.CanReturnToMenu()){
      if(c.network.Complete(*c.retire_control,net::live::OperationStage::Complete,"native_scene_retired")){c.retire_control.reset();c.scene.Begin(c.scope);} // 成功排队后才释放真实退休结果。
    }
    bool visual_hold=false;
#ifdef ISAACCS_E2E
    visual_hold=replica::stream::E2EVisualNeutral();
    if(c.visual_held&&!visual_hold){c.stale_origin=GetTickCount64();c.stale=false;}
    c.visual_held=visual_hold;
#endif
    if(!visual_hold&&c.state==State::AwaitingWorld&&c.scene.DisplayReady()){
      const auto now=GetTickCount64();
      if(c.was_paused&&!c.progress.authority_paused)c.stale_origin=now;
      c.was_paused=c.progress.authority_paused;
      const auto last=(std::max)(c.scene.LastStreamCommit(),c.stale_origin);
      const auto age=last&&now>=last?now-last:0;
      c.stale=!c.progress.authority_paused&&!ClientModCutsceneActive()&&age>=c.config.stale_ms;
      if(c.stale&&age>=c.config.recovery_ms){
        Neutral(c,true);c.network.RevokeInput();c.binary->Recover(state::StateResyncReason::Base);
      }
    }
    // 换层准备、退休或失败时场景不再显示，本地暂停菜单随之关闭；同局换房仍保持 AwaitingWorld，不受影响。
    if(c.state!=State::AwaitingWorld&&c.state!=State::Connecting)ClientPauseMenuReset();
    if(c.state==State::Retiring||c.state==State::Failed){net::live::Event discarded;for(unsigned n=0;n<80&&c.network.PollWorld(discarded);++n){}}
    if(c.state==State::Retiring) {
      // 原生菜单可能在Input更新之前消费缓存；交还前刷新两轮真实松键，不能重放Client按键。
      c.sampled_this_loop=false;c.physical_neutral=false;c.blocked=true;ClientPollInput();
      if(!c.sampled_this_loop||c.physical_neutral)++c.neutral_loops;else c.neutral_loops=0;
      // 只有无原生执行前缀、无借用与在途使用时才能交还本地菜单。
      if(c.network.Finished()&&(!c.binary||c.binary->Drained())&&c.neutral_loops>=2&&MenuReady()&&c.scene.CanReturnToMenu()) {
        ClientIsolationObserve(ClientIsolationKind::Retired,"scene_and_worker_drained");
        c.nightmare.reset();ReleaseNightmareSprites();ResetClientModCutscene();
        c.state=State::Off;ForgetInputDevice();client_active.store(false,std::memory_order_release);
        Trace("client","event=off connection=%llu input_closed=1 worker_finished=1 local_run=0",c.connection);
      }
    } else if(c.state==State::Connecting||c.state==State::AwaitingWorld) {
      const bool was_blocked=c.blocked;c.sampled_this_loop=false;c.focused=Focused();c.ui_blocked=ClientUiBlocked();
      // 本地暂停菜单打开期间与控制台一样封住本机玩法输入；Host 不随之暂停。
      c.blocked=!c.focused||c.ui_blocked||ClientPauseMenuOpen()||ClientModCutsceneActive();
      if(c.blocked&&!was_blocked){c.network.RevokeInput();c.timing={};}
      std::uint64_t target=0;
      // 16 的等待场景仍能向 Authority 发输入；已有目标的构建/替换窗口则清边沿并保持中性。
      const bool input_scene=c.scene.DisplayReady();
      const auto control=c.binary->Control(c.ordinal);
      // 构建中的无视图不是新的绑定；停止发意图并清FIFO，保留旧身份用于重建后的比较。
      if(c.scene.DisplayReady()){
        if(!ClientPlayerControlSame(c.control,control)){Neutral(c);c.timing={};}
        c.control=control;
      }
      // 观测binding_revision与Authority participant generation出自同一绑定；旧A不得给新代次发边沿。
      const bool group_ready=control.present&&control.consumers&&control.binding_revision==c.generation;
      c.input_ready=input_scene&&!c.blocked&&c.progress.input_enabled&&c.state==State::AwaitingWorld&&
                    group_ready&&c.progress.session_state==core::SessionState::Active&&c.generation&&
                    c.timing.Target(c.progress.server_tick,net::TimingClockMs(),target);
      ClientPollInput();
      // 本地暂停菜单（原版 PauseScreen）：与原生 Game::ProcessInput 一样在输入泵之后读取菜单动作。
      // 只有 Host 正常游玩或 Host 自己暂停时允许打开；已打开的菜单在转场等分支中仍可操作和关闭。
      const bool pause_native=c.state==State::AwaitingWorld&&c.scene.NativeDisplay();
      {
        // 诊断：物理 ESC 按下沿（不依赖原生动作映射）记录菜单判定的全部输入，定位“按 ESC 无反应”。
        static bool esc_was_down=false;
        const bool esc_down=(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
        if(esc_down&&!esc_was_down&&c.state==State::AwaitingWorld){
          unsigned branch=0xFFFFFFFFu;const bool known=ClientNativeDisplayBranch(branch);
          ClientPauseDiagnose(c.focused,c.ui_blocked,pause_native,known,branch);
        }
        esc_was_down=esc_down;
      }
      if(c.state==State::AwaitingWorld&&!(c.focused&&!c.ui_blocked&&pause_native)&&!ClientPauseMenuOpen()&&ClientPauseKeyPressed()){
        // 诊断：按下暂停键但外层条件不满足，记录是哪一项挡住。
        Trace("client_pause","event=open_refused reason=gate focused=%u console=%u native=%u ready=%u",
              unsigned(c.focused),unsigned(c.ui_blocked),unsigned(pause_native),unsigned(c.scene.DisplayReady()));
      }
      if(c.state==State::AwaitingWorld&&c.focused&&!c.ui_blocked&&pause_native){
        unsigned branch=0xFFFFFFFFu;
        const bool may_open=ClientNativeDisplayBranch(branch)&&
            (branch==unsigned(state::DisplayBranch::Normal)||branch==unsigned(state::DisplayBranch::PauseScreen));
        const auto pause=ClientPauseMenuInput(c.ordinal,may_open,branch);
        if(pause==ClientPauseAction::Exit){
          // 原生“退出”会保存并结束本局；客机改为离开会话，待本轮显示推进后再退休。
          c.pause_exit=true;c.input_ready=false;Neutral(c);
          Trace("client","event=pause_menu_exit connection=%llu",c.connection);
        }else if(pause==ClientPauseAction::Opened&&!c.blocked){
          // 本帧刚打开：本帧采样不再发送，并像进入阻断一样撤销输入资格。
          c.blocked=true;c.input_ready=false;Neutral(c);c.network.RevokeInput();c.timing={};
        }
      }
      if(!c.sampled_this_loop){++c.device_missing;Neutral(c,true);}
      if(!c.input_ready)
        Neutral(c);
      // 输入发送（2026-11-20 改，预测插值阶段 2 补充）：每帧按本帧最新采样发送，不再要求本帧收到新的 server_tick。
      // Host 的 Participant::Begin 对 tick t 只用目标恰为 t 的命令，没有就按中性输入处理；旧规则在 TickProgress
      // 到达抖动时会漏掉一些 tick（本帧未收到新 tick 不发、下一帧目标跳 2），Host 与本机预测都在这些 tick 不加速。
      //  1) 目标前进：为 (last_target, target] 中的每个 tick 各发一条同一采样的命令，最多 kInputFillTicks 条；
      //     边沿只放在最后一条（目标 tick），与旧规则相同。
      //  2) 目标未前进但本帧采样的移动／瞄准／按住位变了：以更高序号对同一目标重发一次（Host 取该 tick 最高序号），
      //     让本帧按键立刻进入该 tick；重发不带边沿，边沿留给下一个新目标，避免同 tick 边沿冲突。
      if(c.eligible&&c.sampled_this_loop&&target&&c.last_target&&target>=c.last_target) {
        constexpr std::uint64_t kInputFillTicks=4;
        const bool advance=target>c.last_target;
        const bool changed=c.sample.move_x!=c.last_sent.move_x||c.sample.move_y!=c.last_sent.move_y||
                           c.sample.aim_x!=c.last_sent.aim_x||c.sample.aim_y!=c.last_sent.aim_y||
                           c.sample.held_flags!=c.last_sent.held_flags||c.sample.mod_held!=c.last_sent.mod_held;
        std::uint64_t first=target;
        if(advance)first=(std::max)(c.last_target+1,target>=kInputFillTicks?target-kInputFillTicks+1:1);
        else if(!changed)first=target+1; // 同一目标、采样未变：本帧不发
        // 发送一条命令；返回 false 表示已转入失败状态。
        const auto send_one=[&](std::uint64_t tick,bool with_edge)->bool{
          if(c.sequence==UINT64_MAX){Failure(c,"client_input_exhausted");return false;}
          auto sample=c.sample;
          const auto edge=with_edge&&c.edge_count?c.edges[c.edge_head]:ClientRuntime::InputEdge{};
          sample.edge_flags=edge.actions;sample.mod_edge=edge.keys;
          net::live::Intent intent{sample,c.generation,c.sequence+1,tick,c.scope,c.route};
          if(!c.network.Send(intent)){Failure(c,"client_input_queue_full");return false;}
          ++c.sequence;++c.sent;c.last_target=tick;c.last_tick=c.progress.server_tick;c.last_sent=sample;c.sample.edge_flags=0;
          if(tick!=target)++c.fill_sent;else if(!advance)++c.resent;
          c.latency.Record(c.sequence,tick,c.progress.server_tick,sample.move_x||sample.move_y,c.timing.rate_ticks,c.timing.rate_ms);
          c.scene.RecordMotionInput(c.generation,c.sequence,tick,sample.move_x,sample.move_y);
#ifdef ISAACCS_E2E
          replica::stream::E2EInput("physical-input",{c.ordinal,0,0,sample.edge_flags,c.generation,c.sequence,tick,c.progress.server_tick,{sample.move_x,sample.move_y,sample.aim_x,sample.aim_y}});
#endif
          if(sample.edge_flags||sample.mod_edge){
            c.sent_edge_mask|=sample.edge_flags;
            c.edge_head=(c.edge_head+1)%unsigned(c.edges.size());--c.edge_count;
          }
          if(Settings.diagnostics)Trace("client_input","connection=%llu ordinal=%u generation=%llu sequence=%llu target=%llu move=%d,%d aim=%d,%d held=%u edge=%u",c.connection,c.ordinal,c.generation,c.sequence,tick,intent.state.move_x,intent.state.move_y,intent.state.aim_x,intent.state.aim_y,intent.state.held_flags,intent.state.edge_flags);
          return true;
        };
        for(auto tick=first;tick<=target;++tick)
          if(!send_one(tick,advance&&tick==target))break;
      } else if(c.eligible&&c.sampled_this_loop&&target&&!c.last_target) {
        // 首条命令（连接、换场景后 last_target 归零）：只发目标 tick 本身，不向前补。
        if(c.sequence==UINT64_MAX)
          Failure(c,"client_input_exhausted");
        else {
          auto sample=c.sample;
          const auto edge=c.edge_count?c.edges[c.edge_head]:ClientRuntime::InputEdge{};
          sample.edge_flags=edge.actions;sample.mod_edge=edge.keys;
          net::live::Intent intent{sample,c.generation,c.sequence+1,target,c.scope,c.route};
          if(c.network.Send(intent)) {
            ++c.sequence;++c.sent;c.last_target=target;c.last_tick=c.progress.server_tick;c.sample.edge_flags=0;c.last_sent=sample;
            c.latency.Record(c.sequence,target,c.progress.server_tick,sample.move_x||sample.move_y,c.timing.rate_ticks,c.timing.rate_ms);
            c.scene.RecordMotionInput(c.generation,c.sequence,target,sample.move_x,sample.move_y);
#ifdef ISAACCS_E2E
            replica::stream::E2EInput("physical-input",{c.ordinal,0,0,sample.edge_flags,c.generation,c.sequence,target,c.progress.server_tick,{sample.move_x,sample.move_y,sample.aim_x,sample.aim_y}});
#endif
            c.sent_edge_mask|=sample.edge_flags;
            if(c.edge_count){c.edge_head=(c.edge_head+1)%unsigned(c.edges.size());--c.edge_count;}
            if(Settings.diagnostics)Trace("client_input","connection=%llu ordinal=%u generation=%llu sequence=%llu target=%llu move=%d,%d aim=%d,%d held=%u edge=%u",c.connection,c.ordinal,c.generation,c.sequence,target,intent.state.move_x,intent.state.move_y,intent.state.aim_x,intent.state.aim_y,intent.state.held_flags,intent.state.edge_flags);
          } else Failure(c,"client_input_queue_full");
        }
      }
      if(c.network.Finished()&&c.state!=State::Failed){
        // worker可在本轮耗时预检中结束；Poll优先交付已发布的终态，不能用通用文字抢占真实首因。
        net::live::Event ending;
        if(c.network.Poll(ending)&&(ending.type==net::live::Event::Type::Failed||ending.type==net::live::Event::Type::Stopped))Failure(c,ending.reason);
        else Failure(c,"client_transport_finished");
      }
    }
    // 输入owner已经完成唯一一次平台泵；显示根位于本轮world安装之后，音频／光标在相机后。
    // 失焦只封本端输入；显示仍消费 Host 状态。只有本地控制台／菜单阻断显示批次。
    // 输入泵可能刚处理本机窗口变化。先进入恢复，再让显示推进读取本轮图形资源。
    // 覆盖层按本机控制组序号排除本机身体；Host 暂停时世界显示时间停住。
    c.scene.SetMotionContext(c.ordinal,c.progress.authority_paused);
    if(!recover_graphics()&&!AdvanceClientModCutscene(c.scene.CanReturnToMenu())&&
       !c.scene.AdvanceDisplay(c.ui_blocked))Failure(c,"client_display_schedule");
    // 暂停菜单“退出”：与面板“离开会话”走同一正常停止路径，回到主菜单。
    if(c.pause_exit){c.pause_exit=false;StopClient();}
    // 本轮Body已经进入原生，随后Render会显示；只读计数，不影响调度。
    c.latency.BodyObserved(c.scene.AppliedBodyCount());
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
  if(!ReadPhysicalActions(device,actions,&c.map_pressed)){Neutral(c,true);return true;}
  if(c.sampled_this_loop){Failure(c,"client_duplicate_device_sample");return true;}
  c.sampled_this_loop=true;++c.sampled;
  c.physical_neutral=!c.map_pressed;
  for(unsigned i=0;i<12;++i)if(actions.values[i]!=0||actions.pressed[i]||actions.triggered[i])c.physical_neutral=false;
  const bool same=c.baseline&&c.source==device;
  if(!same){Neutral(c,true);c.shooting={};}
  unsigned held=0;for(unsigned i=0;i<4;++i)if(actions.pressed[8+i])held|=1u<<i;
  core::input::RawInputSample raw{};raw.move_x=actions.values[1]-actions.values[0];raw.move_y=actions.values[3]-actions.values[2];
  raw.held_flags=(held&14u)|((held&1u)?core::kHeldBomb:0u);raw.edge_flags=same&&!c.blocked?held&~c.held:0;
  core::InputStateDraftV2 sample{};
  if(!core::CanonicalizeShooting(raw,{actions.values[4],actions.values[5],actions.values[6],actions.values[7]},c.shooting,sample)) {
    Failure(c,"client_noncanonical_sample");return true;
  }
  SampleModKeys(sample.mod_held,sample.mod_edge);
  if(sample.mod_held)c.physical_neutral=false;
  if(!same||c.blocked)sample.mod_edge=0;
  c.source=device;c.baseline=true;c.held=held;
  // 与Host物理源相同：真实上升沿按轮次进入有界FIFO，重复同键也不位或合并。
  const bool eligible=c.input_ready;
  if(!eligible) {c.edge_head=c.edge_count=0;sample.edge_flags=0;sample.mod_edge=0;}
  else if(sample.edge_flags||sample.mod_edge) {
    if(c.edge_count==c.edges.size()){Failure(c,"client_edge_queue_full");return true;}
    c.edges[(c.edge_head+c.edge_count)%c.edges.size()]={sample.edge_flags,sample.mod_edge};++c.edge_count;
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
// 丢失结束包时按接收时限归还绘制；切换路由后旧过场立即失效。
bool ClientRenderNightmare() noexcept {
  if(!IsMain()||!ClientBusy()||client->terminal_frozen||TopOwner()!=Owner::Render)return false;
  auto &c=*client;
  if(!c.nightmare)return false;
  if(c.nightmare_route!=c.route||GetTickCount64()-c.nightmare_received>1000){
    c.nightmare.reset();ReleaseNightmareSprites();
    return false;
  }
  return RenderNightmareFrame(*c.nightmare);
}
// 同局换房交接期间仍有完整旧画面，菜单回调及等待提示留给真正的连接等待。
bool ClientRenderTransitionHold() noexcept {
  if(ClientModCutsceneActive()||ClientRenderNightmare())return true;
  if(!IsMain()||!ClientBusy()||!render_observation.active)return false;
  const auto state=client->state;
  return (state==State::PreparingWorld||state==State::AwaitingWorld)&&
         client->scene.RenderBinaryHistory(render_observation);
}
// Manager Render 保留清屏和 Swap，场景绘制由其原生 Game::Render 分派完成。
ClientRenderScope::ClientRenderScope(bool enabled) noexcept
  :exceptions_(std::uncaught_exceptions()) {
  if(!enabled||!IsMain()||!ClientBusy())return;
  if(render_observation.active){client->scene.Fail("client_render_reentrant");return;}
  client->scene.RenderWindow();
  active_=client->scene.Borrow(ClientPurpose::RenderObservation,render_observation);
  if(active_&&client->scene.NativeDisplay()){
    native_render_routed=client->scene.RouteNativeRender(true);
    if(!native_render_routed)Failure(*client,"client_manager_render_route");
  }
}
ClientRenderScope::~ClientRenderScope() noexcept {
  if(!active_||!client)return;
  if(native_render_routed){client->scene.RouteNativeRender(false);native_render_routed=false;}
  if(std::uncaught_exceptions()!=exceptions_)client->scene.Fail("client_render_exception");
  client->scene.Release(render_observation);
}
ClientGameRenderScope::ClientGameRenderScope(void *game) noexcept:exceptions_(std::uncaught_exceptions()) {
  if(!ClientBusy()){
#ifdef ISAACCS_E2E
    if(IsMain()&&Runtime())TraceBinaryBloodRender(game,"host");
#endif
    call_=true;return;
  }
  if(!IsMain()){foreign_client.store(true,std::memory_order_release);return;}
  if(!client)return;
  if(ClientModCutsceneActive()||ClientRenderNightmare())return;
  if(client->scene.DisplayGraphicsChanged()||!client->scene.NativeDisplay()){
    // 新房的显示根可能先把 Manager 切到游戏态，仍沿用同一张旧画面直到提交。
    ClientRenderTransitionHold();
    CountDispatch(*client,ClientEntry::GameRender,ClientEntryDecision::ExpectedSkip);return;}
  if(TopOwner()!=Owner::Render||!render_observation.active||!client->scene.Valid(render_observation)||
     client->scene.NativeRoots().game!=game||native_render_dispatch){Failure(*client,"client_game_render_context");return;}
  if(!client->scene.Borrow(ClientPurpose::Graphics,native_render_graphics))return;
  native_render_dispatch.emplace(ClientOperation::PresentDraw,ClientEntry::PresentationDraw);
  if(!native_render_dispatch->Ready()||ClientDispatchBefore(ClientEntry::PresentationDraw)!=ClientEntryDecision::CallOriginal||
     !client->scene.BeginNativeRender()){
    native_render_dispatch.reset();client->scene.Release(native_render_graphics);Failure(*client,"client_game_render_begin");return;}
  active_=call_=true;
#ifdef ISAACCS_E2E
  TraceBinaryBloodRender(game,"client");
#endif
}
ClientGameRenderScope::~ClientGameRenderScope() noexcept {
  if(!active_||!client)return;
  client->scene.EndNativeRender();ClientDispatchReturned(ClientEntry::PresentationDraw);
  const bool success=native_render_dispatch&&native_render_dispatch->Succeeded()&&std::uncaught_exceptions()==exceptions_;
  native_render_dispatch.reset();client->scene.Release(native_render_graphics);
  if(!success)Failure(*client,"client_game_render_incomplete");
#ifdef ISAACCS_E2E
  else if(const auto table=client->scene.PublishedBlocks())E09ClientRender(*table);
#endif
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
  ReleaseNightmareSprites();
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
  return client->network.Finished()&&client->binary->Drained()&&client->scene.CanReturnToMenu();
}
// 只在已发布的客机画面上报告当前连接，重连时由新的连接序号重置 HUD 采样。
std::string ClientHudTimingFields() {
  if(!IsMain()||!ClientBusy()||!client||!client->binary||!client->scene.DisplayReady())return "ok=1\nrole=Off\n";
  return "ok=1\nrole=Client\nepoch="+std::to_string(client->connection)+"\n"+client->binary->InstallTimingFields();
}
// 详细有界聚合只在显式诊断命令读取；普通每帧status不展开128个范围。
std::string ClientStreamFields(unsigned page) {
  if(!IsMain()||!client||page>=32)return "ok=0\nerror=stream_client_unavailable\n";
  const auto &c=*client;std::string out="ok=1\nstream_diagnostics_version=1\n";
  out+=Settings.stream.Fields("stream.requested.");out+=c.config.Fields("stream.effective.");
  out+=replica::stream::LimitsFields(c.network.EffectiveLimits(),"stream.negotiated.");out+=replica::stream::LimitsFields(c.network.EffectiveFastLimits(),"stream.negotiated_fast.");
  out+=c.network.TransmitMetrics().Fields("stream.transmit.",page);
  out+=c.network.Metrics().Fields("stream.receive.",page);
  if(c.binary)out+=c.binary->Diagnostics(page);
  if(page==0){out+=c.latency.Fields();out+=c.scene.MotionFields();out+=ModStateFields();}
  const auto t=c.network.TransferStatus();
  const auto add=[&](const char *name,std::uint64_t value){out+="stream."+std::string(name)+"="+std::to_string(value)+"\n";};
  add("fragments",t.fragments);add("duplicates",t.duplicates);add("conflicts",t.conflicts);add("decoded",t.decoded);add("decoded_bytes",t.decoded_bytes);add("decode_us",t.decode_us);add("expired",t.expired);add("ack_windows",t.ack_windows);
  add("state_stale",c.stale);add("authority_paused",c.progress.authority_paused);
  add("native_sample_ms",GetTickCount64());
  add("native_world_publications",c.scene.AppliedWorldCount());add("native_body_publications",c.scene.AppliedBodyCount());
  add("owner_diagnostics_missing",c.owner_diagnostics_missing);add("failure_owner_count",c.first_owners.size());
  for(unsigned i=page*34;i<(std::min)(unsigned(c.first_owners.size()),page*34+34);++i){const auto &f=c.first_owners[i];out+="stream.failure_owner."+std::to_string(i)+"="+std::to_string(f.ticket)+"/"+std::to_string(f.kind)+"/"+std::to_string(f.phase)+"/"+std::to_string(f.resource)+"/"+std::to_string(f.lua)+"/"+std::to_string(f.context)+"/"+std::to_string(f.cpu)+"/"+std::to_string(f.gpu)+"/"+std::to_string(f.failed)+"\n";}
  add("first_failure_native_owners",c.first_scene_failure.native_owners);add("first_failure_grid_owners",c.first_scene_failure.grid_owners);add("first_failure_pending_owners",c.first_scene_failure.pending_owners);add("first_failure_instance",c.first_scene_failure.generations.instance);add("first_failure_resource",c.first_scene_failure.generations.resource);
  add("first_failure_attempt",c.first_scene_failure.attempt);add("first_failure_prefix",c.first_scene_failure.failure_executed_prefix);add("first_failure_key_run",c.first_scene_failure.failed_key.run);add("first_failure_key_id",c.first_scene_failure.failed_key.id);add("first_failure_key_space",c.first_scene_failure.failed_key.space);
  out+="stream.first_wait_reason="+std::string(c.first_scene_failure.stream_wait_reason.data())+"\n";add("first_wait_kind",c.first_scene_failure.stream_wait_kind);add("first_wait_revision",c.first_scene_failure.stream_wait_revision);
  out+="stream.first_reason="+c.error+"\nstream.first_phase="+c.first_scene_failure.failure_phase.data()+"\nstream.first_native_reason="+c.first_scene_failure.failure_reason.data()+"\nstream.cleanup_failure="+c.cleanup_failure.retirement_failure.data()+"\nstream.stop_reason="+c.stop_reason+"\n";
  PROCESS_MEMORY_COUNTERS_EX pm{};pm.cb=sizeof(pm);if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pm),sizeof(pm))){add("process_private_bytes",pm.PrivateUsage);add("process_working_set_bytes",pm.WorkingSetSize);}else out+="stream.process_memory=unavailable\n";
  const auto pools=memory::ReplicationBudget()->Snapshot();for(unsigned i=0;i<3;++i){out+="stream.pool."+std::to_string(i)+"="+std::to_string(pools[i].current)+"/"+std::to_string(pools[i].high_water)+"/"+std::to_string(pools[i].limit)+"\n";}
  out+=memory::ReplicationBudget()->FailureFields();
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
  s+="client.target_source=network\n";
  const auto add=[&](const char *name,std::uint64_t value){s+="client."+std::string(name)+"="+std::to_string(value)+"\n";};
  // 退休卡住时直接报告尚未满足的实际条件，避免把资源释放误报为完整退出。
  add("retire_worker_finished",c.network.Finished());add("retire_binary_drained",!c.binary||c.binary->Drained());
  add("retire_neutral_loops",c.neutral_loops);add("retire_menu_ready",MenuReady());add("retire_scene_ready",c.scene.CanReturnToMenu());
  for(unsigned i=0;i<c.original_begin.size();++i)add((std::string("original_begin.")+kIsolationOriginals[i]).c_str(),c.original_begin[i]);
  add("connection",c.connection);add("participant",c.ordinal);add("roster",c.roster);add("generation",c.generation);
  add("wire_session",c.scope.session);add("wire_connection",c.scope.connection);
  add("server_tick",c.progress.server_tick);add("sent",c.sent);add("sequence",c.sequence);
  add("accepted",c.progress.accepted);add("processed",c.input_ack);add("processed_tick",c.input_ack_tick);
  add("sampled",c.sampled);add("manager_skipped",c.manager_skipped);add("device_missing",c.device_missing);
  // 纯值诊断明确区分焦点/UI屏蔽、服务器资格和输入时序，避免再次仅凭收包猜测。
  add("focused",c.focused);add("ui_blocked",c.ui_blocked);add("input_enabled",c.progress.input_enabled);
  add("input_ready",c.input_ready);add("round_trip_ms",c.timing.round_trip_ms);
  add("timing_uncertainty_ms",c.timing.timing_uncertainty_ms);
  add("clock_rate_ticks",c.timing.rate_ticks);add("clock_rate_ms",c.timing.rate_ms);add("last_target",c.last_target);
  // 采样/发包动作的纯值证据；验收不能拿中性心跳的处理序号冒充物理按键已生效。
  s+="client.sample_move="+std::to_string(c.sample.move_x)+","+std::to_string(c.sample.move_y)+"\n";
  s+="client.sample_aim="+std::to_string(c.sample.aim_x)+","+std::to_string(c.sample.aim_y)+"\n";
  add("sent_edge_mask",c.sent_edge_mask);add("input_fill_sent",c.fill_sent);add("input_resent",c.resent);
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
  s+="client.stop_reason="+c.stop_reason+"\n";
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
  // 只在显式status查询采进程实际内存；估算的模型成本与操作系统值分开，不写逐tick日志。
  PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);
  if(K32GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),sizeof(memory))){
    add("process_working_set_bytes",memory.WorkingSetSize);add("process_private_bytes",memory.PrivateUsage);
  }
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
  if(Settings.diagnostics&&!Settings.world_e2e_token.empty()){
    void *dc=nullptr,*context=nullptr;
    if(ClientRendererHandles(dc,context))add("renderer_window",reinterpret_cast<std::uintptr_t>(WindowFromDC(static_cast<HDC>(dc))));
  }
  s+="client.native_phase="+std::string(result.phase.data())+"\n";
  s+="client.stream_wait_reason="+std::string(result.stream_wait_reason.data())+"\nclient.stream_wait_kind="+std::to_string(result.stream_wait_kind)+"\nclient.stream_wait_revision="+std::to_string(result.stream_wait_revision)+"\n";
  s+="client.native_reason="+std::string(result.reason.data())+"\n";
  s+="client.failure_domain="+std::string(result.failed_domain.data())+"\n";
  s+="client.failure_phase="+std::string(result.failure_phase.data())+"\n";
  s+="client.failure_reason="+std::string(result.failure_reason.data())+"\n";
  s+="client.recovery="+std::string(result.recovery.data())+"\n";
  s+="client.error="+c.error+"\n";return s;
}
} // namespace cs::game
