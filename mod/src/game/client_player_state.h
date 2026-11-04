#pragma once
#include "game/client_players_native.h"
#include <map>

namespace cs::replica { struct EntityObjectTarget; }
namespace cs::game {
// I11—I15的不可变最终量；不携带原生对象、配置地址、容器头或会继续运行的RNG。
struct ClientPlayerState final {
  struct Active final {int item=0,item_pool=0,charge=0,battery=0,sub=0,cooldown=0,var_data=0,backup_slot=-1;float partial=0;};
  struct Pocket final {int id=0;unsigned type=0;};
  struct Queue final {int item=0,type=0,charge=0,var_data=0;unsigned flags=0;};
  struct Timed final {int id=0,count=0,timer=0;};
  struct Innate final {std::map<int,int> persistent;std::vector<Timed> temporary;};
  struct Spoof final {std::vector<int> blocked;std::map<std::string,Innate> groups;};
  std::vector<ClientEntityField> fields;
  int player_type=0;
  std::vector<std::string> not_applicable;
  std::vector<int> collectibles,voided;
  std::map<int,int> wisps;
  std::vector<std::array<std::int16_t,2>> smelted;
  std::array<unsigned,2> trinkets{};
  std::array<Active,4> active{};
  std::array<Pocket,4> pocket{};
  Queue queue;
  std::array<Spoof,2> spoofs;
  // 完整公开查询仍归同一实体组件，供后续HUD/角色模块读取；不由查询值反造私有状态。
};
std::shared_ptr<const ClientPlayerState> ClientDescribePlayerState(const replica::EntityObjectTarget &);
const char *ClientPlayerStateDisposition(const std::string &) noexcept;
bool ClientPlayerStateApplicable(const std::string &,int player_type) noexcept;
// SDK实现只在当前主线程、当前use内安装/回读；后续完整adapter仍承担资源/角色/关系。
bool ClientPlayerStateFields(void *,const ClientPlayerDescription &,bool check,const char *&);
bool ClientPlayerStatePreflight(const ClientPlayerDescription &,const char *&);
bool ClientPlayerStateRetire(void *,const char *&);
} // namespace cs::game
