#include "game/client/scene/client_scene.h"
#include "game/client/display/client_display.h"
#include "game/client/scene/client_preparation_native.h"
#include "game/common/context.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <cstdio>
#include <cstring>
#include <charconv>
#include <cmath>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
constexpr unsigned kMaximumInFlight=16;
// 每个连接都重新构造 ClientScene，故进程级 epoch 防止旧票据命中新连接同序号。
std::atomic<std::uint64_t> next_scene_epoch{0};
std::uint64_t NewSceneEpoch() noexcept {
  auto value=next_scene_epoch.load(std::memory_order_relaxed);
  while(value!=UINT64_MAX){
    if(next_scene_epoch.compare_exchange_weak(value,value+1,std::memory_order_acq_rel))return value+1;
  }
  return 0;
}
}
ClientScene::ClientScene() noexcept=default;
ClientScene::~ClientScene() {ClientDisplayDestroy(display_);}
// 新连接只建立等待身份；从不把等待 Scope 的 run/room 伪装成原生场景。
void ClientScene::Begin(const replica::Scope &scope) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!CanReturnToMenu()){FailureResult(ClientResultKind::PartialNativeFailure,"begin","old_owner_inflight");state_=ClientSceneState::Failed;return;}
  waiting_scope_=scope;
  retirement_failed_=recovery_forbidden_=false;
  if(!display_history_||display_history_scope_.session!=scope.session||
     (scope.run&&display_history_scope_.run!=scope.run)){
    if(display_history_&&!DiscardBinaryHistory()){
      Fail("binary_history_retirement");return;
    }
    ClientDisplayDestroy(display_);display_=nullptr;display_history_=false;
  }
  binary_scene_=binary_published_=false;
  motion_.Reset();
  retirement_complete_=false;
  native_dispatch_prefix_=false;
  applied_revision_=stream_last_commit_=0;
  const auto epoch=NewSceneEpoch();
  if(!epoch){FailureResult(ClientResultKind::BudgetExhausted,"begin","scene_generation_exhausted");state_=ClientSceneState::Failed;return;}
  generations_.scene=epoch;
  state_=ClientSceneState::Waiting;result_={};result_.scope=scope;
  FailureResult(ClientResultKind::WaitingDependency,"waiting","world_payload_unavailable");
  current_dc_=current_context_=observed_dc_=observed_context_=nullptr;
}
void ClientScene::FailureResult(ClientResultKind kind,const char *phase,const char *reason) noexcept {
  if(kind==ClientResultKind::Retiring&&phase&&std::strcmp(phase,"retired")&&!result_.retirement_cause[0])
    std::snprintf(result_.retirement_cause.data(),result_.retirement_cause.size(),"%s",reason?reason:"retire");
  if(kind==ClientResultKind::ContextOrThreadFault)recovery_forbidden_=true;
  if((kind==ClientResultKind::UnsupportedRequirement||kind==ClientResultKind::PartialNativeFailure)&&reason&&
     (std::strstr(reason,"budget")||std::strstr(reason,"capacity")))kind=ClientResultKind::BudgetExhausted;
  if(kind==ClientResultKind::PartialNativeFailure||kind==ClientResultKind::ContextOrThreadFault||kind==ClientResultKind::UnsupportedRequirement||kind==ClientResultKind::BudgetExhausted){
    if(result_.native_written&&!result_.retirement_cause[0])std::snprintf(result_.retirement_cause.data(),result_.retirement_cause.size(),"%s",reason?reason:"unknown");
    if(phase&&std::strstr(phase,"retire")&&!result_.retirement_failure[0])std::snprintf(result_.retirement_failure.data(),result_.retirement_failure.size(),"%s",reason?reason:"unknown");
    if(!result_.failure_phase[0]){
      result_.failure_executed_prefix=result_.executed_prefix;
      std::snprintf(result_.failure_phase.data(),result_.failure_phase.size(),"%s",phase?phase:"unknown");
      std::snprintf(result_.failure_reason.data(),result_.failure_reason.size(),"%s",reason?reason:"unknown");
      if(!result_.failed_domain[0])std::snprintf(result_.failed_domain.data(),result_.failed_domain.size(),"%s",
        phase&&std::strstr(phase,"grid")?"grid":phase&&std::strstr(phase,"objects")?"entity":phase&&std::strstr(phase,"presentation")?"presentation":"room-grid");
    }
  }
  if((kind==ClientResultKind::PartialNativeFailure||kind==ClientResultKind::ContextOrThreadFault||kind==ClientResultKind::UnsupportedRequirement||kind==ClientResultKind::BudgetExhausted)&&
     (result_.kind!=kind||std::strcmp(result_.phase.data(),phase?phase:"unknown")||std::strcmp(result_.reason.data(),reason?reason:"unknown")))
    Trace("client_apply_failure","revision=%llu attempt=%llu kind=%u phase=%s reason=%s prefix=%u written=%u",
      result_.revision,result_.attempt,unsigned(kind),phase?phase:"unknown",reason?reason:"unknown",result_.executed_prefix,unsigned(result_.native_written));
  result_.kind=kind;
  std::snprintf(result_.phase.data(),result_.phase.size(),"%s",phase?phase:"unknown");
  std::snprintf(result_.reason.data(),result_.reason.size(),"%s",reason?reason:"unknown");
  result_.generations=generations_;
  result_.native_committed=false;
  result_.committed_domains=0;
  result_.non_player_entity_applied=result_.player_support_applied=result_.full_entity_applied=false;
  result_.player_bodies_applied=result_.player_relations_applied=result_.player_resources_applied=result_.player_domain_applied=false;
  std::snprintf(result_.recovery.data(),result_.recovery.size(),"%s",kind==ClientResultKind::NativeCommitted?"none":result_.native_written?"retire_actual_owners_then_rebuild":"preserve_applied_or_replace_unwritten_target");
}
bool ClientScene::NoteNativeDispatchEntry() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(result_.executed_prefix==UINT32_MAX){
    FailureResult(ClientResultKind::BudgetExhausted,"dispatch","native_prefix_budget");
    state_=ClientSceneState::Failed;return false;
  }
  native_dispatch_prefix_=true;
  ++result_.executed_prefix;
  return true;
}
// 原生批次由 ClientBinaryScene 推进，这里只维护共享上下文和借用退休。
void ClientScene::Pump() noexcept {
  if(IsMain()&&TopOwner()==Owner::Manager&&ContextHealthy()&&!InBlockedContext())ContextWindow();
}
bool ClientScene::DisplayReady() const noexcept {
  return IsMain()&&binary_published_&&published_blocks_&&state_==ClientSceneState::Displaying&&
         !foreign_fault_.load(std::memory_order_acquire);
}
void ClientScene::PublishBinary(state::ClientStateBlockTableRef blocks,std::uint8_t streams) noexcept {
  const auto revision=blocks->streams[0]->Context().target_version;
  published_blocks_=std::move(blocks);
  if(streams&1)ClientDisplayPublish(display_);
  if(!binary_published_||applied_revision_!=revision)stream_last_commit_=GetTickCount64();
  if(streams&1)++applied_world_count_;
  if(streams&2)++applied_body_count_;
  binary_published_=true;applied_revision_=revision;
  state_=ClientSceneState::Displaying;
  // 安装刚写完的权威运动值进入覆盖层历史（设计 6.1）；读的是覆盖层外的原生对象。
  {const auto &table=*published_blocks_;
    const auto frame=[&](unsigned i)->std::uint32_t{return table.streams[i]?table.streams[i]->Context().game_frame:0u;};
    motion_.Installed(NativeRoots().room,streams,table.route.scope.scene,frame(0),frame(1),&table);}
  FailureResult(ClientResultKind::NativeCommitted,"binary_apply","binary_native_committed");
  result_.native_committed=true;result_.old_display_valid=true;result_.revision=revision;
}
void ClientScene::FinishBinary() noexcept {
  published_blocks_.reset();motion_.Reset();
  binary_scene_=binary_published_=false;native_dispatch_prefix_=false;applied_revision_=0;
  result_.native_written=result_.native_committed=result_.old_display_valid=false;result_.executed_prefix=0;
}
ClientDisplayStep ClientScene::PrepareBinaryDisplay(const ClientDisplayWorldRef &target) noexcept {
  if(!DestructiveReady())return {false,false,"binary_display_waiting_for_borrows"};
  if(!display_)display_=ClientDisplayCreate(target);
  if(!display_)return {false,false,"binary_display_allocation"};
  const auto old=state_;state_=ClientSceneState::Applying;ClientDisplayStep step;
  {ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::NativeApply)==ClientEntryDecision::CallOriginal){
      step=ClientDisplayBeforeInstall(display_,target,ClientPreparationRoots());
      ClientDispatchReturned(ClientEntry::NativeApply);
      if(!dispatch.Succeeded())step={false,true,"binary_display_before_dispatch"};
    }}
  if(state_==ClientSceneState::Applying)state_=old;
  return step;
}
ClientDisplayStep ClientScene::InstallBinaryDisplay(const ClientDisplayWorldRef &target) noexcept {
  if(!display_||!DestructiveReady())return {false,false,"binary_display_install_window"};
  const auto old=state_;state_=ClientSceneState::Applying;ClientDisplayStep step;
  {ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply,ClientEntryMask(ClientEntry::BackdropEffects));
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::NativeApply)==ClientEntryDecision::CallOriginal){
      step=ClientDisplayInstall(display_,target,ClientPreparationRoots());
      ClientDispatchReturned(ClientEntry::NativeApply);
      if(!dispatch.Succeeded())step={false,true,"binary_display_install_dispatch"};
    }}
  if(state_==ClientSceneState::Applying)state_=old;
  return step;
}
// 首次捕获放在Sprite重绑和原生字段读回之后，仍使用同一Manager窗口。
ClientDisplayStep ClientScene::CaptureBinaryDisplay(const ClientDisplayWorldRef &target) noexcept {
  if(!display_||!DestructiveReady())return {false,false,"binary_display_capture_window"};
  const auto old=state_;
  state_=ClientSceneState::Applying;
  ClientDisplayStep step;
  {
    ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::NativeApply)==ClientEntryDecision::CallOriginal){
      step=ClientDisplayFinishInstall(display_,target,ClientPreparationRoots());
      ClientDispatchReturned(ClientEntry::NativeApply);
      if(!dispatch.Succeeded())step={false,true,"binary_display_capture_dispatch"};
    }
  }
  if(state_==ClientSceneState::Applying)state_=old;
  return step;
}
ClientDisplayStep ClientScene::InstallBinaryAudio(const ClientDisplayMusicRequest &music,ClientDisplayArray<ClientDisplaySound> loops) noexcept {
  const auto old=state_;state_=ClientSceneState::Applying;
  const auto step=ClientDisplayAudio(display_,music,loops);
  if(state_==ClientSceneState::Applying)state_=old;
  return step;
}
ClientDisplayStep ClientScene::RestoreBinaryBoss(const ClientDisplayBoss &boss) noexcept {
  const auto old=state_;state_=ClientSceneState::Applying;
  const auto step=ClientDisplayRestoreBoss(display_,boss);
  if(state_==ClientSceneState::Applying)state_=old;
  return step;
}
ClientDisplayStep ClientScene::ReplayBinaryEvent(const ClientDisplayEvent &event) noexcept {
  const auto old=state_;state_=ClientSceneState::Applying;
  const auto step=ClientDisplayEventReplay(display_,event);
  if(state_==ClientSceneState::Applying)state_=old;
  return step;
}
bool ClientScene::RetireBinaryDisplay(bool preserve_audio) noexcept {
  if(!display_)return true;
  if(!DestructiveReady())return false;
  const auto old=state_;state_=ClientSceneState::Retiring;bool done=false;
  {ClientDispatchScope dispatch(ClientOperation::Retire,ClientEntry::NativeTeardown);
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::NativeTeardown)==ClientEntryDecision::CallOriginal){
      done=ClientDisplayRetire(display_,false,preserve_audio);
      ClientDispatchReturned(ClientEntry::NativeTeardown);done=done&&dispatch.Succeeded();
    }}
  if(state_==ClientSceneState::Retiring)state_=old;
  return done;
}
bool ClientScene::PreserveBinaryRoom() noexcept {
  if(!display_||!DestructiveReady())return false;
  const auto old=state_;state_=ClientSceneState::Applying;bool done=false;
  {ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::NativeApply)==ClientEntryDecision::CallOriginal){
      done=ClientDisplayPreserveRoom(display_,ClientPreparationRoots());
      ClientDispatchReturned(ClientEntry::NativeApply);done=done&&dispatch.Succeeded();
    }}
  if(state_==ClientSceneState::Applying)state_=old;
  if(done){display_history_=true;display_history_scope_=waiting_scope_;}
  return done;
}
// 只借本地图形快照，跨房间退休时不访问旧房间、实体、Lua 或待提交的新根。
bool ClientScene::RenderBinaryHistory(const ClientBorrowToken &observation) noexcept {
  return display_history_&&!DisplayReady()&&state_!=ClientSceneState::Failed&&
         observation.purpose==ClientPurpose::RenderObservation&&Valid(observation)&&
         GraphicsWindowValid()&&ClientDisplayRenderHistory(display_);
}
bool ClientScene::ReleaseBinaryDisplayRoots(bool keep_history) noexcept {
  if(!display_)return true;
  if(!DestructiveReady())return false;
  const auto old=state_;state_=ClientSceneState::Retiring;bool done=false;
  {ClientDispatchScope dispatch(ClientOperation::Retire,ClientEntry::NativeTeardown);
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::NativeTeardown)==ClientEntryDecision::CallOriginal){
      done=ClientDisplayReleaseRoots(display_);
      ClientDispatchReturned(ClientEntry::NativeTeardown);done=done&&dispatch.Succeeded();
    }}
  if(state_==ClientSceneState::Retiring)state_=old;
  if(done&&!keep_history){ClientDisplayDestroy(display_);display_=nullptr;display_history_=false;}
  return done;
}
bool ClientScene::DiscardBinaryHistory() noexcept {
  if(!display_history_)return true;
  if(!RetireBinaryDisplay()||!ReleaseBinaryDisplayRoots())return false;
  display_history_=false;return true;
}
bool ClientScene::NativeDisplay() const noexcept {return DisplayReady()&&ClientDisplayCurrent(display_,NativeRoots());}
bool ClientScene::AdvanceDisplay(bool local_blocked) noexcept {
  const auto blocks=PublishedBlocks();
  if(!blocks||!ClientDisplayHasWorld(display_))return true;
  // 发布后的帧号或根绑定变化须撤销当前显示资格。
  ClientDisplayIssue issue;
  if(!ClientDisplayCurrent(display_,NativeRoots(),&issue)){
    ClientDisplayReportIssue(display_,issue,"before_advance",blocks->route.route,blocks->route.scope.scene,applied_revision_);
    Fail("display_world_not_current");return false;
  }
  ClientBorrowToken graphics{};if(!Borrow(ClientPurpose::Graphics,graphics))return false;
  bool ok=false;
  {ClientDispatchScope dispatch(ClientOperation::PresentMaintenance,ClientEntry::PresentationMaintenance);
    if(dispatch.Ready()&&ClientDispatchBefore(ClientEntry::PresentationMaintenance)==ClientEntryDecision::CallOriginal){
      // 覆盖层先求本轮显示值，相机阶段与随后的 Game::Render 共用同一张写入表。
      const auto room=NativeRoots().room;
      motion_.Prepare(room,motion_ordinal_,motion_paused_,published_blocks_.get());
      const ClientDisplayCameraHook camera{this,
        [](void *p)noexcept{auto &s=*static_cast<ClientScene *>(p);return s.motion_.Enter(s.NativeRoots().room);},
        [](void *p)noexcept{auto &s=*static_cast<ClientScene *>(p);s.motion_.Exit(s.NativeRoots().room);}};
      ok=ClientDisplayAdvance(display_,NativeRoots(),local_blocked,camera);
      ClientDispatchReturned(ClientEntry::PresentationMaintenance);ok=ok&&dispatch.Succeeded();}}
  Release(graphics);
  if(!ok){
    ClientDisplayCurrent(display_,NativeRoots(),&issue);
    ClientDisplayReportIssue(display_,issue,"after_advance",blocks->route.route,blocks->route.scope.scene,applied_revision_);
    Fail("display_advance_failed");
  }
  return ok;
}
bool ClientScene::BeginNativeRender() noexcept {
  const auto blocks=PublishedBlocks();
  if(!blocks||render_blocks_||!NativeDisplay()||!ClientDisplayRenderBegin(display_,NativeRoots()))return false;
  render_blocks_=blocks;
  // Render 读 pos＋posOffset（RE-P4）：覆盖层在此写入显示位置，EndNativeRender 写回。
  motion_render_=motion_.Enter(NativeRoots().room);
  return true;
}
bool ClientScene::RouteNativeRender(bool begin) noexcept{return ClientDisplayRouteRender(display_,begin);}
void ClientScene::EndNativeRender() noexcept {
  const auto blocks=render_blocks_;
  // 先写回权威值，再做 Render 写集纠正与当前性检查；Render 抛出时由外层作用域析构同样走到这里。
  if(motion_render_){motion_.Exit(NativeRoots().room);motion_render_=false;}
  ClientDisplayRenderEnd(display_,NativeRoots());render_blocks_.reset();
  ClientDisplayIssue issue;
  if(!DisplayReady()||!ClientDisplayCurrent(display_,NativeRoots(),&issue)){
    if(blocks)ClientDisplayReportIssue(display_,issue,"after_render",blocks->route.route,blocks->route.scope.scene,applied_revision_);
    Fail("display_render_failed");
  }
}
std::vector<ClientOwnerFact> ClientScene::RetainedFacts() const {
  std::vector<ClientOwnerFact> out;if(!IsMain())return out;
  for(const auto &r:retained_)if(r.ticket.id)out.push_back({r.ticket.id,r.generations.resource,r.generations.lua,r.generations.context,unsigned(r.kind),unsigned(r.phase),r.cpu_queued,r.gpu_queued,r.failed});
  return out;
}
ClientNativeRoots ClientScene::NativeRoots() const noexcept {
  return DisplayReady()?ClientDisplayRoots(display_):ClientNativeRoots{};
}
bool ClientScene::Enter(ClientPurpose purpose) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_)return false;
  if(foreign_fault_.exchange(false,std::memory_order_acq_rel)){
    FailureResult(ClientResultKind::ContextOrThreadFault,"enter","foreign_use_attempt");state_=ClientSceneState::Failed;return false;
  }
  if(!ContextHealthy()||InBlockedContext()) {
    FailureResult(ClientResultKind::ContextOrThreadFault,"enter","owner_or_thread");return false;
  }
  if(purpose==ClientPurpose::RenderObservation){
    if(TopOwner()!=Owner::Render||
       active_observation_==kMaximumInFlight)return false;
    ++active_observation_;return true;
  }
  // 分派票据只维持用途帧寿命，不授权任何原生写入或图形操作。
  if(purpose==ClientPurpose::Dispatch){
    if((TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
       active_dispatch_==kMaximumInFlight)return false;
    ++active_dispatch_;return true;
  }
  if(purpose==ClientPurpose::LocalSettings)return TopOwner()!=Owner::None;
  if(purpose==ClientPurpose::NativeRetire) {
    if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return false;
    if(TopOwner()!=Owner::Manager||active_native_)return false;
    ++active_native_;return true;
  }
  if(state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  if(purpose==ClientPurpose::Lua) {
    if(lua_closing_||!lua_state_||(TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
       active_lua_==kMaximumInFlight){FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_phase_or_budget");return false;}
    ++active_lua_;return true;
  }
  if(purpose==ClientPurpose::Graphics) {
    if(!GraphicsWindowValid()||
       active_graphics_==kMaximumInFlight) {
      FailureResult(ClientResultKind::ContextOrThreadFault,"graphics","os_context_or_phase");return false;
    }
    ++active_graphics_;return true;
  }
  FailureResult(ClientResultKind::UnsupportedRequirement,"enter","purpose_unavailable");return false;
}
void ClientScene::Leave(ClientPurpose purpose) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  unsigned *count=purpose==ClientPurpose::Lua?&active_lua_:
                  purpose==ClientPurpose::Graphics?&active_graphics_:
                  purpose==ClientPurpose::RenderObservation?&active_observation_:
                  purpose==ClientPurpose::Dispatch?&active_dispatch_:&active_native_;
  if(purpose==ClientPurpose::LocalSettings)return;
  if(!*count){FailureResult(ClientResultKind::ContextOrThreadFault,"leave","unbalanced_use");state_=ClientSceneState::Failed;return;}
  --*count;
  DrainRetained();
}
ClientScene::RetainedEntry *ClientScene::FindRetained(const ClientRetainedTicket &ticket) noexcept {
  if(!ticket.id)return nullptr;
  for(auto &entry:retained_)if(entry.ticket.id==ticket.id&&
      entry.ticket.scene==ticket.scene&&entry.ticket.instance==ticket.instance)return &entry;
  return nullptr;
}
// 逐次比较 OS-current 与真正 renderer window；缓存的句柄只用于识别代次，不充当许可。
bool ClientScene::GraphicsWindowValid() const noexcept {
  if(!IsMain()||context_closing_||
     (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
     !current_dc_||!current_context_)return false;
  void *dc=nullptr,*context=nullptr;
  return ClientRendererHandles(dc,context)&&dc==current_dc_&&context==current_context_&&
      wglGetCurrentDC()==dc&&wglGetCurrentContext()==context;
}
// 资源 admission 与 release 分离：旧修订不再可用，但其真正 owner 仍保留到有效阶段退休。
bool ClientScene::ResourceUseValid(const RetainedEntry &entry) const noexcept {
  if(entry.generations.scene!=generations_.scene||
     entry.generations.instance!=generations_.instance||
     entry.generations.resource!=generations_.resource)return false;
  if(entry.kind==ClientRetainedKind::LuaRegistry)
    return !lua_closing_&&lua_state_&&entry.generations.lua==generations_.lua;
  if(entry.kind==ClientRetainedKind::Graphics||entry.phase==ClientReleasePhase::ManagerContext||
     entry.phase==ClientReleasePhase::RenderContext)
    return entry.generations.context==generations_.context&&GraphicsWindowValid();
  return true;
}
bool ClientScene::DestructiveReady() const noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(foreign_fault_.load(std::memory_order_acquire)||draining_||lua_closing_||context_closing_||
     !ContextHealthy()||InBlockedContext()||TopOwner()!=Owner::Manager||
     active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_)return false;
  for(const auto &borrow:borrows_)if(borrow.id)return false;
  for(const auto &owner:retained_)if(owner.ticket.id&&
      (owner.cpu_queued||owner.gpu_queued||owner.releasing||owner.failed))return false;
  // 未登记于本方表的原生 raw 图像队列仍必须为空；这不是 Render 返回或 GPU idle 推断。
  return ClientNativeCpuQueuesEmpty();
}
// 调用方在已核主线程阶段登记其合法持有的资源；同一指针只接受一个释放 owner。
bool ClientScene::Retain(ClientRetainedKind kind,ClientReleasePhase phase,void *owned,ClientRetainedRelease release,
                         ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(ticket.id)return false;
  ticket={};
  if(!owned||!release||!generations_.scene||draining_||!ContextHealthy()||InBlockedContext()||
     state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  // 这里只登记调用方已合法拥有的对象寿命，不执行构造或应用，也不授予 NativeApply。
  if(kind!=ClientRetainedKind::Native&&kind!=ClientRetainedKind::LuaRegistry&&
     kind!=ClientRetainedKind::Graphics)return false;
  if(kind==ClientRetainedKind::Native&&(TopOwner()!=Owner::Manager||
     (phase!=ClientReleasePhase::Manager&&phase!=ClientReleasePhase::ManagerContext)))return false;
  if((kind==ClientRetainedKind::LuaRegistry&&phase!=ClientReleasePhase::LuaState)||
     (kind==ClientRetainedKind::Graphics&&phase!=ClientReleasePhase::RenderContext&&
      phase!=ClientReleasePhase::ManagerContext))return false;
  if(kind==ClientRetainedKind::LuaRegistry&&(lua_closing_||!lua_state_||
      (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)))return false;
  if(kind==ClientRetainedKind::Graphics&&!GraphicsWindowValid())return false;
  if(phase==ClientReleasePhase::LuaState&&!lua_state_)return false;
  if((phase==ClientReleasePhase::RenderContext||phase==ClientReleasePhase::ManagerContext)&&
     (!GraphicsWindowValid()||
      TopOwner()!=(phase==ClientReleasePhase::RenderContext?Owner::Render:Owner::Manager)))return false;
  for(const auto &entry:retained_)if(entry.ticket.id&&entry.owned==owned){
    FailureResult(ClientResultKind::UnsupportedRequirement,"retain","duplicate_native_owner");return false;
  }
  RetainedEntry *free=nullptr;
  for(auto &entry:retained_)if(!entry.ticket.id){free=&entry;break;}
  if(!free||next_retained_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"retain","native_owner_budget");return false;}
  ticket={++next_retained_,generations_.scene,generations_.instance};
  *free={ticket,kind,phase,owned,release,generations_,0,0,false,false,false};
  ++result_.pending_owners;return true;
}
bool ClientScene::QueueCpuUse(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_||state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->retiring||owner->releasing||owner->failed||!ResourceUseValid(*owner)||
     !ContextHealthy()||InBlockedContext()||owner->cpu_queued==kMaximumInFlight)return false;
  if(owner->kind==ClientRetainedKind::Native&&TopOwner()!=Owner::Manager)return false;
  if(owner->kind==ClientRetainedKind::LuaRegistry&&
     (!lua_state_||(TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)))return false;
  if(owner->kind==ClientRetainedKind::Graphics&&!GraphicsWindowValid())return false;
  ++owner->cpu_queued;return true;
}
bool ClientScene::QueueGpuUse(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_||state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->kind!=ClientRetainedKind::Graphics||owner->retiring||owner->releasing||owner->failed||
     !ResourceUseValid(*owner)||!ContextHealthy()||InBlockedContext()||!GraphicsWindowValid()||
     owner->gpu_queued==kMaximumInFlight)return false;
  ++owner->gpu_queued;return true;
}
void ClientScene::CpuUseComplete(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner||!owner->cpu_queued){Fail("cpu_use_unbalanced");return;}
  if(!ContextHealthy()||InBlockedContext()||
     (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)){
    Fail("cpu_complete_without_owner");return;
  }
  --owner->cpu_queued;DrainRetained();
}
void ClientScene::GpuUseComplete(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner||!owner->gpu_queued){Fail("gpu_use_unbalanced");return;}
  // 只有持原 context 的真实 fence 消费方能报告；禁止错误线程或旧 context 替它归零。
  if(owner->generations.context!=generations_.context||!GraphicsWindowValid()){
    Fail("gpu_complete_without_context");return;
  }
  --owner->gpu_queued;DrainRetained();
}
void ClientScene::RetireRetained(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner)return; // 已完成或重复取消，不能污染当前连接。
  owner->retiring=true;DrainRetained();
}
bool ClientScene::UseLuaRegistry(const ClientRetainedTicket &ticket,void *&owned) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  owned=nullptr;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->kind!=ClientRetainedKind::LuaRegistry||owner->retiring||owner->releasing||owner->failed||
     !ResourceUseValid(*owner)||!active_lua_||draining_||
     !ContextHealthy()||InBlockedContext())return false;
  owned=owner->owned;return true;
}
// CPU 队列和 GPU 完成各由真实消费方显式回报；Render 返回、Lua unref 均不冒充完成。
void ClientScene::DrainRetained() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(state_==ClientSceneState::Failed)
    for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  if(draining_||!ContextHealthy()||InBlockedContext()||
     active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_)return;
  for(const auto &borrow:borrows_)if(borrow.id)return;
  draining_=true;
  for(auto &owner:retained_){
    if(!owner.ticket.id||!owner.retiring||owner.releasing||owner.failed||
       owner.cpu_queued||owner.gpu_queued)continue;
    if(!ContextHealthy()||InBlockedContext()||active_native_||active_lua_||
       active_graphics_||active_observation_||active_dispatch_)break;
    // 旧资源代次禁止新借用，但原 owner 的独占引用仍须在其 VM/context 有效时释放。
    bool phase_valid=false;
    switch(owner.phase){
      case ClientReleasePhase::Manager:phase_valid=TopOwner()==Owner::Manager;break;
      case ClientReleasePhase::LuaState:
        phase_valid=lua_state_&&owner.generations.lua==generations_.lua&&
              (TopOwner()==Owner::Manager||TopOwner()==Owner::Render||
               (lua_closing_&&TopOwner()==Owner::None));break;
      case ClientReleasePhase::RenderContext:
        phase_valid=current_dc_&&current_context_&&
              owner.generations.context==generations_.context&&
              (TopOwner()==Owner::Render||(context_closing_&&TopOwner()==Owner::None))&&
              wglGetCurrentDC()==current_dc_&&wglGetCurrentContext()==current_context_;break;
      case ClientReleasePhase::ManagerContext:
        phase_valid=current_dc_&&current_context_&&
              owner.generations.context==generations_.context&&TopOwner()==Owner::Manager&&
              wglGetCurrentDC()==current_dc_&&wglGetCurrentContext()==current_context_;break;
    }
    if(!phase_valid)continue;
    if(owner.kind==ClientRetainedKind::Graphics||owner.phase==ClientReleasePhase::RenderContext||
       owner.phase==ClientReleasePhase::ManagerContext){
      void *dc=nullptr,*context=nullptr;
      if(!ClientRendererHandles(dc,context)||dc!=current_dc_||context!=current_context_||
         !ClientNativeCpuQueuesEmpty())continue;
    }
    owner.releasing=true;
    if(!owner.release(owner.owned)){
      // false 表示释放可能已有原生执行前缀；禁止自动重试同一回调。
      owner.releasing=false;owner.failed=true;retirement_failed_=true;
      if(!result_.retirement_failure[0])std::snprintf(result_.retirement_failure.data(),result_.retirement_failure.size(),"native_release_failed");
      FailureResult(ClientResultKind::PartialNativeFailure,"drain","native_release_failed");
      state_=ClientSceneState::Failed;continue;
    }
    owner={};
    if(result_.pending_owners)--result_.pending_owners;
  }
  draining_=false;
  FinishRetirementIfDrained();
}
void ClientScene::FinishRetirementIfDrained() noexcept {
  if(retirement_complete_)return;
  if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return;
  if(active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_)return;
  for(const auto &entry:borrows_)if(entry.id)return;
  for(const auto &owner:retained_)if(owner.ticket.id)return;
  if(binary_scene_||HasNativePrefix())return;
  // 二进制协调器已退休原生成员，且本场景的 CPU、GPU 和 Lua 借用均已退尽。
  InvalidateInstance();
  retirement_complete_=true;
  FailureResult(ClientResultKind::Retiring,"retired","native_safe_menu");
}
// 借用只能在同一主线程 owner 栈内使用；不同代次即使地址相同也失效。
bool ClientScene::Borrow(ClientPurpose purpose,ClientBorrowToken &out) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(out.active)return false;
  out={};
  BorrowEntry *free=nullptr;
  for(auto &entry:borrows_)if(!entry.id){free=&entry;break;}
  if(!free||next_borrow_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"borrow","borrow_budget_or_id");return false;}
  if(!Enter(purpose))return false;
  const auto id=++next_borrow_;
  const auto owner_token=OwnerToken(TopOwner());
  if(!owner_token){Leave(purpose);return false;}
  *free={id,generations_.scene,owner_token,purpose};
  out={id,purpose,generations_,owner_token,true};
  return true;
}
bool ClientScene::Valid(const ClientBorrowToken &borrow) const noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  bool registered=false;
  for(const auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose&&
      entry.scene==borrow.generations.scene&&entry.owner_token==borrow.owner_token){registered=true;break;}
  if(!borrow.active||!ContextHealthy()||InBlockedContext()||
     !registered||
     borrow.owner_token==0||borrow.owner_token!=OwnerToken(TopOwner())||
     borrow.generations.scene!=generations_.scene)return false;
  if(borrow.purpose==ClientPurpose::RenderObservation)return TopOwner()==Owner::Render;
  if(borrow.purpose==ClientPurpose::LocalSettings)return true;
  if((state_==ClientSceneState::Failed||state_==ClientSceneState::Retiring)&&
     borrow.purpose!=ClientPurpose::NativeRetire&&borrow.purpose!=ClientPurpose::Dispatch)return false;
  if(borrow.generations.instance!=generations_.instance||
     borrow.generations.resource!=generations_.resource)return false;
  if(borrow.purpose==ClientPurpose::Lua)
    return !lua_closing_&&lua_state_&&borrow.generations.lua==generations_.lua;
  if(borrow.purpose==ClientPurpose::Graphics)
    return borrow.generations.context==generations_.context&&GraphicsWindowValid();
  return true;
}
void ClientScene::Release(ClientBorrowToken &borrow) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!borrow.active)return;
  BorrowEntry *registered=nullptr;
  for(auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose&&
      entry.scene==borrow.generations.scene&&entry.owner_token==borrow.owner_token){registered=&entry;break;}
  if(!registered){borrow.active=false;return;}
  // 取消/失败只是关闭 admission；退出原有栈不要求重新取得已经撤销的使用许可。
  // 正常运行下的错误栈或意外代次变化仍报告，但总会归还本票据一次，绝不等待自己。
  if(!Valid(borrow)&&state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed){
    FailureResult(ClientResultKind::ContextOrThreadFault,"borrow","borrow_invalidated");state_=ClientSceneState::Failed;
  }
  registered->id=0;
  borrow.active=false;Leave(borrow.purpose);
}
bool ClientScene::ConsumeForeignFault() noexcept {
  if(!IsMain())return false;
  return foreign_fault_.exchange(false,std::memory_order_acq_rel);
}
// renderer 身份与 OS-current 分开观察：临时切到 ImGui context 不制造原 context 的新代次。
void ClientScene::ContextWindow() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(TopOwner()!=Owner::Render&&TopOwner()!=Owner::Manager)return;
  if(context_closing_)return;
  void *dc=nullptr,*context=nullptr;
  const bool known=ClientRendererHandles(dc,context);
  if(!known)dc=context=nullptr;
  if(dc!=observed_dc_||context!=observed_context_) {
    if(active_graphics_){
      FailureResult(ClientResultKind::ContextOrThreadFault,"render","context_replaced_inflight");
      state_=ClientSceneState::Failed;
    }
    if(observed_context_)for(const auto &owner:retained_)if(owner.ticket.id&&
        (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext)){
      FailureResult(ClientResultKind::ContextOrThreadFault,"render","context_replaced_with_owner");
      state_=ClientSceneState::Failed;break;
    }
    observed_dc_=dc;observed_context_=context;
    if(generations_.context==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"render","context_generation_exhausted");state_=ClientSceneState::Failed;return;}
    ++generations_.context;
  }
  current_dc_=dc;current_context_=context;
  if(dc!=wglGetCurrentDC()||context!=wglGetCurrentContext())current_dc_=current_context_=nullptr;
  DrainRetained();
}
void ClientScene::RenderWindow() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(TopOwner()==Owner::Render)ContextWindow();
}
// Lua state 身份在主线程观察；替换/消失即换代，旧 registry 借用不能复活。
void ClientScene::ObserveLuaState(void *state) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(state==lua_state_)return;
  if(lua_state_)for(const auto &owner:retained_)if(owner.ticket.id&&owner.phase==ClientReleasePhase::LuaState){
    FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_replaced_with_owner");
    state_=ClientSceneState::Failed;break;
  }
  lua_state_=state;
  if(generations_.lua==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"lua","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.lua;
  if(active_lua_){FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_replaced_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::LuaClosing() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(lua_closing_){Fail("lua_close_reentrant");return;}
  for(auto &owner:retained_)if(owner.ticket.id&&owner.phase==ClientReleasePhase::LuaState)owner.retiring=true;
  lua_closing_=true;
  DrainRetained();
  lua_closing_=false;
  if(generations_.lua==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"lua_close","lua_generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.lua;
  lua_state_=nullptr;
  for(const auto &owner:retained_)if(owner.ticket.id&&owner.phase==ClientReleasePhase::LuaState){FailureResult(ClientResultKind::PartialNativeFailure,"lua_close","lua_owner_inflight");state_=ClientSceneState::Failed;break;}
  if(active_lua_){FailureResult(ClientResultKind::ContextOrThreadFault,"lua_close","borrow_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::ContextClosing() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(context_closing_){Fail("context_close_reentrant");return;}
  for(auto &owner:retained_)if(owner.ticket.id&&
      (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext))owner.retiring=true;
  context_closing_=true;
  DrainRetained();
  context_closing_=false;
  current_dc_=current_context_=observed_dc_=observed_context_=nullptr;
  for(const auto &owner:retained_)if(owner.ticket.id&&
      (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext)){
    FailureResult(ClientResultKind::PartialNativeFailure,"context_close","context_owner_inflight");state_=ClientSceneState::Failed;break;}
  if(generations_.context==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"context_close","context_generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.context;
  if(active_graphics_){FailureResult(ClientResultKind::ContextOrThreadFault,"context_close","draw_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::InvalidateInstance() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(generations_.instance==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"instance","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.instance;InvalidateResource();
}
void ClientScene::InvalidateResource() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(generations_.resource==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"resource","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.resource;
  // 旧语义关联即刻关闭 admission；实际 owner 仍由 Drain 在 CPU/VM/context 条件满足后释放。
  for(auto &owner:retained_)if(owner.ticket.id&&owner.generations.resource!=generations_.resource)
    owner.retiring=true;
}
void ClientScene::Retire(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  SuspendBinaryDisplay();result_.old_display_valid=false;
  if(!result_.retirement_cause[0])std::snprintf(result_.retirement_cause.data(),result_.retirement_cause.size(),"%s",reason?reason:"stop");
  state_=ClientSceneState::Retiring;FailureResult(ClientResultKind::Retiring,"retire",reason);
  for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  DrainRetained();
}
void ClientScene::Fail(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  SuspendBinaryDisplay();result_.old_display_valid=false;
  state_=ClientSceneState::Failed;FailureResult(ClientResultKind::PartialNativeFailure,"failed",reason);
  for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  DrainRetained();
}
bool ClientScene::CanReturnToMenu() const noexcept {
  if(binary_scene_)return false;
  if(!IsMain()||foreign_fault_.load(std::memory_order_acquire))return false;
  for(const auto &entry:borrows_)if(entry.id)return false;
  for(const auto &owner:retained_)if(owner.ticket.id)return false;
  return !HasNativePrefix()&&!active_native_&&!active_lua_&&!active_graphics_&&
         !active_observation_&&!active_dispatch_;
}
} // namespace cs::game

