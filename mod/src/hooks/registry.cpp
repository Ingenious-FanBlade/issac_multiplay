#include "HookSystem.h"
#include "hook_capacity.h"
#include "runtime.h"
#include "hooks/random_runtime.h"
#include "game/client_player_isolation.h"
#include "game/bridge.h"
#include <cstring>
namespace cs {
namespace {
// 唯一待注册清单，静态构造阶段不触碰 REPENTOGON。
// 容量与行数来自生成的 hook_capacity.h，生成期已校验行数不超限；
// 这里保留运行时检查，避免手工改动后静默丢弃。
Hook *pending[kHookPendingCapacity]{};
size_t used = 0;
// 容量不足时不静默跳过：保留前 8 条被拒记录，供 PrepareBindings 停用对应
// 能力组并写入 trace。计数另记，避免拒收超过保留容量时丢失总数。
constexpr size_t kRejectedKeep = 8;
Hook *rejected[kRejectedKeep]{};
size_t rejected_count = 0;
size_t rejected_total = 0;
} // namespace
void AddPending(Hook *h) noexcept {
  if (!h)
    return;
  if (used < kHookPendingCapacity) {
    pending[used++] = h;
    return;
  }
  ++rejected_total;
  if (rejected_count < kRejectedKeep)
    rejected[rejected_count++] = h;
}
Hook *const *Hooks(size_t &n) noexcept {
  n = used;
  return pending;
}
HookCapacity PendingHookCapacity() noexcept {
  return {kHookPendingCapacity, used, rejected_total};
}
Hook *const *RejectedHooks(size_t &count) noexcept {
  count = rejected_count;
  return rejected;
}
void RegisterHooks() {
  // tag 1.1.3 的公开注册入口是构造函数，元数据及对象常驻到进程结束。
  size_t installed = 0, deferred = 0;
  for (size_t i = 0; i < used; ++i) {
    auto &h = *pending[i];
    if (!Capabilities[size_t(h.group)])
      continue;
    new FunctionHook(h.name, *h.type, h.entry, h.super, h.priority);
    h.registered = true;
    ++installed;
    Trace("hook_registered", "%s group=%s priority=%d", h.id,
          GroupNames[size_t(h.group)], h.priority);
  }
  // 容量与安装结果必须显式可见：注册数不等于安装数，超限也不能静默。
  const auto capacity = PendingHookCapacity();
  Trace("hook_summary",
        "capacity=%u rows=%u used=%u rejected=%u installed=%u deferred=%u",
        unsigned(capacity.capacity), unsigned(kHookManifestRows),
        unsigned(capacity.used), unsigned(capacity.rejected),
        unsigned(installed), unsigned(deferred));
}
} // namespace cs
namespace cs::game {
// 构造前核对真实trampoline；持久保护从连接准备之前生效并延续到最后退出writer。
bool ClientPlayerIsolationInstalled() noexcept {
  if(!IsMain())return false;
  const char *required[]={"native_rng_ctor","native_rng_seed","native_rng_next","native_rng_int","native_rng_float",
    "native_mt_seed","native_mt_next","native_global_random","native_global_seed",
    "client_entity_local_sound","client_entity_retire_sound","client_player_update","client_game_start",
    "client_entity_draw_player","client_entity_shadow_player","pgd_unlock","pgd_counter","pgd_local","pgd_cloud",
    "load_game_state","gamestate_write","gamestate_read","gamestate_delete","gamestate_delete_rerun",
    "manager_set_save_slot","manager_save_game_state","manager_record_completion","game_save_state",
    "pgd_add_challenge","pgd_add_collection","pgd_add_boss","pgd_add_bestiary_kill","pgd_encounter","pgd_unlock_steam",
    "gamestate_clear","gamestate_write_rerun","gamestate_read_rerun","isaac_shutdown"};
  size_t count=0;const auto hooks=Hooks(count);
  for(const char *id:required){bool installed=false;for(size_t i=0;i<count;++i){const auto &h=*hooks[i];
    if(!std::strcmp(h.id,id)){installed=h.registered&&h.super&&*h.super;break;}}
    if(!installed)return false;}
  for(unsigned i=0;i<5;++i)if(!RuntimeRandomHookInstalled(i))return false;
  return ClientLuaGuardReady()&&ClientCallbackGuardReady();
}
bool ClientObjectPreStatsIsolationInstalled() noexcept {
  if(!Capabilities[size_t(Group::Client)])return false;
  size_t count=0;auto hooks=Hooks(count);
  for(size_t i=0;i<count;++i){const auto &h=*hooks[i];
    if(std::strcmp(h.id,"client_player_pre_stats")==0)return h.registered&&h.super&&*h.super;
  }
  return false;
}
bool ClientEntityInitIsolationInstalled() noexcept {
  bool random=false,sound=false,play=false;size_t count=0;auto hooks=Hooks(count);for(size_t i=0;i<count;++i){const auto &h=*hooks[i];
    if(std::strcmp(h.id,"native_global_random")==0)random=h.registered&&h.super&&*h.super;
    if(std::strcmp(h.id,"client_entity_retire_sound")==0)sound=h.registered&&h.super&&*h.super;
    if(std::strcmp(h.id,"client_entity_local_sound")==0)play=h.registered&&h.super&&*h.super;
  }return random&&sound&&play;
}
bool ClientEntityDrawIsolationInstalled() noexcept {
  const char *required[]={"client_entity_draw_player","client_entity_shadow_player","client_player_palette","client_entity_draw_npc","client_entity_draw_tear","client_entity_draw_projectile","client_entity_draw_bomb",
    "client_entity_draw_familiar","client_entity_draw_knife","client_entity_draw_pickup","client_entity_draw_laser","client_entity_draw_slot",
    "client_entity_draw_effect","client_entity_draw_dummy","client_entity_shadow_base","client_entity_shadow_npc","client_entity_shadow_familiar",
    "client_entity_shadow_laser","client_entity_post_base","client_entity_post_laser","client_entity_draw_paused","client_entity_owned_final","native_text_render"};
  size_t count=0;auto hooks=Hooks(count);
  for(const auto *id:required){bool found=false;for(size_t i=0;i<count;++i){const auto &h=*hooks[i];if(!std::strcmp(h.id,id)){found=h.registered&&h.super&&*h.super;break;}}if(!found)return false;}
  return true;
}
}
