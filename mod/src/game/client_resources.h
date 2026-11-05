#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace cs::game {
// 资源描述跨 C++20 模型与 C++17 SDK 边界只携带自有值；无 Lua/native 句柄。
struct ClientPoseVector final { float x=0,y=0; };
struct ClientPoseColor final {
  std::array<float,4> tint{1,1,1,1},colorize{};
  std::array<float,3> offset{};
};
struct ClientLayerPose final {
  unsigned id=0,flags=0,wrap_s=0,wrap_t=0;
  std::string name,sheet;
  ClientPoseVector size{1,1},position{},crop{};
  ClientPoseColor color{};
  float rotation=0;
  bool visible=true,flip_x=false,flip_y=false;
  std::array<unsigned,5> blend{0,1,7,1,7};
  // 时间帧与原生动画帧数组下标分别表达；缺失不猜测独立层历史。
  int main_frame=-1,overlay_frame=-1;
};
struct ClientResourcePose final {
  // 已构造 ANM 可以合法没有加载资源；与缺失字段/Null引用不同，不擅自选择默认动画。
  bool loaded=true;
  std::string path,animation,overlay_animation;
  int frame=0,overlay_frame=0;
  bool overlay_present=false,overlay_first=false,flip_x=false,flip_y=false;
  ClientPoseVector offset{},scale{1,1};
  ClientPoseColor color{};
  // 冠军颜色是ANM独立的第二份ColorMod；不能由Entity.Color或普通Sprite.Color代替。
  bool champion_color_present=false;
  ClientPoseColor champion_color{};
  // 官方shader路径由早期Lua setter观察复制；每层与整Sprite分别拥有两类shader。
  bool shaders_present=false;
  std::array<std::string,2> shaders{};
  std::vector<std::array<std::string,2>> layer_shaders;
  float rotation=0,playback_speed=1;
  unsigned flags=0;
  std::vector<ClientLayerPose> layers;
  // 可选精确采样由正式观测提供；selector 是动画内层顺序，不是时间帧。
  bool exact_phase=false;
  // 停止Entity/ANM Update与“原姿态是否标记播放中”分开；NPC overlay draw会读取此位。
  bool main_playing=false,overlay_playing=false;
  float main_phase=0,overlay_phase=0;
  std::vector<int> main_selectors,overlay_selectors,main_null_selectors,overlay_null_selectors;
};
// 先构造私有资源候选检查全部层，再写入实体原有 Sprite；失败保留真实写入事实。
struct ClientResourceResult final {
  bool complete=false,written=false;
  unsigned layers=0;
  const char *reason="resource_not_applied";
};
// 仅主线程读取的聚合计数；不保存对象地址或逐资源日志。
struct ClientResourceStatistics final { std::uint64_t unchanged=0,reused_graphics=0,candidate_loads=0; };
ClientResourceStatistics ClientResourcesStatistics() noexcept;
bool ClientNativeCpuQueuesEmpty() noexcept;
ClientResourceResult ClientApplyResourcePose(void *sprite,const ClientResourcePose &) noexcept;
// 可选首因供实际原生后态诊断使用；不改变完整匹配要求。
bool ClientResourcePoseMatches(void *sprite,const ClientResourcePose &,const char **reason=nullptr) noexcept;
// 写前核对现有图像/ANM是否可承接姿态；不Load、不构造、不改变selector。
bool ClientResourcePoseReusable(void *,const ClientResourcePose &) noexcept;
// 只在RegisterClasses后、bundled/mod脚本前包装四个具名setter；原setter恰好调用一次。
bool ClientResourceInstallShaderBindings(void *lua_state) noexcept;
// 当前Lua受保护调用内复制路径；只查询已观察路径，不猜测未知shader或保存userdata。
bool ClientResourceCaptureShaderPaths(void *lua_state,void *sprite) noexcept;
} // namespace cs::game
