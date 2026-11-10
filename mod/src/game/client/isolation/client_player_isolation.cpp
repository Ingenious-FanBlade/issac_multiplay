#include "game/client/isolation/client_player_isolation.h"
#include "game/client/isolation/client_isolation.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <atomic>
namespace cs::game {
namespace {
thread_local ClientPlayerIsolationScope *current=nullptr;
std::atomic<std::uint64_t> denied{0};
}
// 不扩大Scene分派许可；构造/安装/关系/退休分别需要Manager，draw仅需要Render。
ClientPlayerIsolationScope::ClientPlayerIsolationScope(ClientPlayerOperation op) noexcept : operation_(op) {
  if(!ClientBusy())return;
  if(!IsMain()||!ContextHealthy()||InBlockedContext()||!ClientPersistenceBlocked()||
      !ClientLuaGuardReady()||!ClientCallbackGuardReady()||
      !HasOwner(op==ClientPlayerOperation::Draw?Owner::Render:Owner::Manager)){
    ClientCallbackFault("player_native_purpose_context");return;
  }
  previous_=current;current=this;active_=true;
}
ClientPlayerIsolationScope::~ClientPlayerIsolationScope(){
  if(!active_)return;
  if(current!=this){ClientCallbackFault("player_native_purpose_unbalanced");return;}
  current=previous_;
}
bool ClientPlayerRandomDenied() noexcept {
  if(!ClientBusy())return false;
  if(IsMain()&&!current)return false; // 18/19自己的具名局部表现流继续由其owner核定。
  // J273 Player ctor→Entity ctor和本方采用的基Init没有随机调用需求，不虚构白名单。
  denied.fetch_add(1,std::memory_order_relaxed);ClientIsolationObserve(ClientIsolationKind::RandomRejected,"player_random");ClientCallbackFault("player_unexpected_random_source");return true;
}
std::uint64_t ClientPlayerRandomDenials() noexcept {return denied.load(std::memory_order_relaxed);}
} // namespace cs::game
