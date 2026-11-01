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
}
// 新连接只建立等待身份；从不把等待 Scope 的 run/room 伪装成原生场景。
void ClientScene::Begin(const replica::Scope &scope) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!CanReturnToMenu()){FailureResult(ClientResultKind::PartialNativeFailure,"begin","old_owner_inflight");state_=ClientSceneState::Failed;return;}
  waiting_scope_=scope;target_scope_={};applied_scope_={};target_.reset();applied_.reset();
  target_revision_=applied_revision_=0;
  if(generations_.scene==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"begin","scene_generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.scene;
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
  if(foreign_fault_.exchange(false,std::memory_order_acq_rel)){
    FailureResult(ClientResultKind::ContextOrThreadFault,"enter","foreign_use_attempt");state_=ClientSceneState::Failed;return false;
  }
  if(!ContextHealthy()||InBlockedContext()) {
    FailureResult(ClientResultKind::ContextOrThreadFault,"enter","owner_or_thread");return false;
  }
  if(purpose==ClientPurpose::RenderObservation){
    if(state_==ClientSceneState::Failed||TopOwner()!=Owner::Render||
       active_observation_==kMaximumInFlight)return false;
    ++active_observation_;return true;
  }
  if(purpose==ClientPurpose::LocalSettings)return TopOwner()!=Owner::None;
  if(purpose==ClientPurpose::NativeRetire) {
    if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return false;
    if(TopOwner()!=Owner::Manager||active_native_)return false;
    ++active_native_;return true;
  }
  if(state_==ClientSceneState::Waiting||state_==ClientSceneState::Retiring||
     state_==ClientSceneState::Failed||!target_) {
    FailureResult(ClientResultKind::UnsupportedRequirement,"enter","native_scene_unavailable");return false;
  }
  if(purpose==ClientPurpose::NativeApply){
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
  if((state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)&&
     !active_native_&&!active_lua_&&!active_graphics_&&!active_observation_) {
    target_.reset();target_revision_=0;InvalidateInstance();result_.generations=generations_;
  }
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
  *free={id,purpose};
  out={id,purpose,generations_,OwnerToken(TopOwner()),true};
  return true;
}
bool ClientScene::Valid(const ClientBorrowToken &borrow) const noexcept {
  if(!IsMain())return false;
  bool registered=false;
  for(const auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose){registered=true;break;}
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
  for(auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose){registered=&entry;break;}
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
    observed_dc_=dc;observed_context_=context;
    current_dc_=dc;current_context_=context;
    if(generations_.context==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"render","context_generation_exhausted");state_=ClientSceneState::Failed;return;}
    ++generations_.context;
  }
}
// Lua state 身份在主线程观察；替换/消失即换代，旧 registry 借用不能复活。
void ClientScene::ObserveLuaState(void *state) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(state==lua_state_)return;
  lua_state_=state;
  if(generations_.lua==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"lua","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.lua;
  if(active_lua_){FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_replaced_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::LuaClosing() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(generations_.lua==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"lua_close","lua_generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.lua;
  lua_state_=nullptr;
  if(active_lua_){FailureResult(ClientResultKind::ContextOrThreadFault,"lua_close","borrow_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::ContextClosing() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  current_dc_=current_context_=nullptr;
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
  if(!active_native_&&!active_lua_&&!active_graphics_&&!active_observation_){target_.reset();target_revision_=0;InvalidateInstance();result_.generations=generations_;}
}
void ClientScene::Fail(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  state_=ClientSceneState::Failed;FailureResult(ClientResultKind::PartialNativeFailure,"failed",reason);
  if(!active_native_&&!active_lua_&&!active_graphics_&&!active_observation_){target_.reset();target_revision_=0;InvalidateInstance();result_.generations=generations_;}
}
bool ClientScene::CanReturnToMenu() const noexcept {
  if(!IsMain()||foreign_fault_.load(std::memory_order_acquire))return false;
  for(const auto &entry:borrows_)if(entry.id)return false;
  return !HasNativePrefix()&&
         !active_native_&&!active_lua_&&!active_graphics_&&!active_observation_&&!applied_;
}
} // namespace cs::game
