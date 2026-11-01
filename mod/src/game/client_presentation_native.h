#pragma once
#include "game/client_native_world.h"
#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace cs::game {
// 唯一Hook清单的Minimap原链前后只读诊断；主体自行限定当前Client与输出次数。
void ClientPresentationMinimapDiagnostic(void *map,bool after) noexcept;
// 心形缓存中的字符串由本方owner持有；原生HUD只在同步draw栈中借用。
struct ClientPresentationHeart final {
  bool visible=false,eternal=false,golden=false,fading=false;
  int flash=0;
  std::string animation,overlay;
};
struct ClientPresentationStat final {
  float value=0,delta=0,previous=0,threshold=0;
  int countdown=0;
};
// FXLayers的实际随机选择结果，而不是客户端重新读取XML/seed选择覆盖图。
struct ClientPresentationFXPose final {
  float x=0,y=0,velocity_x=0,velocity_y=0;
  bool flip_x=false,flip_y=false;
};
struct ClientPresentationFX final {
  unsigned id=0,pass=0,min_filter=0,mag_filter=0,wrap_s=0,wrap_t=0,shader=0;
  std::string image;
  std::vector<ClientPresentationFXPose> poses;
};
// ray的动画相位同样来自Authority缓存；native draw的phase自增在离栈时恢复。
struct ClientPresentationRay final {
  float opacity=0,speed=0,phase=0;
  unsigned min_filter=0,mag_filter=0,wrap_s=0,wrap_t=0,shader=0;
  std::string image;
};
struct ClientPresentationRayGroup final {
  std::array<float,7> geometry{};
  std::vector<ClientPresentationRay> rays;
};
// 展示输入只含服务器最终值；原生指针只在当前主线程借用中解析。
struct ClientPresentationDescription final {
  float floor[11]{1,1,1,1,0,0,0,0,0,0,0};
  float wall[11]{1,1,1,1,0,0,0,0,0,0,0};
  float shadow[5]{0,0,0,1,0},light[5]{1,1,1,1,0};
  float shadow_alpha=1,water_amount=0,lighting_alpha=0;
  // 803E20实际layer4输入色；先只闭合普通Basement无特殊darkness/lightning的分支。
  bool ordinary_light_tint=false;
  bool hud_visible=true,room_fx_present=false;
  // HUD采用Authority最终显示条件；合作选择面板仅支持来源明确关闭的代表路径。
  bool hud_achievement_blocked=false,hud_seed_blocked=false,hud_custom_run=false,hud_rerun=false,hud_debug=false;
  unsigned hud_challenge=0,hud_difficulty=0,effective_curses=0;
  std::array<int,8> coop_selection{};
  int minimap_state=0,minimap_shake_duration=0;
  float minimap_small_alpha=0,minimap_big_alpha=0;
  std::array<float,2> minimap_shake{};
  ClientPresentationFX minimap_question,shading;
  bool shading_present=false;
  std::array<ClientPresentationHeart,24> hearts{};
  std::array<ClientPresentationStat,10> stats{};
  int red_flash=0,soul_flash=0,gold_flash=0;
  float drop_scale=1;
  bool combined_devil_angel=false;
  bool fx_loaded=false;
  unsigned fx_ray_groups=0;
  int fx_poop_countdown=0;
  std::array<float,2> fx_average{};
  std::array<float,4> fx_bounds{};
  std::array<float,2> screen_shake{},render_scroll{};
  std::vector<ClientPresentationFX> fx_layers;
  // FXLayers+30是有符号的已选组索引；-1代表Authority当前未采用光线组。
  int fx_ray_group=-1;
  bool fx_ray_lighting=false,fx_ray_darkness=false;
  std::vector<ClientPresentationRayGroup> ray_groups;
};
struct ClientPresentationStep final {
  bool done=false,written=false,failed=false;
  const char *reason="presentation_waiting";
};
struct ClientNativePresentation;
ClientNativePresentation *ClientPresentationCreate() noexcept;
void ClientPresentationDestroy(ClientNativePresentation *) noexcept;
ClientPresentationStep ClientPresentationPrepare(ClientNativePresentation *,const ClientNativeRoots &,
                                                const ClientPresentationDescription &) noexcept;
ClientPresentationStep ClientPresentationDraw(ClientNativePresentation *,const ClientNativeRoots &) noexcept;
ClientPresentationStep ClientPresentationRetire(ClientNativePresentation *,const ClientNativeRoots &) noexcept;
// 正式观测扩展：只复制当前原生显示缓存，不重算概率、不运行HUD/Entity Update。
int ClientPresentationCaptureLua(void *lua_state);
// 仅真实HUD draw栈内覆盖只读显示查询；其它调用仍走官方原链。
bool ClientPresentationHudAchievementQuery(bool &result) noexcept;
} // namespace cs::game
