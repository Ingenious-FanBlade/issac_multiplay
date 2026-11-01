#include "game/client_scene.h"
#include "game/context.h"
#include "game/bridge.h"
#include "runtime.h"
#include <cstdio>
#include <cstring>
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
// 新连接只建立等待身份；从不把等待 Scope 的 run/room 伪装成原生场景。
void ClientScene::Begin(const replica::Scope &scope) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!CanReturnToMenu()){FailureResult(ClientResultKind::PartialNativeFailure,"begin","old_owner_inflight");state_=ClientSceneState::Failed;return;}
  waiting_scope_=scope;target_scope_={};applied_scope_={};target_.reset();applied_.reset();
  target_revision_=applied_revision_=0;
  const auto epoch=NewSceneEpoch();
  if(!epoch){FailureResult(ClientResultKind::BudgetExhausted,"begin","scene_generation_exhausted");state_=ClientSceneState::Failed;return;}
  generations_.scene=epoch;
  state_=ClientSceneState::Waiting;result_={};result_.scope=scope;
  FailureResult(ClientResultKind::WaitingDependency,"waiting","world_payload_unavailable");
  current_dc_=current_context_=observed_dc_=observed_context_=nullptr;
}
void ClientScene::FailureResult(ClientResultKind kind,const char *phase,const char *reason) noexcept {
  result_.kind=kind;
  std::snprintf(result_.phase.data(),result_.phase.size(),"%s",phase?phase:"unknown");
  std::snprintf(result_.reason.data(),result_.reason.size(),"%s",reason?reason:"unknown");
  result_.generations=generations_;
  result_.native_committed=false;
}
// 现阶段模型载荷没有进入 Client worker；任何直接调用均不可被误认为 native Applied。
bool ClientScene::AdmitTarget(const replica::Scope &scope,std::uint64_t revision,
                              const replica::Value &value) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed||
     active_native_||active_lua_||active_graphics_||active_observation_){
    FailureResult(ClientResultKind::Retiring,"admit","owner_unavailable");return false;
  }
  result_.scope=scope;result_.revision=revision;
  if(next_attempt_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"admit","attempt_exhausted");return false;}
  result_.attempt=++next_attempt_;
  if(!scope.session||!scope.connection||!scope.run||!scope.scene||!revision||
     scope.session!=waiting_scope_.session||scope.connection!=waiting_scope_.connection) {
    FailureResult(ClientResultKind::WaitingDependency,"admit","formal_scope_required");return false;
  }
  try {
    replica::CheckValue(value);
    replica::ValidateModel(value);
    if(value["publishable"]!=true||replica::ReadScope(value["scope"])!=scope||
       replica::Unsigned(value["revision"])!=revision) {
      FailureResult(ClientResultKind::UnsupportedRequirement,"admit","target_identity_or_coverage");return false;
    }
    for(const auto &borrow:borrows_)if(borrow.id){
      FailureResult(ClientResultKind::Retiring,"admit","borrow_inflight");return false;
    }
    if(target_&&scope!=target_scope_){
      FailureResult(ClientResultKind::UnsupportedRequirement,"admit","scene_transition_unavailable");return false;
    }
    if(target_&&revision==target_revision_) {
      FailureResult(*target_==value?ClientResultKind::DuplicateTarget:ClientResultKind::UnsupportedRequirement,
                    "admit",*target_==value?"duplicate_target":"same_version_conflict");return false;
    }
    if(target_&&revision<target_revision_) {
      FailureResult(ClientResultKind::WaitingDependency,"admit","stale_revision");return false;
    }
    if(applied_&&(scope!=applied_scope_||revision<=applied_revision_)) {
      FailureResult(ClientResultKind::WaitingDependency,"admit","already_applied_revision");return false;
    }
    auto copy=std::make_unique<replica::Json>(replica::Copy(value));
    target_=std::move(copy);target_scope_=scope;target_revision_=revision;
  } catch(...) {FailureResult(ClientResultKind::UnsupportedRequirement,"admit","invalid_or_unavailable_target");return false;}
  // T 有独立所有权；没有原生 adapter 时仍不得进入准备或改动 A。
  FailureResult(ClientResultKind::UnsupportedRequirement,"admit","native_adapter_unavailable");
  return false;
}
// 每个原生、Lua、图形用途都要求已知主线程和 owner；图形另要求当前 OS context。
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
  if(purpose==ClientPurpose::LocalSettings)return TopOwner()!=Owner::None;
  if(purpose==ClientPurpose::NativeRetire) {
    if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return false;
    if(TopOwner()!=Owner::Manager||active_native_)return false;
    ++active_native_;return true;
  }
  if(state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  if(purpose==ClientPurpose::NativeApply){
    if(state_==ClientSceneState::Waiting||!target_){FailureResult(ClientResultKind::UnsupportedRequirement,"apply","target_unavailable");return false;}
    if(TopOwner()!=Owner::Manager){FailureResult(ClientResultKind::ContextOrThreadFault,"apply","native_phase");return false;}
    FailureResult(ClientResultKind::UnsupportedRequirement,"apply","native_adapter_unavailable");return false;
  }
  if(purpose==ClientPurpose::Lua) {
    if(!lua_state_||(TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
       active_lua_==kMaximumInFlight){FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_phase_or_budget");return false;}
    ++active_lua_;return true;
  }
  if(purpose==ClientPurpose::Graphics) {
    if(TopOwner()!=Owner::Render||!current_dc_||!current_context_||
       wglGetCurrentDC()!=current_dc_||wglGetCurrentContext()!=current_context_||
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
                  purpose==ClientPurpose::RenderObservation?&active_observation_:&active_native_;
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
// 调用方在已核主线程阶段登记其合法持有的资源；同一指针只接受一个释放 owner。
bool ClientScene::Retain(ClientRetainedKind kind,ClientReleasePhase phase,void *owned,ClientRetainedRelease release,
                         ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(ticket.id)return false;
  ticket={};
  if(!owned||!release||!generations_.scene||draining_||!ContextHealthy()||InBlockedContext()||
     state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  // 这里只登记调用方已合法拥有的对象寿命，不执行构造或应用，也不授予 NativeApply。
  if(kind==ClientRetainedKind::Native&&TopOwner()!=Owner::Manager)return false;
  if((kind==ClientRetainedKind::LuaRegistry&&phase!=ClientReleasePhase::LuaState)||
     (kind==ClientRetainedKind::Graphics&&phase!=ClientReleasePhase::RenderContext))return false;
  if(kind==ClientRetainedKind::LuaRegistry&&(!lua_state_||
      (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)))return false;
  if(kind==ClientRetainedKind::Graphics&&
     (TopOwner()!=Owner::Render||!current_dc_||!current_context_||
      wglGetCurrentDC()!=current_dc_||wglGetCurrentContext()!=current_context_))return false;
  if(phase==ClientReleasePhase::LuaState&&!lua_state_)return false;
  if((phase==ClientReleasePhase::RenderContext||phase==ClientReleasePhase::ManagerContext)&&
     (!current_dc_||!current_context_||
      wglGetCurrentDC()!=current_dc_||wglGetCurrentContext()!=current_context_||
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
  if(draining_)return false;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->retiring||owner->failed||owner->generations!=generations_||
     !ContextHealthy()||InBlockedContext()||owner->cpu_queued==kMaximumInFlight)return false;
  if(owner->kind==ClientRetainedKind::Native&&TopOwner()!=Owner::Manager)return false;
  if(owner->kind==ClientRetainedKind::LuaRegistry&&
     (!lua_state_||(TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)))return false;
  if(owner->kind==ClientRetainedKind::Graphics&&
     (TopOwner()!=Owner::Render||!current_dc_||!current_context_||
      wglGetCurrentDC()!=current_dc_||wglGetCurrentContext()!=current_context_))return false;
  ++owner->cpu_queued;return true;
}
bool ClientScene::QueueGpuUse(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_)return false;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->kind!=ClientRetainedKind::Graphics||owner->retiring||owner->failed||
     owner->generations!=generations_||!ContextHealthy()||InBlockedContext()||
     TopOwner()!=Owner::Render||!current_dc_||!current_context_||
     wglGetCurrentDC()!=current_dc_||wglGetCurrentContext()!=current_context_||
     owner->gpu_queued==kMaximumInFlight)return false;
  ++owner->gpu_queued;return true;
}
void ClientScene::CpuUseComplete(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner||!owner->cpu_queued){Fail("cpu_use_unbalanced");return;}
  --owner->cpu_queued;DrainRetained();
}
void ClientScene::GpuUseComplete(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner||!owner->gpu_queued){Fail("gpu_use_unbalanced");return;}
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
  owned=nullptr;
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  auto *owner=FindRetained(ticket);
  if(!owner||owner->kind!=ClientRetainedKind::LuaRegistry||owner->retiring||owner->releasing||owner->failed||
     owner->generations!=generations_||!active_lua_||draining_||
     !ContextHealthy()||InBlockedContext())return false;
  owned=owner->owned;return true;
}
// CPU 队列和 GPU 完成各由真实消费方显式回报；Render 返回、Lua unref 均不冒充完成。
void ClientScene::DrainRetained() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(state_==ClientSceneState::Failed)
    for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  if(draining_||!ContextHealthy()||InBlockedContext()||
     active_native_||active_lua_||active_graphics_||active_observation_)return;
  for(const auto &borrow:borrows_)if(borrow.id)return;
  draining_=true;
  for(auto &owner:retained_){
    if(!owner.ticket.id||!owner.retiring||owner.releasing||owner.failed||
       owner.cpu_queued||owner.gpu_queued)continue;
    if(!ContextHealthy()||InBlockedContext()||active_native_||active_lua_||
       active_graphics_||active_observation_)break;
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
    owner.releasing=true;
    if(!owner.release(owner.owned)){
      // false 表示释放可能已有原生执行前缀；禁止自动重试同一回调。
      owner.releasing=false;owner.failed=true;
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
  if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return;
  if(active_native_||active_lua_||active_graphics_||active_observation_)return;
  for(const auto &entry:borrows_)if(entry.id)return;
  for(const auto &owner:retained_)if(owner.ticket.id)return;
  if(target_){target_.reset();target_revision_=0;InvalidateInstance();result_.generations=generations_;}
}
// 借用只能在同一主线程 owner 栈内使用；不同代次即使地址相同也失效。
bool ClientScene::Borrow(ClientPurpose purpose,ClientBorrowToken &out) noexcept {
  if(out.active)return false;
  out={};
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  BorrowEntry *free=nullptr;
  for(auto &entry:borrows_)if(!entry.id){free=&entry;break;}
  if(!free||next_borrow_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"borrow","borrow_budget_or_id");return false;}
  if(!Enter(purpose))return false;
  const auto id=++next_borrow_;
  const auto owner_token=OwnerToken(TopOwner());
  *free={id,generations_.scene,owner_token,purpose};
  out={id,purpose,generations_,owner_token,true};
  return true;
}
bool ClientScene::Valid(const ClientBorrowToken &borrow) const noexcept {
  if(!IsMain())return false;
  bool registered=false;
  for(const auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose&&
      entry.scene==borrow.generations.scene&&entry.owner_token==borrow.owner_token){registered=true;break;}
  if(!borrow.active||!ContextHealthy()||InBlockedContext()||
     !registered||borrow.generations!=generations_||
     borrow.owner_token==0||borrow.owner_token!=OwnerToken(TopOwner())||
     borrow.generations.scene!=generations_.scene)return false;
  if(borrow.purpose==ClientPurpose::RenderObservation)return TopOwner()==Owner::Render;
  if(borrow.purpose==ClientPurpose::Lua)
    return lua_state_&&borrow.generations.lua==generations_.lua;
  if(borrow.purpose==ClientPurpose::Graphics)
    return current_dc_&&current_context_&&borrow.generations.context==generations_.context&&
           wglGetCurrentDC()==current_dc_&&wglGetCurrentContext()==current_context_;
  return borrow.generations.instance==generations_.instance&&
         borrow.generations.resource==generations_.resource;
}
void ClientScene::Release(ClientBorrowToken &borrow) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!borrow.active)return;
  BorrowEntry *registered=nullptr;
  for(auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose&&
      entry.scene==borrow.generations.scene&&entry.owner_token==borrow.owner_token){registered=&entry;break;}
  if(!registered){borrow.active=false;return;}
  if(!Valid(borrow)){FailureResult(ClientResultKind::ContextOrThreadFault,"borrow","borrow_invalidated");state_=ClientSceneState::Failed;}
  registered->id=0;
  Leave(borrow.purpose);borrow.active=false;
}
bool ClientScene::ConsumeForeignFault() noexcept {
  if(!IsMain())return false;
  return foreign_fault_.exchange(false,std::memory_order_acq_rel);
}
// Render owner 每次重新读取 OS current；变化使旧图形借用失效，不主动切换 context。
void ClientScene::RenderWindow() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(TopOwner()!=Owner::Render)return;
  void *dc=nullptr,*context=nullptr;
  const bool known=ClientRendererHandles(dc,context);
  auto *os_dc=wglGetCurrentDC();auto *os_context=wglGetCurrentContext();
  if(!known||dc!=os_dc||context!=os_context)dc=context=nullptr;
  if(dc!=observed_dc_||context!=observed_context_) {
    if(observed_context_)for(const auto &owner:retained_)if(owner.ticket.id&&
        (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext)){
      FailureResult(ClientResultKind::ContextOrThreadFault,"render","context_replaced_with_owner");
      state_=ClientSceneState::Failed;break;
    }
    observed_dc_=dc;observed_context_=context;
    current_dc_=dc;current_context_=context;
    if(generations_.context==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"render","context_generation_exhausted");state_=ClientSceneState::Failed;return;}
    ++generations_.context;
  }
  DrainRetained();
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
}
void ClientScene::Retire(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  state_=ClientSceneState::Retiring;FailureResult(ClientResultKind::Retiring,"retire",reason);
  for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  DrainRetained();
}
void ClientScene::Fail(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  state_=ClientSceneState::Failed;FailureResult(ClientResultKind::PartialNativeFailure,"failed",reason);
  for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  DrainRetained();
}
bool ClientScene::CanReturnToMenu() const noexcept {
  if(!IsMain()||foreign_fault_.load(std::memory_order_acquire))return false;
  for(const auto &entry:borrows_)if(entry.id)return false;
  for(const auto &owner:retained_)if(owner.ticket.id)return false;
  return !HasNativePrefix()&&
         !active_native_&&!active_lua_&&!active_graphics_&&!active_observation_&&!applied_;
}
} // namespace cs::game
