#pragma once
#include "replica/player_plan.h"
#include "replica/player_limits.h"
#include "game/client/player/client_player_view.h"
namespace cs::game {
// 预算是实际T/解析值的使用量，不把全局四槽按参与者复制，也不把Null资源当已加载图片。
struct ClientPlayerBudget final {
  std::uint64_t source_bytes=0,source_nodes=0,owned_bytes=0;
  unsigned participants=0,group_nodes=0,bodies=0,roster=0,non_roster=0,backups=0;
  unsigned inventory=0,effects=0,history=0,costumes=0,weapons=0,slots=0,records=0,hud_owners=0;
  unsigned references=0,resources=0,layers=0,selectors=0,steps=0;
};
ClientPlayerBudget ClientMeasurePlayers(const replica::PlayerApplyPlan &,const std::vector<ClientPlayerDescription> &);
} // namespace cs::game
