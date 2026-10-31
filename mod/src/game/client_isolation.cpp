#include "game/client_isolation.h"
#include "game/bridge.h"
#include "runtime.h"
#include <atomic>
#include <cstring>
#include <sstream>
namespace cs::game {
namespace {
// 使用固定目录而非逐调用分配；非主线程也只操作原子计数，不进入Lua或游戏对象。
constexpr const char *entries[]={"game_update","post_update","room_update","odd_helper","entity_list",
  "player_update","player_native","start_game","restore","room_lifecycle","door_update","door_nested",
  "new_room","game_exit","join_pass","move","bomb","active_item","card_pill","shoot","drop",
  "pgd_unlock","pgd_counter","pgd_local","pgd_cloud","gamestate_write","gamestate_delete",
  "gamestate_delete_rerun","stage_transition","entity_kill","revive_ghost","load_game_state"};
std::atomic<std::uint64_t> blocked[sizeof(entries)/sizeof(entries[0])]{};
std::atomic<bool> progress_protected{false};
}
bool ClientProgressProtected() noexcept {return progress_protected.load(std::memory_order_acquire);}
void ClientProtectionBegin() noexcept {progress_protected.store(true,std::memory_order_release);}
void ClientProtectionEnd() noexcept {progress_protected.store(false,std::memory_order_release);}
bool ClientSuppress(const char *entry) noexcept {
  if(!ClientBusy())return false;
  for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);++i)if(!std::strcmp(entry,entries[i])){
    blocked[i].fetch_add(1,std::memory_order_relaxed);
    if(Settings.diagnostics)Trace("client_isolation","entry=%s original=0",entry);
    return true;
  }
  return false;
}
std::string ClientIsolationFields(){std::ostringstream o;o<<"client.progress_protected="<<ClientProgressProtected()<<'\n';
  for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);++i)o<<"client.blocked."<<entries[i]<<'='<<blocked[i].load(std::memory_order_relaxed)<<'\n';return o.str();}
} // namespace cs::game
