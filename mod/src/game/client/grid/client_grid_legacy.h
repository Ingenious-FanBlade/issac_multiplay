#pragma once
#include "game/client/entity/client_objects_native.h"

namespace cs::game {
// 仅供尚未退役的旧JSON运行链使用；禁止把这些值隐式转换为正式二进制影子描述。
// 只允许目录中具名公开成员；不是可以从网络调用任意 Lua 方法的 RPC。
enum class LegacyClientGridValueKind : unsigned { Integer, Boolean, Text };
struct LegacyClientGridField final {
  unsigned cast=0;
  std::string name,text;
  LegacyClientGridValueKind kind=LegacyClientGridValueKind::Integer;
  int integer=0;
  bool boolean=false;
  bool null_text=false;
};
// 资源仍由原生内嵌 ANM 拥有；TimerPlate 的 Null 与空资源分开。
struct LegacyClientGridSprite final {
  unsigned role=0; // 0=主、1=Door extra、2=TimerPlate、3/4=Fire 专用。
  bool present=false;
  ClientResourcePose pose;
};
struct LegacyClientGridEmber final {float x=0,y=0,vx=0,vy=0;int age=0,max_life=0;};
// 保存描述八项保持原值；本地构造完成与来源 Initialized 没有相互推导关系。
struct LegacyClientGridDescription final {
  ClientObjectKey key{},room{};
  std::uint64_t revision=0;
  int index=0,type=0,variant=0,state=0,var_data=0,collision=0;
  int desc_state=0,desc_var_data=0,spawn_count=0;
  unsigned spawn_seed=0,variable_seed=0,water_clip=0,cast_mask=0;
  unsigned spawn_frame=0;
  bool source_initialized=false,grid_member=false;
  unsigned door_aliases=0;
  ClientPoseVector position{},render_position{};
  std::vector<LegacyClientGridField> fields;
  std::vector<LegacyClientGridSprite> sprites;
  int big_rock_frame=0;
  int fire_timer=0,fire_style=0;
  bool door_animation_mode=false;
  unsigned door_light_flags=0;
  std::vector<LegacyClientGridEmber> embers;
  std::vector<ClientPoseVector> rubble;
};
enum class ClientGridOperation : unsigned { Remove, Create, Change, Replace, BindDoors, Check };
struct LegacyClientGridCommand final {
  ClientGridOperation operation=ClientGridOperation::Check;
  LegacyClientGridDescription value;
};
} // namespace cs::game
