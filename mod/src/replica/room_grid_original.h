#pragma once
#include "replica/room_grid.h"
#include "replica/model.h"
#include <string>
namespace cs::replica {
// 从真实Authority普通状态与独立Render资源结果构造完整Room/Grid应用；纯值，不读取游戏对象。
RoomGrid PrepareOriginalRoomGrid(const std::string &packet);
// 纯值适配内部共用DOM，避免大房间在同一候选中反复编码再解析。
RoomGrid PrepareOriginalRoomGridValue(Json packet);
RoomGrid PrepareRoomGridValue(Json root);
Json ComposeRoomGridValue(const Value &model,Json display);
}
