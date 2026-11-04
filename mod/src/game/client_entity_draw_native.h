#pragma once
#include "game/client_native_world.h"
#include "game/client_entity_e2e.h"
namespace cs::game {
// 整帧借用Authority普通窗口表现输入；还原全局随机/帧值后才离开唯一Scene绘制。
class ClientEntityFrameScope final {
public:
  explicit ClientEntityFrameScope(const ClientNativeRoots &);
  ~ClientEntityFrameScope();
  bool Ready() const noexcept {return ready_;}
  ClientEntityFrameScope(const ClientEntityFrameScope &)=delete;
  ClientEntityFrameScope &operator=(const ClientEntityFrameScope &)=delete;
private:void *state_=nullptr;bool ready_=false;
};
// 唯一Hook清单的逐类Render/Shadow/Post入口共用此保护；水面间接调用也不能绕过。
class ClientEntityDrawScope final {
public:
  ClientEntityDrawScope(void *entity,void *offset,unsigned pass);
  ~ClientEntityDrawScope();
  bool CallOriginal() const noexcept {return original_;}
  ClientEntityDrawScope(const ClientEntityDrawScope &)=delete;
  ClientEntityDrawScope &operator=(const ClientEntityDrawScope &)=delete;
private:
#ifdef ISAACCS_E2E
  ClientEntityTestMeasure cpu_{"draw_scope_inclusive"};
  // 只保存本次真实draw入口的值；不把原生地址交给故障状态或Rust。
  std::uint64_t player_test_run_=0,player_test_id_=0;
  unsigned player_test_space_=0,player_test_pass_=0;
#endif
  void *state_=nullptr;bool original_=true;
};
bool ClientEntityDrawPaused(bool &) noexcept;
// 原版Deathmatch会把Type=1纳入NPC的数值判断；普通C/S仅使用真实Player/NPC布局。
bool ClientEntityOrdinaryTypeContext() noexcept;
// Abyss helper含fade/Reset/Update；本用途只画Source已安装的最终ANM，不执行这些writer。
bool ClientEntityDrawOwnedFinal(void *entity,void *sprite,void *offset);
bool ClientEntityDrawIsolationInstalled() noexcept;
// 未知异常可能留下raw CPU借用；Scene不得仅凭队列为空就销毁其Entity/ANM owner。
bool ClientEntityDrawDrained() noexcept;
bool ClientPlayerDrawPalette(void *output,void *player) noexcept;
} // namespace cs::game
