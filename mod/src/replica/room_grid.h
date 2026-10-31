#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace cs::replica {
// Room/Grid专用展示表示；只含自有值，与同一Scene的原生资源一起发布和退休。
// 不构造原生Room/Grid，不提供任何玩法setter或远程调用。
struct RoomGrid final {
  struct Cell final {
    bool inside=false;
    int collision=0,path=0,rail=0,occlusion=0;
    float x=0,y=0;
  };
  std::string model,descriptor;
  std::uint64_t session=0,connection=0,run=0,floor=0,dimension=0,room=0,scene=0,revision=0;
  unsigned shape=0,grids=0,doors=0,parts=0,types=0,classes=0,references=0;
  std::vector<Cell> cells;
};
// 严格解析完整域模型并生成17的资源描述；任何失败都不修改当前发布版本。
RoomGrid PrepareRoomGrid(const std::string &model);
// 15的规范复制模型接入：状态/身份/保存描述只来自同版本Room/Grid component，
// display只补充Authority已裁决的几何与视觉部件，不能覆写复制状态。
std::string ComposeRoomGrid(const std::string &replica_model,const std::string &display);
// 只读当前专用状态；返回owned JSON，可用于后续域适配，绝不跨线程持有Player/Grid。
std::string RoomGridObject(const RoomGrid &,const std::string &identity);
} // namespace cs::replica
