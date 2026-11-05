#pragma once
#include "game/client_players_native.h"

namespace cs::replica { struct PlayerTarget; struct PlayerBodyTarget; struct EntityObjectTarget; }
namespace cs::game {
// History只表示同一身体的有序获得记录；与库存、共享Getter及本地HUD缓存分别拥有。
struct ClientPlayerHistoryView final {
  struct Item final {int time=0,id=0,stage=0,stage_type=0,room=0,pool=0;bool trinket=false;};
  ClientObjectKey body{};
  std::uint64_t entity_scope=0,player_scope=0,revision=0,shape=0;
  std::vector<Item> items;
};
// I16—I20的不可变身体子值；协议只携带值和稳定Key，私有偏移由产品内具名表决定。
struct ClientPlayerEquipment final {
  struct Effect final {int type=0,id=0,count=0,cooldown=0;bool item_present=false;};
  struct Costume final {
    int item=0,item_type=0,priority=0,default_skin=0,skin=0,player_type=0;
    bool item_present=false,item_animation=false,flying=false,overlay=false,skin_alt=false,overwrite=false,state_only=false;
    ClientResourcePose sprite;
  };
  struct Layer final {int costume=-1,layer=0,priority=0;bool body=false;};
  struct Weapon final {
    bool present=false;
    int type=0,num_fired=0;
    unsigned modifiers=0;
    float delay=0,max_delay=0,charge=0,max_charge=0;
    bool axis_aligned=false;
    ClientPoseVector direction{};
    ClientObjectReference owner{},main{};
    std::array<float,2> charge_bar{};
    std::map<std::string,ClientObjectReference> links;
    std::map<std::string,ClientResourcePose> sprites;
  };
  bool effects_disabled=false,effects_evaluate=false;
  std::uint32_t effects_cache=0;
  std::vector<Effect> effects;
  std::array<int,15> forms{};
  std::shared_ptr<const ClientPlayerHistoryView> history;
  bool history_mutation_disabled=false;
  // 身体/头/手使用实际物理ANM槽；没有凭概念虚构独立HeadSprite或HandSprite。
  ClientResourcePose main;
  std::string head_animation;
  std::array<ClientResourcePose,13> inline_sprites;
  std::vector<Costume> costumes;
  std::array<Layer,15> layers{};
  std::array<Weapon,5> weapons{};
};
std::map<ClientObjectKey,std::shared_ptr<const ClientPlayerEquipment>> ClientDescribePlayerEquipment(const replica::PlayerTarget &);
// 只替换同源Body/Head渲染组；库存、武器和History仍取精确普通依赖。
std::shared_ptr<const ClientPlayerEquipment> ClientStreamPlayerEquipment(const ClientPlayerEquipment &,const replica::EntityObjectTarget &);
const char *ClientPlayerEquipmentDisposition(const std::string &) noexcept;
// 每个原生身体持有一个子owner；异常保留实际前缀，不能以重试Destroy掩盖不确定释放。
struct ClientPlayerEquipmentOwner;
ClientPlayerEquipmentOwner *ClientPlayerEquipmentCreate() noexcept;
void ClientPlayerEquipmentDestroy(ClientPlayerEquipmentOwner *) noexcept;
bool ClientPlayerEquipmentPreflight(const ClientPlayerDescription &,const char *&);
bool ClientPlayerEquipmentFields(void *,const ClientPlayerDescription &,bool check,const char *&);
bool ClientPlayerEquipmentResources(ClientPlayerEquipmentOwner *,void *,const ClientPlayerDescription &,bool check,const char *&);
bool ClientPlayerRenderCompatible(ClientPlayerEquipmentOwner *,void *,const ClientPlayerEquipment &) noexcept;
bool ClientPlayerRenderInstall(ClientPlayerEquipmentOwner *,void *,const ClientPlayerEquipment &,bool,const char *&);
bool ClientPlayerEquipmentRelations(ClientPlayerEquipmentOwner *,void *,const ClientPlayerDescription &,const ClientEntityOwners &,const ClientNativeRoots &,bool check,bool clear,const char *&);
bool ClientPlayerEquipmentRetire(ClientPlayerEquipmentOwner *,void *,const char *&);
} // namespace cs::game
