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
// 专用状态只保存真实有限槽；非Player的clot/wisp/locust/胎儿/DarkEsau沿Entity19唯一owner。
struct ClientPlayerSpecialState final {
  struct AfterImage final {int duration=0;ClientPoseVector position{};};
  // 原生可选动画序列的draw读集；不传链指针，玩法推进用的两个word明确不安装。
  struct Record final {ClientPoseVector position{};int animation=-1,frame=0,second_animation=-1,second_frame=0;};
  struct Image final {
    bool present=false;
    std::string path;
    unsigned shader=0,stride=0,min_filter=0,mag_filter=0,wrap_s=0,wrap_t=0;
    std::vector<unsigned char> attributes;
  };
  std::array<int,16> inventory_history{};
  std::array<unsigned,8> crafting{};
  std::array<unsigned,6> poop{};
  int crafting_item=0,crafting_pool=0;
  std::vector<AfterImage> afterimages;
  std::array<Image,2> images;
  std::vector<Record> records;
  int record_cursor=-1;
  ClientResourcePose record_sprite;
};
std::shared_ptr<const ClientPlayerSpecialState> ClientDescribePlayerSpecial(const replica::EntityObjectTarget &);
bool ClientPlayerSpecialFields(void *,const ClientPlayerDescription &,bool check,const char *&);
bool ClientPlayerDrawResources(void *,const ClientPlayerDescription &,bool check,const char *&);
bool ClientPlayerDrawRecords(void *,const ClientPlayerDescription &,bool check,const char *&);
bool ClientPlayerDrawPreflight(const ClientPlayerDescription &,const char *&) noexcept;
// 公开Lua补充调用栈内写出纯值；不导出lua_State/原生Player到模型层。
void ClientPlayerDrawCapture(void *lua,void *player);
ClientPlayerAdapter ClientPlayerBuiltinAdapter();
std::shared_ptr<const ClientPlayerRoleState> ClientDescribePlayerRole(const replica::EntityObjectTarget &);
// 同一T的全部身体一起核对，含四组八体；不按ordinal索引roster，不将ControllerIndex当身份。
void ClientPlayerRolesPrepare(const std::vector<ClientPlayerDescription> &,const replica::PlayerTarget &);
const char *ClientPlayerRoleDisposition(const std::string &) noexcept;
bool ClientPlayerRoleFields(void *,const ClientPlayerDescription &,bool check,const char *&);
// 必须在全体storage链接完成后调用；公开条件getter与storage分别回读。
bool ClientPlayerRoleRelations(void *,const ClientPlayerDescription &,const ClientEntityOwners &,const ClientNativeRoots &,const char *&);
} // namespace cs::game
