#pragma once
#include "game/client/resources/client_resources.h"
#include "replica/model.h"
namespace cs::game {
// 从正式 resource_pose 公共字段解析；缺项、非有限值、未知 shader 不补默认最终值。
bool ClientParseResourcePose(const replica::Value &,ClientResourcePose &,std::string &reason) noexcept;
}
