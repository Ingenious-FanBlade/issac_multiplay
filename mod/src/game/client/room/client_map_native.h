#pragma once
#include "game/client/resources/client_resources.h"
#include <array>
#include <vector>

namespace cs::game {
// 只复制 Minimap 实际读取的保存记录投影，不构造 Entity、不恢复离房世界。
// I24 可复用此来源；该投影不能被当成完整 EntitySaveState 或探索历史。
struct ClientMapSavedIcon final {int type=0,variant=0,subtype=0,storage3=0;ClientPoseVector position{};};
struct ClientMapDescriptor final {int storage=-1,source_slot=0;std::vector<ClientMapSavedIcon> icons;};
// 六个平滑环仍由本地 Camera 构造器拥有；Client 不运行 Camera Update。
struct ClientCameraState final {
  std::array<ClientPoseVector,5> vectors{}; // +60、+6c、+74、+7c、+84，排除 +68 Room*。
  ClientPoseVector focus_tail{};
  bool overridden=false,focus_present=false;
  float smoothing=0;
};
struct ClientMinimapConfiguration final {
  ClientPoseVector anchor{},padding{},size1{},size2{};
  float offset=0,border_scale=0,alpha=0;
  int width=0,height=0,icons=0;
};
struct ClientMapState final {
  ClientCameraState camera;
  std::array<ClientMinimapConfiguration,2> configs{};
  std::array<unsigned,2> game_flags{};
};
struct ClientMapTarget final {ClientMapState state;std::vector<ClientMapDescriptor> descriptors;};
int ClientMapDescriptorCaptureLua(void *);
int ClientMapStateCaptureLua(void *);
// 仅当前原生楼层/维度的来源投影，调用期间暂借保存数组，离栈恢复原 owner。
} // namespace cs::game
