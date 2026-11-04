#pragma once
#include "game/client_players_native.h"

namespace cs::replica { struct EntityObjectTarget; struct PlayerTarget; }
namespace cs::game {
// 注册是应用责任选择；心量/资源/飞行均消费Source最终量，不由角色名推算。
struct ClientPlayerRolePolicy final {int type;const char *name;unsigned family,task;};
const ClientPlayerRolePolicy *ClientPlayerRole(int type) noexcept;
const char *ClientPlayerRoleClassification(int type,unsigned variant) noexcept;
// I21—I25的纯值结果；形态、活动、可见、可消费和hologram保持独立。
struct ClientPlayerRoleState final {
  int item_state=0,suplex_state=0,suplex_countdown=0;
  ClientPoseVector suplex_target{},suplex_land{};
  bool is_sub=false,hologram=false;
  // Hologram实际由exists/type/twin双方PlayerIndex导出，不虚构hologram标志位。
  int player_index=-1;
};
std::shared_ptr<const ClientPlayerRoleState> ClientDescribePlayerRole(const replica::EntityObjectTarget &);
// 同一T的全部身体一起核对，含四组八体；不按ordinal索引roster，不将ControllerIndex当身份。
void ClientPlayerRolesPrepare(const std::vector<ClientPlayerDescription> &,const replica::PlayerTarget &);
const char *ClientPlayerRoleDisposition(const std::string &) noexcept;
bool ClientPlayerRoleFields(void *,const ClientPlayerDescription &,bool check,const char *&);
// 必须在全体storage链接完成后调用；公开条件getter与storage分别回读。
bool ClientPlayerRoleRelations(void *,const ClientPlayerDescription &,const ClientEntityOwners &,const ClientNativeRoots &,const char *&);
} // namespace cs::game
