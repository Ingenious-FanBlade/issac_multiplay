#include "game/client/isolation/client_isolation.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "replica/stream_e2e.h"
#include <array>
#include <atomic>
namespace cs::game {
namespace {
constexpr const char *names[]={"begin","native_call","native_return","expected_skip",
  "rejected_entry","lua_skip","own_lua","native_persistence","lua_persistence",
  "random_rejected","fault","recover","stop","retired","observation_lost","mod_install"};
std::array<std::atomic<std::uint64_t>,unsigned(ClientIsolationKind::Count)> counts{};
}
void ClientIsolationObserve(ClientIsolationKind kind,const char *entry) noexcept {
  const auto i=unsigned(kind);if(i>=counts.size())return;
  // 退出保护可能比ClientBusy更长；最后一个Shutdown writer仍属于隔离窗口。
  if(!ClientPersistenceBlocked()&&kind!=ClientIsolationKind::Retired)return;
  counts[i].fetch_add(1,std::memory_order_relaxed);
#ifdef ISAACCS_E2E
  const bool main=IsMain();
  const auto identity=main?ClientIsolationCurrent():ClientIsolationIdentity{};
  replica::stream::E2EIsolation(names[i],entry?entry:"unknown",main,
    ClientPersistenceBlocked(),main&&ContextHealthy()&&!InBlockedContext(),
    main?unsigned(TopOwner()):0,
    {identity.local_connection,identity.connection,identity.route,identity.scene,identity.instance,
     identity.resource,identity.lua,identity.context,identity.phase});
#else
  (void)entry;
#endif
}
std::string ClientIsolationFields(){
  std::string out="client.isolation.version=1\n";
  out+="client.isolation.persistence_blocked="+std::to_string(ClientPersistenceBlocked())+"\n";
  for(unsigned i=0;i<counts.size();++i)out+="client.isolation."+std::string(names[i])+"="+std::to_string(counts[i].load(std::memory_order_relaxed))+"\n";
  return out;
}
} // namespace cs::game
