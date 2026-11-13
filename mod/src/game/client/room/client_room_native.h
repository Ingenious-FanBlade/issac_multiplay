#pragma once
#include "game/client/resources/client_resources.h"
#include <array>
#include <cstdint>
namespace cs::game {
// Room只安装已裁决事实。计数和轨道不运行Update；L区域由同一几何版本的实际来源给出。
struct ClientRoomState final {
  bool initialized=false,red_heart_damage=false,pacifist=false,ambush_active=false;
  int frame=0,game_frame=0,clear_delay=0,greed_timer=0,greed_wave=0,ambush_wave=0,ambush_max=0,ambush_bosses=0;
  // J273仅核定7220低字节的绘制条件；不采用SDK误标的int duration及未知高位。
  std::uint8_t render_flag_7220=0;
  int total_money=0,shop_item=0,shop_discount=0,greed_subtype=0,poop_count=0,keeper_coins=0,pits=0;
  unsigned boss=0;
  std::array<float,4> bounds{};
  std::array<float,8> l_area{};
  std::array<int,8> l_tiles{};
  std::array<int,8> door_positions{};
  std::array<bool,448> valid{},interior{};
  std::array<std::uint8_t,448> rails{};
  // GetGridPath 为 +76c 原始整数；只复制活动extent，尾部448..455没有已知用途。
  std::vector<int> grid_paths;
  // 原生Grid灯光消费的五项只读Authority显示条件；Client draw按用途供应，不重新裁决路线。
  std::array<int,5> grid_light_queries{};
  bool rails_present=false;
  ClientResourcePose rail_sprite;
};
// 正式只读来源；仅在现有主线程采集窗口复制，失败返回nil并使域不完整。
int ClientRoomConfigCaptureLua(void *);
int ClientRoomStateCaptureLua(void *);
int ClientRoomBackdropSpritesLua(void *);
int ClientRoomVeinSpritesLua(void *);
int ClientRoomMapSpritesLua(void *);
// 写入只发生于既有Manager用途窗口；Room析构清除内部状态，Game的全局借用由world owner恢复。
// 水面预绘制内部也会选择rail/pitrail帧；同样保护原选择器数组，外层负责渲染上下文恢复。
} // namespace cs::game
